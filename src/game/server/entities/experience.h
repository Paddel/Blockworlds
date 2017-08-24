#pragma once

#include <game/server/entity.h>

class CExperience : public CEntity
{
public:
	CExperience(CGameWorld *pGameWorld, vec2 Pos, int Amount, int TargetID);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Amount;
	int m_TargetID;
};