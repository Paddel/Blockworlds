
#include <game/server/gamecontext.h>

#include "lastmanblocking.h"

CLastManBlocking::CLastManBlocking(CGameMap *pGameMap)
	: CGameEvent(CGameEvent::EVENT_LASTMANBLOCKING, pGameMap)
{

}

void CLastManBlocking::DoWinCheck()
{
	if (m_NumParticipants <= 1)
	{
		if (m_NumParticipants == 1)
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (GameMap()->m_apPlayers[i] == 0x0 || GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
					continue;

				SetWinner(i);
			}
		}

		EndEvent();
	}
}

void CLastManBlocking::OnPlayerKilled(int ClientID)
{
	if (GameMap()->m_apPlayers[ClientID] == 0x0 || GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent == false)
		return;

	GameServer()->SendChatTarget(ClientID, "You are eliminated..");

	ResumeClient(ClientID);
}