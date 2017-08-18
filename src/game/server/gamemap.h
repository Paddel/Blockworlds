#pragma once

#include <game/layers.h>
#include <game/collision.h>

#include "gameworld.h"
#include "eventhandler.h"

class CMap;
class IServer;
class CGameContext;
class CPlayer;
class CTranslateItem;

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
	CLayers m_Layers;
	CCollision m_Collision;
	CGameWorld m_World;
	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];
	int m_RoundStartTick;

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool OnEntity(int Index, vec2 Pos);

public:
	CGameMap(CMap *pMap);
	~CGameMap();

	bool Init(CGameContext *pGameServer);
	int FreePlayerSlot();
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

	CMap *Map() const { return m_pMap; };
	CGameContext *GameServer() const { return m_pGameServer; };
	IServer *Server() const { return m_pServer; }
	CLayers *Layers() { return &m_Layers; };
	CCollision *Collision() { return &m_Collision; };
	CGameWorld *World() { return &m_World; };
	CEventHandler *Events() { return &m_Events; }
};