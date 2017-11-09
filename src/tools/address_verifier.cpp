
#include <stdlib.h> //rand
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/database.h>
#include <mastersrv/mastersrv.h>
#include <game/version.h>


#define PORT 8303

CNetServer *pNet;

int GameType = 0;
int Flags = 0;

const char *pMap = "dm1";
const char *pServerName = "unnamed server";

static const int NumMasters = 4;
NETADDR aMasterServers[NumMasters] = { { 0,{ 0 },0 } };
const char *pMasterNames[NumMasters] =
{ "master1.teeworlds.com:8300", "master2.teeworlds.com:8300",
"master3.teeworlds.com:8300", "master4.teeworlds.com:8300"};

const char *PlayerNames[16] = { 0 };
int PlayerScores[16] = { 0 };
int NumPlayers = 0;
int MaxPlayers = 8;

char aInfoMsg[1024];
int aInfoMsgSize;
NETSOCKET Socket;

CDatabase Database;

static void SendHeartBeats()
{
	static unsigned char aData[sizeof(SERVERBROWSE_HEARTBEAT) + 2];
	CNetChunk Packet;

	mem_copy(aData, SERVERBROWSE_HEARTBEAT, sizeof(SERVERBROWSE_HEARTBEAT));

	Packet.m_ClientID = -1;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_HEARTBEAT) + 2;
	Packet.m_pData = &aData;

	/* supply the set port that the master can use if it has problems */
	aData[sizeof(SERVERBROWSE_HEARTBEAT)] = PORT >> 8;
	aData[sizeof(SERVERBROWSE_HEARTBEAT) + 1] = PORT & 0xff;

	for (int i = 0; i < NumMasters; i++)
	{
		Packet.m_Address = aMasterServers[i];
		pNet->SendConnless(&Packet, Socket);
	}
}

static void WriteStr(const char *pStr)
{
	int l = str_length(pStr) + 1;
	mem_copy(&aInfoMsg[aInfoMsgSize], pStr, l);
	aInfoMsgSize += l;
}

static void WriteInt(int i)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%d", i);
	WriteStr(aBuf);
}

static void BuildInfoMsg(int Token)
{
	char aBuf[32];
	mem_zero(&aInfoMsg, sizeof(aInfoMsg));
	aInfoMsgSize = 0;

	aInfoMsgSize = sizeof(SERVERBROWSE_INFO);
	mem_copy(aInfoMsg, SERVERBROWSE_INFO, aInfoMsgSize);
	str_format(aBuf, sizeof(aBuf), "%d", Token); WriteStr(aBuf);

	WriteStr(GAME_VERSION);
	WriteStr(pServerName);
	WriteStr(pMap);
	WriteInt(GameType);
	str_format(aBuf, sizeof(aBuf), "%d", Flags); WriteStr(aBuf);

	str_format(aBuf, sizeof(aBuf), "%d", NumPlayers); WriteStr(aBuf); // num players
	str_format(aBuf, sizeof(aBuf), "%d", MaxPlayers); WriteStr(aBuf); // max players
	str_format(aBuf, sizeof(aBuf), "%d", NumPlayers); WriteStr(aBuf); // num clients
	str_format(aBuf, sizeof(aBuf), "%d", MaxPlayers); WriteStr(aBuf); // max clients

	for (int i = 0; i < NumPlayers; i++)
	{
		WriteStr(PlayerNames[i]);
		WriteInt(PlayerScores[i]);
	}
}

static void SendServerInfo(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = aInfoMsgSize;
	p.m_pData = aInfoMsg;
	pNet->SendConnless(&p, Socket);
}

static void SendFWCheckResponse(NETADDR *pAddr)
{
	CNetChunk p;
	p.m_ClientID = -1;
	p.m_Address = *pAddr;
	p.m_Flags = NETSENDFLAG_CONNLESS;
	p.m_DataSize = sizeof(SERVERBROWSE_FWRESPONSE);
	p.m_pData = SERVERBROWSE_FWRESPONSE;
	pNet->SendConnless(&p, Socket);
}

