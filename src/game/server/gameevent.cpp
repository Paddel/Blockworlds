

#include <engine/server.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>

#include "gameevent.h"

#define TIME_COUNTDOWN 119 // in seconds
#define MINIMUM_PLAYERS 2
#define MAXIMUM_TIME 60 * 5

CGameEvent::CGameEvent(int Type, CGameMap *pGameMap)
{
	m_Type = Type;
	m_pGameMap = pGameMap;
	m_pGameServer = pGameMap->GameServer();
	m_pServer = pGameMap->Server();

	m_CreateTick = Server()->Tick();
	m_Started = false;
	m_NumParticipants = 0;
	m_Ending = false;

	Init();
}

void CGameEvent::Init()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0)
			continue;
		GameMap()->m_apPlayers[i]->m_SubscribeEvent = false;
	}


	OnInit();
}

void CGameEvent::Start()
{
	OnStarted();
	GameMap()->SendBroadcast("");

	mem_zero(&m_CharState, sizeof(m_CharState));

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0 || GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
			continue;

		if (GameMap()->m_apPlayers[i]->GetCharacter())
		{
			m_CharState[i].m_Alive = true;
			m_CharState[i].m_Pos = GameMap()->m_apPlayers[i]->GetCharacter()->m_Pos;
			mem_copy(&m_CharState[i].m_aWeapons, GameMap()->m_apPlayers[i]->GetCharacter()->Weapons(), sizeof(m_CharState[i].m_aWeapons));
			m_CharState[i].m_ActiveWeapon = GameMap()->m_apPlayers[i]->GetCharacter()->GetActiveWeapon();
		}

		if (GameMap()->m_apPlayers[i]->TryRespawnEvent() == false)
			GameMap()->m_apPlayers[i]->m_SubscribeEvent = false;
	}

	m_Started = true;
}


void CGameEvent::UpdateParticipants()
{
	m_NumParticipants = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0 || GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
			continue;
		m_NumParticipants++;
	}
}

void CGameEvent::Tick()
{
	UpdateParticipants();

	if (OnCountdown() == true)
	{
		static int64 s_BroadcastUpdate = 0;
		if (s_BroadcastUpdate < Server()->Tick())
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Event %s starts in ", EventName());
			GameServer()->StringTime(m_CreateTick + Server()->TickSpeed() * TIME_COUNTDOWN, aBuf, sizeof(aBuf));
			str_fcat(aBuf, sizeof(aBuf), "\n                 %i/%i participants", m_NumParticipants, MINIMUM_PLAYERS);
			str_fcat(aBuf, sizeof(aBuf), "\n        Write '/sub' to take part!");
			GameMap()->SendBroadcast(aBuf);
			s_BroadcastUpdate = Server()->Tick() + Server()->TickSpeed() * 0.4f;
		}
	}
	else
	{
		if (m_Started == false)
			Start();

		OnTick();
		DoWinCheck();
	}

	if (m_CreateTick + Server()->TickSpeed() * (MAXIMUM_TIME + TIME_COUNTDOWN) < Server()->Tick())
	{
		GameMap()->SendBroadcast("Timelimit for event exceeded");
		EndEvent();
	}

	if(m_Ending)
		GameMap()->EndEvent();
}

bool CGameEvent::OnCountdown()
{
	return m_CreateTick + Server()->TickSpeed() * TIME_COUNTDOWN > Server()->Tick();
}

void CGameEvent::SetWinner(int ClientID)
{
	if (Server()->ClientIngame(ClientID) == false)
		return;

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		GameServer()->SendChatTarget(ClientID, "You are not logged in! Bad luck..");
		return;
	}

	GameServer()->SendChatTarget(ClientID, "+2 pages!");
	GameServer()->SendChatTarget(ClientID, "(+3) WeaponKits (use /weapons)!");
	GameServer()->SendChatTarget(ClientID, "+15 Blockpoints");
	GameServer()->SendChatTarget(ClientID, "Temoporary access to passivemode for 2Hours! (Anti WB)");

	Server()->GetClientInfo(ClientID)->m_AccountData.m_Pages += 2;
	Server()->GetClientInfo(ClientID)->m_AccountData.m_WeaponKits += 3;
	Server()->GetClientInfo(ClientID)->m_AccountData.m_BlockPoints += 15;
	Server()->GetClientInfo(ClientID)->m_InviolableTime = Server()->Tick() + Server()->TickSpeed() * 60 * 60 * 2;

	GameServer()->AccountsHandler()->Save(ClientID);

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s won the event", Server()->ClientName(ClientID));
	GameMap()->SendBroadcast(aBuf);
}

void CGameEvent::ClientSubscribe(int ClientID)
{
	if (OnCountdown() == false || GameMap()->m_apPlayers[ClientID] == 0x0)
		return;
	
	GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent = !GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent;
}

void CGameEvent::ResumeClient(int ClientID)
{
	if (GameMap()->m_apPlayers[ClientID] == 0x0 || GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent == false)
		return;

	//ForcePos
	if (m_CharState[ClientID].m_Alive == true)
	{
		GameMap()->m_apPlayers[ClientID]->ForceSpawn(m_CharState[ClientID].m_Pos);
		mem_copy(GameMap()->m_apPlayers[ClientID]->GetCharacter()->Weapons(), &m_CharState[ClientID].m_aWeapons, sizeof(m_CharState[ClientID].m_aWeapons));
		GameMap()->m_apPlayers[ClientID]->GetCharacter()->SetActiveWeapon(m_CharState[ClientID].m_ActiveWeapon);
	}
	else
		GameMap()->m_apPlayers[ClientID]->KillCharacter();

	GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent = false;
}

void CGameEvent::EndEvent()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0 || GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
			continue;

		ResumeClient(i);
	}

	m_Ending = true;
}

void CGameEvent::PlayerKilled(int ClientID, vec2 Pos)
{
	if (m_Started == true)
		OnPlayerKilled(ClientID, Pos);
}

void CGameEvent::PlayerBlocked(int ClientID, bool Dead, vec2 Pos)
{
	if (m_Started == true)
		PlayerBlocked(ClientID, Dead, Pos);
}