
#include <engine/shared/config.h>
#include <game/version.h>
#include <game/server/gamecontext.h>
#include <game/server/balancing.h>


#include "chatcommands.h"

static char SignArrowLeft[] = { '\xE2','\x86', '\x90', 0 };
static char SignArrowRight[] = { '\xE2','\x86', '\x92', 0 };

CChatCommandsHandler::CChatCommandsHandler()
{
	
}

CChatCommandsHandler::~CChatCommandsHandler()
{
	m_lpChatCommands.delete_all();
}

void CChatCommandsHandler::ComHelp(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	CChatCommandsHandler *pThis = pGameServer->ChatCommandsHandler();
	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pGameServer->SendChatTarget(ClientID, "Use this command to get assistance to a chatcommand. Possible input: /help cmdlist");
		return;
	}

	const char *pCommandName = pResult->GetString(0);

	if (pCommandName[0] == '/')
		pCommandName++;

	if (str_length(pCommandName) == 0)
		return;

	CChatCommand *pCommand = 0x0;
	for (int i = 0; i < pThis->m_lpChatCommands.size(); i++)
	{
		if (str_comp_nocase(pThis->m_lpChatCommands[i]->m_pName, pCommandName) != 0)
			continue;
		pCommand = pThis->m_lpChatCommands[i];
		break;
	}

	if (pCommand == 0x0)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' is not a chatcommand.", pCommandName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	pGameServer->SendChatTarget(ClientID, pCommand->m_pHelp);
}

void CChatCommandsHandler::ComCmdlist(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	CChatCommandsHandler *pThis = pGameServer->ChatCommandsHandler();
	char aBuf[512];
	str_copy(aBuf, "Commands:", sizeof(aBuf));

	for (int i = 0; i < pThis->m_lpChatCommands.size(); i++)
	{
		CChatCommand *pCommand = pThis->m_lpChatCommands[i];
		if (pCommand->m_Flags & CHATCMDFLAG_HIDDEN
			|| (pCommand->m_Flags & CHATCMDFLAG_MOD && pThis->Server()->GetClientAuthed(ClientID) < IServer::AUTHED_MOD)
			|| (pCommand->m_Flags & CHATCMDFLAG_ADMIN && pThis->Server()->GetClientAuthed(ClientID) < IServer::AUTHED_ADMIN))
			continue;

		str_fcat(aBuf, sizeof(aBuf), " %s,", pCommand->m_pName);
	}
	aBuf[str_length(aBuf) - 1] = '\0';//remove the last ,

	pGameServer->SendChatTarget(ClientID, aBuf);
}

void CChatCommandsHandler::ComPause(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];
	if (pPlayer == 0x0)
		return;

	if (pPlayer->GetTeam() == TEAM_SPECTATORS)
		return;

	pPlayer->TogglePause();
}

