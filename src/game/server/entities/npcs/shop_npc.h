#pragma once

#include <game/server/entities/npc.h>

class CShopNpc : public CNpc
{
private:
	int m_Effect;

public:
	CShopNpc(CGameWorld *pGameWorld, int ClientID, int Effect);

	virtual void Snap(int SnappingClient);
};
