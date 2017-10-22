

#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

#include <engine/server.h>
#include <engine/server/translator.h>

// this include should perhaps be removed
#include "entities/character.h"

struct CCharState
{
	bool m_Alive;
	vec2 m_Pos;
	bool m_aWeapons[NUM_WEAPONS];
	int m_ActiveWeapon;
	bool m_Endless;
	int m_NumJumps;
	bool m_Freezed;
};

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, CGameMap *pGameMap, int ClientID, int Team);
	~CPlayer();

	bool TryRespawnQuick();
	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void DoPlayerTuning();
	bool CanBeDeathnoted();
	bool HideIdentity();
	bool InEvent();
	bool TrySubscribeToEvent();
	bool CanChallengeMatch(int TargetID);
	void TryChallengeMatch(int TargetID, const char *pBlockpoints);
	static void MatchRequest(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);
	void MovePlayer(CGameWorld *pGameWorld);
	bool InGameMatch();

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();
	void OnCharacterDead();

	CTranslateItem *GetTranslateItem() { return &m_TranslateItem; };
	bool GetPause() const { return m_Pause; }
	void TogglePause() { m_Pause = !m_Pause; }
	IServer::CClientInfo *ClientInfo() { return m_pClientInfo; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CGameMap *GameMap() const { return m_pGameMap; }
	CGameWorld *GameWorld() const { return m_pGameWorld; }

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;
	bool m_SubscribeEvent;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_DieTick;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	//scoresystem -- do not use these variables
	int m_AttackedBy;
	int64 m_AttackedByTick;
	bool m_Blocked;
	int64 m_UnblockedTick;
	// -- 

	int64 m_CreateTick;

	int64 m_LastDeathnote;
	bool  m_UseSpawnState;
	CCharState m_SpawnState;
	int m_AutomuteScore;
	int64 m_LastExpAccountAlert;

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;
	CGameMap *m_pGameMap;
	CGameWorld *m_pGameWorld;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;
	CTranslateItem m_TranslateItem;
	IServer::CClientInfo *m_pClientInfo;

	CTuningParams m_Tuning;
	CTuningParams m_LastTuning;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;
	bool m_Pause;

	bool m_FirstInput;
};

#endif
