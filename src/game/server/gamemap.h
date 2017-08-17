#pragma once

class CMap;
class CGameContext;

#include <game/layers.h>
#include <game/collision.h>

#include "gameworld.h"
#include "eventhandler.h"

class CGameMap
{
	friend class CGameController;
private:
	CMap *m_pMap;
	CGameContext *m_pGameServer;
	CLayers m_Layers;
	CCollision m_Collision;
	CGameWorld m_World;
	CEventHandler m_Events;
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];

public:
	CGameMap(CMap *pMap);
	~CGameMap();

	bool Init(CGameContext *pGameServer);

	CMap *Map() const { return m_pMap; };
	CGameContext *GameServer() const { return m_pGameServer; };
	CLayers *Layers() { return &m_Layers; };
	CCollision *Collision() { return &m_Collision; };
	CGameWorld *World() { return &m_World; };
	CEventHandler *Events() { return &m_Events; }
};