void CChatCommandsHandler::ComInfo(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	char aBuf[512];
	pGameServer->SendChatTarget(ClientID, "Blockworlds made by 13x37");
	str_format(aBuf, sizeof(aBuf), "Version %s", GAMEMODE_VERSION);
	pGameServer->SendChatTarget(ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Based on Teeworlds %s", GAME_VERSION);
	pGameServer->SendChatTarget(ClientID, aBuf);
	pGameServer->SendChatTarget(ClientID, "Hosted by exec");
	pGameServer->SendChatTarget(ClientID, "www.13x37.com");
}

void CChatCommandsHandler::ComWhisper(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	const char *pMessage = pResult->GetString(0);
	int Length = str_length(pMessage);

	int NumPersons = 0;
	int TargetID = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameServer->Server()->ClientIngame(i) == false || i == ClientID)
			continue;

		const char *pReceiverName = pGameServer->Server()->ClientName(i);
		int ReceiverLength = str_length(pReceiverName);
		if (ReceiverLength > Length || pMessage[ReceiverLength] != ' ' ||
			str_comp_nocase_num(pReceiverName, pMessage, ReceiverLength) != 0)
			continue;

		NumPersons++;
		TargetID = i;
	}

	if (NumPersons == 0)
	{
		if (str_length(pGameServer->Server()->ClientName(ClientID)) < Length
			&& str_comp_nocase_num(pGameServer->Server()->ClientName(ClientID), pMessage, str_length(pGameServer->Server()->ClientName(ClientID))) == 0)
			pGameServer->SendChatTarget(ClientID, "You cannot whisper to yourself");
		else
			pGameServer->SendChatTarget(ClientID, "Invalid receiver name");
	}
	else if (NumPersons > 1)
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%i possible receivers found");
		pGameServer->SendChatTarget(ClientID, aBuf);
	}
	else
	{
		char aBuf[512];
		pMessage += str_length(pGameServer->Server()->ClientName(TargetID)) + 1;

		if (g_Config.m_SvWhisperSrv == 1)
		{
			str_format(aBuf, sizeof(aBuf), "[%s %s] %s", SignArrowRight, pGameServer->Server()->ClientName(TargetID), pMessage);
			pGameServer->SendChatTarget(ClientID, aBuf);
			str_format(aBuf, sizeof(aBuf), "[%s %s] %s", SignArrowLeft, pGameServer->Server()->ClientName(ClientID), pMessage);
			pGameServer->SendChatTarget(TargetID, aBuf);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "[ %s %s %s ] %s", pGameServer->Server()->ClientName(ClientID), SignArrowRight, pGameServer->Server()->ClientName(TargetID), pMessage);

			CNetMsg_Sv_Chat Msg;
			Msg.m_Team = 0;
			Msg.m_pMessage = aBuf;
			Msg.m_ClientID = pGameServer->Server()->UsingMapItems(ClientID) - 1;
			pGameServer->Server()->SendMsgFinal(&Msg, MSGFLAG_VITAL, ClientID);
			Msg.m_ClientID = pGameServer->Server()->UsingMapItems(TargetID) - 1;
			pGameServer->Server()->SendMsgFinal(&Msg, MSGFLAG_VITAL, TargetID);
		}
	}
}

void CChatCommandsHandler::ComAccount(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "Account System V. 3.17");
	pGameServer->SendChatTarget(ClientID, "Use \"/login name password\" to login and \"/register name password\" to create an account.");
	pGameServer->SendChatTarget(ClientID, "If you are logged in you can change your password with \"/password oldpass newpass\".");
	pGameServer->SendChatTarget(ClientID, "You can log out anytime by writing \"/logout\".");
	pGameServer->SendChatTarget(ClientID, "Your progress will be saved automaticly!");
}

void CChatCommandsHandler::ComEmote(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	const char *pEmoteName = pResult->GetString(0);
	int Time = 2;
	if (pResult->NumArguments() == 2)
		Time = clamp(pResult->GetInteger(1), 0, 1000);

	int Emote = -1;
	if (str_comp_nocase(pEmoteName, "normal") == 0)
		Emote = EMOTE_NORMAL;
	else if (str_comp_nocase(pEmoteName, "pain") == 0)
		Emote = EMOTE_PAIN;
	else if (str_comp_nocase(pEmoteName, "happy") == 0)
		Emote = EMOTE_HAPPY;
	else if (str_comp_nocase(pEmoteName, "surprise") == 0)
		Emote = EMOTE_SURPRISE;
	else if (str_comp_nocase(pEmoteName, "angry") == 0)
		Emote = EMOTE_ANGRY;
	else if (str_comp_nocase(pEmoteName, "blink") == 0)
		Emote = EMOTE_BLINK;

	if (Emote == -1)
	{
		pGameServer->SendChatTarget(ClientID, "Invalid emote. Possible emotes are normal/pain/happy/surprise/angry/blink");
		return;
	}

	pGameServer->Server()->GetClientInfo(ClientID)->m_EmoteType = Emote;
	pGameServer->Server()->GetClientInfo(ClientID)->m_EmoteStop = pGameServer->Server()->Tick() + pGameServer->Server()->TickSpeed() * Time;
}

