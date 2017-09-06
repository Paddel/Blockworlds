#pragma once

#include <base/tl/array.h>
#include <game/server/entity.h>

class CLaserGun : public CEntity
{
	struct CLaserBullet
	{
		vec2 m_Pos;
		vec2 m_Vel;
		int64 m_SpawnTick;
		int m_ID;
	};

private:
	int m_LaserGunID;
	int m_Type;
	int64 m_ActiveTime;
	int64 m_FireTime;
	array <CLaserBullet *> m_lpBullets;

	void DoBulletSpawn();
	void DoBulletMove(int Index);
	void DoBulletMove();

public:
	CLaserGun(CGameWorld *pGameWorld, vec2 Pos, int ID, int Type);
	~CLaserGun();

	void SetActive(int Seconds);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	int GetID() const { return m_LaserGunID; }
};