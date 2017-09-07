


#include <base/math.h>
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/engine.h>
#include <engine/mapengine.h>
#include <engine/masterserver.h>
#include <engine/server.h>
#include <engine/storage.h>

#include <engine/shared/compression.h>
#include <engine/shared/config.h>
#include <engine/shared/datafile.h>
#include <engine/shared/filecollection.h>
#include <engine/shared/packer.h>

#include <mastersrv/mastersrv.h>

#include "database.h"
#include "register.h"
#include "server.h"

#if defined(CONF_FAMILY_WINDOWS)
	#define _WIN32_WINNT 0x0501
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

static const char *StrLtrim(const char *pStr)
{
	while(*pStr && *pStr >= 0 && *pStr <= 32)
		pStr++;
	return pStr;
}

static void StrRtrim(char *pStr)
{
	int i = str_length(pStr);
	while(i >= 0)
	{
		if(pStr[i] < 0 || pStr[i] > 32)
			break;
		pStr[i] = 0;
		i--;
	}
}


CSnapIDPool::CSnapIDPool()
{
	Reset();
}

void CSnapIDPool::Reset()
{
	for(int i = 0; i < MAX_IDS; i++)
	{
		m_aIDs[i].m_Next = i+1;
		m_aIDs[i].m_State = 0;
	}

	m_aIDs[MAX_IDS-1].m_Next = -1;
	m_FirstFree = 0;
	m_FirstTimed = -1;
	m_LastTimed = -1;
	m_Usage = 0;
	m_InUsage = 0;
}


void CSnapIDPool::RemoveFirstTimeout()
{
	int NextTimed = m_aIDs[m_FirstTimed].m_Next;

	// add it to the free list
	m_aIDs[m_FirstTimed].m_Next = m_FirstFree;
	m_aIDs[m_FirstTimed].m_State = 0;
	m_FirstFree = m_FirstTimed;

	// remove it from the timed list
	m_FirstTimed = NextTimed;
	if(m_FirstTimed == -1)
		m_LastTimed = -1;

	m_Usage--;
}

int CSnapIDPool::NewID()
{
	int64 Now = time_get();

	// process timed ids
	while(m_FirstTimed != -1 && m_aIDs[m_FirstTimed].m_Timeout < Now)
		RemoveFirstTimeout();

	int ID = m_FirstFree;
	dbg_assert(ID != -1, "id error");
	if(ID == -1)
		return ID;
	m_FirstFree = m_aIDs[m_FirstFree].m_Next;
	m_aIDs[ID].m_State = 1;
	m_Usage++;
	m_InUsage++;
	return ID;
}

void CSnapIDPool::TimeoutIDs()
{
	// process timed ids
	while(m_FirstTimed != -1)
		RemoveFirstTimeout();
}

void CSnapIDPool::FreeID(int ID)
{
	if(ID < 0)
		return;
	dbg_assert(m_aIDs[ID].m_State == 1, "id is not alloced");

	m_InUsage--;
	m_aIDs[ID].m_State = 2;
	m_aIDs[ID].m_Timeout = time_get()+time_freq()*5;
	m_aIDs[ID].m_Next = -1;

	if(m_LastTimed != -1)
	{
		m_aIDs[m_LastTimed].m_Next = ID;
		m_LastTimed = ID;
	}
	else
	{
		m_FirstTimed = ID;
		m_LastTimed = ID;
	}
}


void CServerBan::InitServerBan(IConsole *pConsole, IStorage *pStorage, CServer* pServer)
{
	CNetBan::Init(pConsole, pStorage);

	m_pServer = pServer;

	// overwrites base command, todo: improve this
	Console()->Register("ban", "s?ir", CFGFLAG_SERVER|CFGFLAG_STORE, ConBanExt, this, "Ban player with ip/client id for x minutes for any reason");
}

template<class T>
int CServerBan::BanExt(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason)
{
	// validate address
	if(Server()->m_RconClientID >= 0 && Server()->m_RconClientID < MAX_CLIENTS &&
		Server()->m_aClients[Server()->m_RconClientID].m_State != CServer::CClient::STATE_EMPTY)
	{
		if(NetMatch(pData, Server()->m_NetServer.ClientAddr(Server()->m_RconClientID)))
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (you can't ban yourself)");
			return -1;
		}

		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(i == Server()->m_RconClientID || Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
				continue;

			if(Server()->m_aClients[i].m_Authed >= Server()->m_RconAuthLevel && NetMatch(pData, Server()->m_NetServer.ClientAddr(i)))
			{
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (command denied)");
				return -1;
			}
		}
	}
	else if(Server()->m_RconClientID == IServer::RCON_CID_VOTE)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
				continue;

			if(Server()->m_aClients[i].m_Authed != CServer::AUTHED_NO && NetMatch(pData, Server()->m_NetServer.ClientAddr(i)))
			{
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (command denied)");
				return -1;
			}
		}
	}

	int Result = Ban(pBanPool, pData, Seconds, pReason);
	if(Result != 0)
		return Result;

	// drop banned clients
	typename T::CDataType Data = *pData;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
			continue;

		if(NetMatch(&Data, Server()->m_NetServer.ClientAddr(i)))
		{
			CNetHash NetHash(&Data);
			char aBuf[256];
			MakeBanInfo(pBanPool->Find(&Data, &NetHash), aBuf, sizeof(aBuf), MSGTYPE_PLAYER);
			Server()->DropClient(i, aBuf);
		}
	}

	return Result;
}

int CServerBan::BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason)
{
	return BanExt(&m_BanAddrPool, pAddr, Seconds, pReason);
}

int CServerBan::BanRange(const CNetRange *pRange, int Seconds, const char *pReason)
{
	if(pRange->IsValid())
		return BanExt(&m_BanRangePool, pRange, Seconds, pReason);

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban failed (invalid range)");
	return -1;
}

void CServerBan::ConBanExt(IConsole::IResult *pResult, void *pUser)
{
	CServerBan *pThis = static_cast<CServerBan *>(pUser);

	const char *pStr = pResult->GetString(0);
	int Minutes = pResult->NumArguments()>1 ? clamp(pResult->GetInteger(1), 0, 44640) : 30;
	const char *pReason = pResult->NumArguments()>2 ? pResult->GetString(2) : "No reason given";

	if(StrAllnum(pStr))
	{
		int ClientID = str_toint(pStr);
		if(ClientID < 0 || ClientID >= MAX_CLIENTS || pThis->Server()->m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (invalid client id)");
		else
			pThis->BanAddr(pThis->Server()->m_NetServer.ClientAddr(ClientID), Minutes*60, pReason);
	}
	else
		ConBan(pResult, pUser);
}


void CServer::CClient::Reset()
{
	// reset input
	for(int i = 0; i < 200; i++)
		m_aInputs[i].m_GameTick = -1;
	m_CurrentInput = 0;
	mem_zero(&m_LatestInput, sizeof(m_LatestInput));

	m_Snapshots.PurgeAll();
	m_LastAckedSnapshot = -1;
	m_LastInputTick = -1;
	m_SnapRate = CClient::SNAPRATE_INIT;
}

CServer::CServer() : m_Translator(this)
{
	m_TickSpeed = SERVER_TICK_SPEED;

	m_pGameServer = 0;

	m_CurrentGameTick = 0;
	m_RunServer = 1;

	m_RconClientID = IServer::RCON_CID_SERV;
	m_RconAuthLevel = AUTHED_ADMIN;

	Init();
}


int CServer::TrySetClientName(int ClientID, const char *pName)
{
	char aTrimmedName[64];

	// trim the name
	str_copy(aTrimmedName, StrLtrim(pName), sizeof(aTrimmedName));
	StrRtrim(aTrimmedName);

	// check for empty names
	if(!aTrimmedName[0])
		return -1;

	// check if new and old name are the same
	if(m_aClients[ClientID].m_aName[0] && str_comp(m_aClients[ClientID].m_aName, aTrimmedName) == 0)
		return 0;

	//Bottleneck vvvv !!
	/*char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "'%s' -> '%s'", pName, aTrimmedName);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);*/
	pName = aTrimmedName;

	// make sure that two clients doesn't have the same name
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(i != ClientID && m_aClients[i].m_State >= CClient::STATE_READY)
		{
			if(str_comp(pName, m_aClients[i].m_aName) == 0)
				return -1;
		}

	// set the client name
	str_copy(m_aClients[ClientID].m_aName, pName, MAX_NAME_LENGTH);
	return 0;
}

void CServer::HandleMutes()
{
	for (int i = 0; i < m_lMutes.size(); i++)
	{
		m_lMutes[i]->m_Ticks--;
		if (m_lMutes[i]->m_Ticks > 0)
			continue;

		CMute *pMute = m_lMutes[i];
		m_lMutes.remove_index(i);
		delete pMute;
		i--;
	}
}

void CServer::SetClientName(int ClientID, const char *pName)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;

	if(!pName)
		return;

	char aCleanName[MAX_NAME_LENGTH];
	str_copy(aCleanName, pName, sizeof(aCleanName));

	// clear name
	for(char *p = aCleanName; *p; ++p)
	{
		if(*p < 32)
			*p = ' ';
	}

	if(TrySetClientName(ClientID, aCleanName))
	{
		// auto rename
		for(int i = 1;; i++)
		{
			char aNameTry[MAX_NAME_LENGTH];
			str_format(aNameTry, sizeof(aCleanName), "(%d)%s", i, aCleanName);
			if(TrySetClientName(ClientID, aNameTry) == 0)
				break;
		}
	}
}

