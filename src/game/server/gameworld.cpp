
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/server/entity.h>
#include <game/server/entities/npc.h>

#include "gameworld.h"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld(int WorldType, CGameMap *pGameMap)
{
	m_WorldType = WorldType;
	m_pGameMap = pGameMap;
	m_pGameServer = pGameMap->GameServer();
	m_pServer = pGameMap->Server();

	m_ResetRequested = false;
	for(int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;

	m_Core.m_ppFirst = (CNpc **)&m_apFirstEntityTypes[ENTTYPE_NPC];
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		while(m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if(within_reach(pEnt->m_Pos, Pos, Radius + pEnt->m_ProximityRadius) == true)
		{
			if(ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

int CGameWorld::FindTees(vec2 Pos, float Radius, CEntity **ppEnts, int Max)
{
	int Num = 0;
	for (CEntity *pEnt = m_apFirstEntityTypes[ENTTYPE_CHARACTER]; pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if (within_reach(pEnt->m_Pos, Pos, Radius + pEnt->m_ProximityRadius) == true)
		{
			if (ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if (Num == Max)
				break;
		}
	}

	for (CEntity *pEnt = m_apFirstEntityTypes[ENTTYPE_NPC]; pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if (within_reach(pEnt->m_Pos, Pos, Radius + pEnt->m_ProximityRadius) == true)
		{
			if (ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if (Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for(CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
		dbg_assert(pCur != pEnt, "err");
#endif

	// insert it
	if(m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
		}
	}

	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if(pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	// update all objects
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Tick();
			pEnt = m_pNextTraverseEntity;
		}

	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->TickDefered();
			pEnt = m_pNextTraverseEntity;
		}

	RemoveEntities();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		if(within_reach(p->m_Pos, IntersectPos, p->m_ProximityRadius + Radius) == true)
		{
			if(within_reach(Pos0, IntersectPos, ClosestLen) == true)
			{
				float Len = distance(Pos0, IntersectPos);
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

CEntity *CGameWorld::IntersectTee(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CEntity *pClosest = 0;

	for (CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		if (p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		if(within_reach(p->m_Pos, IntersectPos, p->m_ProximityRadius + Radius) == true)
		{
			if (within_reach(Pos0, IntersectPos, ClosestLen) == true)
			{
				float Len = distance(Pos0, IntersectPos);
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	for (CNpc *p = (CNpc *)FindFirst(ENTTYPE_NPC); p; p = (CNpc *)p->TypeNext())
	{
		if (p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		if (within_reach(p->m_Pos, IntersectPos, p->m_ProximityRadius + Radius) == true)
		{
			if (within_reach(Pos0, IntersectPos, ClosestLen) == true)
			{
				float Len = distance(Pos0, IntersectPos);
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis, bool IntersectCollision)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		if (within_reach(Pos, p->m_Pos, p->m_ProximityRadius + Radius) == false)
			continue;

		if (within_reach(Pos, p->m_Pos, ClosestRange) == false)
			continue;

		if(IntersectCollision && GameMap()->Collision()->IntersectLine(Pos, p->m_Pos, 0x0, 0x0) != 0)
			continue;

		float Len = distance(Pos, p->m_Pos);
		ClosestRange = Len;
		pClosest = p;
	}

	return pClosest;
}

CEntity *CGameWorld::ClosestTee(vec2 Pos, float Radius, CEntity *pNotThis, bool IntersectCollision)
{
	// Find other players
	float ClosestRange = Radius * 2;
	CEntity *pClosest = 0;

	for (CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
	{
		if (p == pNotThis)
			continue;

		if (within_reach(Pos, p->m_Pos, p->m_ProximityRadius + Radius) == false)
			continue;

		if (within_reach(Pos, p->m_Pos, ClosestRange) == false)
			continue;

		if (IntersectCollision && GameMap()->Collision()->IntersectLine(Pos, p->m_Pos, 0x0, 0x0) != 0)
			continue;

		float Len = distance(Pos, p->m_Pos);
		ClosestRange = Len;
		pClosest = p;
	}

	for (CNpc *p = (CNpc *)FindFirst(ENTTYPE_CHARACTER); p; p = (CNpc *)p->TypeNext())
	{
		if (p == pNotThis)
			continue;

		if (within_reach(Pos, p->m_Pos, p->m_ProximityRadius + Radius) == false)
			continue;

		if (within_reach(Pos, p->m_Pos, ClosestRange) == false)
			continue;

		float Len = distance(Pos, p->m_Pos);
		ClosestRange = Len;
		pClosest = p;
	}

	return pClosest;
}
