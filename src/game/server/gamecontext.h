

#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <base/tl/array.h>

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/layers.h>
#include <game/voting.h>

#include <game/server/components_global/inquiries.h>
#include <game/server/components_global/accounts.h>
#include <game/server/components_global/chatcommands.h>
#include <game/server/components_global/votemenu.h>
#include <game/server/components_global/cosmetics.h>
#include <game/server/components_global/broadcast.h>

//TODO: remove includes

#include "player.h"

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;
	CChatCommandsHandler m_ChatCommandsHandler;
	CAccountsHandler m_AccountsHandler;
	CInquiriesHandler m_InquiriesHandler;
	CVoteMenuHandler m_VoteMenuHandler;
	CCosmeticsHandler m_CosmeticsHandler;
	CBroadcastHandler m_BroadcastHandler;

	class CComponentGlobal *m_apComponents[16];
	int m_NumComponents;

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConBlockmapSet(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConMutePlayer(IConsole::IResult *pResult, void *pUserData);
	static void ConUnmutePlayer(IConsole::IResult *pResult, void *pUserData);
	static void ConPassivePlayer(IConsole::IResult *pResult, void *pUserData);
	static void ConVIPSet(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAccountsystemupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainAccountForceupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainShutdownupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	void InitComponents();

	bool m_Resetting;
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CChatCommandsHandler *ChatCommandsHandler() { return &m_ChatCommandsHandler; }
	CAccountsHandler *AccountsHandler() { return &m_AccountsHandler; }
	CInquiriesHandler *InquiriesHandler() { return &m_InquiriesHandler; }
	CVoteMenuHandler *VoteMenuHandler() { return &m_VoteMenuHandler; }
	CCosmeticsHandler *CosmeticsHandler() { return &m_CosmeticsHandler; }
	CBroadcastHandler *BroadcastHandler() { return &m_BroadcastHandler; }

	CGameContext();
	~CGameContext();

	CPlayer *m_apPlayers[MAX_CLIENTS];

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);
	bool TryJoinTeam(int Team, int ClientID);
	const char *GetTeamName(int Team);
	void StringTime(int64 Tick, char *pSrc, int SrcSize);

	int m_LockTeams;

	int m_NumVoteOptions;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(CGameMap *pGameMap, vec2 Pos, float AngleMod, int Amount);
	void CreateExplosion(CGameMap *pGameMap, vec2 Pos, int Owner);
	void CreateHammerHit(CGameMap *pGameMap, vec2 Pos);
	void CreatePlayerSpawn(CGameMap *pGameMap, vec2 Pos);
	void CreateDeath(CGameMap *pGameMap, vec2 Pos, int Who);
	void CreateSound(CGameMap *pGameMap, vec2 Pos, int Sound, int Mask=-1);
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_SPEC=-1,
	};

	// network
	void SendChatTarget(int To, const char *pText);
	void SendChatClan(int ClientID, const char *pText);
	void SendChat(int ClientID, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);

	void SpreadTuningParams();
	void SendTuningParams(int ClientID);

	//handles
	void HandleInactive();
	void HandleBlockSystem();

	void DoGeneralTuning();

	//score system
	void BlockSystemFinish(int ClientID, vec2 Pos, bool Kill);
	void BlockSystemAttack(int Attacker, int Victim, bool Hook);

	void GiveExperience(int ClientID, int Amount);
	void SetLevel(int ClientID, int Level);
	void GiveClanExperience(int ClientID, int Amount);
	void SetClanLevel(int ClientID, int Level);

	static void ResultBuySkinmani(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	void OnBuySkinmani(int ClientID, const char *pName);
	static void ResultBuyGundesign(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	void OnBuyGundesign(int ClientID, const char *pName);
	static void ResultBuyKnockouteffect(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	void OnBuyKnockout(int ClientID, const char *pName);
	static void ResultBuyExtra(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	void OnBuyExtra(int ClientID, const char *pName);

	bool OnExtrasCallvote(int ClientID, const char *pCommand);

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID, bool MapSwitching);
	virtual void OnClientLeave(int ClientID, const char *pReason);
	virtual void OnClientMapchange(int ClientID, CGameMap *pGameMapFrom, CGameMap *pGameMapTo);
	virtual void OnClientDrop(int ClientID, const char *pReason, CGameMap *pGameMap, bool MapSwitching);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool GameMapInit(CGameMap *pMap);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	virtual bool CanShutdown();

	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *FakeVersion();
	virtual const char *NetVersion();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1<<ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
