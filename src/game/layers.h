

#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <engine/mapengine.h>
#include <game/mapitems.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	class IMap *m_pMap;

public:
	CLayers();
	void Init(IMap *pEngineMap);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;
};

#endif