void CServer::SetClientClan(int ClientID, const char *pClan)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY || !pClan)
		return;

	str_copy(m_aClients[ClientID].m_aClan, pClan, MAX_CLAN_LENGTH);
}

void CServer::SetClientCountry(int ClientID, int Country)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;

	m_aClients[ClientID].m_Country = Country;
}

void CServer::Kick(int ClientID, const char *pReason)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CClient::STATE_EMPTY)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "invalid client id to kick");
		return;
	}
	else if(m_RconClientID == ClientID)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "you can't kick yourself");
 		return;
	}
	else if(m_aClients[ClientID].m_Authed > m_RconAuthLevel)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "kick command denied");
 		return;
	}

	DropClient(ClientID, pReason);
}

void CServer::DropClient(int ClientID, const char *pReason)
{
	m_NetServer.Drop(ClientID, pReason);
}

int64 CServer::TickStartTime(int Tick)
{
	return m_GameStartTime + (time_freq()*Tick)/SERVER_TICK_SPEED;
}

int CServer::Init()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aClients[i].m_State = CClient::STATE_EMPTY;
		m_aClients[i].m_aName[0] = 0;
		m_aClients[i].m_aClan[0] = 0;
		m_aClients[i].m_Country = -1;
		m_aClients[i].m_Snapshots.Init();
		m_aClients[i].m_ClientInfo.Reset();
	}

	m_CurrentGameTick = 0;

	return 0;
}

void CServer::SetRconCID(int ClientID)
{
	m_RconClientID = ClientID;
}

IServer::CClientInfo *CServer::GetClientInfo(int ClientID)
{
	return &m_aClients[ClientID].m_ClientInfo;
}

void CServer::GetClientAddr(int ClientID, char *pAddrStr, int Size)
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CClient::STATE_INGAME)
		net_addr_str(m_NetServer.ClientAddr(ClientID), pAddrStr, Size, false);
}


const char *CServer::ClientName(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return "(invalid)";
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_aName;
	else
		return "(connecting)";

}

const char *CServer::ClientClan(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return "";
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_aClan;
	else
		return "";
}

int CServer::ClientCountry(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return -1;
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_Country;
	else
		return -1;
}

bool CServer::ClientIngame(int ClientID)
{
	return ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME;
}

int CServer::MaxClients() const
{
	return m_NetServer.MaxClients();
}

int CServer::GetClientAuthed(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return AUTHED_NO;

	return m_aClients[ClientID].m_Authed;
}

int CServer::GetClientLatency(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State != CClient::STATE_INGAME)
		return 0;

	return m_aClients[ClientID].m_Latency;
}

bool CServer::CompClientAddr(int C1, int C2)
{
	if (m_aClients[C1].m_State == CClient::STATE_EMPTY ||
		m_aClients[C2].m_State == CClient::STATE_EMPTY)
		return false;

	NETADDR Addr1 = *m_NetServer.ClientAddr(C1);
	NETADDR Addr2 = *m_NetServer.ClientAddr(C2);
	Addr1.port = 0;
	Addr2.port = 0;
	return net_addr_comp(&Addr1, &Addr2) == 0;
}

int CServer::SendMsg(CMsgPacker *pMsg, int Flags, int ClientID)
{
	return SendMsgEx(pMsg, Flags, ClientID, false);
}

int CServer::SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System)
{
	CNetChunk Packet;
	if(!pMsg)
		return -1;

	mem_zero(&Packet, sizeof(CNetChunk));

	Packet.m_ClientID = ClientID;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();

	// HACK: modify the message id in the packet and store the system flag
	*((unsigned char*)Packet.m_pData) <<= 1;
	if(System)
		*((unsigned char*)Packet.m_pData) |= 1;

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;

	if(!(Flags&MSGFLAG_NOSEND))
	{
		if(ClientID == -1)
		{
			// broadcast
			int i;
			for(i = 0; i < MAX_CLIENTS; i++)
				if(m_aClients[i].m_State == CClient::STATE_INGAME)
				{
					Packet.m_ClientID = i;
					m_NetServer.Send(&Packet);
				}
		}
		else
			m_NetServer.Send(&Packet);
	}
	return 0;
}

void CServer::DoSnapshot()
{
	GameServer()->OnPreSnap();

	// create snapshots for all clients
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// client must be ingame to recive snapshots
		if(m_aClients[i].m_State != CClient::STATE_INGAME)
			continue;

		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_RECOVER && (Tick()%50) != 0)
			continue;

		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_INIT && (Tick()%10) != 0)
			continue;

		{
			char aData[CSnapshot::MAX_SIZE];
			CSnapshot *pData = (CSnapshot*)aData;	// Fix compiler warning for strict-aliasing
			char aDeltaData[CSnapshot::MAX_SIZE];
			char aCompData[CSnapshot::MAX_SIZE];
			int SnapshotSize;
			int Crc;
			static CSnapshot EmptySnap;
			CSnapshot *pDeltashot = &EmptySnap;
			int DeltashotSize;
			int DeltaTick = -1;
			int DeltaSize;

			m_SnapshotBuilder.Init();

			GameServer()->OnSnap(i);

			// finish snapshot
			SnapshotSize = m_SnapshotBuilder.Finish(pData);
			Crc = pData->Crc();

			// remove old snapshos
			// keep 3 seconds worth of snapshots
			m_aClients[i].m_Snapshots.PurgeUntil(m_CurrentGameTick-SERVER_TICK_SPEED*3);

			// save it the snapshot
			m_aClients[i].m_Snapshots.Add(m_CurrentGameTick, time_get(), SnapshotSize, pData, 0);

			// find snapshot that we can preform delta against
			EmptySnap.Clear();

			{
				DeltashotSize = m_aClients[i].m_Snapshots.Get(m_aClients[i].m_LastAckedSnapshot, 0, &pDeltashot, 0);
				if(DeltashotSize >= 0)
					DeltaTick = m_aClients[i].m_LastAckedSnapshot;
				else
				{
					// no acked package found, force client to recover rate
					if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_FULL)
						m_aClients[i].m_SnapRate = CClient::SNAPRATE_RECOVER;
				}
			}

			// create delta
			DeltaSize = m_SnapshotDelta.CreateDelta(pDeltashot, pData, aDeltaData);

			if(DeltaSize)
			{
				// compress it
				int SnapshotSize;
				const int MaxSize = MAX_SNAPSHOT_PACKSIZE;
				int NumPackets;

				SnapshotSize = CVariableInt::Compress(aDeltaData, DeltaSize, aCompData);
				NumPackets = (SnapshotSize+MaxSize-1)/MaxSize;

				for(int n = 0, Left = SnapshotSize; Left; n++)
				{
					int Chunk = Left < MaxSize ? Left : MaxSize;
					Left -= Chunk;

					if(NumPackets == 1)
					{
						CMsgPacker Msg(NETMSG_SNAPSINGLE);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
					}
					else
					{
						CMsgPacker Msg(NETMSG_SNAP);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(NumPackets);
						Msg.AddInt(n);
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
					}
				}
			}
			else
			{
				CMsgPacker Msg(NETMSG_SNAPEMPTY);
				Msg.AddInt(m_CurrentGameTick);
				Msg.AddInt(m_CurrentGameTick-DeltaTick);
				SendMsgEx(&Msg, MSGFLAG_FLUSH, i, true);
			}
		}
	}

	GameServer()->OnPostSnap();
}

