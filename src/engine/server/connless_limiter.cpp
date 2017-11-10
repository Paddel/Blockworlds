
#include <engine/shared/config.h>
#include <engine/server/server.h>

#include "connless_limiter.h"

#define FLOODING_NUM 80

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
}

void CConnlessLimiter::ResultAddrCheck(void *pQueryData, bool Error, void *pUserData)
{
	if (Error == true)
		return;

	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;

	if (pQueryResult->m_lpResultRows.size() == 1)
		pResultData->m_pThis->m_pServer->SendServerInfo(&pResultData->m_Addr, pResultData->m_Token,
			pResultData->m_pMap, pResultData->m_Socket, pResultData->m_Info64, pResultData->m_Offsets, true);

	dbg_msg(0, "a %i", pQueryResult->m_lpResultRows.size() == 1);

	delete pResultData;
}

void CConnlessLimiter::Init(class CServer *pServer)
{
	m_pServer = pServer;

	m_Database.Init(g_Config.m_SvDbAccAddress, g_Config.m_SvDbAccName, g_Config.m_SvDbAccPassword, g_Config.m_SvDbAccSchema);
}

void CConnlessLimiter::Tick()
{
	m_Database.Tick();
}

bool CConnlessLimiter::AllowInfo(const NETADDR *pAddr, int Token, CMap *pMap, NETSOCKET Socket, bool Info64, int Offsets)
{
	char aQuery[QUERY_MAX_STR_LEN];
	char aAddrStr[NETADDR_MAXSTRSIZE];

	m_InquiriesPerSecond += 1 + (int)Info64;
	if (m_LastMesurement < time_get())
	{
		if (m_InquiriesPerSecond >= FLOODING_NUM)
			m_FloodDetectionTime = time_get() + time_freq() * 3;

		m_InquiriesPerSecond = 0;
		m_LastMesurement = time_get() + time_freq();
	}

	if (m_FloodDetectionTime < time_get())
		return true;

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