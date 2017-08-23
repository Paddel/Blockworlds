
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

void CAccountsHandler::TestFunc(void *pResultData, bool Error, void *pUserData)
{
	dbg_msg(0, "in");
	if (Error)
	{
		dbg_msg(0, "error");
		return;
	}

	MYSQL_RES *pResult = (MYSQL_RES *)pResultData;
	int count = (int)pResult->row_count;
	dbg_msg(0, "rows %i", count);
	for(int i = 0; i < count; i++)
	{
		dbg_msg(0, "catching new field");
		MYSQL_ROW Field = mysql_fetch_row(pResult);
		int affected = mysql_num_fields(pResult);

		for (int x = 0; x < affected; x++)
		{
			dbg_msg(0, "%i %i %s", i, x, CDatabase::GetDatabaseValue(Field[x]));
		}
	}
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