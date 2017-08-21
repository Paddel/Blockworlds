#pragma once

#include <engine/mapengine.h>
#include <engine/shared/mapchecker.h>

#include "register.h"

class CServer;
class CGameMap;

class CMap
{
private:
	CEngineMap m_EngineMap;
	CMapChecker m_MapChecker;
	CRegister m_Register;
	NETSOCKET m_Socket;

	CServer *m_pServer;
	IStorage *m_pStorage;
	CGameMap *m_pGameMap;
	char m_aFileName[128];
	unsigned m_MapCrc;
	unsigned int m_MapSize;
	unsigned char *m_pMapData;
	int m_Port;

	bool LoadMapFile();
public:
	CMap(CServer *pServer);
	~CMap();

	bool Init(const char *pFileName, int Port);
	void BlockNet();
	void UnblockNet(NETSOCKET Socket, int Port);
	void Tick();
	bool HasFreePlayerSlot();
	void SendServerMsg(const char *pMsg);

	bool HasNetSocket();
	bool ClientOnMap(int ClientID);

	const char *GetFileName() const { return m_aFileName; };
	unsigned GetMapCrc() const { return m_MapCrc; };
	unsigned int GetMapSize() const { return m_MapSize; };
	const unsigned char *MapData() const { return m_pMapData; };
	NETSOCKET GetSocket() const { return m_Socket; };
	int GetPort() const { return m_Port; };

	CEngineMap *EngineMap() { return &m_EngineMap; };
	CRegister *Register() { return &m_Register; };
	CServer *Server() const { return m_pServer; };
	IStorage *Storage() const { return m_pStorage; };
	CGameMap *GameMap() const { return m_pGameMap; };
};