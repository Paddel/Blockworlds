
#include <base/math.h>

#include "layers.h"

CLayers::CLayers()
{
	m_GroupsNum = 0;
	m_GroupsStart = 0;
	m_LayersNum = 0;
	m_LayersStart = 0;
	m_pGameGroup = 0;
	m_pGameLayer = 0;
	m_pMap = 0;
	m_NumExtrasLayer = 0;
	m_apExtrasData = 0x0;
	m_apExtrasTiles = 0x0;
	m_aExtrasWidth = 0x0;
	m_aExtrasHeight = 0x0;
}

CLayers::~CLayers()
{
	delete[] m_apExtrasData;
	delete[] m_apExtrasTiles;
	delete m_aExtrasWidth;
	delete m_aExtrasHeight;
}

void CLayers::InitGameLayer()
{
	for (int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);

			if (pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
				if (pTilemap->m_Flags&TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if (m_pGameGroup->m_Version >= 2)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}
				}
			}
		}
	}
}

void CLayers::InitExtraLayers()
{
	for (int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if (pLayer->m_Type == LAYERTYPE_EXTRAS)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
				if (pTilemap->m_ExtraVersion != EXTRA_VERSION)
				{
					dbg_msg("layers", "Extras layer could not be loaded due to the incorrect version");
					continue;
				}

				m_NumExtrasLayer++;
			}
		}
	}

	m_apExtrasData = new CExtrasData*[m_NumExtrasLayer];
	m_apExtrasTiles = new CTile*[m_NumExtrasLayer];
	m_aExtrasWidth = new int[m_NumExtrasLayer];
	m_aExtrasHeight = new int[m_NumExtrasLayer];

	int Counter = 0;
	for (int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if (pLayer->m_Type == LAYERTYPE_EXTRAS)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
				if (pTilemap->m_ExtraVersion != EXTRA_VERSION)
					continue;

				m_apExtrasData[Counter] = static_cast<CExtrasData *>(m_pMap->GetData(pTilemap->m_ExtrasData));
				m_apExtrasTiles[Counter] = static_cast<CTile *>(m_pMap->GetData(pTilemap->m_Data));
				m_aExtrasWidth[Counter] = pTilemap->m_Width;
				m_aExtrasHeight[Counter] = pTilemap->m_Height;
				Counter++;
			}
		}
	}
}

void CLayers::Init(IMap *pEngineMap)
{
	m_pMap = pEngineMap;
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);

	InitGameLayer();
	InitExtraLayers();
}

CMapItemGroup *CLayers::GetGroup(int Index) const
{
	return static_cast<CMapItemGroup *>(m_pMap->GetItem(m_GroupsStart+Index, 0, 0));
}

CMapItemLayer *CLayers::GetLayer(int Index) const
{
	return static_cast<CMapItemLayer *>(m_pMap->GetItem(m_LayersStart+Index, 0, 0));
}

int CLayers::ExtrasIndex(int Index, float x, float y)
{
	int Nx = clamp(round_to_int(x) / 32, 0, m_aExtrasWidth[Index] - 1),
		Ny = clamp(round_to_int(y) / 32, 0, m_aExtrasHeight[Index] - 1);
	return Ny * m_aExtrasWidth[Index] + Nx;
}

bool CLayers::IsHookThrough(vec2 Last, vec2 Pos)
{
	for (int i = 0; i < m_NumExtrasLayer; i++)
	{
		int Tile = GetExtrasTile(i)[ExtrasIndex(i, round_to_int(Pos.x), round_to_int(Pos.y))].m_Index;
		if (Tile == EXTRAS_HOOKTHROUGH)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_TOP && Last.y <= Pos.y)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_BOTTOM && Last.y >= Pos.y)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_LEFT && Last.x >= Pos.x)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_RIGHT && Last.x <= Pos.x)
			return true;
	}
	return false;
}