#pragma once

#include <game/server/entities/npc.h>

class CClanNpc : public CNpc
{
private:
	char m_aName[32];
	int m_CheckingClan;
	int m_HighestLevel;
	int m_HighestExperience;
public:
	CClanNpc(CGameWorld *pGameWorld, int ClientID);

	virtual void Tick();
	virtual void Snap(int SnappingClient);
};
