#pragma once

#include <game/server/entities/npc.h>

class CShopNpc : public CNpc
{
private:
	char m_aEffectName[64];

public:
	CShopNpc(CGameWorld *pGameWorld, int ClientID, const char *pEffectName);

	virtual void Snap(int SnappingClient);
};
