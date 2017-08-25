
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include "accounts.h"

#define TABLE_NAME "accounts"

struct CResultData
{
	CGameContext *m_pGameServer;
	int m_ClientID;
	char m_aName[32];
	char m_aPassword[32];
};

CAccountsHandler::CAccountsHandler()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;
}

void CAccountsHandler::CreateTables()
{
	char aQuery[QUERY_MAX_STR_LEN];

	str_format(aQuery, sizeof(aQuery), "CREATE TABLE IF NOT EXISTS %s"
		"(name VARCHAR(32) BINARY NOT NULL, password VARCHAR(32) BINARY NOT NULL,"
		"address VARCHAR(47), vip INT(2) DEFAULT 0, pages INT DEFAULT 0, level INT DEFAULT 1,"
		"experience INT DEFAULT 0, weaponkits INT DEFAULT 0, PRIMARY KEY(name)) CHARACTER SET utf8; ", TABLE_NAME);
	m_Database.Query(aQuery, 0x0, 0x0);
}

void CAccountsHandler::ResultLogin(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;

	if (Error || pQueryResult->m_lpResultRows.size() > 1)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00001");
		delete pResultData;
		return;
	}

	if(pQueryResult->m_lpResultRows.size() == 0)//acount does not exist
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Wrong username or password!");
		delete pResultData;
		return;
	}

	CDatabase::CResultRow *pRow = pQueryResult->m_lpResultRows[0];

	if (pRow->m_lpResultFields.size() != 8)//wrong database structure
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00002");
		delete pResultData;
		return;
	}

	if(str_comp(pRow->m_lpResultFields[1], pResultData->m_aPassword) != 0)//wrong password
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Wrong username or password!");
		delete pResultData;
		return;
	}

	//check if already logged in
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameServer->Server()->GetClientInfo(i)->m_LoggedIn == false)
			continue;

		if (str_comp(pGameServer->Server()->GetClientInfo(i)->m_AccountData.m_aName, pResultData->m_aName) == 0)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Account already in use by '%s'!", pGameServer->Server()->ClientName(i));
			pGameServer->SendChatTarget(pResultData->m_ClientID, aBuf);
			delete pResultData;
			return;
		}
	}

	IServer::CAccountData *pFillingData = &pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_AccountData;
	pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_LoggedIn = true;

	str_copy(pFillingData->m_aName, pRow->m_lpResultFields[0], sizeof(pFillingData->m_aName));
	str_copy(pFillingData->m_aPassword, pRow->m_lpResultFields[1], sizeof(pFillingData->m_aPassword));
	pFillingData->m_Vip = str_toint(pRow->m_lpResultFields[3]);
	pFillingData->m_Pages = str_toint(pRow->m_lpResultFields[4]);
	pFillingData->m_Level = str_toint(pRow->m_lpResultFields[5]);
	pFillingData->m_Experience = str_toint(pRow->m_lpResultFields[6]);
	pFillingData->m_WeaponKits = str_toint(pRow->m_lpResultFields[7]);

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "Successful logged in as '%s'", pResultData->m_aName);
	pGameServer->SendChatTarget(pResultData->m_ClientID, aBuf);

	if (g_Config.m_SvAccountForce == 1 && pGameServer->m_apPlayers[pResultData->m_ClientID] &&
		pGameServer->m_apPlayers[pResultData->m_ClientID]->GetTeam() == TEAM_SPECTATORS)
		pGameServer->m_apPlayers[pResultData->m_ClientID]->SetTeam(0, false);
	else if(pGameServer->m_apPlayers[pResultData->m_ClientID] != 0x0)
		pGameServer->m_apPlayers[pResultData->m_ClientID]->KillCharacter(WEAPON_GAME);

	delete pResultData;
}

void CAccountsHandler::ResultRegister(void *pQueryData, bool Error, void *pUserData)
{
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;

	if (Error)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Account does already exist");
		delete pResultData;
		return;
	}

	pGameServer->SendChatTarget(pResultData->m_ClientID, "Registration successful!");
	pGameServer->AccountsHandler()->Login(pResultData->m_ClientID, pResultData->m_aName, pResultData->m_aPassword);

	delete pResultData;
}

void CAccountsHandler::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_Database.Init(g_Config.m_SvDbAccAddress, g_Config.m_SvDbAccName, g_Config.m_SvDbAccPassword, g_Config.m_SvDbAccSchema);
}


void CAccountsHandler::Tick()
{
	m_Database.Tick();

	if (m_Database.CreateTables() == true)
		CreateTables();
}

void CAccountsHandler::Login(int ClientID, const char *pName, const char *pPassword)
{
	int NameLength = str_length(pName);
	int PasswortLength = str_length(pPassword);

	if (NameLength == 0 || PasswortLength == 0)
		return;

	if(g_Config.m_SvAccountsystem == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Accountsystem disabled on this server!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn)
	{
		GameServer()->SendChatTarget(ClientID, "You are already logged in!");
		return;
	}

	if (NameLength * sizeof(char) >= sizeof(IServer::CAccountData::m_aName))
	{
		GameServer()->SendChatTarget(ClientID, "Loginname too long!");
		return;
	}
	if (PasswortLength * sizeof(char) >= sizeof(IServer::CAccountData::m_aPassword))
	{
		GameServer()->SendChatTarget(ClientID, "Loginpassword too long!");
		return;
	}

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;
	str_copy(pResultData->m_aPassword, pPassword, sizeof(pResultData->m_aPassword));
	str_copy(pResultData->m_aName, pName, sizeof(pResultData->m_aName));

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s WHERE name=", TABLE_NAME);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	m_Database.Query(aQuery, ResultLogin, pResultData);
}