void CChatCommandsHandler::ComClan(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "Clan System V. 2.23");
	pGameServer->SendChatTarget(ClientID, "By joining a clan your account gets linked to a clan.");
	pGameServer->SendChatTarget(ClientID, "Use \"/clan_create name\" to create a clan and \"/clan_invite name\" to invite a player");
	pGameServer->SendChatTarget(ClientID, "Use \"/clan_leave\" to leave your current clan. As a leader the clan will be closed!");
	pGameServer->SendChatTarget(ClientID, "Write \"/clan_leader\" to see all commands for a clan leader");
}

void CChatCommandsHandler::ComRules(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	pGameServer->SendChatTarget(ClientID, "1. Do not abuse buggs");
	pGameServer->SendChatTarget(ClientID, "2. Do not cheat");
	pGameServer->SendChatTarget(ClientID, "3. Be fair at events");
	pGameServer->SendChatTarget(ClientID, "Violating these rules may result in a punishment");
}

void CChatCommandsHandler::ComPages(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	char aBuf[256];
	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "Login to see your deathnote pages");
		return;
	}

	int Pages = pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Pages;
	str_format(aBuf, sizeof(aBuf), "You have %d pages in your Deathnote!", Pages);
	if (Pages > 0)
		str_fcat(aBuf, sizeof(aBuf), " Write \"/deathnote name\" to kill your enemies", Pages);

	pGameServer->SendChatTarget(ClientID, aBuf);
}

void CChatCommandsHandler::ComWeaponkit(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	char aBuf[256];
	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "Login to use a weaponkit");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip == false && pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_WeaponKits == 0)
	{
		pGameServer->SendChatTarget(ClientID, "You do not have any weaponkits");
		return;
	}


	CCharacter *pChr = pGameServer->GetPlayerChar(ClientID);
	if (pChr == 0x0 || pChr->IsAlive() == false)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be alive to use a weaponkit");
		return;
	}

	if(pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip == false && pChr->InSpawnZone() == false)
	{
		pGameServer->SendChatTarget(ClientID, "You have to be at the spawn to use a weaponkit");
		return;
	}

	bool WeaponsGiven = false;
	for (int i = 0; i < NUM_WEAPONS; i++)
	{
		if (i == WEAPON_NINJA || pChr->Weapons()[i] == true)
			continue;

		pChr->Weapons()[i] = true;
		WeaponsGiven = true;
	}

	if (WeaponsGiven == false)
	{
		pGameServer->SendChatTarget(ClientID, "You already have all weapons");
		return;
	}

	if(pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip == false)
		pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_WeaponKits--;

	str_format(aBuf, sizeof(aBuf), "Successfully used a weaponkit! %i kit%s remaining",
		pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_WeaponKits, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_WeaponKits == 1 ? "" : "s");
	pGameServer->SendChatTarget(ClientID, aBuf);
	pGameServer->AccountsHandler()->Save(ClientID);
}

void CChatCommandsHandler::ComLobby(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if(pGameServer->Server()->MoveLobby(ClientID) == false)
		pGameServer->SendChatTarget(ClientID, "You could not be moved to the lobby");
}

void CChatCommandsHandler::ComDetach(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{

	pGameServer->Server()->GetClientInfo(ClientID)->m_Detached = !pGameServer->Server()->GetClientInfo(ClientID)->m_Detached;
	if(pGameServer->Server()->GetClientInfo(ClientID)->m_Detached)
		pGameServer->SendChatTarget(ClientID, "Your player has been detached");
	else
		pGameServer->SendChatTarget(ClientID, "Your player has been attached");
}

void CChatCommandsHandler::ComLogin(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pResult->NumArguments() == 2)
		pGameServer->AccountsHandler()->Login(ClientID, pResult->GetString(0), pResult->GetString(1));
	else
		pGameServer->SendChatTarget(ClientID, "Use '/login name password' to login. If you dont have an account, you need to register one first with '/register'");
}

void CChatCommandsHandler::ComLogout(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in.");
		return;
	}

	if (pGameServer->m_apPlayers[ClientID] != 0x0)
		pGameServer->m_apPlayers[ClientID]->m_SubscribeEvent = false;

	pGameServer->AccountsHandler()->Logout(ClientID);
}