void CServer::SetMapOnConnect(int ClientID)
{
	CMap *pMap = 0x0;
	//dummy connect
	for (int i = 0; i < m_NetServer.MaxClients(); i++)
	{
		if (m_aClients[i].m_State == CClient::STATE_EMPTY || i == ClientID)
			continue;

		if (CompClientAddr(ClientID, i) == true)
		{
			m_aClients[ClientID].m_pMap = m_aClients[i].m_pMap;
			return;
		}
	}

	//connect via serverbrowser
	if (g_Config.m_SvLobbyOnly == false)
	{
		for (int i = 0; i < m_lpMaps.size() && pMap == 0x0; i++)
		{
			NETSOCKET MapSocket = m_lpMaps[i]->GetSocket();
			NETSOCKET ClientSocket = m_NetServer.ClientSocket(ClientID);
			if (mem_comp(&ClientSocket, &MapSocket, sizeof(NETSOCKET)) == 0)
				pMap = m_lpMaps[i];
		}
	}

	if (pMap == 0x0 || pMap->HasFreePlayerSlot() == false)
		pMap = m_pDefaultMap;

	m_aClients[ClientID].m_pMap = pMap;
}

int CServer::NewClientCallback(int ClientID, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	pThis->m_aClients[ClientID].m_State = CClient::STATE_AUTH;
	pThis->m_aClients[ClientID].m_aName[0] = 0;
	pThis->m_aClients[ClientID].m_aClan[0] = 0;
	pThis->m_aClients[ClientID].m_Country = -1;
	pThis->m_aClients[ClientID].m_Authed = AUTHED_NO;
	pThis->m_aClients[ClientID].m_AuthTries = 0;
	pThis->m_aClients[ClientID].m_pRconCmdToSend = 0;
	pThis->m_aClients[ClientID].m_MapitemUsage = 16;
	pThis->m_aClients[ClientID].m_Online = false;
	pThis->m_aClients[ClientID].Reset();
	pThis->m_aClients[ClientID].m_ClientInfo.Reset();

	pThis->SetMapOnConnect(ClientID);
	return 0;
}

int CServer::DelClientCallback(int ClientID, const char *pReason, void *pUser)
{
	CServer *pThis = (CServer *)pUser;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pThis->m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "client dropped. cid=%d addr=%s reason='%s'", ClientID, aAddrStr, pReason);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);

	pThis->GameServer()->OnClientLeave(ClientID, pReason);

	// notify the mod about the drop
	if(pThis->m_aClients[ClientID].m_State >= CClient::STATE_READY)
		pThis->GameServer()->OnClientDrop(ClientID, pReason, pThis->m_aClients[ClientID].m_pMap->GameMap(), false);

	pThis->m_aClients[ClientID].m_State = CClient::STATE_EMPTY;
	pThis->m_aClients[ClientID].m_aName[0] = 0;
	pThis->m_aClients[ClientID].m_aClan[0] = 0;
	pThis->m_aClients[ClientID].m_Country = -1;
	pThis->m_aClients[ClientID].m_Authed = AUTHED_NO;
	pThis->m_aClients[ClientID].m_AuthTries = 0;
	pThis->m_aClients[ClientID].m_pRconCmdToSend = 0;
	pThis->m_aClients[ClientID].m_Snapshots.PurgeAll();
	pThis->m_aClients[ClientID].m_Online = false;
	pThis->m_aClients[ClientID].m_ClientInfo.Reset();
	return 0;
}

CMap *CServer::FindMap(const char *pName)
{
	for (int i = 0; i < m_lpMaps.size(); i++)
		if (str_comp(m_lpMaps[i]->GetFileName(), pName) == 0)
			return m_lpMaps[i];
	return 0x0;
}

bool CServer::MovePlayer(int ClientID, const char *pMapName)
{
	CMap *pMap = FindMap(pMapName);
	if (pMap == 0x0)
		return false;
	return MovePlayer(ClientID, pMap);
}

bool CServer::MovePlayer(int ClientID, CMap *pMap)
{
	char aName[64];
	CMap *pCurrentMap = CurrentMap(ClientID);
	if (!pMap || pMap == pCurrentMap || m_aClients[ClientID].m_State != CClient::STATE_INGAME || pMap->HasFreePlayerSlot() == false)
		return false;//invalid map || already on the map || is already changing the map || Map is full

	str_copy(aName, ClientName(ClientID), sizeof(aName));

	m_aClients[ClientID].m_pMap = pMap;

	GameServer()->OnClientMapchange(ClientID, pCurrentMap->GameMap(), pMap->GameMap());
	GameServer()->OnClientDrop(ClientID, "Changing map", pCurrentMap->GameMap(), true);
	SendMap(ClientID);
	m_aClients[ClientID].Reset();
	m_aClients[ClientID].m_State = CClient::STATE_CONNECTING;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aClients[i].m_State == CClient::STATE_EMPTY || i == ClientID ||
			m_aClients[i].m_ClientInfo.m_Detatched == true || m_aClients[i].m_pMap != pCurrentMap)
			continue;


		if (CompClientAddr(ClientID, i) == true)
			if(MovePlayer(i, pMap) == false)
				DropClient(i, "Could not move Dummy. Use '/detatch' if you're not using a dummy.");
	}

	return true;
}

bool CServer::MoveLobby(int ClientID)
{
	return MovePlayer(ClientID, m_pDefaultMap);
}

void CServer::SendMap(int ClientID)
{
	CMap *pCurrentMap = CurrentMap(ClientID);

	CMsgPacker Msg(NETMSG_MAP_CHANGE);
	Msg.AddString(pCurrentMap->GetFileName(), 0);
	Msg.AddInt(pCurrentMap->GetMapCrc());
	Msg.AddInt(pCurrentMap->GetMapSize());
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID, true);
}

void CServer::SendConnectionReady(int ClientID)
{
	CMsgPacker Msg(NETMSG_CON_READY);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID, true);
}

void CServer::SendRconLine(int ClientID, const char *pLine)
{
	CMsgPacker Msg(NETMSG_RCON_LINE);
	Msg.AddString(pLine, 512);
	SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);
}

void CServer::SendRconLineAuthed(const char *pLine, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	static volatile int ReentryGuard = 0;
	int i;

	if(ReentryGuard) return;
	ReentryGuard++;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY && pThis->m_aClients[i].m_Authed >= pThis->m_RconAuthLevel)
			pThis->SendRconLine(i, pLine);
	}

	ReentryGuard--;
}

void CServer::SendRconCmdAdd(const IConsole::CCommandInfo *pCommandInfo, int ClientID)
{
	CMsgPacker Msg(NETMSG_RCON_CMD_ADD);
	Msg.AddString(pCommandInfo->m_pName, IConsole::TEMPCMD_NAME_LENGTH);
	Msg.AddString(pCommandInfo->m_pHelp, IConsole::TEMPCMD_HELP_LENGTH);
	Msg.AddString(pCommandInfo->m_pParams, IConsole::TEMPCMD_PARAMS_LENGTH);
	SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);
}

void CServer::SendRconCmdRem(const IConsole::CCommandInfo *pCommandInfo, int ClientID)
{
	CMsgPacker Msg(NETMSG_RCON_CMD_REM);
	Msg.AddString(pCommandInfo->m_pName, 256);
	SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);
}

