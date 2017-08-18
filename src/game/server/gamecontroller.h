

#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.h>

class CGameMap;

class CGameController
{
	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

protected:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }

	void HandleInactive();

public:

	CGameController(class CGameContext *pGameServer);
	~CGameController();

	void Tick();
	bool CanJoinTeam(int Team, int NotThisID);
	const char *GetTeamName(int Team);
};

#endif
