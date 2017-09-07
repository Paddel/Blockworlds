
#pragma once

#include <game/gamecore.h>

class CNpc;

class CSrvWorldCore
{
public:
	CSrvWorldCore()
	{
		m_ppFirst = 0x0;
		mem_zero(m_apCharacters, sizeof(m_apCharacters));
	}

	CTuningParams m_Tuning;
	class CSrvCharacterCore *m_apCharacters[MAX_CLIENTS];
	CNpc **m_ppFirst;

	void NextCoreItem(CSrvCharacterCore **ppCore, int *pClientID, CNpc **pNpc);
	CSrvCharacterCore *SearchCore(int ClientID);
};

class CSrvCharacterCore
{
protected:
	CSrvWorldCore *m_pWorld;
	CCollision *m_pCollision;

public:
	vec2 m_Pos;
	vec2 m_Vel;

	vec2 m_HookPos;
	vec2 m_HookDir;
	int m_HookTick;
	int m_HookState;
	int m_HookedPlayer;
	bool m_Inviolable;
	bool m_EndlessHook;

	int m_Jumped;
	int m_AirJumps;
	int m_MaxAirJumps;

	int m_Direction;
	int m_Angle;
	CNetObj_PlayerInput m_Input;

	int m_TriggeredEvents;

	void Init(CSrvWorldCore *pWorld, CCollision *pCollision);
	void Reset();
	void Tick(bool UseInput);
	void TickPredict(bool UseInput);
	void Move();

	void Read(const CNetObj_CharacterCore *pObjCore);
	void Write(CNetObj_CharacterCore *pObjCore);
	void Quantize();
};
