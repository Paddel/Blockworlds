
#include <game/version.h>
#include <game/server/gamecontext.h>

#include "chatcommands.h"

CChatCommandsHandler::CChatCommandsHandler()
{
	m_pGameServer = 0x0;
	m_pConsole = 0x0;
}

CChatCommandsHandler::~CChatCommandsHandler()
{
	m_lpChatCommands.delete_all();
}

void CChatCommandsHandler::ComHelp(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID)
{
	CChatCommandsHandler *pThis = pGameContext->ChatCommandsHandler();
	char aBuf[256];

	if (pResult->NumArguments() == 0)
	{
		pGameContext->SendChatTarget(ClientID, "Use this command to get help to a chatcommand. Possible input: /help cmdlist");
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
		pGameContext->SendChatTarget(ClientID, aBuf);
		return;
	}

	pGameContext->SendChatTarget(ClientID, pCommand->m_pHelp);
}

void CChatCommandsHandler::ComCmdlist(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID)
{
	CChatCommandsHandler *pThis = pGameContext->ChatCommandsHandler();
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

	pGameContext->SendChatTarget(ClientID, aBuf);
}

void CChatCommandsHandler::ComPause(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID)
{
	CPlayer *pPlayer = pGameContext->m_apPlayers[ClientID];
	if (pPlayer == 0x0)
		return;

	pPlayer->TogglePause();
}

void CChatCommandsHandler::ComInfo(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID)
{
	char aBuf[512];
	pGameContext->SendChatTarget(ClientID, "Blockworlds made by 13x37");
	str_format(aBuf, sizeof(aBuf), "Version %s", GAMEMODE_VERSION);
	pGameContext->SendChatTarget(ClientID, aBuf);
	str_format(aBuf, sizeof(aBuf), "Based on Teeworlds %s", GAME_VERSION);
	pGameContext->SendChatTarget(ClientID, aBuf);
	pGameContext->SendChatTarget(ClientID, "Hosted by Google");
	pGameContext->SendChatTarget(ClientID, "www.13x37.com");
}

void CChatCommandsHandler::ComWhisper(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID)
{
	const char *pMessage = pResult->GetString(0);
	int Length = str_length(pMessage);

	int NumPersons = 0;
	int TargetID = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pGameContext->Server()->ClientIngame(i) == false || i == ClientID)
			continue;

		const char *pReceiverName = pGameContext->Server()->ClientName(i);
		int ReceiverLength = str_length(pReceiverName);
		if (ReceiverLength > Length || pMessage[ReceiverLength] != ' ' ||
			str_comp_nocase_num(pReceiverName, pMessage, ReceiverLength) != 0)
			continue;

		NumPersons++;
		TargetID = i;
	}

	if (NumPersons == 0)
	{
		if (str_length(pGameContext->Server()->ClientName(ClientID)) < Length
			&& str_comp_nocase_num(pGameContext->Server()->ClientName(ClientID), pMessage, str_length(pGameContext->Server()->ClientName(ClientID))) == 0)
			pGameContext->SendChatTarget(ClientID, "You cannot whisper to yourself");
		else
			pGameContext->SendChatTarget(ClientID, "Invalid receiver name");
	}
	else if (NumPersons > 1)
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%i possible receivers found");
		pGameContext->SendChatTarget(ClientID, aBuf);
	}
	else
	{
		char aBuf[512];
		pMessage += str_length(pGameContext->Server()->ClientName(TargetID)) + 1;
		str_format(aBuf, sizeof(aBuf), "[-> %s] %s", pGameContext->Server()->ClientName(TargetID), pMessage);
		pGameContext->SendChatTarget(ClientID, aBuf);
		str_format(aBuf, sizeof(aBuf), "[<- %s] %s", pGameContext->Server()->ClientName(ClientID), pMessage);
		pGameContext->SendChatTarget(TargetID, aBuf);
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

void CChatCommandsHandler::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pConsole = pGameServer->Console();

	Register("help", "?s", 0, ComHelp,">--- BOOOM ---<");
	Register("info", "", 0, ComInfo, "Gamemode BW364 made by 13x37");
	Register("pause", "", CHATCMDFLAG_SPAMABLE, ComPause, "Detaches the camera from you tee to let you discover the map");
	Register("whisper", "r", 0, ComWhisper, "Personal message to anybody on this server");

	Register("cmdlist", "", CHATCMDFLAG_HIDDEN, ComCmdlist, "Sends you a list of all available chatcommands");
	Register("timeout", "", CHATCMDFLAG_HIDDEN, 0x0, "Timoutprotection not implemented");
	Register("w", "r", CHATCMDFLAG_HIDDEN, ComWhisper, "Personal message to anybody on this server");
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