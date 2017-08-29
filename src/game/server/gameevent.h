#pragma once

#include <base/system.h>
#include <base/vmath.h>
#include <game/server/player.h>

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
	virtual ~CGameEvent() {}

	virtual void OnTick() {}
	virtual void Snap(int SnappingClient) {}
	virtual const char *EventName() = 0;

	virtual void OnPlayerBlocked(int ClientID, bool Dead, vec2 Pos) {}
	virtual void OnPlayerKilled(int ClientID) {}

	
	void Tick();
	bool OnCountdown();
	void SetWinner(int ClientID);
	void ClientSubscribe(int ClientID);
	void ResumeClient(int ClientID);
	void EndEvent();
	void PlayerKilled(int ClientID);
	void PlayerBlocked(int ClientID, bool Dead, vec2 Pos);

	CGameMap *GameMap() const { return m_pGameMap; }
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};