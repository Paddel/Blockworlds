
#include <base/system.h>

#if defined(CONF_FAMILY_WINDOWS)
#include "winsock2.h"
#endif

#include <mysql.h>

#include "accounts.h"

CAccountsHandler::CAccountsHandler()
{
	m_pGameServer = 0x0;
}

void CAccountsHandler::TestFunc(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pData = (CDatabase::CQueryData *)pQueryData;
	dbg_msg(0, "in");
	if (Error)
	{
		dbg_msg(0, "error");
		return;
	}



	for (int i = 0; i < pData->m_lpResultRows.size(); i++)
		for (int x = 0; x < pData->m_lpResultRows[i]->m_lpResultFields.size(); x++)
			dbg_msg(0, "%i %i %s", i, x, pData->m_lpResultRows[i]->m_lpResultFields[x]);
}

void CAccountsHandler::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_Database.Init("78.47.53.206", "taschenrechner", "hades", "teeworlds");

	thread_sleep(2000);
	m_Database.QueryThread("SELECT * FROM accounts WHERE Slot > 0", TestFunc, m_pGameServer);
}


void CAccountsHandler::Tick()
{
	m_Database.Tick();
}