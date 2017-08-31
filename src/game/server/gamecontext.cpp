

#include <new>
#include <base/math.h>
#include <engine/server/map.h>
#include <engine/shared/config.h>
#include <engine/mapengine.h>
#include <engine/console.h>
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include <game/server/entities/experience.h>

#include "component.h"
#include "balancing.h"
#include "gamemap.h"
#include "gamecontext.h"

CGameContext::CGameContext()
{
	m_Resetting = 0;
	m_pServer = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;

	m_pVoteOptionHeap = new CHeap();
}

CGameContext::~CGameContext()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	if(!m_Resetting)
		delete m_pVoteOptionHeap;
}

void CGameContext::InitComponents()
{
	m_NumComponents = 0;

	m_apComponents[m_NumComponents++] = &m_ChatCommandsHandler;
	m_apComponents[m_NumComponents++] = &m_AccountsHandler;
	m_apComponents[m_NumComponents++] = &m_InquiriesHandler;
	m_apComponents[m_NumComponents++] = &m_VoteMenuHandler;
	m_apComponents[m_NumComponents++] = &m_CosmeticsHandler;

	for (int i = 0; i < m_NumComponents; i++)
	{
		m_apComponents[i]->m_pGameServer = this;
		m_apComponents[i]->m_pServer = Server();
	}
}

class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

bool CGameContext::TryJoinTeam(int Team, int ClientID)
{
	if (m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS &&
		Team == 0 && g_Config.m_SvAccountForce == 1 && Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		SendBroadcast("Register/Login first to join the game", ClientID);
		return false;
	}

	return true;
}

const char *CGameContext::GetTeamName(int Team)
{
	if (Team == 0)
		return "game";
	return "spectators";
}

void CGameContext::StringTime(int64 Tick, char *pSrc, int SrcSize)
{
	int64 DeltaTime = Server()->Tick() - Tick;
	if (Server()->Tick() < Tick)
		DeltaTime = Tick - Server()->Tick();

	int Days = DeltaTime / Server()->TickSpeed() / 60 / 60 / 24;
	int Hours = DeltaTime / Server()->TickSpeed() / 60 / 60 - Days * 24;
	int Minutes = DeltaTime / Server()->TickSpeed() / 60 - Hours * 60 - Days * 60 * 24;
	int Seconds = DeltaTime / Server()->TickSpeed() - Minutes * 60 - Hours * 60 * 60 - Days * 60 * 60 * 24;
	int ConnectCount = 0 + (Days > 0 ? 1 : 0) + (Hours > 0 ? 1 : 0) + (Minutes > 0 ? 1 : 0) + (Seconds > 0 ? 1 : 0);

	if (Days > 0)
	{
		str_fcat(pSrc, SrcSize, "%i day%s", Days, Days > 1 ? "s" : "");
		ConnectCount--;
	}

	if (Hours > 0)
	{
		if(Days > 0)
			str_fcat(pSrc, SrcSize, "%s", ConnectCount == 1 ? " and " : ", ");
		str_fcat(pSrc, SrcSize, "%i hour%s", Hours, Hours > 1 ? "s" : "");
		ConnectCount--;
	}
	if (Minutes > 0)
	{
		if (Days > 0 || Hours > 0)
			str_fcat(pSrc, SrcSize, "%s", ConnectCount == 1 ? " and " : ", ");
		str_fcat(pSrc, SrcSize, "%i minute%s",Minutes, Minutes > 1 ? "s" : "");
		ConnectCount--;
	}
	if (Seconds > 0)
	{
		if (Days > 0 || Hours > 0 || Minutes > 0)
			str_fcat(pSrc, SrcSize, "%s", ConnectCount == 1 ? " and " : ", ");
		str_fcat(pSrc, SrcSize, "%i second%s", Seconds, Seconds > 1 ? "s" : "");
		ConnectCount--;
	}
}

void CGameContext::CreateDamageInd(CGameMap *pGameMap, vec2 Pos, float Angle, int Amount)
{
	float a = 3 * 3.14159f / 2 + Angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(Amount+2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)pGameMap->Events()->Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd));
		if(pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f*256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(CGameMap *pGameMap, vec2 Pos)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)pGameMap->Events()->Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}


