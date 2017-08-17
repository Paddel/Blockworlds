#pragma once

#include <engine/mapengine.h>
#include <engine/shared/mapchecker.h>

class CServer;
class CGameMap;

class CMap
{
	friend class CServer;//TODO: remove this

private:
	CEngineMap m_EngineMap;
	CMapChecker m_MapChecker;

	CServer *m_pServer;
	IStorage *m_pStorage;
	CGameMap *m_pGameMap;
	char m_aFileName[128];
	unsigned m_MapCrc;
	int m_MapSize;
	unsigned char *m_pMapData;

	bool LoadMapFile();
public:
	CMap(CServer *pServer);
	~CMap();

	bool Init(const char *pFileName);

	CEngineMap *EngineMap() { return &m_EngineMap; };
	CServer *Server() const { return m_pServer; };
	IStorage *Storage() const { return m_pStorage; };
	CGameMap *GameMap() const { return m_pGameMap; };
};