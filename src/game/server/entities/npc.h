#pragma once

#include <engine/server/translator.h>
#include <game/server/srv_gamecore.h>
#include <game/server/entity.h>

class CNpc : public CEntity
{
protected:
	static const int ms_PhysSize = 28;
	int m_ClientID;
	bool m_Alive;
	CTranslateItem m_TranslateItem;
	CSrvCharacterCore m_Core;

public:
	CNpc(CGameWorld *pGameWorld, int ClientID);
	virtual ~CNpc();

	void Spawn(vec2 Pos);

	int GetCID() const { return m_ClientID; }
	CTranslateItem *GetTranslateItem() { return &m_TranslateItem; };
	CSrvCharacterCore *Core() { return &m_Core; };

	virtual void Reset();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void Push(vec2 Force, int From) { m_Core.m_Vel += Force; }
};