void CGameContext::CreateExplosion(CGameMap *pGameMap, vec2 Pos, int Owner, int Weapon, bool NoDamage)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (!NoDamage)
	{
		// deal damage
		CEntity *apEnts[MAX_CLIENTS];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = pGameMap->World()->FindTees(Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - Pos;
			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if((int)Dmg)
				apEnts[i]->Push(ForceDir*Dmg*2, Owner);
		}
	}
}

void CGameContext::CreatePlayerSpawn(CGameMap *pGameMap, vec2 Pos)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)pGameMap->Events()->Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn));
	if(ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(CGameMap *pGameMap, vec2 Pos, int ClientID)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)pGameMap->Events()->Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(CGameMap *pGameMap, vec2 Pos, int Sound, int Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)pGameMap->Events()->Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask);
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if(Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if(Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}


void CGameContext::SendChatTarget(int To, const char *pText)
{
	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
}

void CGameContext::SendChatClan(int ChatterClientID, const char *pText)
{
	if (Server()->GetClientInfo(ChatterClientID)->m_LoggedIn == false)
	{
		SendChatTarget(ChatterClientID, "Log in to use clan chat");
		return;
	}

	if (Server()->GetClientInfo(ChatterClientID)->m_pClan == 0x0)
	{
		SendChatTarget(ChatterClientID, "You are currently not member of a clan");
		return;
	}


	if (g_Config.m_Debug)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%d(clan):%s: %s", ChatterClientID, Server()->ClientName(ChatterClientID), pText);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "clanchat", aBuf);
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->GetClientInfo(i)->m_LoggedIn && Server()->GetClientInfo(ChatterClientID)->m_pClan == Server()->GetClientInfo(i)->m_pClan)
		{
			CNetMsg_Sv_Chat Msg;
			Msg.m_Team = 1;
			Msg.m_ClientID = ChatterClientID;
			Msg.m_pMessage = pText;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, i);
		}
	}
}

void CGameContext::SendChat(int ChatterClientID, const char *pText)
{
	if (g_Config.m_Debug)
	{
		char aBuf[256];
		if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
			str_format(aBuf, sizeof(aBuf), "%d:%s: %s", ChatterClientID, Server()->ClientName(ChatterClientID), pText);
		else
			str_format(aBuf, sizeof(aBuf), "*** %s", pText);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "chat", aBuf);
	}

	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = ChatterClientID;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}


void CGameContext::SendBroadcast(const char *pText, int ClientID)
{
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SpreadTuningParams()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
		SendTuningParams(i);
}

void CGameContext::SendTuningParams(int ClientID)
{
	if (m_apPlayers[ClientID] == 0x0 || m_apPlayers[ClientID]->m_IsReady == false)
		return;

	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)m_apPlayers[ClientID]->Tuning();
	for(unsigned i = 0; i < sizeof(CTuningParams)/sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::HandleInactive()
{
	// check for inactive players
	if (g_Config.m_SvInactiveKickTime > 0)
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && Server()->GetClientAuthed(i) == IServer::AUTHED_ADMIN)
			{
				if (Server()->Tick() > m_apPlayers[i]->m_LastActionTick + g_Config.m_SvInactiveKickTime*Server()->TickSpeed() * 60)
				{
					switch (g_Config.m_SvInactiveKick)
					{
					case 0:
					{
						// move player to spectator
						m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
					}
					break;
					case 1:
					{
						Server()->Kick(i, "Kicked for inactivity");
					}
					break;
					}
				}
			}
		}
	}
}

