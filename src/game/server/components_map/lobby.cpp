
#include <engine/shared/config.h>
#include <engine/server.h>
#include <engine/server/map.h>
#include <game/server/gamemap.h>
#include <game/server/entities/npcs/clan_npc.h>

#include "lobby.h"

CLobby::~CLobby()
{
	m_lpLobbyMapcounts.delete_all();
}

CGameMap *CLobby::FindGameMap(const char *pName)
{
	CGameMap *pGameMap = 0x0;
	for (int i = 0; i < Server()->GetNumMaps(); i++)
	{
		if (str_comp(Server()->GetGameMap(i)->Map()->GetFileName(), pName) != 0)
			continue;
		pGameMap = Server()->GetGameMap(i);
		break;
	}
	return pGameMap;
}

void CLobby::Init()
{
	m_pLayers = GameMap()->Layers();

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

				if (Tile == EXTRAS_PLAYERCOUNT)
				{
					CLobbyPlayercount *pPlayercount = new CLobbyPlayercount();
					str_copy(pPlayercount->m_aMapName, ExtrasData.m_aData, sizeof(pPlayercount->m_aMapName));
					pPlayercount->m_Pos = Pos;
					m_lpLobbyMapcounts.add(pPlayercount);
				}
			}
		}
	}

	CMapItemLayerTilemap *pTileMap = Layers()->GameLayer();
	CTile *pTiles = (CTile *)GameMap()->Map()->EngineMap()->GetData(pTileMap->m_Data);

	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width + x].m_Index;

			if (Index == TILE_BESTCLAN)
			{
				vec2 Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

				CNpc *pNpc = new CClanNpc(GameMap()->World(), GameMap()->FreeNpcSlot());
				pNpc->Spawn(Pos);
			}

		}
	}
}

void CLobby::Tick()
{
	char aBuf[8];
	for (int i = 0; i < m_lpLobbyMapcounts.size(); i++)
	{
		CLobbyPlayercount *pPlayercount = m_lpLobbyMapcounts[i];

		CGameMap *pGameMap = FindGameMap(pPlayercount->m_aMapName);
		if (pGameMap == 0x0)
		{
			dbg_msg("playercount", "Map %s not found", pPlayercount->m_aMapName);
			delete m_lpLobbyMapcounts[i];
			m_lpLobbyMapcounts.remove_index(i);
			i--;
			continue;
		}


		str_format(aBuf, sizeof(aBuf), "%i", pGameMap->NumPlayers());
		GameMap()->AnimationHandler()->Laserwrite(aBuf, pPlayercount->m_Pos, 10.0f, 1);
	}
}