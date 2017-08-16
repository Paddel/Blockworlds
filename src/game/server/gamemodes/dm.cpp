

#include "dm.h"


CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "DM";
}

void CGameControllerDM::Tick()
{
	IGameController::Tick();
}
