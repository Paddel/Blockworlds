
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/balancing.h>

#include "accounts.h"

//TODO: difference between duplicate entries and real errors

#define TABLE_ACCOUNTS "accounts"
#define TABLE_CLANS "clans"

#define COLUMN_NUM_ACCOUNT 16
#define COLUMN_NUM_CLAN 5

inline bool CheckValidChars(const char *pStr)
{
	int Len = str_length(pStr);
	for (int i = 0; i < Len; i++)
		if ((pStr[i] < 'a' || pStr[i] > 'z') &&
			(pStr[i] < 'A' || pStr[i] > 'Z') &&
			(pStr[i] < '0' || pStr[i] > '9'))
			return false;
	 
	/*if (pStr[0] == ' ' || pStr[Len - 1] == ' ')
		return false;*/
	return true;
}

struct CResultData
{
	CGameContext *m_pGameServer;
	int m_ClientID;
	char m_aName[32];
	char m_aPassword[32];
};

CAccountsHandler::CAccountsHandler()
{
	m_ClanSystemError = false;
}

void CAccountsHandler::CreateTables()
{
	char aQuery[QUERY_MAX_STR_LEN];

	str_format(aQuery, sizeof(aQuery), "CREATE TABLE IF NOT EXISTS %s"
		"(name VARCHAR(32) BINARY NOT NULL, leader VARCHAR(11) BINARY NOT NULL"
		", level INT DEFAULT 1, experience INT DEFAULT 0, members INT DEFAULT 1"
		", PRIMARY KEY(name)"
		") CHARACTER SET utf8 COLLATE utf8_bin;", TABLE_CLANS);
	m_Database.QueryOrderly(aQuery, 0x0, 0x0);

	str_format(aQuery, sizeof(aQuery), "CREATE TABLE IF NOT EXISTS %s"
		"(name VARCHAR(11) BINARY NOT NULL, password VARCHAR(16) BINARY NOT NULL"
		", address VARCHAR(47), vip INT(2) DEFAULT 0, pages INT DEFAULT 0, level INT DEFAULT 1"
		", experience INT DEFAULT 0, weaponkits INT DEFAULT 0, ranking INT DEFAULT 1000"
		", clan VARCHAR(32), blockpoints INT DEFAULT 0, knockouts VARCHAR(256) DEFAULT NULL"
		", gundesign VARCHAR(256) DEFAULT NULL, skinmani VARCHAR(256) DEFAULT NULL, extras VARCHAR(256) DEFAULT NULL"
		", timestamp DATETIME"
		", PRIMARY KEY(name)"
		", CONSTRAINT fk_clan FOREIGN KEY (clan) REFERENCES %s(name) ON DELETE SET NULL ON UPDATE NO ACTION"
		")CHARACTER SET utf8 COLLATE utf8_bin;", TABLE_ACCOUNTS, TABLE_CLANS);
	m_Database.QueryOrderly(aQuery, 0x0, 0x0);

	//load clan data
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s", TABLE_CLANS);
	m_Database.QueryOrderly(aQuery, ResultLoadClans, this);
}

void CAccountsHandler::ResultLoadClans(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CAccountsHandler *pThis = (CAccountsHandler *)pUserData;

	if (Error)
	{
		dbg_msg("Accounts", "Error loading clans!");
		pThis->m_ClanSystemError = true;
		return;
	}

	if (pQueryResult->m_lpResultRows.size() == 0)
	{
		dbg_msg("Accounts", "No clans in database found.");
		return;
	}

	if (pQueryResult->m_lpResultRows[0]->m_lpResultFields.size() != COLUMN_NUM_CLAN)//wrong database structure
	{
		dbg_msg("Accounts", "Database structure for clans outdated.");
		pThis->m_ClanSystemError = true;
		return;
	}

	for (int i = 0; i < pQueryResult->m_lpResultRows.size(); i++)
	{
		CDatabase::CResultRow *pRow = pQueryResult->m_lpResultRows[i];
		IServer::CClanData *pClan = pThis->CreateClan(pRow);
		if(pClan == 0x0)
			dbg_msg("Accounts", "Error creating clan! %s", pRow->m_lpResultFields[0]);
		else
			pThis->m_lpClans.add(pClan);
	}

	dbg_msg("Accounts", "All clans have been loaded");
}