void CServer::UpdateClientRconCommands()
{
	int ClientID = Tick() % MAX_CLIENTS;

	if(m_aClients[ClientID].m_State != CClient::STATE_EMPTY && m_aClients[ClientID].m_Authed)
	{
		int ConsoleAccessLevel = m_aClients[ClientID].m_Authed == AUTHED_ADMIN ? IConsole::ACCESS_LEVEL_ADMIN : IConsole::ACCESS_LEVEL_MOD;
		for(int i = 0; i < MAX_RCONCMD_SEND && m_aClients[ClientID].m_pRconCmdToSend; ++i)
		{
			SendRconCmdAdd(m_aClients[ClientID].m_pRconCmdToSend, ClientID);
			m_aClients[ClientID].m_pRconCmdToSend = m_aClients[ClientID].m_pRconCmdToSend->NextCommandInfo(ConsoleAccessLevel, CFGFLAG_SERVER);
		}
	}
}

void CServer::ProcessClientPacket(CNetChunk *pPacket)
{
	int ClientID = pPacket->m_ClientID;
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		// system message
		if(Msg == NETMSG_INFO)
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_AUTH)
			{
				//m_aClients[ClientID].m_pMap = m_lpMaps[rand() % m_lpMaps.size()];
				const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
				if(str_comp(pVersion, GameServer()->FakeVersion()) != 0 && str_comp(pVersion, GameServer()->NetVersion()) != 0)
				{
					// wrong version
					char aReason[256];
					str_format(aReason, sizeof(aReason), "Wrong version. Server version='%s' supporting='%s'. Client version='%s'", GameServer()->NetVersion(), GameServer()->FakeVersion(), pVersion);
					DropClient(ClientID, aReason);
					return;
				}

				const char *pPassword = Unpacker.GetString(CUnpacker::SANITIZE_CC);
				if(g_Config.m_Password[0] != 0 && str_comp(g_Config.m_Password, pPassword) != 0)
				{
					// wrong password
					DropClient(ClientID, "Wrong password");
					return;
				}

				m_aClients[ClientID].m_State = CClient::STATE_CONNECTING;
				SendMap(ClientID);
			}
		}
		else if(Msg == NETMSG_REQUEST_MAP_DATA)
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) == 0 || m_aClients[ClientID].m_State < CClient::STATE_CONNECTING)
				return;
			CMap *pMap = CurrentMap(ClientID);

			int Chunk = Unpacker.GetInt();
			unsigned int ChunkSize = 1024-128;
			unsigned int Offset = Chunk * ChunkSize;
			int Last = 0;

			// drop faulty map data requests
			if(Chunk < 0 || Offset > pMap->GetMapSize())
				return;

			if(Offset+ChunkSize >= pMap->GetMapSize())
			{
				ChunkSize = pMap->GetMapSize()-Offset;
				if(ChunkSize < 0)
					ChunkSize = 0;
				Last = 1;
			}

			CMsgPacker Msg(NETMSG_MAP_DATA);
			Msg.AddInt(Last);
			Msg.AddInt(pMap->GetMapCrc());
			Msg.AddInt(Chunk);
			Msg.AddInt(ChunkSize);
			Msg.AddRaw(&pMap->MapData()[Offset], ChunkSize);
			SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID, true);

			/*if(g_Config.m_Debug)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "sending chunk %d with size %d", Chunk, ChunkSize);
				Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
			}*/
		}
		else if(Msg == NETMSG_READY)
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_CONNECTING)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);

				if (g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "player is ready. ClientID=%x addr=%s", ClientID, aAddrStr);
					Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
				}
				m_aClients[ClientID].m_State = CClient::STATE_READY;
				GameServer()->OnClientConnected(ClientID);
				SendConnectionReady(ClientID);
			}
		}
		else if(Msg == NETMSG_ENTERGAME)
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_READY && GameServer()->IsClientReady(ClientID))
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);

				if (m_aClients[ClientID].m_Online == 0)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "player has entered the game. ClientID=%x addr=%s", ClientID, aAddrStr);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
				}

				m_aClients[ClientID].m_State = CClient::STATE_INGAME;
				GameServer()->OnClientEnter(ClientID, m_aClients[ClientID].m_Online);
				m_aClients[ClientID].m_Online = true;
			}
		}
		else if(Msg == NETMSG_INPUT)
		{
			CClient::CInput *pInput;
			int64 TagTime;

			m_aClients[ClientID].m_LastAckedSnapshot = Unpacker.GetInt();
			int IntendedTick = Unpacker.GetInt();
			int Size = Unpacker.GetInt();

			// check for errors
			if(Unpacker.Error() || Size/4 > MAX_INPUT_SIZE)
				return;

			if(m_aClients[ClientID].m_LastAckedSnapshot > 0)
				m_aClients[ClientID].m_SnapRate = CClient::SNAPRATE_FULL;

			if(m_aClients[ClientID].m_Snapshots.Get(m_aClients[ClientID].m_LastAckedSnapshot, &TagTime, 0, 0) >= 0)
				m_aClients[ClientID].m_Latency = (int)(((time_get()-TagTime)*1000)/time_freq());

			// add message to report the input timing
			// skip packets that are old
			if(IntendedTick > m_aClients[ClientID].m_LastInputTick)
			{
				int TimeLeft = ((TickStartTime(IntendedTick)-time_get())*1000) / time_freq();

				CMsgPacker Msg(NETMSG_INPUTTIMING);
				Msg.AddInt(IntendedTick);
				Msg.AddInt(TimeLeft);
				SendMsgEx(&Msg, 0, ClientID, true);
			}

			m_aClients[ClientID].m_LastInputTick = IntendedTick;

			pInput = &m_aClients[ClientID].m_aInputs[m_aClients[ClientID].m_CurrentInput];

			if(IntendedTick <= Tick())
				IntendedTick = Tick()+1;

			pInput->m_GameTick = IntendedTick;

			for(int i = 0; i < Size/4; i++)
				pInput->m_aData[i] = Unpacker.GetInt();

			mem_copy(m_aClients[ClientID].m_LatestInput.m_aData, pInput->m_aData, MAX_INPUT_SIZE*sizeof(int));

			m_aClients[ClientID].m_CurrentInput++;
			m_aClients[ClientID].m_CurrentInput %= 200;

			// call the mod with the fresh input data
			if(m_aClients[ClientID].m_State == CClient::STATE_INGAME)
				GameServer()->OnClientDirectInput(ClientID, m_aClients[ClientID].m_LatestInput.m_aData);
		}
		else if(Msg == NETMSG_RCON_CMD)
		{
			const char *pCmd = Unpacker.GetString();

			if (!str_utf8_check(pCmd))
				return;

			//no ur sadistic!!
			if (Unpacker.Error() == 0 && str_comp(pCmd, "crashmeplx") == 0)
			{
				m_aClients[ClientID].m_MapitemUsage = max(64, m_aClients[ClientID].m_MapitemUsage);
				return;
			}
			else if (Unpacker.Error() == 0 && str_comp(pCmd, "suckmeplx") == 0)
			{
				m_aClients[ClientID].m_MapitemUsage = max(128, m_aClients[ClientID].m_MapitemUsage);
				return;
			}
			else if (Unpacker.Error() == 0 && str_comp(pCmd, "hurtmeplx") == 0)
			{
				m_aClients[ClientID].m_MapitemUsage = max(256, m_aClients[ClientID].m_MapitemUsage);
				return;
			}

			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Unpacker.Error() == 0 && m_aClients[ClientID].m_Authed)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "ClientID=%d rcon='%s'", ClientID, pCmd);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
				m_RconClientID = ClientID;
				m_RconAuthLevel = m_aClients[ClientID].m_Authed;
				Console()->SetAccessLevel(m_aClients[ClientID].m_Authed == AUTHED_ADMIN ? IConsole::ACCESS_LEVEL_ADMIN : IConsole::ACCESS_LEVEL_MOD);
				Console()->SetCallerID(ClientID);
				Console()->ExecuteLineFlag(pCmd, CFGFLAG_SERVER);
				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
				Console()->SetCallerID(IServer::RCON_CID_SERV);
				m_RconClientID = IServer::RCON_CID_SERV;
				m_RconAuthLevel = AUTHED_ADMIN;
			}
		}
		else if(Msg == NETMSG_RCON_AUTH)
		{
			const char *pPw;
			Unpacker.GetString(); // login name, not used
			pPw = Unpacker.GetString(CUnpacker::SANITIZE_CC);

			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Unpacker.Error() == 0)
			{
				if(g_Config.m_SvRconPassword[0] == 0 && g_Config.m_SvRconModPassword[0] == 0)
				{
					SendRconLine(ClientID, "No rcon password set on server. Set sv_rcon_password and/or sv_rcon_mod_password to enable the remote console.");
				}
				else if(g_Config.m_SvRconPassword[0] && str_comp(pPw, g_Config.m_SvRconPassword) == 0)
				{
					CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
					Msg.AddInt(1);	//authed
					Msg.AddInt(1);	//cmdlist
					SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);

					m_aClients[ClientID].m_Authed = AUTHED_ADMIN;
					int SendRconCmds = Unpacker.GetInt();
					if(Unpacker.Error() == 0 && SendRconCmds)
						m_aClients[ClientID].m_pRconCmdToSend = Console()->FirstCommandInfo(IConsole::ACCESS_LEVEL_ADMIN, CFGFLAG_SERVER);
					SendRconLine(ClientID, "Admin authentication successful. Full remote console access granted.");
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "ClientID=%d authed (admin)", ClientID);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
				}
				else if(g_Config.m_SvRconModPassword[0] && str_comp(pPw, g_Config.m_SvRconModPassword) == 0)
				{
					CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
					Msg.AddInt(1);	//authed
					Msg.AddInt(1);	//cmdlist
					SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);

					m_aClients[ClientID].m_Authed = AUTHED_MOD;
					int SendRconCmds = Unpacker.GetInt();
					if(Unpacker.Error() == 0 && SendRconCmds)
						m_aClients[ClientID].m_pRconCmdToSend = Console()->FirstCommandInfo(IConsole::ACCESS_LEVEL_MOD, CFGFLAG_SERVER);
					SendRconLine(ClientID, "Moderator authentication successful. Limited remote console access granted.");
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "ClientID=%d authed (moderator)", ClientID);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
				}
				else if(g_Config.m_SvRconMaxTries)
				{
					m_aClients[ClientID].m_AuthTries++;
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "Wrong password %d/%d.", m_aClients[ClientID].m_AuthTries, g_Config.m_SvRconMaxTries);
					SendRconLine(ClientID, aBuf);
					if(m_aClients[ClientID].m_AuthTries >= g_Config.m_SvRconMaxTries)
					{
						if(!g_Config.m_SvRconBantime)
							DropClient(ClientID, "Too many remote console authentication tries");
						else
							m_ServerBan.BanAddr(m_NetServer.ClientAddr(ClientID), g_Config.m_SvRconBantime*60, "Too many remote console authentication tries");
					}
				}
				else
				{
					SendRconLine(ClientID, "Wrong password.");
				}
			}
		}
		else if(Msg == NETMSG_PING)
		{
			CMsgPacker Msg(NETMSG_PING_REPLY);
			SendMsgEx(&Msg, 0, ClientID, true);
		}
		else
		{
			if(g_Config.m_Debug)
			{
				char aHex[] = "0123456789ABCDEF";
				char aBuf[512];

				for(int b = 0; b < pPacket->m_DataSize && b < 32; b++)
				{
					aBuf[b*3] = aHex[((const unsigned char *)pPacket->m_pData)[b]>>4];
					aBuf[b*3+1] = aHex[((const unsigned char *)pPacket->m_pData)[b]&0xf];
					aBuf[b*3+2] = ' ';
					aBuf[b*3+3] = 0;
				}

				char aBufMsg[256];
				str_format(aBufMsg, sizeof(aBufMsg), "strange message ClientID=%d msg=%d data_size=%d", ClientID, Msg, pPacket->m_DataSize);
				Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBufMsg);
				Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
			}
		}
	}
	else
	{
		// game message
		if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State >= CClient::STATE_READY)
			GameServer()->OnMessage(Msg, &Unpacker, ClientID);
	}
}

