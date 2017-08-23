#pragma once

#include <engine/server/database.h>

class CGameContext;

class CAccountsHandler
{
private:
	CDatabase m_Database;
	CGameContext *m_pGameServer;

	static void TestFunc(void *pResultData, bool Error, void *pUserData);
public:
	CAccountsHandler();

	void Init(CGameContext *pGameServer);
	void Tick();
};