
#include <engine/shared/config.h>
#include <engine/server.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>

#include "gameevent.h"

#define TIME_COUNTDOWN 119 // in seconds
#define MINIMUM_PLAYERS 8
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

	m_pGameWorld = pGameMap->CreateNewWorld(CGameWorld::WORLDTYPE_EVENT);;

	Init();
}

CGameEvent::~CGameEvent()
{
	GameMap()->DeleteWorld(m_pGameWorld);
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
			GameMap()->m_apPlayers[pChr->Core()->m_HookedPlayer]->m_SubscribeEvent == true)
		{
			pChr->Core()->m_HookState = HOOK_RETRACTED;
			pChr->Core()->m_HookedPlayer = -1;
		}


		if (GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
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

		bool CouldSubscribe = GameMap()->m_apPlayers[i]->TrySubscribeToEvent();
		if (g_Config.m_DbgGame == 1)
			CouldSubscribe = true;

		GameMap()->m_apPlayers[i]->MovePlayer(m_pGameWorld);

		if (GameMap()->m_apPlayers[i]->TryRespawnQuick() == false || CouldSubscribe == false)
		{
			GameMap()->m_apPlayers[i]->MovePlayer(GameMap()->MainWorld());
			GameMap()->m_apPlayers[i]->m_SubscribeEvent = false;
			continue;
		}

		GameMap()->m_apPlayers[i]->GetCharacter()->Freeze(3.0f);
	}

	m_Started = true;
	OnStarted();
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
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "Event %s starts in ", EventName());
		GameServer()->StringTime(m_CreateTick + Server()->TickSpeed() * TIME_COUNTDOWN, aBuf, sizeof(aBuf));
		str_fcat(aBuf, sizeof(aBuf), "\n%i participants (min %i)", m_NumParticipants, MINIMUM_PLAYERS);
		str_fcat(aBuf, sizeof(aBuf), "\nWrite '/sub' to take part!");

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameMap()->m_apPlayers[i] == 0x0)
				continue;

			GameServer()->BroadcastHandler()->AddSideCast(i, aBuf);
		}
	}
	else
	{
		if (m_Started == false && m_NumParticipants < MINIMUM_PLAYERS)
		{
			GameMap()->SendBroadcast("Not enough participants to start event");
			m_Ending = true;
		}
		else
		{
			if (m_Started == false)
				Start();

			OnTick();
			DoWinCheck();
		}
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

void CGameEvent::TestStartEvent()
{
	m_CreateTick = Server()->Tick() - Server()->TickSpeed() * TIME_COUNTDOWN;
	Start();
}

void CGameEvent::SetWinner(int ClientID)
{
	if (Server()->ClientIngame(ClientID) == false || GameMap()->m_apPlayers[ClientID] == 0x0)
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
	if (GameMap()->m_apPlayers[ClientID] == 0x0 || GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent == false)// other conditions (if buggy)
		return;

	GameMap()->m_apPlayers[ClientID]->MovePlayer(GameMap()->MainWorld());

	mem_copy(&GameMap()->m_apPlayers[ClientID]->m_SpawnState, &m_aCharStates[ClientID], sizeof(CCharState));
	GameMap()->m_apPlayers[ClientID]->m_UseSpawnState = true;

	GameMap()->m_apPlayers[ClientID]->KillCharacter();

	GameMap()->m_apPlayers[ClientID]->m_SubscribeEvent = false;
}

void CGameEvent::EndEvent()
{
	m_Ending = true;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i] == 0x0 || GameMap()->m_apPlayers[i]->m_SubscribeEvent == false)
			continue;

		ResumeClient(i);
	}

	GameMap()->SetEventCooldown();
}

void CGameEvent::PlayerKilled(int ClientID)
{
	if (m_Started == true && m_Ending == false)
		OnPlayerKilled(ClientID);
}

void CGameEvent::PlayerBlocked(int ClientID, vec2 Pos)
{
	if (m_Started == true && m_Ending == false)
		OnPlayerBlocked(ClientID, Pos);
}