void CGameContext::BlockSystemFinish(int ClientID, vec2 Pos, bool Kill)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	CCharacter *pChr = GetPlayerChar(ClientID);
	if (pPlayer == 0x0 || pChr == 0x0 || pChr->IsAlive() == false || pPlayer->m_AttackedBy == ClientID)
		return;

	if (pPlayer->m_AttackedBy != -1 && pPlayer->m_AttackedByTick + Server()->TickSpeed() * 6.5f > Server()->Tick() &&// if getting attacked more than 6.5 seconds ago does not count
		pChr->IsFreezed())
	{
		CCharacter *pChrAttacker = GetPlayerChar(pPlayer->m_AttackedBy);
		if (pChrAttacker == 0x0 || pChrAttacker->IsFreezed())//do not give blocked exp
			return;

		int ExperienceAmount = 0;
		if (pPlayer->m_UnblockedTick + Server()->TickSpeed() * 60 < Server()->Tick() && pChr->InBlockZone() && pChr->GameMap()->NumPlayers() >= 8)
		{
			pPlayer->m_UnblockedTick = Server()->Tick();
			ExperienceAmount = 3;

			if (Server()->GetClientInfo(pPlayer->m_AttackedBy)->m_LoggedIn == true)
				Server()->GetClientInfo(pPlayer->m_AttackedBy)->m_AccountData.m_BlockPoints++;
		}

		//ranking
		if (Server()->GetClientInfo(pPlayer->m_AttackedBy)->m_LoggedIn == true &&
			Server()->GetClientInfo(ClientID)->m_LoggedIn == true)
		{
			NewRankings(Server()->GetClientInfo(pPlayer->m_AttackedBy)->m_AccountData.m_Ranking,
				Server()->GetClientInfo(ClientID)->m_AccountData.m_Ranking);
			m_AccountsHandler.Save(ClientID);
			m_AccountsHandler.Save(pPlayer->m_AttackedBy);
		}

		//exp
		if (g_Config.m_SvScoreSystem == 1)
			new CExperience(pChrAttacker->GameWorld(), Pos, ExperienceAmount, pPlayer->m_AttackedBy);

		//knockout effect
		if(Kill)
			m_CosmeticsHandler.DoKnockoutEffect(pPlayer->m_AttackedBy, Pos);

		//for events
		pChr->GameMap()->PlayerBlocked(ClientID, Kill, Pos);

		pPlayer->m_AttackedBy = -1;
	}
}

void CGameContext::BlockSystemAttack(int Attacker, int Victim, bool Hook)
{
	CPlayer *pPlayerVictim = m_apPlayers[Victim];
	CCharacter *pChrVictim = GetPlayerChar(Victim);
	if (pPlayerVictim == 0x0 || pChrVictim == 0x0 || pChrVictim->IsFreezed() == true || Attacker == Victim)
		return;

	float Renew = 2.0f;
	if (Hook) // prefere Hooks 
		Renew = 1.0f;

	if (pPlayerVictim->m_AttackedByTick + Server()->TickSpeed() * Renew < Server()->Tick())
	{
		pPlayerVictim->m_AttackedBy = Attacker;
		pPlayerVictim->m_AttackedByTick = Server()->Tick();
	}
	else if (pPlayerVictim->m_AttackedByTick == Server()->Tick())
		pPlayerVictim->m_AttackedBy = -1;//attacked by more than 1 Players. Noone gets exp
}

void CGameContext::HandleBlockSystem()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = m_apPlayers[i];
		CCharacter *pChr = GetPlayerChar(i);
		if (pPlayer == 0x0 || pChr == 0x0 || pChr->IsAlive() == false)
			continue;

		//reset blocked
		if (m_apPlayers[i]->m_Blocked == true && pChr->IsFreezed() == false)
			m_apPlayers[i]->m_Blocked = false;

		//set blocked
		if (m_apPlayers[i]->m_Blocked == false && pChr->IsFreezed() == true && pChr->FreezeTick() + Server()->TickSpeed() * 3.7f < Server()->Tick())
		{
			m_apPlayers[i]->m_Blocked = true;
			BlockSystemFinish(i, pChr->m_Pos, false);
		}

		int HookedPlayer = pChr->Core()->m_HookedPlayer;
		if (pChr->Core()->m_HookState == HOOK_GRABBED && pChr->Core()->m_HookTick >= Server()->TickSpeed() * 0.5f &&
			HookedPlayer >= 0 && HookedPlayer < MAX_CLIENTS) //not bots
			BlockSystemAttack(i, HookedPlayer, true);
	}
}

void CGameContext::DoGeneralTuning()
{
	m_Tuning.m_GunCurvature = 0.0f;
	m_Tuning.m_GunSpeed = 1400.0f;
}