static void CheckNewAddess(NETADDR& Addr)
{
	char aAddrStr[NETADDR_MAXSTRSIZE];
	char aQuery[QUERY_MAX_STR_LEN];

	net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), false);

	str_format(aQuery, sizeof(aQuery), "REPLACE INTO address_verifier(address, date) VALUES('%s', NOW())", aAddrStr);
	Database.Query(aQuery, 0x0, 0x0);
	dbg_msg(0, "Queried");
}

static int Run()
{
	int64 NextHeartBeat = 0;
	NETADDR BindAddr;
	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_ALL;
	BindAddr.port = PORT;
	mem_zero(&Socket, sizeof(Socket));

	Socket = net_udp_create(BindAddr, 0);
	if (!Socket.type)
	{
		//str_format(aBuf, sizeof(aBuf), "couldn't open socket. port %d might already be in use", Port);
		dbg_msg("server", "Could not open socket");
		return 0;
	}

	pNet->Open(Socket, 0, 0, 0, 0);

	while (1)
	{
		CNetChunk p;
		pNet->Update();
		while (pNet->Recv(&p, Socket))
		{
			if (p.m_ClientID == -1)
			{
				if (p.m_DataSize == sizeof(SERVERBROWSE_GETINFO) + 1 &&
					mem_comp(p.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					BuildInfoMsg(((unsigned char *)p.m_pData)[sizeof(SERVERBROWSE_GETINFO)]);
					SendServerInfo(&p.m_Address);
					CheckNewAddess(p.m_Address);
				}
				else if (p.m_DataSize == sizeof(SERVERBROWSE_FWCHECK) &&
					mem_comp(p.m_pData, SERVERBROWSE_FWCHECK, sizeof(SERVERBROWSE_FWCHECK)) == 0)
				{
					SendFWCheckResponse(&p.m_Address);
				}
				else if (p.m_DataSize == sizeof(SERVERBROWSE_FWOK) &&
					mem_comp(p.m_pData, SERVERBROWSE_FWOK, sizeof(SERVERBROWSE_FWOK)) == 0)
				{
					static bool FirstOutput = false;
					if (FirstOutput == false)
					{
						dbg_msg("register", "no firewall/nat problems detected");
						FirstOutput = true;
					}
				}
			}
		}

		/* send heartbeats if needed */
		if (NextHeartBeat < time_get())
		{
			NextHeartBeat = time_get() + time_freq()*(15 + (rand() % 15));
			SendHeartBeats();
		}

		thread_sleep(10);
	}
}

int main(int argc, char **argv)
{
	net_init();
	dbg_logger_stdout();

	pNet = new CNetServer;

	Database.Init("31.186.250.72", "Paddel", "Uhrenonkel7845", "blockworlds");

	for (int i = 0; i < NumMasters; i++)
		net_host_lookup(pMasterNames[i], &aMasterServers[i], NETTYPE_ALL);


	while (argc)
	{
		// ?
		/*if(str_comp(*argv, "-m") == 0)
		{
		argc--; argv++;
		net_host_lookup(*argv, &aMasterServers[NumMasters], NETTYPE_IPV4);
		argc--; argv++;
		aMasterServers[NumMasters].port = str_toint(*argv);
		NumMasters++;
		}
		else */if (str_comp(*argv, "-p") == 0)
		{
			argc--; argv++;
			PlayerNames[NumPlayers++] = *argv;
			argc--; argv++;
			PlayerScores[NumPlayers] = str_toint(*argv);
		}
		else if (str_comp(*argv, "-a") == 0)
		{
			argc--; argv++;
			pMap = *argv;
		}
		else if (str_comp(*argv, "-x") == 0)
		{
			argc--; argv++;
			MaxPlayers = str_toint(*argv);
		}
		else if (str_comp(*argv, "-t") == 0)
		{
			argc--; argv++;
			GameType = str_toint(*argv);
		}
		else if (str_comp(*argv, "-f") == 0)
		{
			argc--; argv++;
			Flags = str_toint(*argv);
		}
		else if (str_comp(*argv, "-n") == 0)
		{
			argc--; argv++;
			pServerName = *argv;
		}
		else if (str_comp(*argv, "-d") == 0)
			g_Config.m_DbgDatabase = 1;

	argc--; argv++;
	}

	int RunReturn = Run();

	delete pNet;
	return RunReturn;
}

