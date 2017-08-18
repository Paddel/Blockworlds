

#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include "entities/pickup.h"
#include "gamecontroller.h"
#include "gamecontext.h"
#include "gamemap.h"

CGameController::CGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CGameController::~CGameController()
{
}

void CGameController::HandleInactive()
{
	// check for inactive players
	if (g_Config.m_SvInactiveKickTime > 0)
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
#ifdef CONF_DEBUG
			if (g_Config.m_DbgDummies)
			{
				if (i >= MAX_CLIENTS - g_Config.m_DbgDummies)
					break;
			}
#endif
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !Server()->IsAuthed(i))
			{
				if (Server()->Tick() > GameServer()->m_apPlayers[i]->m_LastActionTick + g_Config.m_SvInactiveKickTime*Server()->TickSpeed() * 60)
				{
					switch (g_Config.m_SvInactiveKick)
					{
					case 0:
					{
						// move player to spectator
						GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
					}
					break;
					case 1:
					{
						// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
						int Spectators = 0;
						for (int j = 0; j < MAX_CLIENTS; ++j)
							if (GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS)
								++Spectators;
						if (Spectators >= g_Config.m_SvSpectatorSlots)
							Server()->Kick(i, "Kicked for inactivity");
						else
							GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
					}
					break;
					case 2:
					{
						// kick the player
						Server()->Kick(i, "Kicked for inactivity");
					}
					}
				}
			}
		}
	}
}

void CGameController::Tick()
{
	HandleInactive();
}

bool CGameController::CanJoinTeam(int Team, int NotThisID)
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
}

const char *CGameController::GetTeamName(int Team)
{
	if (Team == 0)
		return "game";
	return "spectators";
}