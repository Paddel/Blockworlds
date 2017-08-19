#pragma once

#include <game/voting.h>
#include <game/layers.h>
#include <game/collision.h>

#include "gameworld.h"
#include "eventhandler.h"

class CMap;
class IServer;
class CGameContext;
class CPlayer;
class CTranslateItem;
class IConsole;

class CGameMap
{
	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100, 100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

private:
	CMap *m_pMap;
	IServer *m_pServer;
	CGameContext *m_pGameServer;
	IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CGameWorld m_World;
	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];
	int m_RoundStartTick;

	//Voting
	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN = 0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool OnEntity(int Index, vec2 Pos);

	void UpdateVote();
public:
	CGameMap(CMap *pMap);
	~CGameMap();

	bool Init(CGameContext *pGameServer);
	bool FreePlayerSlot();
	bool PlayerJoin(int ClientID);
	void PlayerLeave(int ClientID);
	int FreeNpcSlot();
	CNpc *GetNpc(int ClientID);

	int NumTranslateItems();
	void FillTranslateItems(CTranslateItem *pTranslateItems);

	bool CanSpawn(int Team, vec2 *pOutPos);

	void SnapFakePlayer(int SnappingClient);
	void SnapGameInfo(int SnappingClient);
	void Snap(int SnappingClient);

	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void VoteEnforce(const char *pVote);
	int64 GetVoteCloseTime() const { return m_VoteCloseTime; };
	void SetVoteCreator(int ClientID) { m_VoteCreator = ClientID; };
	void SetVotePos(int Pos) { m_VotePos = Pos; };
	int GetVotePos() const { return m_VotePos; };
	void UpdateVotes() { m_VoteUpdate = true; };

	void Tick();
	void SendChat(int ChatterClientID, int Team, const char *pText);
	void OnClientEnter(int ClientID);

	CMap *Map() const { return m_pMap; };
	CGameContext *GameServer() const { return m_pGameServer; };
	IConsole *Console() const { return m_pConsole; };
	IServer *Server() const { return m_pServer; }
	CLayers *Layers() { return &m_Layers; };
	CCollision *Collision() { return &m_Collision; };
	CGameWorld *World() { return &m_World; };
	CEventHandler *Events() { return &m_Events; }
};