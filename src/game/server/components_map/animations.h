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

public:
	CMapAnimation(vec2 Pos, int64 Tick, CGameMap *pGameMap)
		: m_Pos(Pos), m_Tick(Tick), m_pGameMap(pGameMap) {}
	virtual ~CMapAnimation() {}

	virtual void Tick() = 0;
	virtual void Snap(int SnappingClient) = 0;
	virtual bool Done() = 0;
	vec2 GetPos() const { return m_Pos; }
	int64 GetTick() const { return m_Tick; }
	CGameMap *GameMap();
	IServer *Server();
	CGameContext *GameServer();
};

class CAnimationHandler : public CComponentMap
{
private:
	array<CMapAnimation *> m_lpAnimations;

public:
	CAnimationHandler();

	void Laserwrite(const char *pText, vec2 StartPos, float Size, int Ticks, bool Shotgun = false);
	void DoAnimation(vec2 Pos, int Index);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

	enum
	{
		ANIMATION_LOVE=0,
		ANIMATION_THUNDERSTORM,
	};
};