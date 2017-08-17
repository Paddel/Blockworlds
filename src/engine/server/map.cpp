
#include <engine/storage.h>

#include <game/server/gamemap.h>// Interface ??

#include "server.h"

#include "map.h"

CMap::CMap(CServer *pServer)
{
	m_pServer = pServer;
	m_pStorage = pServer->Storage();
	m_pGameMap = new CGameMap(this);

	m_MapSize = 0;
	m_pMapData = 0x0;
}

CMap::~CMap()
{
	delete m_pGameMap;
	//delete m_pMapData;
	mem_free(m_pMapData);
	m_EngineMap.Unload();
}

bool CMap::LoadMapFile()
{
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", m_aFileName);

	if (!m_MapChecker.ReadAndValidateMap(Storage(), m_aFileName, IStorage::TYPE_ALL))
		return false;

	if (!m_EngineMap.Load(aBuf, Storage()))
	{
		return false;
	}

	// get the crc of the map
	m_MapCrc = m_EngineMap.Crc();

	// load complete map into memory for download
	{
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
		m_MapSize = (int)io_length(File);
		m_pMapData = (unsigned char *)mem_alloc(m_MapSize, 1);
		io_read(File, m_pMapData, m_MapSize);
		io_close(File);
	}
	return true;
}

bool CMap::Init(const char *pFileName)
{
	mem_copy(m_aFileName, pFileName, sizeof(m_aFileName));
	if (LoadMapFile() == false)
		return false;

	return true;
}