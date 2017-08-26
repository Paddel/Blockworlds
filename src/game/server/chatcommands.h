#pragma once

#include <base/tl/array.h>
#include <engine/shared/console.h>

class CGameContext;
class IServer;

class CChatCommandsHandler
{
	typedef void(*FChatCommandCallback)(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	
	enum
	{
		CHATCMDFLAG_HIDDEN=1, // not shown in cmdlist
		CHATCMDFLAG_MOD=2,
		CHATCMDFLAG_ADMIN=4,
		CHATCMDFLAG_SPAMABLE=8,
	};

	struct CChatCommand
	{
		const char *m_pName;
		const char *m_pParams;
		const char *m_pHelp;
		int m_Flags;
		FChatCommandCallback m_pfnCallback;
	};

private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	IConsole *m_pConsole;
	array<CChatCommand *> m_lpChatCommands;

	static void ComHelp(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComCmdlist(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComPause(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComInfo(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComWhisper(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComAccount(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComEmote(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClan(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComRules(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComLogin(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComLogout(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComRegister(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComChangePassword(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanCreate(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanInvite(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanLeave(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanLeader(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanList(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanKick(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);

	void Register(const char *pName, const char *pParams, int Flags, FChatCommandCallback pfnFunc, const char *pHelp);
public:
	CChatCommandsHandler();
	~CChatCommandsHandler();

	void Init(CGameContext *pGameServer);
	bool ProcessMessage(const char *pMsg, int ClientID);

	CGameContext *GameServer() const { return m_pGameServer; };
	IConsole *Console() const { return m_pConsole; };
	IServer *Server() const { return m_pServer; };
};