
#include <engine/masterserver.h>

#ifndef ENGINE_SERVER_REGISTER_H
#define ENGINE_SERVER_REGISTER_H

class CNetServer;
class IEngineMasterServer;
class IConsole;
class CMap;

class CRegister
{
	enum
	{
		REGISTERSTATE_START=0,
		REGISTERSTATE_UPDATE_ADDRS,
		REGISTERSTATE_QUERY_COUNT,
		REGISTERSTATE_HEARTBEAT,
		REGISTERSTATE_REGISTERED,
		REGISTERSTATE_ERROR
	};

	struct CMasterserverInfo
	{
		NETADDR m_Addr;
		int m_Count;
		int m_Valid;
		int64 m_LastSend;
	};

	CNetServer *m_pNetServer;
	IEngineMasterServer *m_pMasterServer;
	IConsole *m_pConsole;
	CMap *m_pMap;

	int m_RegisterState;
	int64 m_RegisterStateStart;
	int m_RegisterFirst;
	int m_RegisterCount;
	int m_Port;

	CMasterserverInfo m_aMasterserverInfo[IMasterServer::MAX_MASTERSERVERS];
	int m_RegisterRegisteredServer;

	void RegisterNewState(int State);
	void RegisterSendFwcheckresponse(NETADDR *pAddr);
	void RegisterSendHeartbeat(NETADDR Addr);
	void RegisterSendCountRequest(NETADDR Addr);
	void RegisterGotCount(struct CNetChunk *pChunk);

public:
	CRegister();
	void Init(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole, CMap *pMap);
	void RegisterUpdate(int Nettype);
	int RegisterProcessPacket(struct CNetChunk *pPacket);
};

#endif
