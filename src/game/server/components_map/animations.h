#pragma once

#include <base/system.h>
#include <base/vmath.h>
#include <base/tl/array.h>
#include <game/server/component.h>

class CMapAnimation
{
private:
	vec2 m_Pos;
	int64 m_Tick;
	CGameMap *m_pGameMap;
	CGameWorld *m_pGameWorld;

public:
	CMapAnimation(vec2 Pos, int64 Tick, CGameMap *pGameMap, CGameWorld *pGameWorld)
		: m_Pos(Pos), m_Tick(Tick), m_pGameMap(pGameMap), m_pGameWorld(pGameWorld) {}
	virtual ~CMapAnimation() {}

	virtual void Tick() = 0;
	virtual void Snap(int SnappingClient) = 0;
	virtual bool Done() = 0;
	vec2 GetPos() const { return m_Pos; }
	int64 GetTick() const { return m_Tick; }
	CGameMap *GameMap();
	IServer *Server();
	CGameContext *GameServer();
	CGameWorld *GameWorld();
};

class CAnimationHandler : public CComponentMap
{
private:
	array<CMapAnimation *> m_lpAnimations;

public:
	CAnimationHandler();

	void Laserwrite(CGameWorld *pGameWorld, const char *pText, vec2 StartPos, float Size, int Ticks, bool Shotgun = false);
	void DoAnimation(CGameWorld *pGameWorld, vec2 Pos, int Index);
	void DoAnimationGundesign(CGameWorld *pGameWorld, vec2 Pos, int Index, vec2 Direction);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	enum
	{
		ANIMATION_LOVE = 0,
		ANIMATION_THUNDERSTORM,
		ANIMATION_STARS_CW,
		ANIMATION_STARS_CCW,
		ANIMATION_STARS_TOC,
	};
};