void CAccountsHandler::Register(int ClientID, const char *pName, const char *pPassword)
{
	int NameLength = str_length(pName);
	int PasswortLength = str_length(pPassword);

	if (g_Config.m_SvAccountsystem == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Accountsystem disabled on this server!");
		return;
	}

	if (NameLength <= 2)
	{
		GameServer()->SendChatTarget(ClientID, "Your name must be at least 3 characters long!");
		return;
	}

	if(PasswortLength <= 3)
	{
		GameServer()->SendChatTarget(ClientID, "Your name must be at least 4 characters long!");
		return;
	}

	if (str_comp(pName, pPassword) == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Password must be different from username!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn)
	{
		GameServer()->SendChatTarget(ClientID, "You are already logged in!");
		return;
	}

	if (NameLength * sizeof(char) >= sizeof(IServer::CAccountData::m_aName))
	{
		GameServer()->SendChatTarget(ClientID, "Loginname too long!");
		return;
	}
	if (PasswortLength * sizeof(char) >= sizeof(IServer::CAccountData::m_aPassword))
	{
		GameServer()->SendChatTarget(ClientID, "Loginpassword too long!");
		return;
	}

	char m_aAddressStr[64];
	Server()->GetClientAddr(ClientID, m_aAddressStr, sizeof(m_aAddressStr));

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;
	str_copy(pResultData->m_aPassword, pPassword, sizeof(pResultData->m_aPassword));
	str_copy(pResultData->m_aName, pName, sizeof(pResultData->m_aName));

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "INSERT INTO %s (name, password, address) VALUES(", TABLE_NAME);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, ",", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pPassword, sizeof(aQuery));
	str_append(aQuery, ",", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, m_aAddressStr, sizeof(aQuery));
	str_append(aQuery, ")", sizeof(aQuery));
	m_Database.Query(aQuery, ResultRegister, pResultData);
}

void CAccountsHandler::Logout(int ClientID)
{
	if (g_Config.m_SvAccountsystem == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Accountsystem disabled on this server!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return;

	Save(ClientID);

	mem_zero(&Server()->GetClientInfo(ClientID)->m_AccountData, sizeof(IServer::CAccountData));
	Server()->GetClientInfo(ClientID)->m_LoggedIn = false;
	GameServer()->SendChatTarget(ClientID, "Successfully logged out");
	if (g_Config.m_SvAccountForce == 1 && GameServer()->m_apPlayers[ClientID] &&
		GameServer()->m_apPlayers[ClientID]->GetTeam() != TEAM_SPECTATORS)
		GameServer()->m_apPlayers[ClientID]->SetTeam(TEAM_SPECTATORS, false);
	else if (GameServer()->m_apPlayers[ClientID] != 0x0)
		GameServer()->m_apPlayers[ClientID]->KillCharacter(WEAPON_GAME);
}

void CAccountsHandler::Save(int ClientID)
{
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return;

	if (g_Config.m_SvAccountsystem == 0)
		return;

	char m_aAddressStr[64];
	Server()->GetClientAddr(ClientID, m_aAddressStr, sizeof(m_aAddressStr));
	IServer::CAccountData *pAccountData = &Server()->GetClientInfo(ClientID)->m_AccountData;

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET ", TABLE_NAME);

	str_append(aQuery, "vip=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_Vip, sizeof(aQuery));

	str_append(aQuery, ",pages=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_Pages, sizeof(aQuery));

	str_append(aQuery, ",level=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_Level, sizeof(aQuery));

	str_append(aQuery, ",experience=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_Experience, sizeof(aQuery));

	str_append(aQuery, ",address=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, m_aAddressStr, sizeof(aQuery));

	str_append(aQuery, ",weaponkits=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_WeaponKits, sizeof(aQuery));

	str_append(aQuery, " WHERE name=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pAccountData->m_aName, sizeof(aQuery));
	m_Database.Query(aQuery, 0x0, 0x0);
}

void CAccountsHandler::ChangePassword(int ClientID, const char *pOldPassword, const char *pNewPassword)
{
	if (g_Config.m_SvAccountsystem == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Accountsystem disabled on this server!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		GameServer()->SendChatTarget(ClientID, "You are not logged in.");
		return;
	}

	if (str_comp(pOldPassword, pNewPassword) == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Old and new password are identical.");
		return;
	}

	if (str_comp(pOldPassword, Server()->GetClientInfo(ClientID)->m_AccountData.m_aPassword) != 0)
	{
		GameServer()->SendChatTarget(ClientID, "Old password is not right!");
		return;
	}


	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET ", TABLE_NAME);

	str_append(aQuery, "password=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pNewPassword, sizeof(aQuery));

	str_append(aQuery, " WHERE name=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, Server()->GetClientInfo(ClientID)->m_AccountData.m_aName, sizeof(aQuery));
	m_Database.Query(aQuery, 0x0, 0x0);

	GameServer()->SendChatTarget(ClientID, "Password successfully changed");
}

bool CAccountsHandler::CanShutdown()
{
	if (m_Database.NumRunningThreads() > 0)
		return false;

	return true;
}