#pragma once

#include "animations.h"

class CAnimLove : public CMapAnimation
{
	static const int NUM = 4;
	int m_IDs[NUM];
	vec2 m_Pos[NUM];
	bool m_Spawned[NUM];

public:
	CAnimLove(vec2 Pos, int64 Tick, CGameMap *pGameMap) : CMapAnimation(Pos, Tick, pGameMap)
	{

		for (int i = 0; i < NUM; i++)
		{
			m_IDs[i] = Server()->SnapNewID();
			m_Spawned[i] = false;
		}

		m_Pos[0] = GetPos();
		m_Pos[1] = GetPos() + vec2(12.0f, 0);
		m_Pos[2] = GetPos() - vec2(12.0f, 0);
		m_Pos[3] = GetPos();
	}

	~CAnimLove()
	{
		for (int i = 0; i < NUM; i++)
			Server()->SnapFreeID(m_IDs[i]);
	}

	virtual void Tick()
	{

		for (int i = 0; i < NUM; i++)
		{
			if (GetTick() + Server()->TickSpeed() * 1.5f * (i / (float)NUM) + Server()->TickSpeed() <= Server()->Tick())
				m_Spawned[i] = false;
			else if (GetTick() + Server()->TickSpeed() * 1.5f * (i / (float)NUM) <= Server()->Tick() && m_Spawned[i] == false)
			{
				m_Spawned[i] = true;
				GameServer()->CreateSound(GameMap(), m_Pos[i], SOUND_PICKUP_HEALTH);
			}
		}

		for (int i = 0; i < NUM; i++)
		{
			if (m_Spawned[i] == false)
				continue;

			m_Pos[i] += vec2(0, -2.0f);
		}
	}

	virtual void Snap(int SnappingClient)
	{
		for (int i = 0; i < NUM; i++)
		{
			if (m_Spawned[i] == false)
				continue;

			CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_IDs[i], sizeof(CNetObj_Pickup)));
			if (pP)
			{
				pP->m_X = (int)m_Pos[i].x;
				pP->m_Y = (int)m_Pos[i].y;
				pP->m_Type = POWERUP_HEALTH;
				pP->m_Subtype = 0;
			}
		}
	}

	virtual bool Done()
	{
		return GetTick() + Server()->TickSpeed() * 3.0f < Server()->Tick();
	}
};

class CAnimThunderstorm : public CMapAnimation
{
	static const int NUM = 25;
	int m_IDs[NUM];
	vec2 m_Pos[NUM];
	bool m_Spawned[NUM];

public:
	CAnimThunderstorm(vec2 Pos, int64 Tick, CGameMap *pGameMap) : CMapAnimation(Pos, Tick, pGameMap)
	{

		for (int i = 0; i < NUM; i++)
		{
			m_IDs[i] = Server()->SnapNewID();
			m_Spawned[i] = false;
		}
	}

	~CAnimThunderstorm()
	{
		for (int i = 0; i < NUM; i++)
			Server()->SnapFreeID(m_IDs[i]);
	}

	virtual void Tick()
	{
		for (int i = 0; i < NUM; i++)
		{
			if (GetTick() + Server()->TickSpeed() * 3.0f * (i/(float)NUM) <= Server()->Tick() && m_Spawned[i] == false)
			{
				m_Spawned[i] = true;
				float Dist = rand() % 128 - 64;
				vec2 Dir = normalize(vec2((rand() % 20 - 10 ) / 10.0f, (rand() % 20 - 10) / 10.0f));
				m_Pos[i] = GetPos() + Dir * Dist;

				GameServer()->CreateSound(GameMap(), m_Pos[i], SOUND_BODY_LAND);
			}
		}
	}

	virtual void Snap(int SnappingClient)
	{
		for (int i = 0; i < NUM; i++)
		{
			if (m_Spawned[i] == false)
				break;

			CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
			if (!pObj)
				return;

			pObj->m_X = (int)m_Pos[i].x;
			pObj->m_Y = (int)m_Pos[i].y;
			pObj->m_FromX = (int)m_Pos[i].x;
			pObj->m_FromY = (int)m_Pos[i].y;
			pObj->m_StartTick = Server()->Tick() - 1;
		}
	}

	virtual bool Done()
	{
		return GetTick() + Server()->TickSpeed() * 3.0f < Server()->Tick();
	}
};