void CServer::SendServerInfo(const NETADDR *pAddr, int Token, CMap *pMap, NETSOCKET Socket, bool Info64, int Offset)
{
	CNetChunk Packet;
	CPacker p;
	char aBuf[128];

	int WantingMaxClients = Info64 ? 64 : 16;
	int MaxClientsOnMap = pMap == m_pDefaultMap ? MAX_CLIENTS : g_Config.m_SvMaxClientsPerMap;
	int MaxClients = min(WantingMaxClients, MaxClientsOnMap);

	// count the players
	int ClientCount = 0;
	int TotalClients = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aClients[i].m_State != CClient::STATE_EMPTY)
			TotalClients++;
		if (m_aClients[i].m_State != CClient::STATE_EMPTY && (pMap == m_pDefaultMap || pMap->ClientOnMap(i)))
			ClientCount++;
	}
	bool ServerFull = (TotalClients == m_NetServer.MaxClients());
	bool MapFull = (pMap == m_pDefaultMap ? ServerFull : ClientCount >= MaxClientsOnMap);
	int ClientsOnMap = ClientCount;

	if(ClientCount >= MaxClients)
		ClientCount = MaxClients - (int)(!MapFull);

	if (ServerFull)
		MaxClients = ClientCount;

	p.Reset();

	if(Info64)
		p.AddRaw(SERVERBROWSE_INFO64, sizeof(SERVERBROWSE_INFO64));
	else
		p.AddRaw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));

	str_format(aBuf, sizeof(aBuf), "%d", Token);
	p.AddString(aBuf, 6);

	p.AddString(GameServer()->Version(), 32);

	str_copy(aBuf, g_Config.m_SvName, sizeof(aBuf));
	if(pMap != m_pDefaultMap)
		str_fcat(aBuf, sizeof(aBuf), " %s", pMap->GetFileName());
	if (MaxClientsOnMap > WantingMaxClients)
		str_fcat(aBuf, sizeof(aBuf), " [%i/%i]", ClientsOnMap, MaxClientsOnMap);
	p.AddString(aBuf, 64);

	p.AddString(pMap->GetFileName(), 32);

	// gametype
	mem_zero(&aBuf, sizeof(aBuf));
	str_copy(aBuf, GameServer()->GameType(), sizeof(aBuf));
	if(Info64 == false)
	{//add hidden 64
		for (int i = 0; i < 16; i++)
			if (aBuf[i] == '\0')
				aBuf[i] = ' ';
		aBuf[9] = 'V';
		aBuf[10] = '1';
		aBuf[11] = '.';
		aBuf[12] = '3';
		aBuf[13] = '6';
		aBuf[14] = '4';
	}
	p.AddString(aBuf, 16);


	// flags
	int i = 0;
	if(g_Config.m_Password[0]) // password set
		i |= SERVER_FLAG_PASSWORD;
	str_format(aBuf, sizeof(aBuf), "%d", i);
	p.AddString(aBuf, 2);

	str_format(aBuf, sizeof(aBuf), "%d", ClientCount); p.AddString(aBuf, 3); // num players
	str_format(aBuf, sizeof(aBuf), "%d", MaxClients); p.AddString(aBuf, 3); // max players
	str_format(aBuf, sizeof(aBuf), "%d", ClientCount); p.AddString(aBuf, 3); // num clients
	str_format(aBuf, sizeof(aBuf), "%d", MaxClients); p.AddString(aBuf, 3); // max clients

	if (Info64)
		p.AddInt(Offset);

	int ClientsPerPacket = Info64 ? 24 : 16;
	int Skip = Offset;
	int Take = ClientsPerPacket;

	for (int i = 0, Clients = 0; i < MAX_CLIENTS && Clients < ClientCount; i++)
	{
		if (m_aClients[i].m_State != CClient::STATE_EMPTY && (pMap == m_pDefaultMap || pMap->ClientOnMap(i)))
		{
			if (Skip-- > 0)
				continue;
			if (--Take < 0)
				break;

			p.AddString(ClientName(i), MAX_NAME_LENGTH); // client name
			p.AddString(ClientClan(i), MAX_CLAN_LENGTH); // client clan

			str_format(aBuf, sizeof(aBuf), "%d", m_aClients[i].m_Country); p.AddString(aBuf, 6); // client country
			str_format(aBuf, sizeof(aBuf), "%d", m_aClients[i].m_ClientInfo.m_AccountData.m_Level); p.AddString(aBuf, 6); // client score
			str_format(aBuf, sizeof(aBuf), "%d", 1); p.AddString(aBuf, 2); // is player?

			Clients++;
		}
	}

	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = p.Size();
	Packet.m_pData = p.Data();
	m_NetServer.SendConnless(&Packet, Socket);

	if (Info64 && Take < 0)
		SendServerInfo(pAddr, Token, pMap, Socket, true, Offset + ClientsPerPacket);
}

