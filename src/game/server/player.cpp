

#include <new>
#include <engine/shared/config.h>
#include <game/server/gamemap.h>
#include <game/server/entities/npc.h>
#include <game/server/gameevent.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = Team;
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();
	m_Vote = 0;
	m_VotePos = 0;
	m_Pause = false;
	m_pClientInfo = Server()->GetClientInfo(ClientID);
	m_AttackedBy = -1;
	m_AttackedByTick = 0;
	m_Blocked = false;
	m_UnblockedTick = Server()->Tick();//hast to be at least 60 seconds on server to give exp
	m_LastDeathnote = Server()->Tick();
	m_SubscribeEvent = false;
	m_pGameMap = pGameServer->Server()->CurrentGameMap(m_ClientID);
	m_UseSpawnState = false;
	m_CreateTick = Server()->Tick();
	m_FirstInput = true;
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
	OnCharacterDead();
}

void CPlayer::Tick()
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	mem_copy(&m_Tuning, &GameMap()->World()->m_Core.m_Tuning, sizeof(m_Tuning));
	DoPlayerTuning();
	if (mem_comp(&m_Tuning, &m_LastTuning, sizeof(m_Tuning)) != 0)
	{
		GameServer()->SendTuningParams(m_ClientID);
		mem_copy(&m_LastTuning, &m_Tuning, sizeof(m_LastTuning));
	}

	Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		int Latency = Server()->GetClientLatency(m_ClientID);
		if(Latency > 0)
		{
			m_Latency.m_Accum += Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(((!m_pCharacter && m_Team == TEAM_SPECTATORS) || m_Pause) && m_SpectatorID == SPEC_FREEVIEW)
		m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

	if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
		m_Spawning = true;

	if(m_pCharacter)
	{
		if(m_pCharacter->IsAlive())
		{
			if(m_Pause == false)
				m_ViewPos = m_pCharacter->m_Pos;
		}
		else
		{
			delete m_pCharacter;
			m_pCharacter = 0;
			OnCharacterDead();
		}
	}
	else if(m_Spawning)
		TryRespawn();

	//automute score
	m_AutomuteScore *= 0.995f;
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if ((m_Team == TEAM_SPECTATORS || m_Pause) && m_SpectatorID != SPEC_FREEVIEW)
	{
		if (m_SpectatorID < MAX_CLIENTS)
		{
			if (GameServer()->GetPlayerChar(m_SpectatorID) != 0x0)
				m_ViewPos = GameServer()->GetPlayerChar(m_SpectatorID)->m_Pos;

			if (GameMap()->m_apPlayers[m_SpectatorID] == 0x0)// on another map or offline
				m_SpectatorID = SPEC_FREEVIEW;
		}
		else
		{
			CNpc *pNpc = GameMap()->GetNpc(m_SpectatorID);
			if (pNpc != 0x0)
				m_ViewPos = pNpc->Core()->m_Pos;
		}
	}

	//update translate item
	m_TranslateItem.m_Team = m_Team;
	m_TranslateItem.m_ClientID = m_ClientID;
	if (GetCharacter())
		m_TranslateItem.m_Pos = GetCharacter()->m_Pos;
	else
		m_TranslateItem.m_Pos = m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	int TranslatedID = Server()->Translate(SnappingClient, m_ClientID);
	if (TranslatedID == -1)
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, TranslatedID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	const char *pClan = "";
	if (ClientInfo()->m_LoggedIn == true && ClientInfo()->m_pClan != 0)
		pClan = ClientInfo()->m_pClan->m_aName;

	StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
	StrToInts(&pClientInfo->m_Clan0, 3, pClan);

	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, TranslatedID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[TranslatedID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = TranslatedID;
	pPlayerInfo->m_Score = 0;
	if(ClientInfo()->m_LoggedIn == true)
		pPlayerInfo->m_Score = ClientInfo()->m_AccountData.m_Level;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if (m_Pause && m_ClientID == SnappingClient)
		pPlayerInfo->m_Team = TEAM_SPECTATORS;

	GameServer()->CosmeticsHandler()->SnapSkinmani(m_ClientID, m_CreateTick, pClientInfo, pPlayerInfo);

	if (HideIdentity() && Server()->GetClientAuthed(SnappingClient) <= IServer::AUTHED_NO)
	{
		StrToInts(&pClientInfo->m_Name0, 4, "");
		StrToInts(&pClientInfo->m_Clan0, 3, "");

		pClientInfo->m_Country = -1;
		StrToInts(&pClientInfo->m_Skin0, 6, "default");
		pClientInfo->m_UseCustomColor = 0;
		pClientInfo->m_ColorBody = 0;
		pClientInfo->m_ColorFeet = 0;

		pPlayerInfo->m_Team = 1;
	}

	if(m_ClientID == SnappingClient && pPlayerInfo->m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, TranslatedID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = Server()->Translate(m_ClientID, m_SpectatorID);
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();
}

void CPlayer::DoPlayerTuning()
{//this is only for client predictions
	if (GetCharacter() && GetCharacter()->IsAlive())
	{
		CCharacter *pChr = GameMap()->World()->ClosestCharacter(GetCharacter()->m_Pos, 32.0f, GetCharacter());

		if (GetCharacter()->IsInviolable() || (pChr != 0x0 && pChr->IsInviolable()))
		{
			m_Tuning.m_PlayerCollision = 0;
			m_Tuning.m_PlayerHooking = 0;
		}
	}
}

bool CPlayer::CanBeDeathnoted()
{
	if (m_SubscribeEvent == true && GameMap()->GetEvent() != 0x0)
		return false;

	if (GameMap()->IsBlockMap() == false)
		return false;

	return true;
}

bool CPlayer::HideIdentity()
{
	if (m_SubscribeEvent == true && GameMap()->GetEvent() != 0x0 && GameMap()->GetEvent()->OnCountdown() == false)
		return true;
	return false;
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if (m_FirstInput)
	{
		if (m_pCharacter != 0x0 && NewInput->m_Hook == 1)
			m_pCharacter->Core()->m_HookState = HOOK_RETRACTED;
		m_FirstInput = false;
	}

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::OnCharacterDead()
{
	GameMap()->PlayerKilled(m_ClientID);
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = 0;
		OnCharacterDead();
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	// clamp the team
	Team = clamp(Team, (int)TEAM_SPECTATORS, 0);
	if(m_Team == Team)
		return;

	m_SubscribeEvent = false;
	m_Pause = false;

	char aBuf[512];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->GetTeamName(Team));
		GameServer()->SendChat(-1, aBuf);
	}

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	m_SpectatorID = SPEC_FREEVIEW;

	if (g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

bool CPlayer::TryRespawnEvent()
{
	vec2 SpawnPos;

	if (m_Team == TEAM_SPECTATORS)
		return false;

	if (!GameMap()->RaceComponents()->CanSpawn(1, &SpawnPos))
		return false;

	if (m_pCharacter != 0x0 && m_pCharacter->IsAlive() == false)
		return false;

	KillCharacter();

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(GameMap()->World());
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(GameMap(), SpawnPos);
	return true;
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if (m_Team == TEAM_SPECTATORS)
		return;

	if (!GameMap()->RaceComponents()->CanSpawn(0, &SpawnPos))
		return;


	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(GameMap()->World());
	m_pCharacter->Spawn(this, SpawnPos);

	if (m_UseSpawnState == true)
	{
		if (m_SpawnState.m_Alive == true)
		{
			mem_copy(m_pCharacter->Weapons(), &m_SpawnState.m_aWeapons, sizeof(m_SpawnState.m_aWeapons));
			m_pCharacter->SetActiveWeapon(m_SpawnState.m_ActiveWeapon);
			m_pCharacter->Core()->m_Pos = m_SpawnState.m_Pos;
			m_pCharacter->m_Pos = m_SpawnState.m_Pos;
			m_pCharacter->Unfreeze();
		}
		m_UseSpawnState = false;
	}

	GameServer()->CreatePlayerSpawn(GameMap(), m_pCharacter->Core()->m_Pos);
}