void CGameContext::OnTick()
{
	DoGeneralTuning();

	for (int i = 0; i < m_NumComponents; i++)
		m_apComponents[i]->Tick();

	for (int i = 0; i < Server()->GetNumMaps(); i++)
		Server()->GetGameMap(i)->Tick();

	HandleInactive();
	HandleBlockSystem();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
		{
			m_apPlayers[i]->Tick();
			m_apPlayers[i]->PostTick();
		}
	}
}

void CGameContext::GiveExperience(int ClientID, int Amount)
{
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == true)
	{
		IServer::CAccountData *pAccountData = &Server()->GetClientInfo(ClientID)->m_AccountData;
		pAccountData->m_Experience += Amount;
		if (pAccountData->m_Experience >= NeededExp(pAccountData->m_Level))
			SetLevel(ClientID, pAccountData->m_Level + 1);
	}
	else
		SendChatTarget(ClientID, "Login/Register an account to receive your experience points");
}

void CGameContext::SetLevel(int ClientID, int Level)
{
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == true)
	{
		IServer::CAccountData *pAccountData = &Server()->GetClientInfo(ClientID)->m_AccountData;
		CCharacter *pChr = GetPlayerChar(ClientID);
		if (Level > pAccountData->m_Level && pChr)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "[LevelUp+]: You are now level %d!", Level);
			SendChatTarget(ClientID, aBuf);
			if (pChr && pChr->IsAlive())
			{
				pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + 2 * Server()->TickSpeed());
				CreateSound(pChr->GameMap(), pChr->m_Pos, SOUND_CTF_CAPTURE);
			}
		}

		pAccountData->m_Experience = 0;
		pAccountData->m_Level = Level;
		m_AccountsHandler.Save(ClientID);
	}
	else
		SendChatTarget(ClientID, "Login/Register an account to receive the levelup");
}

void CGameContext::GiveClanExperience(int ClientID, int Amount)
{
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == true && Server()->GetClientInfo(ClientID)->m_pClan)
	{
		IServer::CClanData *pClanData = Server()->GetClientInfo(ClientID)->m_pClan;
		pClanData->m_Experience += Amount;
		if (pClanData->m_Experience >= NeededClanExp(pClanData->m_Level))
			SetClanLevel(ClientID, pClanData->m_Level + 1);
	}
}

void CGameContext::SetClanLevel(int ClientID, int Level)
{
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false && Server()->GetClientInfo(ClientID)->m_pClan)
	{
		IServer::CClanData *pClanData = Server()->GetClientInfo(ClientID)->m_pClan;
		if (Level > pClanData->m_Level)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), ">- Clan %s reached level %d! -<", pClanData->m_aName, Level);
			SendChat(-1, aBuf);
		}

		pClanData->m_Experience = 0;
		pClanData->m_Level = Level;
		m_AccountsHandler.ClanSave(pClanData);
	}
}

bool CGameContext::OnExtrasCallvote(int ClientID, const char *pCommand)
{
	if (str_comp(pCommand, "inviolable") == 0)
	{
		Server()->GetClientInfo(ClientID)->m_UseInviolable = !Server()->GetClientInfo(ClientID)->m_UseInviolable;
		return true;
	}

	return false;
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

bool CGameContext::GameMapInit(CGameMap *pMap)
{
	pMap->Init(this);
	return true;
}

void CGameContext::OnClientEnter(int ClientID, bool MapSwitching)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);

	m_apPlayers[ClientID]->Respawn();
	char aBuf[512];
	if (MapSwitching == false)
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientID), GetTeamName(m_apPlayers[ClientID]->GetTeam()));
	else
		str_format(aBuf, sizeof(aBuf), "'%s' entered the map and joined the %s", Server()->ClientName(ClientID), GetTeamName(m_apPlayers[ClientID]->GetTeam()));
	
	pGameMap->SendChat(-1, aBuf);

	if (g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	pGameMap->UpdateVotes();
}

