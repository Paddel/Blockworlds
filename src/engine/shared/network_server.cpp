

#include <base/system.h>

#include <engine/console.h>
#include <engine/shared/packer.h>
#include <engine/external/md5/md5.h>
#include <engine/message.h>
#include <engine/shared/protocol.h>

#include "config.h"
#include "netban.h"
#include "network.h"

#define SALT "13#_Th1sI5N0S4LT_#37"

void CNetServer::Open(NETSOCKET Socket, CNetBan *pNetBan, int MaxClients, int MaxClientsPerIP, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this)); // this not well done tho.. what are constructors for?

	m_pNetBan = pNetBan;

	// clamp clients
	m_MaxClients = MaxClients;
	if(m_MaxClients > NET_MAX_CLIENTS)
		m_MaxClients = NET_MAX_CLIENTS;
	if(m_MaxClients < 1)
		m_MaxClients = 1;

	m_MaxClientsPerIP = MaxClientsPerIP;

	m_OpenTime = time_get();

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(Socket, true);
}

int CNetServer::CalcToken(const NETADDR *pAddr)
{
	char aHashIn[512];
	char aAddrStr[NETADDR_MAXSTRSIZE];

	int TimeSalt = time_get()/time_freq();

	net_addr_str(pAddr, aAddrStr, sizeof(aAddrStr), true);
	str_format(aHashIn, sizeof(aHashIn), "%s:%s:%i", aAddrStr, SALT, 0);
	int Result = abs((int)str_quickhash(md5(aHashIn).c_str()));
	return Result;
}

int CNetServer::TryAcceptNewClient(NETSOCKET Socket, NETADDR& Addr, bool Direct)
{
	// only allow a specific number of players with the same ip
	NETADDR ThisAddr = Addr, OtherAddr;
	int FoundAddr = 1;
	int Found = -1;

	ThisAddr.port = 0;
	for (int i = 0; i < MaxClients(); ++i)
	{
		if (m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
			continue;

		OtherAddr = *m_aSlots[i].m_Connection.PeerAddress();
		OtherAddr.port = 0;
		if (!net_addr_comp(&ThisAddr, &OtherAddr))
		{
			if (FoundAddr++ >= m_MaxClientsPerIP)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Only %d players with the same IP are allowed", m_MaxClientsPerIP);
				CNetBase::SendControlMsg(Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf) + 1);
				return -1;
			}
		}
	}

	for (int i = 0; i < MaxClients(); i++)
	{
		if (m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
		{
			Found = i;
			m_aSlots[i].m_Connection.SetSocket(Socket);
			m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr);
			if(Direct == false)
				m_aSlots[i].m_Connection.SetConnected(&Addr);

			if (m_pfnNewClient)
				m_pfnNewClient(i, m_UserPtr);
			break;
		}
	}

	if (Found == -1)
	{
		const char FullMsg[] = "This server is full";
		CNetBase::SendControlMsg(Socket, &Addr, 0, NET_CTRLMSG_CLOSE, FullMsg, sizeof(FullMsg));
	}

	return Found;
}

void CNetServer::SendSpoofCheck(NETSOCKET Socket, NETADDR& Addr)
{
	CNetBase::SendControlMsg(Socket, &Addr, 0, NET_CTRLMSG_CONNECTACCEPT, NULL, 0);

	CMsgPacker MapChangeMsg(NETMSG_MAP_CHANGE);

	MapChangeMsg.AddString("dm1", 0);
	MapChangeMsg.AddInt(0xf2159e6e);
	MapChangeMsg.AddInt(5805);

	CMsgPacker MapDataMsg(NETMSG_MAP_DATA);

	MapDataMsg.AddInt(1); // last chunk
	MapDataMsg.AddInt(0); // crc
	MapDataMsg.AddInt(0); // chunk index
	MapDataMsg.AddInt(0); // map size
	MapDataMsg.AddRaw(NULL, 0); // map data

	CMsgPacker ConReadyMsg(NETMSG_CON_READY);

	CMsgPacker SnapEmptyMsg(NETMSG_SNAPEMPTY);
	int Token = CalcToken(&Addr);
	SnapEmptyMsg.AddInt(Token);
	SnapEmptyMsg.AddInt(Token + 1);

	// send all chunks/msgs in one packet
	const CMsgPacker *Msgs[] = { &MapChangeMsg, &MapDataMsg, &ConReadyMsg,
		&SnapEmptyMsg, &SnapEmptyMsg, &SnapEmptyMsg };
	SendMsgs(Socket, Addr, Msgs, 6);
}

