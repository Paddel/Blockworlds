
#include <engine/shared/config.h>
#include <engine/server/server.h>

#include "connless_limiter.h"

#define FLOODING_NUM 80
#define MAX_QUERYING 700

#define EXTERNAL_INFO_SERVER "78.47.53.206:8303"

#define PORT 5410

struct CResultData
{
	CConnlessLimiter *m_pThis;
	NETADDR m_Addr;
	int m_Token;
	CMap *m_pMap;
	NETSOCKET m_Socket;
	bool m_Info64;
	int m_Offsets;
};

CConnlessLimiter::CConnlessLimiter()
{
	m_InquiriesPerSecond = 0;
	m_LastMesurement = 0;
	m_FloodDetectionTime = 0;
	m_LastUnfilteredResult = 0;
	m_LastFilteredResult = 0;
	m_FilteredInquiries = 0;

	m_LastExtInfoSent = 0;
	m_ExternInfoTime = 0;
}

void CConnlessLimiter::ResultAddrCheck(void *pQueryData, bool Error, void *pUserData)
{
	if (Error == true)
		return;

	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;

	if (pQueryResult->m_lpResultRows.size() == 1)
	{
		pResultData->m_pThis->m_pServer->SendServerInfo(&pResultData->m_Addr, pResultData->m_Token,
			pResultData->m_pMap, pResultData->m_Socket, pResultData->m_Info64, pResultData->m_Offsets, true);
		pResultData->m_pThis->m_FilteredInquiries += 1 + (int)pResultData->m_Info64;
	}

	delete pResultData;
}

void CConnlessLimiter::Init(class CServer *pServer)
{
	m_pServer = pServer;

	net_host_lookup(EXTERNAL_INFO_SERVER, &m_ExternalAddr, NETTYPE_ALL);
	
	NETADDR BindAddr;
	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_ALL;
	BindAddr.port = PORT;
	m_Socket = net_udp_create(BindAddr, 0);
	if (!m_Socket.type)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "couldn't open socket. port %d might already be in use", PORT);
		m_pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}

	m_Database.Init(g_Config.m_SvDbAccAddress, g_Config.m_SvDbAccName, g_Config.m_SvDbAccPassword, g_Config.m_SvDbAccSchema);
}

void CConnlessLimiter::Tick()
{
	m_Database.Tick();

	if (m_LastMesurement < time_get())
	{
		m_LastUnfilteredResult = m_InquiriesPerSecond;
		m_LastFilteredResult = m_FilteredInquiries;

		if (m_InquiriesPerSecond >= FLOODING_NUM)
			m_FloodDetectionTime = time_get() + time_freq() * 3;

		if (m_InquiriesPerSecond >= MAX_QUERYING)
			m_ExternInfoTime = time_get() + time_freq() * 30;

		m_InquiriesPerSecond = 0;
		m_FilteredInquiries = 0;
		m_LastMesurement = time_get() + time_freq();
	}

	static bool s_LastExternInfoActive = false;
	if (s_LastExternInfoActive != ExternInfoActive() || m_LastExtInfoSent < time_get())
	{
		CNetChunk Packet;
		Packet.m_ClientID = -1;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_DataSize = sizeof(EXINFO_ACTIVATE);
		if(ExternInfoActive() == true)
			Packet.m_pData = &EXINFO_ACTIVATE;
		else
			Packet.m_pData = &EXINFO_DEACTIVATE;

		Packet.m_Address = m_ExternalAddr;
		m_pServer->m_NetServer.SendConnless(&Packet, m_Socket);

		m_LastExtInfoSent = time_get() + time_freq() * 10;
	}

	CNetChunk Packet;
	while (m_pServer->m_NetServer.Recv(&Packet, m_Socket))
	{
		if (Packet.m_ClientID == -1)
		{
			if (Packet.m_DataSize > sizeof(EXINFO_INFO) &&
				mem_comp(Packet.m_pData, EXINFO_INFO, sizeof(EXINFO_INFO)) == 0)
			{
				if(ExternInfoActive() == true)
					OnExternalInfo(Packet.m_pData, Packet.m_DataSize);
			}
		}

	}
}

bool CConnlessLimiter::AllowInfo(const NETADDR *pAddr, int Token, CMap *pMap, NETSOCKET Socket, bool Info64, int Offsets)
{
	char aQuery[QUERY_MAX_STR_LEN];
	char aAddrStr[NETADDR_MAXSTRSIZE];

	m_InquiriesPerSecond += 1 + (int)Info64;

	if (g_Config.m_SvConnlessLimier == 0)
		return true;

	if (FilterActive() == false)
		return true;

	if (m_InquiriesPerSecond >= MAX_QUERYING)
		return false;

	net_addr_str(pAddr, aAddrStr, sizeof(aAddrStr), false);
	str_format(aQuery, sizeof(aQuery), "SELECT address FROM address_verifier WHERE address='%s'", aAddrStr);

	CResultData *pResultData = new CResultData();
	pResultData->m_pThis = this;
	pResultData->m_Addr = *pAddr;
	pResultData->m_Token = Token;
	pResultData->m_pMap = pMap;
	pResultData->m_Socket = Socket;
	pResultData->m_Info64 = Info64;
	pResultData->m_Offsets = Offsets;

	m_Database.Query(aQuery, ResultAddrCheck, pResultData);
	return false;
}

void CConnlessLimiter::OnExternalInfo(const void *pData, int DataSize)
{
	int Token;
	char aAddrStr[NETADDR_MAXSTRSIZE];
	NETADDR Addr;
	mem_copy(&Token, (const char*)pData + sizeof(EXINFO_INFO), sizeof(int));
	mem_copy(&aAddrStr, (const char*)pData + sizeof(EXINFO_INFO) + sizeof(int), DataSize - sizeof(EXINFO_INFO) + sizeof(int));

	net_addr_from_str(&Addr, aAddrStr);

	dbg_msg(0, "ext info to %i.%i.%i.%i:%i Token=%i", Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3], Addr.port, Token);

	for (int i = 0; i < m_pServer->m_lpMaps.size(); i++)
	{
		if(m_pServer->m_lpMaps[i]->HasNetSocket() == true)
			m_pServer->SendServerInfo(&Addr, Token, m_pServer->m_lpMaps[i], m_pServer->m_lpMaps[i]->GetSocket(), false, 0, true);
	}
}

bool CConnlessLimiter::FilterActive()
{
	return m_FloodDetectionTime > time_get();
}

bool CConnlessLimiter::ExternInfoActive()
{
	return false;
	//return m_ExternInfoTime > time_get();
}