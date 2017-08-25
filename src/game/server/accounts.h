#pragma once

#include <engine/server/database.h>

class CGameContext;
class IServer;

class CAccountsHandler
{
private:
	CDatabase m_Database;
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	bool m_Inited;

	void CreateTables();

	static void ResultLogin(void *pQueryData, bool Error, void *pUserData);
	static void ResultRegister(void *pQueryData, bool Error, void *pUserData);
public:
	CAccountsHandler();

	void Init(CGameContext *pGameServer);
	void Tick();

	void Login(int ClientID, const char *pName, const char *pPassword);
	void Register(int ClientID, const char *pName, const char *pPassword);
	void Logout(int ClientID);
	void Save(int ClientID);
	void ChangePassword(int ClientID, const char *pOldPassword, const char *pNewPassword);

	bool CanShutdown();

	CGameContext *GameServer() const { return m_pGameServer; };
	IServer *Server() const { return m_pServer; };
};