void CNetServer::ReceiveSpoofCheck(NETSOCKET Socket, NETADDR &Addr, CNetPacketConstruct &Packet)
{
	CNetChunkHeader Header;

	unsigned char *pData = Packet.m_aChunkData;
	pData = Header.Unpack(pData);
	CUnpacker Unpacker;
	Unpacker.Reset(pData, Header.m_Size);
	int Msg = Unpacker.GetInt() >> 1;


	if (Msg == NETMSG_INPUT)
	{
		int Token = Unpacker.GetInt();
		if (Token == CalcToken(&Addr))
			TryAcceptNewClient(Socket, Addr, false);
	}
}

void CNetServer::SendMsgs(NETSOCKET Socket, NETADDR &Addr, const CMsgPacker *Msgs[], int Num)
{
	CNetPacketConstruct m_Construct;
	mem_zero(&m_Construct, sizeof(m_Construct));
	unsigned char *pChunkData = &m_Construct.m_aChunkData[m_Construct.m_DataSize];

	for (int i = 0; i < Num; i++)
	{
		const CMsgPacker *pMsg = Msgs[i];
		CNetChunkHeader Header;
		Header.m_Flags = NET_CHUNKFLAG_VITAL;
		Header.m_Size = pMsg->Size();
		Header.m_Sequence = i + 1;
		pChunkData = Header.Pack(pChunkData);
		mem_copy(pChunkData, pMsg->Data(), pMsg->Size());
		*((unsigned char*)pChunkData) <<= 1;
		*((unsigned char*)pChunkData) |= 1;
		pChunkData += pMsg->Size();
		m_Construct.m_NumChunks++;
	}

	//
	m_Construct.m_DataSize = (int)(pChunkData - m_Construct.m_aChunkData);
	CNetBase::SendPacket(Socket, &Addr, &m_Construct);
}

int CNetServer::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserPtr = pUser;
	return 0;
}

int CNetServer::Close()
{
	// TODO: implement me
	return 0;
}

int CNetServer::Drop(int ClientID, const char *pReason)
{
	// TODO: insert lots of checks here
	/*NETADDR Addr = ClientAddr(ClientID);

	dbg_msg("net_server", "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		ClientID,
		Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3],
		pReason
		);*/
	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_UserPtr);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	return 0;
}

int CNetServer::Update()
{
	int64 Now = time_get();
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
		{
			if(g_Config.m_SvPunishStress && Now - m_aSlots[i].m_Connection.ConnectTime() < time_freq() * 0.75f && NetBan())
			{
				if(NetBan()->BanAddr(ClientAddr(i), 30, "Stressing network") == -1)
					Drop(i, m_aSlots[i].m_Connection.ErrorString());
			}
			else
				Drop(i, m_aSlots[i].m_Connection.ErrorString());
		}
	}

	return 0;
}

