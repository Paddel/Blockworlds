

#ifndef ENGINE_SERVER_SERVER_H
#define ENGINE_SERVER_SERVER_H

#include <base/tl/array.h>

#include <engine/server.h>
#include <engine/masterserver.h>
#include <engine/shared/netban.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>
#include <engine/shared/network.h>
#include <engine/shared/econ.h>

#include "translator.h"
#include "map.h"

struct CMute
{
	NETADDR m_Address;
	int64 m_Ticks;
};

class CSnapIDPool
{
	enum
	{
		MAX_IDS = 32*1024,
	};

	class CID
	{
	public:
		short m_Next;
		short m_State; // 0 = free, 1 = alloced, 2 = timed
		int m_Timeout;
	};

	CID m_aIDs[MAX_IDS];

	int m_FirstFree;
	int m_FirstTimed;
	int m_LastTimed;
	int m_Usage;
	int m_InUsage;

public:

	CSnapIDPool();

	void Reset();
	void RemoveFirstTimeout();
	int NewID();
	void TimeoutIDs();
	void FreeID(int ID);
};


class CServerBan : public CNetBan
{
	class CServer *m_pServer;

	template<class T> int BanExt(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason);

public:
	class CServer *Server() const { return m_pServer; }

	void InitServerBan(class IConsole *pConsole, class IStorage *pStorage, class CServer* pServer);

	virtual int BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason);
	virtual int BanRange(const CNetRange *pRange, int Seconds, const char *pReason);

	static void ConBanExt(class IConsole::IResult *pResult, void *pUser);
};


class CServer : public IServer
{
	class IGameServer *m_pGameServer;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class IEngineMasterServer *m_pMasterServer;
public:
	class IGameServer *GameServer() { return m_pGameServer; }
	class IConsole *Console() { return m_pConsole; }
	class IStorage *Storage() { return m_pStorage; }
	class IEngineMasterServer *MasterServer() { return m_pMasterServer; }

	enum
	{
		MAX_RCONCMD_SEND=16,
	};

	class CClient
	{
	public:

		enum
		{
			STATE_EMPTY = 0,
			STATE_AUTH,
			STATE_CONNECTING,
			STATE_READY,
			STATE_INGAME,

			SNAPRATE_INIT=0,
			SNAPRATE_FULL,
			SNAPRATE_RECOVER
		};

		class CInput
		{
		public:
			int m_aData[MAX_INPUT_SIZE];
			int m_GameTick; // the tick that was chosen for the input
		};

		// connection state info
		int m_State;
		int m_Latency;
		int m_SnapRate;

		int m_LastAckedSnapshot;
		int m_LastInputTick;
		CSnapshotStorage m_Snapshots;

		CInput m_LatestInput;
		CInput m_aInputs[200]; // TODO: handle input better
		int m_CurrentInput;

		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Authed;
		int m_AuthTries;

		bool m_Online;

		CMap *m_pMap;
		int m_MapitemUsage;

		const IConsole::CCommandInfo *m_pRconCmdToSend;
		IServer::CClientInfo m_ClientInfo;

		void Reset();
	};

	CClient m_aClients[MAX_CLIENTS];

	CSnapshotDelta m_SnapshotDelta;
	CSnapshotBuilder m_SnapshotBuilder;
	CSnapIDPool m_IDPool;
	CNetServer m_NetServer;
	CEcon m_Econ;
	CServerBan m_ServerBan;
	CTranslator m_Translator;

	int64 m_GameStartTime;
	int m_RunServer;
	int m_RconClientID;
	int m_RconAuthLevel;
	int m_PrintCBIndex;

	int64 m_Lastheartbeat;

	array<CMap *> m_lpMaps;
	array<CMute *> m_lMutes;
	CMap *m_pDefaultMap;

	CServer();

	int TrySetClientName(int ClientID, const char *pName);
	
	void HandleMutes();

	virtual void SetClientName(int ClientID, const char *pName);
	virtual void SetClientClan(int ClientID, char const *pClan);
	virtual void SetClientCountry(int ClientID, int Country);

