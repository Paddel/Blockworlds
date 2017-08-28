#pragma once

#include <base/system.h>
#include <base/vmath.h>

class CGameMap;
class CGameContext;
class IServer;

class CGameEvent
{
public:
	enum
	{
		EVENT_LASTMANBLOCKING,
		NUM_EVENTS,
	};

	struct CCharState
	{
		bool m_Alive;
		vec2 m_Pos;
		bool m_aWeapons[NUM_WEAPONS];
		int m_ActiveWeapon;
	};

private:
	CGameMap *m_pGameMap;
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	CCharState m_CharState[MAX_CLIENTS];
	int m_Type;
	int64 m_CreateTick;
	bool m_Started;
	bool m_Ending;

	void Init();
	void Start();
	void UpdateParticipants();
protected:
	int m_NumParticipants;

	virtual void OnInit() {}
	virtual void OnStarted() {}
	virtual void DoWinCheck() = 0;

public:
	CGameEvent(int Type, CGameMap *pGameMap);

	virtual void OnTick() {}
	virtual void Snap(int SnappingClient) {}
	virtual const char *EventName() = 0;

	virtual void OnPlayerBlocked(int ClientID, bool Dead, vec2 Pos) {}
	virtual void OnPlayerKilled(int ClientID, vec2 Pos) {}

	
	void Tick();
	bool OnCountdown();
	void SetWinner(int ClientID);
	void ClientSubscribe(int ClientID);
	void ResumeClient(int ClientID);
	void EndEvent();
	void PlayerKilled(int ClientID, vec2 Pos);
	void PlayerBlocked(int ClientID, bool Dead, vec2 Pos);

	CGameMap *GameMap() const { return m_pGameMap; }
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};