void CChatCommandsHandler::ComRegister(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pResult->NumArguments() == 2)
		pGameServer->AccountsHandler()->Register(ClientID, pResult->GetString(0), pResult->GetString(1));
	else
		pGameServer->SendChatTarget(ClientID, "Use '/register name password' to register a new account");
}

void CChatCommandsHandler::ComChangePassword(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	pGameServer->AccountsHandler()->ChangePassword(ClientID, pResult->GetString(0), pResult->GetString(1));
}

void CChatCommandsHandler::ComClanCreate(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	pGameServer->AccountsHandler()->ClanCreate(ClientID, pResult->GetString(0));
}

void CChatCommandsHandler::ComClanInvite(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	const char *pMessage = pResult->GetString(0);

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in into an account");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan == 0x0 ||
		str_comp(pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aLeader, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName) != 0)
	{
		pGameServer->SendChatTarget(ClientID, "You are currently not the leader of a clan");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_Members >= g_Config.m_SvClanMaxMebers)
	{
		pGameServer->SendChatTarget(ClientID, "Maximum number of members in a clan reached");
		return;
	}

	int InvitingID = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameServer->Server()->ClientIngame(i) == false || str_comp(pGameServer->Server()->ClientName(i), pMessage) != 0)
			continue;

		InvitingID = i;
		break;
	}

	if (InvitingID == -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Player '%s' not found", pMessage);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if (InvitingID == ClientID)
	{
		pGameServer->SendChatTarget(ClientID, "You cannot invite yourself");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(InvitingID)->m_LoggedIn == false)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s is not logged in into an account", pGameServer->Server()->ClientName(InvitingID));
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if(pGameServer->Server()->GetClientInfo(InvitingID)->m_AccountData.m_Level < NeededClanJoinLevel())
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s must be at least level %i to join a clan!", pGameServer->Server()->ClientName(InvitingID), NeededClanJoinLevel());
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if(pGameServer->Server()->GetClientInfo(InvitingID)->m_pClan != 0x0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s is already in clan '%s'!", pGameServer->Server()->ClientName(InvitingID), pGameServer->Server()->GetClientInfo(InvitingID)->m_pClan->m_aName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if (pGameServer->InquiriesHandler()->NewInquiryPossible(InvitingID) == false)
	{
		pGameServer->SendChatTarget(ClientID, "Claninvitation cannot be send right now. Try again in a few seconds");
		return;
	}

	unsigned char aData[INQUIRY_DATA_SIZE];
	mem_copy(aData, &pGameServer->Server()->GetClientInfo(ClientID)->m_pClan, sizeof(void *));
	mem_copy(aData + sizeof(void *), &ClientID, sizeof(int));

	CInquiry *pInquiry = new CInquiry(CAccountsHandler::ClanInvite, pGameServer->Server()->Tick() + pGameServer->Server()->TickSpeed() * 15, aData);
	pInquiry->AddOption("accept");
	pInquiry->AddOption("decline");

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s invited you to his clan '%s'",
		pGameServer->Server()->ClientName(ClientID), pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aName);
	pGameServer->InquiriesHandler()->NewInquiry(InvitingID, pInquiry, aBuf);

	str_format(aBuf, sizeof(aBuf), "Claninvite has been sent to %s", pGameServer->Server()->ClientName(InvitingID));
	pGameServer->SendChatTarget(ClientID, aBuf);
}

