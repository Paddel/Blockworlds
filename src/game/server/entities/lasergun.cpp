
#include <engine/server.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>

#include "lasergun.h"

#define RANGE 700.0f

CLaserGun::CLaserGun(CGameWorld *pGameWorld, vec2 Pos, int ID, int Type)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASERGUN)
{
	m_Pos = Pos;
	m_LaserGunID = ID;
	m_Type = Type;

	m_ActiveTime = 0;
	m_FireTime = 0;

	GameWorld()->InsertEntity(this);
}

CLaserGun::~CLaserGun()
{
	for (int i = 0; i < m_lpBullets.size(); i++)
		Server()->SnapFreeID(m_lpBullets[i]->m_ID);
	m_lpBullets.delete_all();
}

void CLaserGun::SetActive(int Seconds)
{
	m_ActiveTime = Server()->Tick() + Server()->TickSpeed() * Seconds;
}

void CLaserGun::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CLaserGun::DoBulletSpawn()
{
	if (m_ActiveTime <= Server()->Tick() || m_FireTime > Server()->Tick())
		return;

	CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, RANGE, 0x0, true);
	if (pChr != 0x0)
	{
		CLaserBullet *pBullet = new CLaserBullet();
		pBullet->m_ID = Server()->SnapNewID();
		pBullet->m_Vel = normalize(pChr->m_Pos - m_Pos);
		pBullet->m_Pos = m_Pos;
		pBullet->m_SpawnTick = Server()->Tick();
		m_lpBullets.add(pBullet);
	}

	m_FireTime = Server()->Tick() + Server()->TickSpeed() * 0.3f;
}

void CLaserGun::DoBulletMove(int Index)
{
	CLaserBullet *pBullet = m_lpBullets[Index];
	vec2 NextPos = pBullet->m_Pos + pBullet->m_Vel;
	pBullet->m_Vel *= 1.1f;
	bool Remove = false;
	vec2 ExplodePos = NextPos;

	if (GameMap()->Collision()->IntersectLine(pBullet->m_Pos, NextPos, &ExplodePos, 0x0) ||
		pBullet->m_SpawnTick + Server()->TickSpeed() * 1.5 < Server()->Tick())
		Remove = true;

	CCharacter *pChr = GameMap()->World()->IntersectCharacter(pBullet->m_Pos, NextPos, 0.0f, ExplodePos);
	if (pChr)
	{
		if (m_Type == 0)
			pChr->Freeze(3.0f);
		else if (m_Type == 1)
			pChr->Unfreeze();
		Remove = true;
	}

	pBullet->m_Pos = NextPos;

	if (Remove)
	{
		if (m_Type == 2)
			GameServer()->CreateExplosion(GameMap(), ExplodePos, WEAPON_WORLD);

		Server()->SnapFreeID(pBullet->m_ID);
		delete pBullet;
		m_lpBullets.remove_index(Index);
		return;
	}
}

void CLaserGun::DoBulletMove()
{
	GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_ENTITIES);
	for (int i = 0; i < m_lpBullets.size(); i++)
		DoBulletMove(i);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_ENTITIES);
}

void CLaserGun::Tick()
{
	DoBulletSpawn();
	DoBulletMove();
}

void CLaserGun::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		pObj->m_X = (int)m_Pos.x;
		pObj->m_Y = (int)m_Pos.y;
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
		pObj->m_StartTick = Server()->Tick() - 1;
	}

	for (int i = 0; i < m_lpBullets.size(); i++)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_lpBullets[i]->m_ID, sizeof(CNetObj_Laser)));
		if (!pObj)
			continue;

		pObj->m_X = (int)m_lpBullets[i]->m_Pos.x;
		pObj->m_Y = (int)m_lpBullets[i]->m_Pos.y;
		pObj->m_FromX = (int)m_lpBullets[i]->m_Pos.x;
		pObj->m_FromY = (int)m_lpBullets[i]->m_Pos.y;
		pObj->m_StartTick = Server()->Tick() - 1;
	}
}