/*
	TODO: chopp up this function into smaller working parts
*/
int CNetServer::Recv(CNetChunk *pChunk, NETSOCKET Socket)
{
	while(1)
	{
		NETADDR Addr;

		// check for a chunk
		if(m_RecvUnpacker.FetchChunk(pChunk))
			return 1;

		// TODO: empty the recvinfo
		int Bytes = net_udp_recv(Socket, &Addr, m_RecvUnpacker.m_aBuffer, NET_MAX_PACKETSIZE);

		// no more packets for now
		if(Bytes <= 0)
			break;

		if(CNetBase::UnpackPacket(m_RecvUnpacker.m_aBuffer, Bytes, &m_RecvUnpacker.m_Data) == 0)
		{
			// check if we just should drop the packet
			char aBuf[128];
			if(NetBan() && NetBan()->IsBanned(&Addr, aBuf, sizeof(aBuf)))
			{
				// banned, reply with a message
				CNetBase::SendControlMsg(Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf)+1);
				continue;
			}

			if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS)
			{
				pChunk->m_Flags = NETSENDFLAG_CONNLESS;
				pChunk->m_ClientID = -1;
				pChunk->m_Address = Addr;
				pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
				pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
				return 1;
			}
			else
			{
				// TODO: check size here
				if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL && m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
				{
					bool Found = false;

					// check if we already got this client
					for(int i = 0; i < MaxClients(); i++)
					{
						if(m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE &&
							net_addr_comp(m_aSlots[i].m_Connection.PeerAddress(), &Addr) == 0)
						{
							Found = true; // silent ignore.. we got this client already
							break;
						}
					}

					// client that wants to connect
					if (!Found)
					{
						if(CheckSpoofing() == false)
							TryAcceptNewClient(Socket, Addr, true);
						else
							SendSpoofCheck(Socket, Addr);
					}
						
				}
				else
				{
					bool Found = false;
					// normal packet, find matching slot
					for(int i = 0; i < MaxClients(); i++)
					{
						if(net_addr_comp(m_aSlots[i].m_Connection.PeerAddress(), &Addr) == 0)
						{
							Found = true;
							if(m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
							{
								if(m_RecvUnpacker.m_Data.m_DataSize)
									m_RecvUnpacker.Start(&Addr, &m_aSlots[i].m_Connection, i);
							}
						}
					}

					if (Found == false && CheckSpoofing())
						ReceiveSpoofCheck(Socket, Addr, m_RecvUnpacker.m_Data);
				}
			}
		}
	}
	return 0;
}

int CNetServer::Send(CNetChunk *pChunk)
{
	if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet %i", pChunk->m_DataSize, pChunk->m_Flags);
		return -1;
	}

	
	if ((pChunk->m_Flags&NETSENDFLAG_CONNLESS) == 0)
	{
		int Flags = 0;
		dbg_assert(pChunk->m_ClientID >= 0, "errornous client id");
		dbg_assert(pChunk->m_ClientID < MaxClients(), "errornous client id");

		if(pChunk->m_Flags&NETSENDFLAG_VITAL)
			Flags = NET_CHUNKFLAG_VITAL;

		if(m_aSlots[pChunk->m_ClientID].m_Connection.QueueChunk(Flags, pChunk->m_DataSize, pChunk->m_pData) == 0)
		{
			if(pChunk->m_Flags&NETSENDFLAG_FLUSH)
				m_aSlots[pChunk->m_ClientID].m_Connection.Flush();
		}
		else
		{
			Drop(pChunk->m_ClientID, "Error sending data");
		}
	}
	else
		dbg_msg("NetServer", "Error sending Packet connless packet");
	
	return 0;
}

int CNetServer::SendConnless(CNetChunk *pChunk, NETSOCKET Socket)
{
	if (pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet %i", pChunk->m_DataSize, pChunk->m_Flags);
		return -1;
	}

	if (pChunk->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// send connectionless packet
		CNetBase::SendPacketConnless(Socket, &pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize);
	}
	else
		dbg_msg("NetServer", "Error sending Packet packet. Packet is not connless");
	return 0;
}

void CNetServer::SetMaxClientsPerIP(int Max)
{
	// clamp
	if(Max < 1)
		Max = 1;
	else if(Max > NET_MAX_CLIENTS)
		Max = NET_MAX_CLIENTS;

	m_MaxClientsPerIP = Max;
}

bool CNetServer::CheckSpoofing()
{
	return g_Config.m_SvAntiSpoof != 0 && g_Config.m_Password[0] == '\0';
}