void CChatCommandsHandler::ComClanLeave(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in into an account");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan == 0x0)
	{
		pGameServer->SendChatTarget(ClientID, "You are currently in no clan");
		return;
	}

	if (str_comp(pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aLeader, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName) == 0)
	{
		unsigned char aData[INQUIRY_DATA_SIZE];
		mem_copy(aData, &pGameServer->Server()->GetClientInfo(ClientID)->m_pClan, sizeof(void *));
		CInquiry *pInquiry = new CInquiry(CAccountsHandler::ClanClose, pGameServer->Server()->Tick() + pGameServer->Server()->TickSpeed() * 10, aData);
		pInquiry->AddOption("yes");
		pInquiry->AddOption("no");
		pGameServer->InquiriesHandler()->NewInquiry(ClientID, pInquiry, "Are you sure you want to close your clan");
	}
	else
	{
		unsigned char aData[INQUIRY_DATA_SIZE];
		mem_copy(aData, &pGameServer->Server()->GetClientInfo(ClientID)->m_pClan, sizeof(void *));
		CInquiry *pInquiry = new CInquiry(CAccountsHandler::ClanLeave, pGameServer->Server()->Tick() + pGameServer->Server()->TickSpeed() * 10, aData);
		pInquiry->AddOption("yes");
		pInquiry->AddOption("no");
		pGameServer->InquiriesHandler()->NewInquiry(ClientID, pInquiry, "Are you sure you want to leave your clan");
	}
}

void CChatCommandsHandler::ComClanLeader(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if(pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan == 0x0 ||
		str_comp(pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aLeader, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName) != 0)
	{
		pGameServer->SendChatTarget(ClientID, "You are currently not the leader of a clan");
		return;
	}

	pGameServer->SendChatTarget(ClientID, "Use \"/clan_list\" to see all members");
	pGameServer->SendChatTarget(ClientID, "Use \"/clan_kick\" to kick a member");
}

void CChatCommandsHandler::ComClanList(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan == 0x0 ||
		str_comp(pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aLeader, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName) != 0)
	{
		pGameServer->SendChatTarget(ClientID, "You are currently not the leader of a clan");
		return;
	}

	pGameServer->AccountsHandler()->ClanList(ClientID, pGameServer->Server()->GetClientInfo(ClientID)->m_pClan);
}

void CChatCommandsHandler::ComClanKick(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	const char *pName = pResult->GetString(0);

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_pClan == 0x0 ||
		str_comp(pGameServer->Server()->GetClientInfo(ClientID)->m_pClan->m_aLeader, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_aName) != 0)
	{
		pGameServer->SendChatTarget(ClientID, "You are currently not the leader of a clan");
		return;
	}

	pGameServer->AccountsHandler()->ClanKick(ClientID, pGameServer->Server()->GetClientInfo(ClientID)->m_pClan, pName);
}

