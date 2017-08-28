
#include <engine/storage.h>

#include <game/server/gamemap.h>//TODO: Create an interface to remove this include

#include "server.h"

#include "map.h"

CMap::CMap(CServer *pServer)
{
	m_pServer = pServer;
	m_pStorage = pServer->Storage();
	m_pGameMap = new CGameMap(this);

	m_MapSize = 0;
	m_pMapData = 0x0;
	mem_zero(&m_Socket, sizeof(m_Socket));
	m_Port = 0;
}

CMap::~CMap()
{
	net_udp_close(m_Socket);
	delete m_pGameMap;
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
		m_MapSize = (unsigned int)io_length(File);
		m_pMapData = (unsigned char *)mem_alloc(m_MapSize, 1);
		io_read(File, m_pMapData, m_MapSize);
		io_close(File);
	}
	return true;
}

bool CMap::Init(const char *pFileName, int Port)
{
	char aBuf[128];

	mem_copy(m_aFileName, pFileName, sizeof(m_aFileName));
	m_Port = Port;

	if (LoadMapFile() == false)
		return false;

	if (Port > 0)
	{
		NETADDR BindAddr;
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = Port;
		m_Socket = net_udp_create(BindAddr, 0);
		if (!m_Socket.type)
		{
			str_format(aBuf, sizeof(aBuf), "couldn't open socket. port %d might already be in use", Port);
			Server()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return false;
		}

		m_Register.Init(&Server()->m_NetServer, Server()->MasterServer(), Server()->Console(), this);
	}

	return true;
}

void CMap::BlockNet()
{
	if (HasNetSocket() == false)
		return;

	mem_zero(&m_Socket, sizeof(m_Socket));
	m_Port = 0;
}

void CMap::UnblockNet(NETSOCKET Socket, int Port)
{
	if (HasNetSocket() == true)
		return;

	m_Socket = Socket;
	m_Port = Port;

	if (!m_Socket.type)
	{
		BlockNet();
		return;
	}

	m_Register.Init(&Server()->m_NetServer, Server()->MasterServer(), Server()->Console(), this);
}

void CMap::Tick()
{
	if(HasNetSocket())
		m_Register.RegisterUpdate(m_Socket.type);
}

bool CMap::HasFreePlayerSlot()
{
	return GameMap()->FreePlayerSlot();
}

void CMap::SendServerMsg(const char *pMsg)
{
	GameMap()->SendChat(-1, pMsg);
}

bool CMap::HasNetSocket()
{
	return m_Port > 0;
}

bool CMap::ClientOnMap(int ClientID)
{
	return GameMap()->PlayerOnMap(ClientID);
}