void CAccountsHandler::ResultLogin(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;
	CAccountsHandler *pThis = pGameServer->AccountsHandler();

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

	if (pRow->m_lpResultFields.size() != COLUMN_NUM_ACCOUNT)//wrong database structure
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
	//address
	pFillingData->m_Vip = str_toint(pRow->m_lpResultFields[3]);
	pFillingData->m_Pages = str_toint(pRow->m_lpResultFields[4]);
	pFillingData->m_Level = str_toint(pRow->m_lpResultFields[5]);
	pFillingData->m_Experience = str_toint(pRow->m_lpResultFields[6]);
	pFillingData->m_WeaponKits = str_toint(pRow->m_lpResultFields[7]);
	pFillingData->m_Ranking = str_toint(pRow->m_lpResultFields[8]);
	//clan
	pFillingData->m_BlockPoints = str_toint(pRow->m_lpResultFields[10]);
	pGameServer->CosmeticsHandler()->FillKnockout(pFillingData, pRow->m_lpResultFields[11]);
	pGameServer->CosmeticsHandler()->FillGundesign(pFillingData, pRow->m_lpResultFields[12]);
	pGameServer->CosmeticsHandler()->FillSkinmani(pFillingData, pRow->m_lpResultFields[13]);
	//pGameServer->CosmeticsHandler()->FillExtras(pFillingData, pRow->m_lpResultFields[14]);
	//timestamp

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "Successfully logged in as '%s'", pResultData->m_aName);
	pGameServer->SendChatTarget(pResultData->m_ClientID, aBuf);

	const char *pClan = pRow->m_lpResultFields[9];
	//link clan
	if (pClan[0] != '\0')
	{
		int Index = -1;
		for (int i = 0; i < pThis->m_lpClans.size(); i++)
		{
			if (str_comp(pThis->m_lpClans[i]->m_aName, pClan) == 0)
			{
				Index = i;
				break;
			}
		}

		if (Index != -1)
			pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_pClan = pThis->m_lpClans[Index];
		else
			pGameServer->SendChatTarget(pResultData->m_ClientID, "Could not link your account to a clan. Please contact an Admin");
	}

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
	CAccountsHandler *pThis = pGameServer->AccountsHandler();

	if(pThis->m_Database.GetConnected() == false)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00010");
		delete pResultData;
		return;
	}

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

void CAccountsHandler::ResultClanCreate(void *pQueryData, bool Error, void *pUserData)
{
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;
	CAccountsHandler *pThis = pGameServer->AccountsHandler();

	for (int i = 0; i < pThis->m_lpClans.size(); i++)
	{
		if (str_comp_nocase(pResultData->m_aName, pThis->m_lpClans[i]->m_aName) == 0)
		{
			pGameServer->SendChatTarget(pResultData->m_ClientID, "Clan already exists");
			delete pResultData;
			return;
		}
	}

	if (Error)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00003");
		delete pResultData;
		return;
	}

	if (pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_pClan != 0x0)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "You are already in a clan!");
		delete pResultData;
		return;
	}

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET clan=", TABLE_ACCOUNTS);
	CDatabase::AddQueryStr(aQuery, pResultData->m_aName, sizeof(aQuery));
	str_append(aQuery, " WHERE name=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_AccountData.m_aName, sizeof(aQuery));
	str_append(aQuery, ";", sizeof(aQuery));
	pThis->m_Database.Query(aQuery, 0x0, 0x0);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Clan '%s' successfully created. Use \"/clan_leader\" for more information!", pResultData->m_aName);
	pGameServer->SendChatTarget(pResultData->m_ClientID, aBuf);

	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s WHERE name=", TABLE_CLANS);
	CDatabase::AddQueryStr(aQuery, pResultData->m_aName, sizeof(aQuery));
	pThis->m_Database.Query(aQuery, ResultAddClan, pResultData);

}

void CAccountsHandler::ResultAddClan(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;
	CAccountsHandler *pThis = pGameServer->AccountsHandler();

	if (Error || pQueryResult->m_lpResultRows.size() != 1)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00004");
		delete pResultData;
		return;
	}

	CDatabase::CResultRow *pRow = pQueryResult->m_lpResultRows[0];

	if (pRow->m_lpResultFields.size() != COLUMN_NUM_CLAN)//wrong database structure
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00005");
		delete pResultData;
		return;
	}

	IServer::CClanData *pClanData = pThis->CreateClan(pRow);
	if (pClanData == 0x0)//wrong database structure
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00006");
		delete pResultData;
		return;
	}

	pThis->m_lpClans.add(pClanData);
	pGameServer->Server()->GetClientInfo(pResultData->m_ClientID)->m_pClan = pClanData;
}

