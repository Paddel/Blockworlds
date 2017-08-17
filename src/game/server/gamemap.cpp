

#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/server/gamecontext.h>

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

bool CGameMap::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;

	m_Layers.Init(Map()->EngineMap());
	m_Collision.Init(&m_Layers);

	m_Events.SetGameServer(pGameServer);
	m_World.SetGameMap(this);

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
				pGameServer->m_pController->OnEntity(Index - ENTITY_OFFSET, Pos, this);
			}
		}
	}

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

int CGameMap::NumTranslateItems()
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			Num++;

	// + npcs
	return Num;
}

void CGameMap::FillTranslateItems(CTranslateItem *pTranslateItems)
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			mem_copy(&pTranslateItems[Num++], m_apPlayers[i]->GetTranslateItem(), sizeof(CTranslateItem));

	// npcs
}