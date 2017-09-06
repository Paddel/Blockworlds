#pragma once

#include <game/server/entity.h>

class CDragger : public CEntity
{
private:
	int m_Target;
	vec2 m_DragPos;
	float m_Strength;

	void UpdateTarget();
public:
	CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};