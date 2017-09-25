
#include <game/server/gamemap.h>
#include <game/server/gameworld.h>
#include <game/server/gamecontext.h>

#include "gamematch.h"

#define MAX_SCORE 10

//handle scoresystem better

CGameMatch::CGameMatch(CGameMap *pGameMap, int Blockpoints)
{
	m_pGameMap = pGameMap;
	m_pGameServer = pGameMap->GameServer();
	m_pServer = pGameMap->Server();

	m_Blockpoints = Blockpoints;
	m_Pot = 0;

	m_pGameWorld = pGameMap->CreateNewWorld(CGameWorld::WORLDTYPE_GAMEMATCH);
	mem_zero(&m_aParticipants, sizeof(m_aParticipants));
	mem_zero(&m_Scores, sizeof(m_Scores));

	m_RoundStartTime = Server()->Tick();
}

CGameMatch::~CGameMatch()
{
	GameMap()->DeleteWorld(m_pGameWorld);
}

void CGameMatch::DoScoreBroadcast()
{
	char aBuf[128] = {};

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false)
			continue;

		str_fcat(aBuf, sizeof(aBuf), "%s: %i\n", Server()->ClientName(i), m_Scores[i]);
	}

	if (aBuf[0] != '\0')
		aBuf[str_length(aBuf) - 1] = '\0';

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false)
			continue;

		GameServer()->BroadcastHandler()->AddSideCast(i, aBuf);
	}
}

int CGameMatch::NumParticipants()
{
	int NumParticipants = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_aParticipants[i] == true)
			NumParticipants++;
	return NumParticipants;
}

void CGameMatch::UpdateParcitipants()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false)
			continue;

		if (GameMap()->m_apPlayers[i] == 0x0 ||
			GameMap()->m_apPlayers[i]->GameMap() != GameMap() ||
			GameMap()->m_apPlayers[i]->GameWorld() != m_pGameWorld)
			m_aParticipants[i] = false;

		if (GameMap()->m_apPlayers[i] != 0x0 && GameMap()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
		{
			GameMap()->m_apPlayers[i]->MovePlayer(GameMap()->MainWorld());
			m_aParticipants[i] = false;
		}
	}
}

void CGameMatch::ResetMatchup()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false || GameMap()->m_apPlayers[i] == 0x0)
			continue;

		if (GameMap()->m_apPlayers[i]->TryRespawnQuick() == false)
		{
			GameMap()->m_apPlayers[i]->MovePlayer(GameMap()->MainWorld());
			m_aParticipants[i] = false;
			continue;
		}

		GameMap()->m_apPlayers[i]->GetCharacter()->Freeze(3.0f);
	}

	m_RoundStartTime = Server()->Tick();
}

void CGameMatch::ScorePlayer(int ClientID)
{
	if (m_aParticipants[ClientID] == false)
		return;

	CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
	if ((pChr != 0x0 && pChr->IsAlive() && pChr->InFreezeZone() == false) || m_RoundStartTime + Server()->TickSpeed() * 3.5f > Server()->Tick())
	{
		m_Scores[ClientID]++;

		if (m_Scores[ClientID] >= MAX_SCORE)
		{
			SetWinner(ClientID);
			End();
			return;
		}
		else
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "Score for '%s'", Server()->ClientName(ClientID));
			SendChat(aBuf);
		}
	}
	else
		SendChat("Draw!");

	ResetMatchup();
}

void CGameMatch::SendChat(const char *pMessage)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false)
			continue;

		GameServer()->SendChatTarget(i, pMessage);
	}
}

void CGameMatch::SetWinner(int ClientID)
{
	char aBuf[128];
	if (m_aParticipants[ClientID] == false)
		return;

	str_format(aBuf, sizeof(aBuf), "%s won", Server()->ClientName(ClientID));

	if (m_Pot > 0 && GameMap()->m_apPlayers[ClientID] != 0x0 && Server()->GetClientInfo(ClientID)->m_LoggedIn == true)
	{
		Server()->GetClientInfo(ClientID)->m_AccountData.m_BlockPoints += m_Pot;
		str_fcat(aBuf, sizeof(aBuf), " and got %i blockpoints", m_Pot - m_Blockpoints);
	}

	str_append(aBuf, "!", sizeof(aBuf));
	SendChat(aBuf);
}

void CGameMatch::CheckDraws()
{
	bool Draw = true;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aParticipants[i] == false)
			continue;

		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if (pChr != 0x0 && pChr->IsAlive() &&
			(pChr->IsFreezed() == false || pChr->FreezeTick() + Server()->TickSpeed() * 5.0f > Server()->Tick()))
			Draw = false;
	}

	if (Draw == true)
	{
		SendChat("Draw!");
		ResetMatchup();
	}
}

void CGameMatch::PlayerBlocked(int ClientID, vec2 Pos)
{
	if (m_aParticipants[ClientID] == false)
		return;

	int RoundWinner = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (ClientID == i || m_aParticipants[i] == false)
			continue;

		RoundWinner = i;
		break;
	}

	if (RoundWinner != -1)
		ScorePlayer(RoundWinner);
}