void CChatCommandsHandler::ComDeathnote(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	const char *pName = pResult->GetString(0);
	char aBuf[256];

	if (pGameServer->m_apPlayers[ClientID] == 0x0)
		return;

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in");
		return;
	}

	if(pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Pages <= 0)
	{
		pGameServer->SendChatTarget(ClientID, "You don't have any Deathnote pages!");
		return;
	}
	
	if (pGameServer->m_apPlayers[ClientID]->m_LastDeathnote + pGameServer->Server()->TickSpeed() * g_Config.m_SvDeathNoteCoolDown > pGameServer->Server()->Tick())
	{
		str_copy(aBuf, "You have to wait ", sizeof(aBuf));
		pGameServer->StringTime(pGameServer->m_apPlayers[ClientID]->m_LastDeathnote + pGameServer->Server()->TickSpeed() * g_Config.m_SvDeathNoteCoolDown, aBuf, sizeof(aBuf));
		str_append(aBuf, " until you can write down a player in your deathnote", sizeof(aBuf));
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	int KillingID = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameServer->Server()->ClientIngame(i) == false || str_comp(pGameServer->Server()->ClientName(i), pName) != 0)
			continue;

		KillingID = i;
		break;
	}

	if (KillingID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Player '%s' not found", pName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	CCharacter *pChr = pGameServer->GetPlayerChar(KillingID);
	if(pChr == 0x0 || pChr->IsAlive() == false)
	{
		str_format(aBuf, sizeof(aBuf), "Player '%s' is not alive", pName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if(pGameServer->m_apPlayers[ClientID]->GameMap() != pChr->GameMap())
	{
		str_format(aBuf, sizeof(aBuf), "Player '%s' is not on your map", pName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	if(pChr->GetPlayer()->CanBeDeathnoted() == false)
	{
		str_format(aBuf, sizeof(aBuf), "Player '%s' cannot be killed right now", pName);
		pGameServer->SendChatTarget(ClientID, aBuf);
		return;
	}

	pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Pages--;
	pChr->GetPlayer()->KillCharacter(WEAPON_WORLD);

	str_format(aBuf, sizeof(aBuf), "%s used a deathnote to kill you!", pGameServer->Server()->ClientName(ClientID));
	pGameServer->SendChatTarget(KillingID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Successfully killed %s. %d pages remaining", pName, pGameServer->Server()->GetClientInfo(ClientID)->m_AccountData.m_Pages);
	pGameServer->SendChatTarget(ClientID, aBuf);

	pGameServer->m_apPlayers[ClientID]->m_LastDeathnote = pGameServer->Server()->Tick();
	
}

void CChatCommandsHandler::ComSubscribe(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID)
{
	if (pGameServer->m_apPlayers[ClientID] == 0x0)
		return;

	if (pGameServer->m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS)
	{
		pGameServer->SendChatTarget(ClientID, "You cannot subscribe to an event as a spectator");
		return;
	}

	if (pGameServer->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pGameServer->SendChatTarget(ClientID, "You are not logged in");
		return;
	}

	bool Before = pGameServer->m_apPlayers[ClientID]->m_SubscribeEvent;
	pGameServer->m_apPlayers[ClientID]->GameMap()->ClientSubscribeEvent(ClientID);
	if (Before != pGameServer->m_apPlayers[ClientID]->m_SubscribeEvent)
	{
		if(pGameServer->m_apPlayers[ClientID]->m_SubscribeEvent == true)
			pGameServer->SendChatTarget(ClientID, "Successfully subscribed to the event");
		else
			pGameServer->SendChatTarget(ClientID, "Successfully unsubscribed to the event");
	}
}

void CChatCommandsHandler::Register(const char *pName, const char *pParams, int Flags, FChatCommandCallback pfnFunc, const char *pHelp)
{
	CChatCommand *pCommand = new CChatCommand();
	pCommand->m_pName = pName;
	pCommand->m_pParams = pParams;
	pCommand->m_Flags = Flags;
	pCommand->m_pfnCallback = pfnFunc;
	pCommand->m_pHelp = pHelp;
	m_lpChatCommands.add(pCommand);
}

void CChatCommandsHandler::Init()
{
	m_pConsole = GameServer()->Console();

	Register("help", "?s", 0, ComHelp,">--- BOOOM ---<");
	Register("info", "", 0, ComInfo, "Gamemode BW364 made by 13x37");
	Register("pause", "", CHATCMDFLAG_SPAMABLE, ComPause, "Detaches the camera from you tee to let you discover the map");
	Register("whisper", "r", 0, ComWhisper, "Personal message to anybody on this server");
	Register("account", "", 0, ComAccount, "Sends you all information about the account system");
	Register("emote", "s?si", 0, ComEmote, "Lets your tee show emotions (normal/pain/happy/surprise/angry/blink)");
	Register("clan", "", 0, ComClan, "Sends you all information about the clan system");
	Register("rules", "", 0, ComRules, "Informs you about the server rules");
	Register("pages", "", 0, ComPages, "Informs you the use of your deathnote");
	Register("weaponkit", "", 0, ComWeaponkit, "Use a weaponkit");
	Register("lobby", "", 0, ComLobby, "Moves you to the lobby");
	Register("detach", "", 0, ComDetach, "Allows you to be on a different map as your dummy");

	Register("cmdlist", "", CHATCMDFLAG_HIDDEN, ComCmdlist, "Sends you a list of all available chatcommands");
	Register("timeout", "", CHATCMDFLAG_HIDDEN, 0x0, "Timoutprotection not implemented");
	Register("w", "r", CHATCMDFLAG_HIDDEN, ComWhisper, "Personal message to anybody on this server");
	Register("login", "?ss", CHATCMDFLAG_HIDDEN, ComLogin, "Login into your Blockworlds account. For more informatino write /account");
	Register("logout", "", CHATCMDFLAG_HIDDEN, ComLogout, "Logout of your Blockworlds account. For more informatino write /account");
	Register("register", "?ss", CHATCMDFLAG_HIDDEN, ComRegister, "Register a new Blockworlds account. For more informatino write /account");
	Register("password", "ss", CHATCMDFLAG_HIDDEN, ComChangePassword, "Set a password to your Blockworlds account. For more informatino write /account");
	Register("clan_create", "r", CHATCMDFLAG_HIDDEN, ComClanCreate, "Create a new clan. For more informatino write /clan");
	Register("clan_invite", "r", CHATCMDFLAG_HIDDEN, ComClanInvite, "Invite a player to your clan. For more informatino write /clan");
	Register("clan_leave", "", CHATCMDFLAG_HIDDEN, ComClanLeave, "Leave your clan. For more informatino write /clan");
	Register("clan_leader", "", CHATCMDFLAG_HIDDEN, ComClanLeader, "Sends you all information about being a clan leader");
	Register("clan_list", "", CHATCMDFLAG_HIDDEN, ComClanList, "Sends a list of all clan members");
	Register("clan_kick", "r", CHATCMDFLAG_HIDDEN, ComClanKick, "Kicks a member out of your clan");
	Register("deathnote", "r", CHATCMDFLAG_HIDDEN, ComDeathnote, "Kill someone on your map via deathnote");
	Register("subscribe", "", CHATCMDFLAG_HIDDEN, ComSubscribe, "Subscribe to a event to take part");
	Register("sub", "", CHATCMDFLAG_HIDDEN, ComSubscribe, "Subscribe to a event to take part");
	Register("hub", "", CHATCMDFLAG_HIDDEN, ComLobby, "Moves you to the lobby");
	Register("weapons", "", CHATCMDFLAG_HIDDEN, ComWeaponkit, "Use a weaponkit");
}

bool CChatCommandsHandler::ProcessMessage(const char *pMsg, int ClientID)
{
	int Length = str_length(pMsg);
	
	if (Length == 0)
		return true;

	CConsole::CResult Result;
	if (Console()->ParseStart(&Result, pMsg, Length + 1) != 0)
		return true;

	CChatCommand *pCommand = 0x0;
	for (int i = 0; i < m_lpChatCommands.size(); i++)
	{
		CChatCommand *pTemp = m_lpChatCommands[i];
		if (str_comp_nocase(pTemp->m_pName, Result.m_pCommand) != 0
			|| (pTemp->m_Flags & CHATCMDFLAG_MOD && Server()->GetClientAuthed(ClientID) < IServer::AUTHED_MOD)
			|| (pTemp->m_Flags & CHATCMDFLAG_ADMIN && Server()->GetClientAuthed(ClientID) < IServer::AUTHED_ADMIN))
			continue;
		pCommand = pTemp;
		break;
	}

	if (pCommand == 0x0)
	{
		if (GameServer()->InquiriesHandler()->OnChatAnswer(ClientID, pMsg))
			return true;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Chatcommand '%s' not found. Use /cmdlist to see all available commands", Result.m_pCommand);
		GameServer()->SendChatTarget(ClientID, aBuf);
		return true;
	}

	if (pCommand->m_pfnCallback == 0x0)
		return true;

	str_format(Result.m_aAllStr, sizeof(Result.m_aAllStr), "%i", ClientID);
	str_format(Result.m_aMeStr, sizeof(Result.m_aMeStr), "%i", ClientID);
	if(Console()->ParseArgs(&Result, pCommand->m_pParams) != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Invalid arguments... Usage: %s %s", pCommand->m_pName, pCommand->m_pParams);
		GameServer()->SendChatTarget(ClientID, aBuf);
		return true;
	}

	pCommand->m_pfnCallback(&Result, GameServer(), ClientID);
	if (pCommand->m_Flags & CHATCMDFLAG_SPAMABLE)
		return false;
	return true;
}