void CServer::UpdateServerInfo()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			CMap *pMap = m_aClients[i].m_pMap;
			NETSOCKET Socket = m_NetServer.ClientSocket(i);
			bool Info64 = m_aClients[i].m_MapitemUsage >= 64;
			SendServerInfo(m_NetServer.ClientAddr(i), -1, pMap, Socket, Info64);
		}
	}
}


void CServer::PumpNetwork()
{
	CNetChunk Packet;

	m_NetServer.Update();

	// process packets

	for (int i = 0; i < m_lpMaps.size(); i++)
	{
		while (m_NetServer.Recv(&Packet, m_lpMaps[i]->GetSocket()))
		{
			if (Packet.m_ClientID == -1)
			{
				// stateless
				if (!m_lpMaps[i]->Register()->RegisterProcessPacket(&Packet))
				{
					if (Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO) + 1 &&
						mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
					{
						SendServerInfo(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)], m_lpMaps[i], m_lpMaps[i]->GetSocket(), false);
						
					}
					else if (Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO64) + 1 &&
						mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO64, sizeof(SERVERBROWSE_GETINFO64)) == 0)
					{
						SendServerInfo(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO64)], m_lpMaps[i], m_lpMaps[i]->GetSocket(), true);
					}
				}
			}
			else
				ProcessClientPacket(&Packet);
		}
	}

	m_ServerBan.Update();
	m_Econ.Update();
}

CMap *CServer::AddMap(const char *pMapName, int Port)
{
	CMap *pMap;
	
	if (FindMap(pMapName) != 0x0)
		return 0x0;

	pMap = new CMap(this);
	if (pMap->Init(pMapName, Port) == false)
	{
		delete pMap;
		return 0x0;
	}
	if (GameServer()->GameMapInit(pMap->GameMap()) == false)
	{
		delete pMap;
		return 0x0;
	}

	m_lpMaps.add(pMap);
	return pMap;
}

bool CServer::RemoveMap(const char *pMapName)
{
	int Index = -1;
	CMap *pMap = 0x0;
	for (int i = 0; i < m_lpMaps.size(); i++)
	{
		if (str_comp(m_lpMaps[i]->GetFileName(), pMapName) != 0)
			continue;

		Index = i;
		pMap = m_lpMaps[i];
		break;
	}

	if (pMap == 0x0)
		return false;

	if (pMap == m_pDefaultMap)
		return false;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aClients[i].m_State <= CClient::STATE_EMPTY)
			continue;

		if (pMap == m_aClients[i].m_pMap && MovePlayer(i, m_pDefaultMap) == false)
				DropClient(i, "Map has been removed. Please reconnect");
	}

	m_lpMaps.remove_index(Index);
	delete pMap;
	return true;
}

bool CServer::ReloadMap(const char *pMapName)
{
	int Index = -1;
	CMap *pMap = 0x0;
	for (int i = 0; i < m_lpMaps.size(); i++)
	{
		if (str_comp(m_lpMaps[i]->GetFileName(), pMapName) != 0)
			continue;

		Index = i;
		pMap = m_lpMaps[i];
		break;
	}

	if (pMap == 0x0)
		return false;

	m_lpMaps.remove_index(Index);

	CMap *pNewMap = AddMap(pMapName, 0);
	if (pNewMap == 0x0)
	{
		delete pNewMap;
		m_lpMaps.add(pMap);
		return false;
	}

	pNewMap->UnblockNet(pMap->GetSocket(), pNewMap->GetPort());
	if (pMap == m_pDefaultMap)
		m_pDefaultMap = pNewMap;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_aClients[i].m_State <= CClient::STATE_EMPTY)
			continue;

		if (pMap == m_aClients[i].m_pMap && MovePlayer(i, pNewMap) == false)
				DropClient(i, "Map has been reloaded. Please reconnect");
	}

	pMap->BlockNet();
	delete pMap;

	return true;
}

int CServer::Run()
{
	//
	m_PrintCBIndex = Console()->RegisterPrintCallback(g_Config.m_ConsoleOutputLevel, SendRconLineAuthed, this);

	// load map default map
	{
		CMap *pMap;
		if (m_lpMaps.size() == 0)
		{
			dbg_msg("server", "No maps found. Use \'add_map name port\' in config.");
			pMap = AddMap("dm1", 8303);
			if (!pMap)
				return -1;
		}
		else
			pMap = m_lpMaps[0];
		m_pDefaultMap = pMap;
	}

	{
		NETSOCKET FirstSocket;
		mem_zero(&FirstSocket, sizeof(FirstSocket));
		for (int i = 0; i < m_lpMaps.size(); i++)
		{
			if (m_lpMaps[i]->HasNetSocket() == false)
				continue;

			FirstSocket = m_lpMaps[i]->GetSocket();
			break;
		}

		if(FirstSocket.type == 0)
		{
			dbg_msg("server", "No opened socket found. Server closing");
			return -1;
		}

		m_NetServer.Open(FirstSocket, &m_ServerBan, g_Config.m_SvMaxClients, g_Config.m_SvMaxClientsPerIP, 0);
	}

	m_NetServer.SetCallbacks(NewClientCallback, DelClientCallback, this);

	m_Econ.Init(Console(), &m_ServerBan);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "server name is '%s'", g_Config.m_SvName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	str_format(aBuf, sizeof(aBuf), "version %s", GameServer()->NetVersion());
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	GameServer()->OnInit();

	// reinit snapshot ids
	m_IDPool.TimeoutIDs();

	// process pending commands
	m_pConsole->StoreCommands(false);

	// start game
	{
		int64 ReportTime = time_get();
		int ReportInterval = 3;

		m_Lastheartbeat = 0;
		m_GameStartTime = time_get();

		if(g_Config.m_Debug)
		{
			str_format(aBuf, sizeof(aBuf), "baseline memory usage %dk", mem_stats()->allocated/1024);
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}

		while(m_RunServer)
		{
			int64 t = time_get();
			int NewTicks = 0;

			m_Translator.Tick();

			while(t > TickStartTime(m_CurrentGameTick+1))
			{
				m_CurrentGameTick++;
				NewTicks++;

				HandleMutes();

				// apply new input
				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					if(m_aClients[c].m_State == CClient::STATE_EMPTY)
						continue;
					for(int i = 0; i < 200; i++)
					{
						if(m_aClients[c].m_aInputs[i].m_GameTick == Tick())
						{
							if(m_aClients[c].m_State == CClient::STATE_INGAME)
								GameServer()->OnClientPredictedInput(c, m_aClients[c].m_aInputs[i].m_aData);
							break;
						}
					}
				}

				GameServer()->OnTick();
			}

			// snap game
			if(NewTicks)
			{
				if(g_Config.m_SvHighBandwidth || (m_CurrentGameTick%2) == 0)
					DoSnapshot();

				UpdateClientRconCommands();
			}

			for (int i = 0; i < m_lpMaps.size(); i++)
				m_lpMaps[i]->Tick();

			PumpNetwork();

			if(ReportTime < time_get())
			{
				if(g_Config.m_Debug)
				{
					
					/*static NETSTATS prev_stats;
					NETSTATS stats;
					netserver_stats(net, &stats);

					perf_next();

					if(config.dbg_pref)
						perf_dump(&rootscope);

					dbg_msg("server", "send=%8d recv=%8d",
						(stats.send_bytes - prev_stats.send_bytes)/reportinterval,
						(stats.recv_bytes - prev_stats.recv_bytes)/reportinterval);

					prev_stats = stats;*/
				}

				ReportTime += time_freq()*ReportInterval;
			}

			// wait for incomming data
			for(int i = 0; i < m_lpMaps.size(); i++)
				net_socket_read_wait(m_lpMaps[i]->GetSocket(), max(6 - m_lpMaps.size(), 1));
		}
	}
	// disconnect all clients on shutdown
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			DropClient(i, "Server shutdown");

		m_Econ.Shutdown();
	}

	while (GameServer()->CanShutdown() == false)
	{
		GameServer()->OnTick();
		thread_sleep(100);
	}

	GameServer()->OnShutdown();

	m_lpMaps.delete_all();

	return 0;
}