void CGameMatch::PlayerKilled(int ClientID)
{
	if (m_aParticipants[ClientID] == false)
		return;

	int RoundWinner = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (ClientID == i || m_aParticipants[i] == false)
			continue;

		RoundWinner = i;
		break;
	}

	if(RoundWinner != -1)
		ScorePlayer(RoundWinner);
}

void CGameMatch::AddParticipant(int ClientID)
{
	m_aParticipants[ClientID] = true;
}

void CGameMatch::Tick()
{
	UpdateParcitipants();
	if (NumParticipants() == 1)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_aParticipants[i] == false)
				continue;
			SetWinner(i);
			break;
		}
		End();
		return;
	}
	else if (NumParticipants() == 0)
	{
		End();
		return;
	}

	DoScoreBroadcast();
	CheckDraws();
}

void CGameMatch::Start()
{
	char aBuf[128];

	mem_zero(&m_aCharStates, sizeof(m_aCharStates));

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0)
			continue;
		//release hooks
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if (pChr != 0x0 && pChr->IsAlive() &&
			pChr->Core()->m_HookedPlayer >= 0 && pChr->Core()->m_HookedPlayer < MAX_CLIENTS &&
			GameMap()->m_apPlayers[pChr->Core()->m_HookedPlayer] &&
			m_aParticipants[pChr->Core()->m_HookedPlayer] == true)
		{
			pChr->Core()->m_HookState = HOOK_RETRACTED;
			pChr->Core()->m_HookedPlayer = -1;
		}


		if (m_aParticipants[i] == false)
			continue;

		if (GameMap()->m_apPlayers[i]->GetCharacter())
		{
			m_aCharStates[i].m_Alive = true;
			m_aCharStates[i].m_Pos = GameMap()->m_apPlayers[i]->GetCharacter()->m_Pos;
			mem_copy(&m_aCharStates[i].m_aWeapons, GameMap()->m_apPlayers[i]->GetCharacter()->Weapons(), sizeof(m_aCharStates[i].m_aWeapons));
			m_aCharStates[i].m_ActiveWeapon = GameMap()->m_apPlayers[i]->GetCharacter()->GetActiveWeapon();
			m_aCharStates[i].m_Endless = GameMap()->m_apPlayers[i]->GetCharacter()->GetEndlessHook();
			m_aCharStates[i].m_NumJumps = GameMap()->m_apPlayers[i]->GetCharacter()->Core()->m_MaxAirJumps;
		}

		GameMap()->m_apPlayers[i]->MovePlayer(m_pGameWorld);

		if (GameMap()->m_apPlayers[i]->TryRespawnQuick() == false)
		{
			GameMap()->m_apPlayers[i]->MovePlayer(GameMap()->MainWorld());
			m_aParticipants[i] = false;
			continue;
		}

		GameMap()->m_apPlayers[i]->GetCharacter()->Freeze(3.0f);
	}

	UpdateParcitipants();
	if (NumParticipants() <= 1)
	{
		End();
		return;
	}
	else
	{
		if (m_Blockpoints > 0)
		{
			bool Error = false;
			for (int i = 0; i < MAX_CLIENTS; i++)
				if (m_aParticipants[i] == true &&
					(GameMap()->m_apPlayers[i] == 0x0 ||
					Server()->GetClientInfo(i)->m_LoggedIn == false ||
					Server()->GetClientInfo(i)->m_AccountData.m_BlockPoints < m_Blockpoints))
					Error = true;
			if (Error == true)
			{
				End();
				return;
			}

			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (m_aParticipants[i] == false)
					continue;
				Server()->GetClientInfo(i)->m_AccountData.m_BlockPoints -= m_Blockpoints;
				m_Pot += m_Blockpoints;
			}

			str_format(aBuf, sizeof(aBuf), "Blockpoints has been taken from your account and added to the pot!", m_Blockpoints);
			SendChat(aBuf);
		}

		str_format(aBuf, sizeof(aBuf), "Block your opponent %i times to win", MAX_SCORE);
		if (m_Pot > 0)
			str_fcat(aBuf, sizeof(aBuf), " and gain %i blockpoints", m_Pot - m_Blockpoints);
		SendChat(aBuf);
	}
}

void CGameMatch::ResumeClient(int ClientID)
{
	if (GameMap()->m_apPlayers[ClientID] == 0x0 || m_aParticipants[ClientID] == false)// other conditions (if buggy)
		return;

	GameMap()->m_apPlayers[ClientID]->MovePlayer(GameMap()->MainWorld());

	mem_copy(&GameMap()->m_apPlayers[ClientID]->m_SpawnState, &m_aCharStates[ClientID], sizeof(CCharState));
	GameMap()->m_apPlayers[ClientID]->m_UseSpawnState = true;

	GameMap()->m_apPlayers[ClientID]->KillCharacter();

	m_aParticipants[ClientID] = false;
}


void CGameMatch::End()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0 || m_aParticipants[i] == false)
			continue;

		ResumeClient(i);
	}

	GameMap()->DeleteGameMatch(this);
}