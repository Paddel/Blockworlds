

#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/npc.h>
#include <game/server/entities/pickup.h>

#include "gamemap.h"

CGameMap::CGameMap(CMap *pMap)
{
	m_pMap = pMap;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;
}

CGameMap::~CGameMap()
{
}

float CGameMap::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for (; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if (pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void CGameMap::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for (int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for (int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for (int c = 0; c < Num; ++c)
				if (m_Collision.CheckPoint(m_aaSpawnPoints[Type][i] + Positions[Index]) ||
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i] + Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if (Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i] + Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if (!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CGameMap::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	if (Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if (Index == ENTITY_SPAWN_RED)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if (Index == ENTITY_SPAWN_BLUE)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
	else if (Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if (Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if (Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
	}
	else if (Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	else if (Index == ENTITY_WEAPON_RIFLE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
	}
	else if (Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
	{
		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
	}

	if (Type != -1)
	{
		CPickup *pPickup = new CPickup(&m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

bool CGameMap::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();

	m_Layers.Init(Map()->EngineMap());
	m_Collision.Init(&m_Layers);

	m_Events.SetGameServer(pGameServer);
	m_World.SetGameMap(this);

	m_RoundStartTick = Server()->Tick();

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Map()->EngineMap()->GetData(pTileMap->m_Data);


	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width + x].m_Index;

			if (Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f + 16.0f, y*32.0f + 16.0f);
				OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}

	CNpc *pNpc = new CNpc(&m_World, FreeNpcSlot());
	pNpc->Spawn(vec2(300, 300));

	pNpc = new CNpc(&m_World, FreeNpcSlot());
	pNpc->Spawn(vec2(300, 300));

	return true;
}

int CGameMap::FreePlayerSlot()
{
	int Slot = -1;
	for (int i = 0; i < g_Config.m_SvMaxClientsPerMap; i++)
		if(m_apPlayers[i] == 0x0)
		{ Slot = i; break; }

	return Slot;
}

bool CGameMap::PlayerJoin(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	int Slot = FreePlayerSlot();
	if (Slot == -1 || pPlayer == 0x0)
		return false;

	m_apPlayers[Slot] = pPlayer;
	return true;
}

void CGameMap::PlayerLeave(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] == pPlayer)
			m_apPlayers[i] = 0x0;
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

bool CGameMap::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, 0);
	EvaluateSpawnType(&Eval, 1);
	EvaluateSpawnType(&Eval, 2);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
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
	m_World.Snap(SnappingClient);
	SnapGameInfo(SnappingClient);
	m_Events.Snap(SnappingClient);
}