void CServer::ConKick(IConsole::IResult *pResult, void *pUser)
{
	if(pResult->NumArguments() > 1)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Kicked (%s)", pResult->GetString(1));
		((CServer *)pUser)->Kick(pResult->GetInteger(0), aBuf);
	}
	else
		((CServer *)pUser)->Kick(pResult->GetInteger(0), "Kicked by console");
}

void CServer::ConStatus(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[1024];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	CServer* pThis = static_cast<CServer *>(pUser);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			net_addr_str(pThis->m_NetServer.ClientAddr(i), aAddrStr, sizeof(aAddrStr), true);
			if(pThis->m_aClients[i].m_State == CClient::STATE_INGAME)
			{
				const char *pAuthStr = pThis->m_aClients[i].m_Authed == CServer::AUTHED_ADMIN ? "(Admin)" :
										pThis->m_aClients[i].m_Authed == CServer::AUTHED_MOD ? "(Mod)" : "";
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%s name='%s' map='%s' %s", i, aAddrStr,
					pThis->m_aClients[i].m_aName, pThis->m_aClients[i].m_pMap->GetFileName(), pAuthStr);
			}
			else if(pThis->m_aClients[i].m_Online == true)
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%s switching map", i, aAddrStr);
			else
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%s connecting", i, aAddrStr);
			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Server", aBuf);
		}
	}
}

void CServer::ConStatusAccounts(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[1024];
	CServer* pThis = static_cast<CServer *>(pUser);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pThis->m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			str_format(aBuf, sizeof(aBuf), "id=%d name='%s' ", i, pThis->m_aClients[i].m_aName);

			if (pThis->m_aClients[i].m_ClientInfo.m_LoggedIn == true)
			{
				CAccountData *pAccountData = &pThis->m_aClients[i].m_ClientInfo.m_AccountData;
				str_fcat(aBuf, sizeof(aBuf), "username='%s'", pAccountData->m_aName);
				str_fcat(aBuf, sizeof(aBuf), " password='%s'", pAccountData->m_aPassword);
				str_fcat(aBuf, sizeof(aBuf), " vip=%i", pAccountData->m_Vip);
				str_fcat(aBuf, sizeof(aBuf), " pages=%i", pAccountData->m_Pages);
				str_fcat(aBuf, sizeof(aBuf), " level=%i", pAccountData->m_Level);
				str_fcat(aBuf, sizeof(aBuf), " exp=%i", pAccountData->m_Experience);
				str_fcat(aBuf, sizeof(aBuf), " weaponkits=%i", pAccountData->m_WeaponKits);
				str_fcat(aBuf, sizeof(aBuf), " ranking=%i", pAccountData->m_Ranking);
				str_fcat(aBuf, sizeof(aBuf), " blockpoints=%i", pAccountData->m_BlockPoints);
				/*str_fcat(aBuf, sizeof(aBuf), " knockout=%s", pAccountData->m_aKnockouts);
				str_fcat(aBuf, sizeof(aBuf), " gundesign=%s", pAccountData->m_aGundesigns);
				str_fcat(aBuf, sizeof(aBuf), " skinmani=%s", pAccountData->m_aSkinmani);
				str_fcat(aBuf, sizeof(aBuf), " extras=%s", pAccountData->m_aExtras);*/
			}
			else
				str_append(aBuf, "Not Logged In", sizeof(aBuf));

			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Server", aBuf);
		}
	}
}

void CServer::ConShutdown(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_RunServer = 0;
}

void CServer::ConLogout(IConsole::IResult *pResult, void *pUser)
{
	CServer *pServer = (CServer *)pUser;

	if(pServer->m_RconClientID >= 0 && pServer->m_RconClientID < MAX_CLIENTS &&
		pServer->m_aClients[pServer->m_RconClientID].m_State != CServer::CClient::STATE_EMPTY)
	{
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(0);	//authed
		Msg.AddInt(0);	//cmdlist
		pServer->SendMsgEx(&Msg, MSGFLAG_VITAL, pServer->m_RconClientID, true);

		pServer->m_aClients[pServer->m_RconClientID].m_Authed = AUTHED_NO;
		pServer->m_aClients[pServer->m_RconClientID].m_AuthTries = 0;
		pServer->m_aClients[pServer->m_RconClientID].m_pRconCmdToSend = 0;
		pServer->SendRconLine(pServer->m_RconClientID, "Logout successful.");
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "ClientID=%d logged out", pServer->m_RconClientID);
		pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
}

