
#include <engine/shared/config.h>

#include "connless_limiter.h"

#define FLOODING_NUM 65

struct CResultData
{
	bool m_Passed;
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

	pResultData->m_Passed = pQueryResult->m_lpResultRows.size() == 1;
}

void CConnlessLimiter::Init(class CServer *pServer)
{
	m_pServer = pServer;

	m_Database.Init(g_Config.m_SvDbAccAddress, g_Config.m_SvDbAccName, g_Config.m_SvDbAccPassword, g_Config.m_SvDbAccSchema);
}

bool CConnlessLimiter::AllowInfo(const NETADDR *pAddr)
{
	char aQuery[QUERY_MAX_STR_LEN];
	char aAddrStr[NETADDR_MAXSTRSIZE];

	m_InquiriesPerSecond++;
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
	pResultData->m_Passed = false;

	m_Database.QueryOrderly(aQuery, ResultAddrCheck, pResultData);
	return pResultData->m_Passed;
}