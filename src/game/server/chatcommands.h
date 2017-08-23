#pragma once

#include <base/tl/array.h>
#include <engine/shared/console.h>

class CGameContext;
class IServer;

class CChatCommandsHandler
{
	typedef void(*FChatCommandCallback)(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);
	
	enum
	{
		CHATCMDFLAG_HIDDEN=1, // not shown in cmdlist
		CHATCMDFLAG_MOD=2,
		CHATCMDFLAG_ADMIN=4,
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

	static void ComHelp(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);
	static void ComCmdlist(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);
	static void ComPause(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);
	static void ComInfo(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);
	static void ComWhisper(CConsole::CResult *pResult, CGameContext *pGameContext, int ClientID);

	void Register(const char *pName, const char *pParams, int Flags, FChatCommandCallback pfnFunc, const char *pHelp);
public:
	CChatCommandsHandler();
	~CChatCommandsHandler();

	void Init(CGameContext *pGameServer);
	void ProcessMessage(const char *pMsg, int ClientID);

	CGameContext *GameServer() const { return m_pGameServer; };
	IConsole *Console() const { return m_pConsole; };
	IServer *Server() const { return m_pServer; };
};