	virtual void Kick(int ClientID, const char *pReason);
	virtual void DropClient(int ClientID, const char *pReason);
	virtual int RconClientID() { return m_RconClientID; }

	int64 TickStartTime(int Tick);

	int Init();

	virtual void SetRconCID(int ClientID);
	virtual IServer::CClientInfo *GetClientInfo(int ClientID);
	virtual void GetClientAddr(int ClientID, char *pAddrStr, int Size);
	virtual const char *ClientName(int ClientID);
	virtual const char *ClientClan(int ClientID);
	virtual int ClientCountry(int ClientID);
	virtual bool ClientIngame(int ClientID);
	virtual int GetClientAuthed(int ClientID);
	virtual int GetClientLatency(int ClientID);
	virtual int MaxClients() const;

	virtual bool CompClientAddr(int C1, int C2);

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID);
	int SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System);

	void DoSnapshot();

	void SetMapOnConnect(int ClientID);

	static int NewClientCallback(int ClientID, void *pUser);
	static int DelClientCallback(int ClientID, const char *pReason, void *pUser);

	virtual CMap *FindMap(const char *pName);
	bool MovePlayer(int ClientID, const char *pMapName);
	bool MovePlayer(int ClientID, CMap *pMap);
	bool MoveLobby(int ClientID);
	void SendMap(int ClientID);
	void SendConnectionReady(int ClientID);
	void SendRconLine(int ClientID, const char *pLine);
	static void SendRconLineAuthed(const char *pLine, void *pUser);

	void SendRconCmdAdd(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void SendRconCmdRem(const IConsole::CCommandInfo *pCommandInfo, int ClientID);
	void UpdateClientRconCommands();

	void ProcessClientPacket(CNetChunk *pPacket);

	void SendServerInfo(const NETADDR *pAddr, int Token, CMap *pMap, NETSOCKET Socket, bool Info64, int Offset = 0);
	void UpdateServerInfo();

	void PumpNetwork();

	CMap *AddMap(const char *pMapName, int Port);
	bool RemoveMap(const char *pMapName);
	bool ReloadMap(const char *pMapName);

	int Run();

	static void ConKick(IConsole::IResult *pResult, void *pUser);
	static void ConStatus(IConsole::IResult *pResult, void *pUser);
	static void ConStatusAccounts(IConsole::IResult *pResult, void *pUser);
	static void ConShutdown(IConsole::IResult *pResult, void *pUser);
	static void ConLogout(IConsole::IResult *pResult, void *pUser);
	static void ConAddMap(IConsole::IResult *pResult, void *pUser);
	static void ConRemoveMap(IConsole::IResult *pResult, void *pUser);
	static void ConReloadMap(IConsole::IResult *pResult, void *pUser);
	static void ConListMaps(IConsole::IResult *pResult, void *pUser);
	static void ConMovePlayer(IConsole::IResult *pResult, void *pUser);
	static void ConReconnectDatabases(IConsole::IResult *pResult, void *pUser);
	static void ConMutePlayer(IConsole::IResult *pResult, void *pUser);
	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainModCommandUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	
	void RegisterCommands();


	virtual int SnapNewID();
	virtual void SnapFreeID(int ID);
	virtual void *SnapNewItem(int Type, int ID, int Size);
	void SnapSetStaticsize(int ItemType, int Size);

	CMap *CurrentMap(int ClientID);
	int CurrentMapIndex(int ClientID);
	CMap *GetMap(int Index);

	virtual int UsingMapItems(int ClientID);
	virtual CGameMap *CurrentGameMap(int ClientID);
	virtual int GetNumMaps();
	virtual CGameMap *GetGameMap(int Index);

	virtual int64 IsMuted(int ClientID);
	virtual void MuteID(int ClientID, int64 Ticks);
	virtual bool UnmuteID(int ClientID);


	virtual int Translate(int For, int ClientID) { return m_Translator.Translate(For, ClientID); };
	virtual int ReverseTranslate(int For, int ClientID) { return m_Translator.ReverseTranslate(For, ClientID); };
};

#endif