void CGameContext::OnClientConnected(int ClientID)
{
	// Check which team the player should be on
	int StartTeam = g_Config.m_SvTournamentMode ? TEAM_SPECTATORS : 0;

	if (StartTeam == 0 && g_Config.m_SvAccountForce == 1 && Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		StartTeam = TEAM_SPECTATORS;

	m_apPlayers[ClientID] = new(ClientID) CPlayer(this, ClientID, StartTeam);
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap->PlayerJoin(ClientID) == 0)
	{
		Server()->DropClient(ClientID, "Map is full");
		return;
	}

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::OnClientLeave(int ClientID, const char *pReason)
{
	m_AccountsHandler.Save(ClientID);
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == true && Server()->GetClientInfo(ClientID)->m_pClan != 0x0)
		m_AccountsHandler.ClanSave(Server()->GetClientInfo(ClientID)->m_pClan);
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason, CGameMap *pGameMap, bool MapSwitching)
{
	if (Server()->ClientIngame(ClientID))
	{
		char aBuf[512];

		if (MapSwitching == false)
		{
			if (pReason && *pReason)
				str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientID), pReason);
			else
				str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientID));
		}
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has moved to %s", Server()->ClientName(ClientID), pGameMap->Map()->GetFileName());

		pGameMap->SendChat(-1, aBuf);

		if (g_Config.m_Debug)
		{
			if (MapSwitching == false)
				str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
			else
				str_format(aBuf, sizeof(aBuf), "move player='%d:%s' %s", ClientID, Server()->ClientName(ClientID), pGameMap->Map()->GetFileName());

			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
	}

	m_apPlayers[ClientID]->OnDisconnect(pReason);

	pGameMap->PlayerLeave(ClientID);

	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	// update spectator modes
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);

	if(!pRawMsg)
	{
		if(g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if(Server()->ClientIngame(ClientID))
	{
		if(MsgID == NETMSGTYPE_CL_SAY)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			int Team = pMsg->m_Team;
			
			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while(*p)
 			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if(pEnd == 0)
					pEnd = pStrOld;

				if(++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
 			}
			if(pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if(Length == 0 || (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat+Server()->TickSpeed()*((15+Length)/16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			if (pMsg->m_pMessage[0] == '/')
			{
				if (m_ChatCommandsHandler.ProcessMessage(pMsg->m_pMessage + 1, ClientID) == false)
					pPlayer->m_LastChat = 0;
			}
			else
			{
				int64 Ticks = Server()->IsMuted(ClientID);
				if (Ticks > 0)
				{
					char aBuf[512];
					str_copy(aBuf, "You are muted for ", sizeof(aBuf));
					StringTime(Server()->Tick() + Ticks, aBuf, sizeof(aBuf));
					SendChatTarget(ClientID, aBuf);
					return;
				}

				pPlayer->m_AutomuteScore += 175 + pPlayer->m_AutomuteScore;
				if (pPlayer->m_AutomuteScore > 1200 && g_Config.m_SvMuteSpam > 0)
				{
					char aBuf[512];
					str_format(aBuf, sizeof(aBuf), "%s has been muted for ", Server()->ClientName(ClientID));
					StringTime(Server()->Tick() + Server()->TickSpeed() * g_Config.m_SvMuteSpam, aBuf, sizeof(aBuf));
					str_append(aBuf, " due to spamming", sizeof(aBuf));
					pGameMap->SendChat(-1, aBuf);
					Server()->MuteID(ClientID, Server()->TickSpeed() * g_Config.m_SvMuteSpam);
					return;
				}

				if(Team)
					SendChatClan(ClientID, pMsg->m_pMessage);
				else
					pGameMap->SendChat(ClientID, pMsg->m_pMessage);
			}
		}
		else if(MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;
			if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				SendChatTarget(ClientID, "Spectators aren't allowed to start a vote.");
				return;
			}

			if(pGameMap->GetVoteCloseTime())
			{
				SendChatTarget(ClientID, "Wait for current vote to end before calling a new one.");
				return;
			}

			int Timeleft = pPlayer->m_LastVoteCall + Server()->TickSpeed()*60 - Now;
			if(pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				char aChatmsg[512] = {0};
				str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft/Server()->TickSpeed())+1);
				SendChatTarget(ClientID, aChatmsg);
				return;
			}

			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_Reason[0] ? pMsg->m_Reason : "No reason given";

			if(str_comp_nocase(pMsg->m_Type, "option") == 0)
			{
				m_VoteMenuHandler.CallVote(ClientID, pMsg->m_Value, pReason);
			}
			else if(str_comp_nocase(pMsg->m_Type, "kick") == 0)
			{
			}
			else if(str_comp_nocase(pMsg->m_Type, "spectate") == 0)
			{
			}
		}
		else if(MsgID == NETMSGTYPE_CL_VOTE)
		{
			if (!pGameMap->GetVoteCloseTime())
				return;

			if(pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if(!pMsg->m_Vote)
					return;

				int NewPos = pGameMap->GetVotePos() + 1;
				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = NewPos;
				pGameMap->SetVotePos(NewPos);
				pGameMap->UpdateVotes();
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if(pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			if(pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendBroadcast("Teams are locked", ClientID);
				return;
			}

			if(pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick())/Server()->TickSpeed();
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Time to wait before changing team: %02d:%02d", TimeLeft/60, TimeLeft%60);
				SendBroadcast(aBuf, ClientID);
				return;
			}

			// Switch team on given client and kill/respawn him
			if(TryJoinTeam(pMsg->m_Team, ClientID))
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				if(pPlayer->GetTeam() == TEAM_SPECTATORS || pMsg->m_Team == TEAM_SPECTATORS)
					pGameMap->UpdateVotes();
				pPlayer->SetTeam(pMsg->m_Team);
				pPlayer->m_TeamChangeTick = Server()->Tick();
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if (pMsg->m_SpectatorID != SPEC_FREEVIEW)
			{
				pMsg->m_SpectatorID = Server()->ReverseTranslate(ClientID, pMsg->m_SpectatorID);
				if (pMsg->m_SpectatorID == -1)
					return;
			}
			
			if((pPlayer->GetTeam() != TEAM_SPECTATORS && !pPlayer->GetPause()) || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID ||
				(g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode+Server()->TickSpeed()*3 > Server()->Tick()))
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if(g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo+Server()->TickSpeed()*5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if(str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientID));
				pGameMap->SendChat(-1, aChatText);
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if(g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote+Server()->TickSpeed() * 0.1f > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);

			int Emote = -1;
			switch (pMsg->m_Emoticon)
			{
			case EMOTICON_EXCLAMATION:
			case EMOTICON_GHOST:
			case EMOTICON_QUESTION:
			case EMOTICON_WTF:
				Emote = EMOTE_SURPRISE;
				break;
			case EMOTICON_DOTDOT:
			case EMOTICON_DROP:
			case EMOTICON_ZZZ:
				Emote = EMOTE_BLINK;
				break;
			case EMOTICON_EYES:
			case EMOTICON_HEARTS:
			case EMOTICON_MUSIC:
				Emote = EMOTE_HAPPY;
				break;
			case EMOTICON_OOP:
			case EMOTICON_SORRY:
			case EMOTICON_SUSHI:
				Emote = EMOTE_PAIN;
				break;
			case EMOTICON_DEVILTEE:
			case EMOTICON_SPLATTEE:
			case EMOTICON_ZOMG:
				Emote = EMOTE_ANGRY;
				break;
			default:
				Emote = EMOTE_NORMAL;
				break;
			}

			CCharacter* pChr = pPlayer->GetCharacter();
			if (pChr && Emote != -1)
				pChr->SetEmote(Emote, Server()->Tick() + 2 * Server()->TickSpeed());
		}
		else if (MsgID == NETMSGTYPE_CL_KILL)
		{
			if(pPlayer->m_LastKill && pPlayer->m_LastKill+Server()->TickSpeed()*3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);
		}
	}
	else
	{
		if(MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if(pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;

			m_VoteMenuHandler.OnClientJoin(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
		}
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if(pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SpreadTuningParams();
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SpreadTuningParams();
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for(int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConBlockmapSet(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	const char *pMapName = pResult->GetString(0);
	bool Value = (bool)pResult->GetInteger(1);
	CGameMap *pGameMap = 0x0;
	for (int i = 0; i < pSelf->Server()->GetNumMaps(); i++)
	{
		if (str_comp(pSelf->Server()->GetGameMap(i)->Map()->GetFileName(), pMapName) != 0)
			continue;
		pGameMap = pSelf->Server()->GetGameMap(i);
		break;
	}

	if (pGameMap == 0x0)
	{
		str_format(aBuf, sizeof(aBuf), "Map '%s' not found!", pMapName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		return;
	}

	if(pGameMap->IsBlockMap() == Value)
	{
		str_format(aBuf, sizeof(aBuf), "Nothing to change", pMapName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		return;
	}

	pGameMap->SetBlockMap(Value);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Value changed");
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments()>2 ? pResult->GetInteger(2) : 0;
	if(!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick()+pSelf->Server()->TickSpeed()*Delay*60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "All players were moved to the %s", pSelf->GetTeamName(Team));
	pSelf->SendChat(-1, aBuf);

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if(pSelf->m_LockTeams)
		pSelf->SendChat(-1, "Teams were locked");
	else
		pSelf->SendChat(-1, "Teams were unlocked");
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	//// check for valid option
	if((!pSelf->Console()->LineIsValid(pCommand) && pCommand[0] != '%') || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	while(*pDescription && *pDescription == ' ')
		pDescription++;

	if(str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	pSelf->m_VoteMenuHandler.AddVote(pDescription, pCommand);
}

void CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	pSelf->m_VoteMenuHandler.RemoveVote(pDescription);
}

void CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";

	if(str_comp_nocase(pType, "option") == 0)
	{
		pSelf->m_VoteMenuHandler.ForceVote(pSelf->Server()->RconClientID(), pValue, pReason);
	}
	else if(str_comp_nocase(pType, "kick") == 0)
	{
	}
	else if(str_comp_nocase(pType, "spectate") == 0)
	{
	}
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_VoteMenuHandler.ClearVotes();
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int CallID = pSelf->Server()->RconClientID();
	if (CallID < 0 || CallID >= MAX_CLIENTS)
		return;

	CGameMap *pGameMap = pSelf->Server()->CurrentGameMap(CallID);
	pGameMap->VoteEnforce(pResult->GetString(0));
}

void CGameContext::ConMutePlayer(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[256];
	CGameContext *pThis = (CGameContext *)pUser;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS);
	int64 Ticks = pThis->Server()->TickSpeed() * pResult->GetInteger(1);
	bool Silent = pResult->NumArguments() == 3 && pResult->GetInteger(2) == 0;

	if (Ticks == 0)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Use 'unmute id' to unmute a player");
		return;
	}

	CGameMap *pGameMap = pThis->Server()->CurrentGameMap(ClientID);
	if (pThis->Server()->ClientIngame(ClientID) == false || pGameMap == 0x0)
	{
		str_format(aBuf, sizeof(aBuf), "Client %i not online", ClientID);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	pThis->Server()->MuteID(ClientID, Ticks);

	str_format(aBuf, sizeof(aBuf), "%s has been muted for ", pThis->Server()->ClientName(ClientID), pResult->GetInteger(1));
	pThis->StringTime(pThis->Server()->Tick() + Ticks, aBuf, sizeof(aBuf));
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	if (Silent == false)
		pGameMap->SendChat(-1, aBuf);
}

void CGameContext::ConUnmutePlayer(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[256];
	CGameContext *pThis = (CGameContext *)pUser;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS);

	if (pThis->Server()->ClientIngame(ClientID) == false)
	{
		str_format(aBuf, sizeof(aBuf), "Client %i not online", ClientID);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}


	if (pThis->Server()->UnmuteID(ClientID) == true)
	{
		str_format(aBuf, sizeof(aBuf), "%s has been unmuted", pThis->Server()->ClientName(ClientID));
		pThis->SendChatTarget(ClientID, "Your chatmute has been removed");
	}
	else
		str_format(aBuf, sizeof(aBuf), "%s is not muted", pThis->Server()->ClientName(ClientID));

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConPassivePlayer(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[256];
	CGameContext *pThis = (CGameContext *)pUser;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS);
	int Time = pResult->GetInteger(1);

	if (pThis->Server()->ClientIngame(ClientID) == false)
	{
		str_format(aBuf, sizeof(aBuf), "Client %i not online", ClientID);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	pThis->Server()->GetClientInfo(ClientID)->m_InviolableTime = pThis->Server()->Tick() + pThis->Server()->TickSpeed() * Time;

	str_format(aBuf, sizeof(aBuf), "Passive time has bee set for %s for %i seconds", pThis->Server()->ClientName(ClientID), Time);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConVIPSet(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[256];
	CGameContext *pThis = (CGameContext *)pUser;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS);
	bool Vip = (bool)pResult->GetInteger(1);

	if (pThis->Server()->ClientIngame(ClientID) == false)
	{
		str_format(aBuf, sizeof(aBuf), "Client %i not online", ClientID);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	if(pThis->Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Client has to be logged in");
		return;
	}

	if (pThis->Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip == Vip)
	{
		str_format(aBuf, sizeof(aBuf), "%s is already %s vip", pThis->Server()->ClientName(ClientID), Vip ? "a" : "no");
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	str_format(aBuf, sizeof(aBuf), "%s is now %s vip", pThis->Server()->ClientName(ClientID), Vip ? "a" : "no");
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pThis->Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip = Vip;
	pThis->m_AccountsHandler.Save(ClientID);
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::ConchainAccountsystemupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	if (pResult->NumArguments() == 1)
	{
		if (pResult->GetInteger(0) != g_Config.m_SvAccountsystem)
		{
			CGameContext *pThis = static_cast<CGameContext *>(pUserData);
			if (pResult->GetInteger(0) <= 0)
			{
				for (int i = 0; i < MAX_CLIENTS; i++)
					pThis->m_AccountsHandler.Logout(i);

				pThis->SendChat(-1, "Accountsystem is disabled now!");
				g_Config.m_SvAccountForce = 0;
			}
			else
				pThis->SendChat(-1, "Accountsystem is enabled now!");
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CGameContext::ConchainAccountForceupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pThis = static_cast<CGameContext *>(pUserData);

	if (g_Config.m_SvAccountsystem == 0 && pResult->NumArguments() > 0)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Accounsystem is disabled");
		return;
	}

	if (pResult->NumArguments() == 1)
	{
		if (pResult->GetInteger(0) >= 1)
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (pThis->m_apPlayers[i] && pThis->Server()->GetClientInfo(i)->m_LoggedIn == false)
					pThis->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, false);
			}
		}
	}

	pfnCallback(pResult, pCallbackUserData);
}

void CGameContext::ConchainShutdownupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pThis = static_cast<CGameContext *>(pUserData);
	pThis->m_AccountsHandler.ClanSaveAll();

	pfnCallback(pResult, pCallbackUserData);
}


void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	//init components
	InitComponents();

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");
	Console()->Register("blockmap_set", "si", CFGFLAG_SERVER, ConBlockmapSet, this, "Sets a map as a blocking map");

	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");
	Console()->Register("mute_player", "ii?iii", CFGFLAG_SERVER, ConMutePlayer, this, "Forbids a player to chat");
	Console()->Register("unmute_player", "i", CFGFLAG_SERVER, ConUnmutePlayer, this, "Unmutes a player to chat");
	Console()->Register("passive_player", "ii", CFGFLAG_SERVER, ConPassivePlayer, this, "Forbids a player to chat");
	Console()->Register("vip_player", "ii", CFGFLAG_SERVER, ConVIPSet, this, "Sets vip status for a player");

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);
	Console()->Chain("sv_accountsystem", ConchainAccountsystemupdate, this);
	Console()->Chain("sv_account_force", ConchainAccountForceupdate, this);
	Console()->Chain("shutdown", ConchainShutdownupdate, this);
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	for (int i = 0; i < m_NumComponents; i++)
		m_apComponents[i]->Init();

	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));
}

void CGameContext::OnShutdown()
{
	
}

void CGameContext::OnSnap(int ClientID)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	pGameMap->Snap(ClientID);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_apPlayers[i])
			m_apPlayers[i]->Snap(ClientID);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	for (int i = 0; i < Server()->GetNumMaps(); i++)
		Server()->GetGameMap(i)->Events()->Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

bool CGameContext::CanShutdown()
{
	if (m_AccountsHandler.CanShutdown() == false)//wait for all accounts to be updatet
		return false;

	return true;
}

const char *CGameContext::GameType() { return g_Config.m_SvFakeGametype ? "DDRaceNetwork" : "BW"; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::FakeVersion() { return GAME_FAKEVERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }
