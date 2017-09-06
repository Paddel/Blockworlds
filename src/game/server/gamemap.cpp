
#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/server/gamecontext.h>
#include <game/server/gameevent.h>
#include <game/server/entities/npc.h>
#include <game/server/gameevents/lastmanblocking.h>

#include "gamemap.h"

CGameMap::CGameMap(CMap *pMap)
{
	m_pMap = pMap;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0x0;

	m_BlockMap = false;
	m_ShopMap = false;
	m_HasArena = false;

	m_NumPlayers = 0;

	m_RandomEventTime = 0;
	m_pGameEvent = 0x0;
}

CGameMap::~CGameMap()
{
	
}

void CGameMap::InitComponents()
{
	m_NumComponents = 0;

	m_apComponents[m_NumComponents++] = &m_Events;
	m_apComponents[m_NumComponents++] = &m_World;
	m_apComponents[m_NumComponents++] = &m_Shop;
	m_apComponents[m_NumComponents++] = &m_AnimationHandler;
	m_apComponents[m_NumComponents++] = &m_Lobby;
	m_apComponents[m_NumComponents++] = &m_RaceComponents;
	m_apComponents[m_NumComponents++] = &m_MapVoting;

	for (int i = 0; i < m_NumComponents; i++)
	{
		m_apComponents[i]->m_pGameMap = this;
		m_apComponents[i]->m_pGameServer = GameServer();
		m_apComponents[i]->m_pServer = Server();
	}

	for (int i = 0; i < m_NumComponents; i++)
		m_apComponents[i]->Init();
}

void CGameMap::FindMapType()
{
	for (int l = 0; l < Layers()->GetNumExtrasLayer(); l++)
	{
		for (int x = 0; x < Layers()->GetExtrasWidth(l); x++)
		{
			for (int y = 0; y < Layers()->GetExtrasHeight(l); y++)
			{
				int Index = y * Layers()->GetExtrasWidth(l) + x;
				int Tile = Layers()->GetExtrasTile(l)[Index].m_Index;
				CExtrasData ExtrasData = Layers()->GetExtrasData(l)[Index];
				vec2 Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

				if (Tile == EXTRAS_ZONE_BLOCK)
					m_BlockMap = true;
				else if (Tile == EXTRAS_SELL_SKINMANI || Tile == EXTRAS_SELL_GUNDESIGN || Tile == EXTRAS_SELL_KNOCKOUT || Tile == EXTRAS_SELL_EXTRAS)
					m_ShopMap = true;
			}
		}
	}

	CMapItemLayerTilemap *pTileMap = Layers()->GameLayer();
	CTile *pTiles = (CTile *)Map()->EngineMap()->GetData(pTileMap->m_Data);

	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width + x].m_Index;

			if (Index >= ENTITY_OFFSET)
			{
				int Entity = Index - ENTITY_OFFSET;
				if (Entity == ENTITY_SPAWN_EVENT)
					m_HasArena = true;
			}
		}
	}
}

void CGameMap::DoMapTunings()
{
	if (m_BlockMap == false)
	{
		m_World.m_Core.m_Tuning.m_PlayerCollision = 0;
		m_World.m_Core.m_Tuning.m_PlayerHooking = 0;
	}
	if (m_ShopMap == true)
	{
		m_World.m_Core.m_Tuning.m_ShotgunSpeed = 0;
		m_World.m_Core.m_Tuning.m_ShotgunLifetime = 100;
		m_World.m_Core.m_Tuning.m_GunSpeed = 0;
		m_World.m_Core.m_Tuning.m_GunLifetime = 100;
	}
}

bool CGameMap::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pConsole = pGameServer->Console();

	m_Layers.Init(Map()->EngineMap());
	m_Collision.Init(&m_Layers);

	m_RoundStartTick = Server()->Tick();

	FindMapType();
	InitComponents();

	return true;
}

bool CGameMap::FreePlayerSlot()
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			Num++;

	return Num < g_Config.m_SvMaxClientsPerMap;
}

bool CGameMap::PlayerJoin(int ClientID)
{
	if (FreePlayerSlot() == false)
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	m_apPlayers[ClientID] = pPlayer;

	m_NumPlayers++;

	// send active vote
	if (m_MapVoting.GetVoteCloseTime())
		m_MapVoting.SendVoteSet(ClientID);
	return true;
}

void CGameMap::PlayerLeave(int ClientID)
{
	m_apPlayers[ClientID] = 0x0;
	m_NumPlayers--;
	m_MapVoting.UpdateVotes();
}

bool CGameMap::PlayerOnMap(int ClientID)
{
	return m_apPlayers[ClientID] != 0x0;
}

int CGameMap::FreeNpcSlot()
{
	int ClientID = MAX_CLIENTS;
	CNpc *pFirst = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC);
	for (CNpc *pNpc = pFirst; pNpc; pNpc = (CNpc *)pNpc->TypeNext())
	{
		if (ClientID != pNpc->GetCID())
			continue;

		ClientID++;
		pNpc = pFirst;
	}

	return ClientID;
}

CNpc *CGameMap::GetNpc(int ClientID)
{
	for (CNpc *pNpc = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pNpc; pNpc = (CNpc *)pNpc->TypeNext())
		if (pNpc->GetCID() == ClientID)
			return pNpc;
	return 0x0;
}

int CGameMap::NumTranslateItems()
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			Num++;

	for (CEntity *pEnt = m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pEnt; pEnt = pEnt->TypeNext())
		Num++;

	return Num;
}

void CGameMap::FillTranslateItems(CTranslateItem *pTranslateItems)
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			mem_copy(&pTranslateItems[Num++], m_apPlayers[i]->GetTranslateItem(), sizeof(CTranslateItem));

	for (CNpc *pNpc = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pNpc; pNpc = (CNpc *)pNpc->TypeNext())
		mem_copy(&pTranslateItems[Num++], pNpc->GetTranslateItem(), sizeof(CTranslateItem));
}

vec2 CGameMap::GetPlayerViewPos(int ClientID)
{
	if (m_apPlayers[ClientID] == 0x0)
		return vec2(0.0f, 0.0f);
	return m_apPlayers[ClientID]->m_ViewPos;
}

void CGameMap::SnapFakePlayer(int SnappingClient)
{
	int ClientID = Server()->UsingMapItems(SnappingClient) - 1;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, ClientID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, "");
	pClientInfo->m_Country = -1;// Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, "");
	pClientInfo->m_UseCustomColor = 0;// m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = 0;// m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = 0;// m_TeeInfos.m_ColorFeet;

	/*CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, ClientID, sizeof(CNetObj_PlayerInfo)));
	if (!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = ClientID;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Team = 0;*/
}

void CGameMap::SnapGameInfo(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if (!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = GAMEFLAG_FLAGS;
	pGameInfoObj->m_GameStateFlags = 0;
	/*if(m_GameOverTick != -1)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;*/
	/*if(m_SuddenDeath)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;*/
	/*if(GameServer()->m_World.m_Paused)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;*/
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = 0;

	pGameInfoObj->m_ScoreLimit = 0;
	pGameInfoObj->m_TimeLimit = 0;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = 0;

	SnapFakePlayer(SnappingClient);
}

void CGameMap::Snap(int SnappingClient)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->Snap(SnappingClient);
	
	SnapGameInfo(SnappingClient);

	for (int i = 0; i < m_NumComponents; i++)
		m_apComponents[i]->Snap(SnappingClient);
}

CGameEvent *CGameMap::CreateGameEvent(int Index)
{
	switch (Index)
	{
	case CGameEvent::EVENT_LASTMANBLOCKING: return new CLastManBlocking(this);
	};

	return 0x0;
}

bool CGameMap::TryVoteRandomEvent(int ClientID)
{
	if (IsBlockMap() == false || m_HasArena == false)
		return false;

	if (m_pGameEvent != 0x0)
	{
		GameServer()->SendChatTarget(ClientID, "You cannot start a event, when there is alreay one running");
		return false;
	}

	if (m_RandomEventTime + Server()->TickSpeed() * g_Config.m_SvEventCooldown > Server()->Tick())
	{
		char aBuf[128];
		str_copy(aBuf, "You have to wait ", sizeof(aBuf));
		GameServer()->StringTime(m_RandomEventTime + Server()->TickSpeed() * g_Config.m_SvEventCooldown, aBuf, sizeof(aBuf));
		str_append(aBuf, " until you can start this vote.", sizeof(aBuf));
		GameServer()->SendChatTarget(ClientID, aBuf);
		return false;
	}

	return true;
}

void CGameMap::StartRandomEvent()
{
	if (m_pGameEvent != 0x0)
		return;

	if (IsBlockMap() == false || m_HasArena == false)
		return;

	int Index = rand() % CGameEvent::NUM_EVENTS;
	m_pGameEvent = CreateGameEvent(Index);
}

void CGameMap::EndEvent()
{
	if (m_pGameEvent == 0x0)
		return;

	delete m_pGameEvent;
	m_pGameEvent = 0x0;

	m_RandomEventTime = Server()->Tick();
}

void CGameMap::ClientSubscribeEvent(int ClientID)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->ClientSubscribe(ClientID);
}

void CGameMap::PlayerBlocked(int ClientID, bool Dead, vec2 Pos)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->PlayerBlocked(ClientID, Dead, Pos);
}

void CGameMap::PlayerKilled(int ClientID)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->PlayerKilled(ClientID);
}

void CGameMap::Tick()
{
	mem_copy(&m_World.m_Core.m_Tuning, GameServer()->Tuning(), sizeof(m_World.m_Core.m_Tuning));
	DoMapTunings();

	for (int i = 0; i < m_NumComponents; i++)
		m_apComponents[i]->Tick();

	if (m_pGameEvent != 0x0)
		m_pGameEvent->Tick();
}

void CGameMap::SendChat(int ChatterClientID, const char *pText)
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

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i] == 0x0)
			continue;

		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_apPlayers[i]->GetCID());
	}
}

void CGameMap::SendBroadcast(const char *pText)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i] == 0x0)
			continue;

		CNetMsg_Sv_Broadcast Msg;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_apPlayers[i]->GetCID());
	}
}

void CGameMap::OnClientEnter(int ClientID)
{
	m_MapVoting.UpdateVotes();
}