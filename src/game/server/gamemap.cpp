
#include <engine/server/map.h>
#include <game/server/gamecontext.h>

#include "gamemap.h"

CGameMap::CGameMap(CMap *pMap)
{
	m_pMap = pMap;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;
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