void CAccountsHandler::ResultClanList(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;
	char aBuf[64];

	if (Error)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00007");
		delete pResultData;
		return;
	}

	pGameServer->SendChatTarget(pResultData->m_ClientID, ">-- CLAN LIST --<");

	for (int i = 0; i < pQueryResult->m_lpResultRows.size(); i++)
	{
		str_format(aBuf, sizeof(aBuf), "[%i] %s", i, pQueryResult->m_lpResultRows[i]->m_lpResultFields[0]);
		pGameServer->SendChatTarget(pResultData->m_ClientID, aBuf);
	}
	delete pResultData;
}

void CAccountsHandler::ResultClanKick(void *pQueryData, bool Error, void *pUserData)
{
	CDatabase::CQueryData *pQueryResult = (CDatabase::CQueryData *)pQueryData;
	CResultData *pResultData = (CResultData *)pUserData;
	CGameContext *pGameServer = pResultData->m_pGameServer;
	CAccountsHandler *pThis = pGameServer->AccountsHandler();
	
	if (Error)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00009");
		delete pResultData;
		return;
	}

	if (pQueryResult->m_AffectedRows <= 0)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Player is not a member of the clan");
		delete pResultData;
		return;
	}

	IServer::CClanData *pClan = 0x0;
	for (int i = 0; i < pThis->m_lpClans.size(); i++)
	{
		if (str_comp(pThis->m_lpClans[i]->m_aName, pResultData->m_aPassword) == 0)
		{
			pClan = pThis->m_lpClans[i];
			break;
		}
	}

	if(pClan == 0x0)
	{
		pGameServer->SendChatTarget(pResultData->m_ClientID, "Internal Server Error. Please contact an admin. Code 0x00008");
		delete pResultData;
		return;
	}

	pClan->m_Members--;
	pThis->ClanSave(pClan);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameServer->Server()->ClientIngame(i) == false ||
			pGameServer->Server()->GetClientInfo(i)->m_LoggedIn == false ||
			pGameServer->Server()->GetClientInfo(i)->m_pClan != pClan ||
			str_comp(pGameServer->Server()->GetClientInfo(i)->m_AccountData.m_aName, pResultData->m_aName) != 0)
			continue;

		pGameServer->Server()->GetClientInfo(i)->m_pClan = 0x0;
		pGameServer->SendChatTarget(i, "You have beed kicked out of your clan!");
		break;
	}

	pGameServer->SendChatTarget(pResultData->m_ClientID, "Member has been kicked!");

	delete pResultData;
}


IServer::CClanData *CAccountsHandler::CreateClan(CDatabase::CResultRow *pRow)
{
	if (pRow->m_lpResultFields.size() != COLUMN_NUM_CLAN)
		return 0x0;

	IServer::CClanData *pClanData = new IServer::CClanData();
	str_copy(pClanData->m_aName, pRow->m_lpResultFields[0], sizeof(pClanData->m_aName));
	str_copy(pClanData->m_aLeader, pRow->m_lpResultFields[1], sizeof(pClanData->m_aLeader));
	pClanData->m_Level = str_toint(pRow->m_lpResultFields[2]);
	pClanData->m_Experience = str_toint(pRow->m_lpResultFields[3]);
	pClanData->m_Members = str_toint(pRow->m_lpResultFields[4]);
	return pClanData;
}

