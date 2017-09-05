

#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <base/vmath.h>
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
	int m_NumExtrasLayer;
	CExtrasData **m_apExtrasData;
	CTile **m_apExtrasTiles;
	int *m_aExtrasWidth;
	int *m_aExtrasHeight;

	void InitGameLayer();
	void InitExtraLayers();

public:
	CLayers();
	~CLayers();
	void Init(IMap *pEngineMap);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;
	int ExtrasIndex(int Index, float x, float y);
	int GetNumExtrasLayer() const { return m_NumExtrasLayer; };
	CExtrasData *GetExtrasData(int Index) const { return m_apExtrasData[Index]; };
	CTile *GetExtrasTile(int Index) const { return m_apExtrasTiles[Index]; };
	int GetExtrasWidth(int Index) const { return m_aExtrasWidth[Index]; };
	int GetExtrasHeight(int Index) const { return m_aExtrasHeight[Index]; };

	bool IsHookThrough(vec2 Last, vec2 Pos);
};

#endif
