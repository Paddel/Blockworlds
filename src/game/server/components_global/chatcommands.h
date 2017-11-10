#pragma once

#include <base/tl/array.h>
#include <engine/shared/console.h>
#include <game/server/component.h>

class CGameContext;
class IServer;

class CChatCommandsHandler : public CComponentGlobal
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
	IConsole *m_pConsole;
	array<CChatCommand *> m_lpChatCommands;

	static void ComHelp(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComCmdlist(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComPause(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComInfo(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComWhisper(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComConverse(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComAccount(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComEmote(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClan(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComRules(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComPages(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComWeaponkit(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComLobby(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComDetach(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComExp(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComChallengeMatch(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComGetCID(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComTele(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComPerformance(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComPerformanceInfo(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComLogin(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComLogout(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComRegister(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComChangePassword(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanCreate(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanInvite(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanLeave(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanExp(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanLeader(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanList(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComClanKick(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComDeathnote(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);
	static void ComSubscribe(CConsole::CResult *pResult, CGameContext *pGameServer, int ClientID);

	void Register(const char *pName, const char *pParams, int Flags, FChatCommandCallback pfnFunc, const char *pHelp);
public:
	CChatCommandsHandler();
	~CChatCommandsHandler();

	virtual void Init();
	bool ProcessMessage(const char *pMsg, int ClientID);

	IConsole *Console() const { return m_pConsole; };
};