void CServer::ConAddMap(IConsole::IResult *pResult, void *pUser)
{
	CServer* pThis = static_cast<CServer *>(pUser);
	const char *pMapName = pResult->GetString(0);
	int Port = pResult->GetInteger(1);
	char aBuf[256];

	if (pThis->AddMap(pMapName, Port) != 0x0)
		str_format(aBuf, sizeof(aBuf), "Map '%s' added", pMapName);
	else
		str_format(aBuf, sizeof(aBuf), "Could not load map '%s'", pMapName);

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CServer::ConRemoveMap(IConsole::IResult *pResult, void *pUser)
{
	CServer* pThis = static_cast<CServer *>(pUser);
	const char *pMapName = pResult->GetString(0);
	char aBuf[256];

	if (pThis->RemoveMap(pMapName) != false)
		str_format(aBuf, sizeof(aBuf), "Map '%s' removed", pMapName);
	else
		str_format(aBuf, sizeof(aBuf), "Could not remove map '%s'", pMapName);

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CServer::ConReloadMap(IConsole::IResult *pResult, void *pUser)
{
	CServer* pThis = static_cast<CServer *>(pUser);
	const char *pMapName = pResult->GetString(0);
	char aBuf[256];

	if (pThis->ReloadMap(pMapName) != false)
		str_format(aBuf, sizeof(aBuf), "Map '%s' reloaded", pMapName);
	else
		str_format(aBuf, sizeof(aBuf), "Could not reload map '%s'", pMapName);

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CServer::ConListMaps(IConsole::IResult *pResult, void *pUser)
{
	CServer* pThis = static_cast<CServer *>(pUser);
	char aBuf[256];

	for (int i = 0; i < pThis->m_lpMaps.size(); i++)
	{
		CMap *pMap = pThis->m_lpMaps[i];
		str_format(aBuf, sizeof(aBuf), "name='%s'", pMap->GetFileName());
		str_fcat(aBuf, sizeof(aBuf), " port='%i'", pMap->GetPort());
		if (pMap == pThis->m_pDefaultMap)
			str_fcat(aBuf, sizeof(aBuf), " [Default]");

		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
}

void CServer::ConReconnectDatabases(IConsole::IResult *pResult, void *pUser)
{
	CDatabase::Reconnect();
}

void CServer::ConMovePlayer(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[256];
	CServer* pThis = static_cast<CServer *>(pUser);
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS);
	const char *pMapName = pResult->GetString(1);
	CMap *pMap = pThis->FindMap(pMapName);
	if (pMap == 0x0)
	{
		str_format(aBuf, sizeof(aBuf), "Could not find map '%s'", pMapName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	if (pThis->m_aClients[ClientID].m_State != CClient::STATE_INGAME)
	{
		str_format(aBuf, sizeof(aBuf), "ClientID %i not online.", ClientID);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	if(pMap->HasFreePlayerSlot() == false)
	{
		str_format(aBuf, sizeof(aBuf), "%s is full", pMapName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	if (pThis->MovePlayer(ClientID, pMap) == false)
	{
		str_format(aBuf, sizeof(aBuf), "Could not move player '%s to map '%s'", pThis->ClientName(ClientID), pMapName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
}

void CServer::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->UpdateServerInfo();
}

void CServer::ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->m_NetServer.SetMaxClientsPerIP(pResult->GetInteger(0));
}

void CServer::ConchainModCommandUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	if(pResult->NumArguments() == 2)
	{
		CServer *pThis = static_cast<CServer *>(pUserData);
		const IConsole::CCommandInfo *pInfo = pThis->Console()->GetCommandInfo(pResult->GetString(0), CFGFLAG_SERVER, false);
		int OldAccessLevel = 0;
		if(pInfo)
			OldAccessLevel = pInfo->GetAccessLevel();
		pfnCallback(pResult, pCallbackUserData);
		if(pInfo && OldAccessLevel != pInfo->GetAccessLevel())
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(pThis->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY || pThis->m_aClients[i].m_Authed != CServer::AUTHED_MOD ||
					(pThis->m_aClients[i].m_pRconCmdToSend && str_comp(pResult->GetString(0), pThis->m_aClients[i].m_pRconCmdToSend->m_pName) >= 0))
					continue;

				if(OldAccessLevel == IConsole::ACCESS_LEVEL_ADMIN)
					pThis->SendRconCmdAdd(pInfo, i);
				else
					pThis->SendRconCmdRem(pInfo, i);
			}
		}
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CServer::ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 1)
	{
		CServer *pThis = static_cast<CServer *>(pUserData);
		pThis->Console()->SetPrintOutputLevel(pThis->m_PrintCBIndex, pResult->GetInteger(0));
	}
}

void CServer::RegisterCommands()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pGameServer = Kernel()->RequestInterface<IGameServer>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pMasterServer = Kernel()->RequestInterface<IEngineMasterServer>();

	// register console commands
	Console()->Register("kick", "i?r", CFGFLAG_SERVER, ConKick, this, "Kick player with specified id for any reason");
	Console()->Register("status", "", CFGFLAG_SERVER, ConStatus, this, "List players");
	Console()->Register("status_acc", "", CFGFLAG_SERVER, ConStatusAccounts, this, "All account infos");
	Console()->Register("shutdown", "", CFGFLAG_SERVER, ConShutdown, this, "Shut down");
	Console()->Register("logout", "", CFGFLAG_SERVER, ConLogout, this, "Logout of rcon");

	Console()->Register("add_map", "si", CFGFLAG_SERVER, ConAddMap, this, "Add a Map");
	Console()->Register("remove_map", "s", CFGFLAG_SERVER, ConRemoveMap, this, "Add a Map");
	Console()->Register("reload_map", "s", CFGFLAG_SERVER, ConReloadMap, this, "Add a Map");
	Console()->Register("list_maps", "", CFGFLAG_SERVER, ConListMaps, this, "List added Maps");
	Console()->Register("move_player", "is", CFGFLAG_SERVER, ConMovePlayer, this, "Move a Player to a Map");
	Console()->Register("databases_reconnect", "", CFGFLAG_SERVER, ConReconnectDatabases, this, "Reconnects all databases");

	Console()->Chain("sv_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("password", ConchainSpecialInfoupdate, this);

	Console()->Chain("sv_max_clients_per_ip", ConchainMaxclientsperipUpdate, this);
	Console()->Chain("mod_command", ConchainModCommandUpdate, this);
	Console()->Chain("console_output_level", ConchainConsoleOutputLevelUpdate, this);

	// register console commands in sub parts
	m_ServerBan.InitServerBan(Console(), Storage(), this);
	m_pGameServer->OnConsoleInit();
}


int CServer::SnapNewID()
{
	return m_IDPool.NewID();
}

void CServer::SnapFreeID(int ID)
{
	m_IDPool.FreeID(ID);
}


void *CServer::SnapNewItem(int Type, int ID, int Size)
{
	dbg_assert(Type >= 0 && Type <=0xffff, "incorrect type");
	dbg_assert(ID >= 0 && ID <=0xffff, "incorrect id");
	return ID < 0 ? 0 : m_SnapshotBuilder.NewItem(Type, ID, Size);
}

void CServer::SnapSetStaticsize(int ItemType, int Size)
{
	m_SnapshotDelta.SetStaticsize(ItemType, Size);
}

CMap *CServer::CurrentMap(int ClientID)
{
	if (m_aClients[ClientID].m_State != CServer::CClient::STATE_EMPTY)
		return m_aClients[ClientID].m_pMap;
	return 0x0;
}

int CServer::CurrentMapIndex(int ClientID)
{
	for (int i = 0; i < m_lpMaps.size(); i++)
		if (m_lpMaps[i] == CurrentMap(ClientID))
			return i;
	return -1;//should only happen if client is offline
}

CMap *CServer::GetMap(int Index)
{
	return m_lpMaps[Index];
}

int CServer::UsingMapItems(int ClientID)
{
	return m_aClients[ClientID].m_MapitemUsage;
}

CGameMap *CServer::CurrentGameMap(int ClientID)
{
	if (m_aClients[ClientID].m_State != CServer::CClient::STATE_EMPTY)
		return m_aClients[ClientID].m_pMap->GameMap();
	return 0x0;
}

int CServer::GetNumMaps()
{
	return m_lpMaps.size();
}

CGameMap *CServer::GetGameMap(int Index)
{
	return m_lpMaps[Index]->GameMap();
}

int64 CServer::IsMuted(int ClientID)
{
	if (m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return 0;

	NETADDR Addr = *m_NetServer.ClientAddr(ClientID);
	Addr.port = 0;

	for (int i = 0; i < m_lMutes.size(); i++)
	{
		if (net_addr_comp(&m_lMutes[i]->m_Address, &Addr) == 0)
			return m_lMutes[i]->m_Ticks;
	}
	
	return 0;
}

void CServer::MuteID(int ClientID, int64 Ticks)
{
	if (m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return;

	NETADDR Addr = *m_NetServer.ClientAddr(ClientID);
	Addr.port = 0;

	bool Found = false;
	for (int i = 0; i < m_lMutes.size() && Found == false; i++)
	{
		if (net_addr_comp(&m_lMutes[i]->m_Address, &Addr) != 0)
			continue;

		m_lMutes[i]->m_Ticks = max(m_lMutes[i]->m_Ticks, Ticks);
		Found = true;
	}

	if (Found == true)
		return;

	CMute *pMute = new CMute();
	pMute->m_Address = Addr;
	pMute->m_Ticks = Ticks;
	m_lMutes.add(pMute);
}

bool CServer::UnmuteID(int ClientID)
{
	if (m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return false;

	NETADDR Addr = *m_NetServer.ClientAddr(ClientID);
	Addr.port = 0;

	for (int i = 0; i < m_lMutes.size(); i++)
	{
		if (net_addr_comp(&m_lMutes[i]->m_Address, &Addr) != 0)
			continue;

		CMute *pMute = m_lMutes[i];
		m_lMutes.remove_index(i);
		delete pMute;
		return true;
	}

	return false;
}

static CServer *CreateServer() { return new CServer(); }

int main(int argc, const char **argv) // ignore_convention
{
#if defined(CONF_FAMILY_WINDOWS)
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-s", argv[i]) == 0 || str_comp("--silent", argv[i]) == 0) // ignore_convention
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			break;
		}
	}
#endif

	CServer *pServer = CreateServer();
	IKernel *pKernel = IKernel::Create();

	random_timeseet();

	// create the components
	IEngine *pEngine = CreateEngine("Teeworlds");
	IEngineMap *pEngineMap = CreateEngineMap();
	IGameServer *pGameServer = CreateGameServer();
	IConsole *pConsole = CreateConsole(CFGFLAG_SERVER|CFGFLAG_ECON);
	IEngineMasterServer *pEngineMasterServer = CreateEngineMasterServer();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_SERVER, argc, argv); // ignore_convention
	IConfig *pConfig = CreateConfig();

	{
		bool RegisterFail = false;

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pServer); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pEngine);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap));
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pGameServer);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConsole);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pStorage);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConfig);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMasterServer*>(pEngineMasterServer)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMasterServer*>(pEngineMasterServer));

		if(RegisterFail)
			return -1;
	}

	pEngine->Init();
	pConfig->Init();
	pEngineMasterServer->Init();
	pEngineMasterServer->Load();

	// register all console commands
	pServer->RegisterCommands();

	// execute autoexec file
	pConsole->ExecuteFile("autoexec.cfg");

	// parse the command line arguments
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	// restore empty config strings to their defaults
	pConfig->RestoreStrings();

	pEngine->InitLogfile();

	// run the server
	dbg_msg("server", "starting...");
	pServer->Run();

	// free
	delete pServer;
	delete pKernel;
	delete pEngineMap;
	delete pGameServer;
	delete pConsole;
	delete pEngineMasterServer;
	delete pStorage;
	delete pConfig;
	return 0;
}

