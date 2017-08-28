#pragma once

#include <game/server/gameevent.h>

class CLastManBlocking : public CGameEvent
{
private:

public:
	CLastManBlocking(CGameMap *pGameMap);

	virtual void DoWinCheck();
	virtual void OnPlayerKilled(int ClientID, vec2 Pos);
	virtual const char *EventName() { return "LMB"; }
};