void CAccountsHandler::Init()
{
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

	if (g_Config.m_SvAccountsystem == 0)
	{
		GameServer()->SendChatTarget(ClientID, "Accountsystem disabled on this server!");
		return;
	}

	if (NameLength == 0 || PasswortLength == 0)
		return;

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

	if (CheckValidChars(pName) == false || CheckValidChars(pPassword) == false)
	{
		GameServer()->SendChatTarget(ClientID, "Valid characters are A-Z and 0-9");
		return;
	}

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;
	str_copy(pResultData->m_aPassword, pPassword, sizeof(pResultData->m_aPassword));
	str_copy(pResultData->m_aName, pName, sizeof(pResultData->m_aName));

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s WHERE name=", TABLE_ACCOUNTS);
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
		GameServer()->SendChatTarget(ClientID, "Your password must be at least 4 characters long!");
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

	if (CheckValidChars(pName) == false || CheckValidChars(pPassword) == false)
	{
		GameServer()->SendChatTarget(ClientID, "Valid characters are A-Z and 0-9");
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
	str_format(aQuery, sizeof(aQuery), "INSERT INTO %s (name, password, address) VALUES(", TABLE_ACCOUNTS);
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

	GameServer()->InquiriesHandler()->Clear(ClientID);

	Save(ClientID);

	mem_zero(&Server()->GetClientInfo(ClientID)->m_AccountData, sizeof(IServer::CAccountData));
	Server()->GetClientInfo(ClientID)->m_LoggedIn = false;
	Server()->GetClientInfo(ClientID)->m_pClan = 0x0;

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
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET ", TABLE_ACCOUNTS);

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

	str_append(aQuery, ",ranking=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_Ranking, sizeof(aQuery));

	str_append(aQuery, ",blockpoints=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pAccountData->m_BlockPoints, sizeof(aQuery));

	str_append(aQuery, ",knockouts=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pAccountData->m_aKnockouts, sizeof(aQuery));

	str_append(aQuery, ",gundesign=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pAccountData->m_aGundesigns, sizeof(aQuery));

	str_append(aQuery, ",skinmani=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pAccountData->m_aSkinmani, sizeof(aQuery));

	str_append(aQuery, ",extras=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pAccountData->m_aExtras, sizeof(aQuery));

	str_append(aQuery, ",timestamp=NOW()", sizeof(aQuery));

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
		GameServer()->SendChatTarget(ClientID, "Old password does not fit!");
		return;
	}

	if (str_length(pNewPassword) * sizeof(char) >= sizeof(IServer::CAccountData::m_aPassword))
	{
		GameServer()->SendChatTarget(ClientID, "Loginpassword too long!");
		return;
	}

	if (str_length(pNewPassword) <= 3)
	{
		GameServer()->SendChatTarget(ClientID, "Your password must be at least 4 characters long!");
		return;
	}

	if(CheckValidChars(pNewPassword) == false)
	{
		GameServer()->SendChatTarget(ClientID, "Valid characters are A-Z and 0-9");
		return;
	}

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET ", TABLE_ACCOUNTS);

	str_append(aQuery, "password=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pNewPassword, sizeof(aQuery));

	str_append(aQuery, " WHERE name=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, Server()->GetClientInfo(ClientID)->m_AccountData.m_aName, sizeof(aQuery));
	m_Database.Query(aQuery, 0x0, 0x0);

	GameServer()->SendChatTarget(ClientID, "Password successfully changed");
	Save(ClientID);
}

void CAccountsHandler::ClanCreate(int ClientID, const char *pName)
{
	int NameLength = str_length(pName);

	if (g_Config.m_SvAccountsystem == 0 || m_ClanSystemError)
	{
		GameServer()->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		GameServer()->SendChatTarget(ClientID, "You need to be logged in to create a clan!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_pClan != 0x0)
	{
		GameServer()->SendChatTarget(ClientID, "You are already in a clan!");
		return;
	}

	if (Server()->GetClientInfo(ClientID)->m_AccountData.m_Level < NeededClanCreateLevel())
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "You need to be at least level %i to create a clan!", NeededClanCreateLevel());
		GameServer()->SendChatTarget(ClientID, aBuf);
		return;
	}

	if (CheckValidChars(pName) == false)
	{
		GameServer()->SendChatTarget(ClientID, "Valid characters are A-Z and 0-9");
		return;
	}

	for (int i = 0; i < m_lpClans.size(); i++)
	{
		if (str_comp_nocase(pName, m_lpClans[i]->m_aName) == 0)
		{
			GameServer()->SendChatTarget(ClientID, "Clan already exists");
			return;
		}
	}

	if (NameLength < 2)
	{
		GameServer()->SendChatTarget(ClientID, "Clanname must be at leat 2 characters long!");
		return;
	}

	if (NameLength * sizeof(char) >= sizeof(IServer::CClanData::m_aName))
	{
		GameServer()->SendChatTarget(ClientID, "Clanname too long!");
		return;
	}

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;
	pResultData->m_aPassword[0] = '\0';
	str_copy(pResultData->m_aName, pName, sizeof(pResultData->m_aName));

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "INSERT INTO %s (name, leader) VALUES(", TABLE_CLANS);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, ",", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, Server()->GetClientInfo(ClientID)->m_AccountData.m_aName, sizeof(aQuery));
	str_append(aQuery, ");", sizeof(aQuery));

	m_Database.Query(aQuery, ResultClanCreate, pResultData);
}

void CAccountsHandler::ClanSave(IServer::CClanData *pClanData)
{
	if (g_Config.m_SvAccountsystem == 0 || m_ClanSystemError)
		return;

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET ", TABLE_CLANS);

	str_append(aQuery, "level=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pClanData->m_Level, sizeof(aQuery));

	str_append(aQuery, ",experience=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pClanData->m_Experience, sizeof(aQuery));

	str_append(aQuery, ",members=", sizeof(aQuery));
	CDatabase::AddQueryInt(aQuery, pClanData->m_Members, sizeof(aQuery));

	str_append(aQuery, " WHERE name=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pClanData->m_aName, sizeof(aQuery));
	m_Database.Query(aQuery, 0x0, 0x0);
}

void CAccountsHandler::ClanSaveAll()
{
	if (g_Config.m_SvAccountsystem == 0 || m_ClanSystemError)
		return;

	for (int i = 0; i < m_lpClans.size(); i++)
		ClanSave(m_lpClans[i]);
}

void CAccountsHandler::ClanList(int ClientID, IServer::CClanData *pClanData)
{
	if (g_Config.m_SvAccountsystem == 0 || m_ClanSystemError)
	{
		GameServer()->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;

	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT name FROM %s WHERE clan =", TABLE_ACCOUNTS);
	CDatabase::AddQueryStr(aQuery, pClanData->m_aName, sizeof(aQuery));
	str_append(aQuery, ";", sizeof(aQuery));
	m_Database.Query(aQuery, ResultClanList, pResultData);
}

void CAccountsHandler::ClanKick(int ClientID, IServer::CClanData *pClanData, const char *pName)
{
	if (g_Config.m_SvAccountsystem == 0 || m_ClanSystemError)
	{
		GameServer()->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}

	if (str_comp(pClanData->m_aLeader, pName) == 0)
	{
		GameServer()->SendChatTarget(ClientID, "You cannot kick yourself");
		return;
	}

	CResultData *pResultData = new CResultData();
	pResultData->m_pGameServer = GameServer();
	pResultData->m_ClientID = ClientID;
	str_copy(pResultData->m_aName, pName, sizeof(pResultData->m_aName));
	str_copy(pResultData->m_aPassword, pClanData->m_aName, sizeof(pResultData->m_aPassword));
	
	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "UPDATE %s SET clan=NULL WHERE name=", TABLE_ACCOUNTS);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, " AND clan=", sizeof(aQuery));
	CDatabase::AddQueryStr(aQuery, pClanData->m_aName, sizeof(aQuery));
	str_append(aQuery, ";", sizeof(aQuery));
	m_Database.Query(aQuery, ResultClanKick, pResultData);
}

void CAccountsHandler::ClanInvite(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer)
{
	CAccountsHandler *pThis = pGameServer->AccountsHandler();

	if (g_Config.m_SvAccountsystem == 0 || pThis->m_ClanSystemError)
	{
		pGameServer->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}


	char aBuf[64];
	IServer::CClanData *pClan = *((IServer::CClanData **)pData);
	int LeaderID = (int)*(pData + sizeof(void *));

	if (OptionID == -1 || pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(LeaderID, "No response to your claninvitation");
	}
	else if (OptionID == 0)
	{
		char aQuery[QUERY_MAX_STR_LEN];
		str_format(aQuery, sizeof(aQuery), "UPDATE %s SET clan=", TABLE_ACCOUNTS);
		CDatabase::AddQueryStr(aQuery, pClan->m_aName, sizeof(aQuery));
		str_append(aQuery, " WHERE name=", sizeof(aQuery));
		CDatabase::AddQueryStr(aQuery, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName, sizeof(aQuery));
		str_append(aQuery, ";", sizeof(aQuery));
		pThis->m_Database.Query(aQuery, 0x0, 0x0);

		pGameServer->Server()->GetClientInfo(ClientID)->m_pClan = pClan;

		str_format(aBuf, sizeof(aBuf), "%s joined your clan as '%s'", pGameServer->Server()->ClientName(ClientID),
			pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName);
		pGameServer->SendChatTarget(LeaderID, aBuf);

		str_format(aBuf, sizeof(aBuf), "You joined the clan '%s'. Write /clan for more information", pClan->m_aName);
		pGameServer->SendChatTarget(ClientID, aBuf);

		pClan->m_Members++;
		pThis->ClanSave(pClan);
	}
	else if (OptionID == 1)
	{
		str_format(aBuf, sizeof(aBuf), "%s declined your claninvitation", pGameServer->Server()->ClientName(ClientID));
		pGameServer->SendChatTarget(LeaderID, aBuf);
		pGameServer->SendChatTarget(ClientID, "Claninvitation declined!");
	}
}

void CAccountsHandler::ClanClose(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer)
{
	CAccountsHandler *pThis = pGameServer->AccountsHandler();
	IServer::CClanData *pClan = *((IServer::CClanData **)pData);

	if (g_Config.m_SvAccountsystem == 0 || pThis->m_ClanSystemError)
	{
		pGameServer->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}

	if (OptionID == 0)
	{
		char aQuery[QUERY_MAX_STR_LEN];
		str_format(aQuery, sizeof(aQuery), "DELETE FROM %s WHERE name=", TABLE_CLANS);
		CDatabase::AddQueryStr(aQuery, pClan->m_aName, sizeof(aQuery));
		str_append(aQuery, ";", sizeof(aQuery));
		pThis->m_Database.Query(aQuery, 0x0, 0x0);

		//informate all online members
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pGameServer->Server()->ClientIngame(i) == false ||
				pGameServer->Server()->GetClientInfo(i)->m_LoggedIn == false ||
				pGameServer->Server()->GetClientInfo(i)->m_pClan != pClan)
				continue;

			pGameServer->Server()->GetClientInfo(i)->m_pClan = 0x0;
			pGameServer->SendChatTarget(i, "Your clan has been closed!");
		}

		//update saved clans
		for (int i = 0; i < pThis->m_lpClans.size(); i++)
		{
			if (pThis->m_lpClans[i] == pClan)
			{
				pThis->m_lpClans.remove_index(i);
				delete pClan;
				break;
			}
		}
	}
	else
		pGameServer->SendChatTarget(ClientID, "Closing clan aborted");
}

void CAccountsHandler::ClanLeave(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer)
{
	CAccountsHandler *pThis = pGameServer->AccountsHandler();
	IServer::CClanData *pClan = *((IServer::CClanData **)pData);
	char aBuf[64];

	if (g_Config.m_SvAccountsystem == 0 || pThis->m_ClanSystemError)
	{
		pGameServer->SendChatTarget(ClientID, "Clansystem disabled on this server!");
		return;
	}

	if (OptionID == 0 && pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn)
	{
		char aQuery[QUERY_MAX_STR_LEN];
		str_format(aQuery, sizeof(aQuery), "UPDATE %s SET clan=NULL WHERE name=", TABLE_ACCOUNTS);
		CDatabase::AddQueryStr(aQuery, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName, sizeof(aQuery));
		str_append(aQuery, ";", sizeof(aQuery));
		pThis->m_Database.Query(aQuery, 0x0, 0x0);

		str_format(aBuf, sizeof(aBuf), "%s left the clan!", pGameServer->Server()->ClientName(ClientID));

		//informate all online members
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (pGameServer->Server()->ClientIngame(i) == false ||
				pGameServer->Server()->GetClientInfo(i)->m_LoggedIn == false ||
				pGameServer->Server()->GetClientInfo(i)->m_pClan != pClan)
				continue;

			pGameServer->SendChatTarget(i, aBuf);
		}

		pGameServer->Server()->GetClientInfo(ClientID)->m_pClan = 0x0;

		pClan->m_Members--;
		pThis->ClanSave(pClan);

	}
	else
		pGameServer->SendChatTarget(ClientID, "Leaving clan aborted");
}

bool CAccountsHandler::CanShutdown()
{
	if (m_Database.NumRunningThreads() > 0)
		return false;

	return true;
}