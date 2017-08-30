#pragma once

#include <base/tl/array.h>
#include <engine/server/database.h>
#include <game/server/component.h>

class CAccountsHandler : public CComponentGlobal
{
private:
	CDatabase m_Database;
	array<IServer::CClanData *> m_lpClans;
	bool m_Inited;
	bool m_ClanSystemError;

	void CreateTables();

	static void ResultLoadClans(void *pQueryData, bool Error, void *pUserData);
	static void ResultLogin(void *pQueryData, bool Error, void *pUserData);
	static void ResultRegister(void *pQueryData, bool Error, void *pUserData);
	static void ResultClanCreate(void *pQueryData, bool Error, void *pUserData);
	static void ResultAddClan(void *pQueryData, bool Error, void *pUserData);
	static void ResultClanList(void *pQueryData, bool Error, void *pUserData);
	static void ResultClanKick(void *pQueryData, bool Error, void *pUserData);

	IServer::CClanData *CreateClan(CDatabase::CResultRow *pRow);
public:
	CAccountsHandler();

	virtual void Init();
	virtual void Tick();

	void Login(int ClientID, const char *pName, const char *pPassword);
	void Register(int ClientID, const char *pName, const char *pPassword);
	void Logout(int ClientID);
	void Save(int ClientID);
	void ChangePassword(int ClientID, const char *pOldPassword, const char *pNewPassword);

	void ClanCreate(int ClientID, const char *pName);
	void ClanSave(IServer::CClanData *pClanData);
	void ClanSaveAll();
	void ClanList(int ClientID, IServer::CClanData *pClanData);
	void ClanKick(int ClientID, IServer::CClanData *pClanData, const char *pName);
	static void ClanInvite(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	static void ClanClose(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	static void ClanLeave(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);

	bool CanShutdown();
};