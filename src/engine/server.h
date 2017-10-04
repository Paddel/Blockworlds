

#ifndef ENGINE_SERVER_H
#define ENGINE_SERVER_H

#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>

#include "kernel.h"
#include "message.h"

class CGameMap;

class IServer : public IInterface
{
	MACRO_INTERFACE("server", 0)
protected:
	int m_CurrentGameTick;
	int m_TickSpeed;

public:
	
	enum
	{
		AUTHED_NO = 0,
		AUTHED_MOD,
		AUTHED_ADMIN,
	};

	struct CClanData
	{
		char m_aName[32];
		char m_aLeader[32];
		int m_Level;
		int m_Experience;
		int m_Members;
	};

	struct CAccountData
	{
		char m_aName[11];
		char m_aPassword[16];
		char m_aAddress[47];
		bool m_Vip;
		int m_Pages;
		int m_Level;
		int m_Experience;
		int m_WeaponKits;
		int m_Ranking;
		int m_BlockPoints;
		char m_aKnockouts[256];
		char m_aGundesigns[256];
		char m_aSkinmani[256];
		char m_aExtras[256];
	};

	struct CClientInfo
	{//this data is shared by game and engine and does not get resettet on map change
		bool m_LoggedIn;
		CAccountData m_AccountData;
		CClanData *m_pClan;

		int m_CurrentKnockout;
		int m_CurrentGundesign;
		int m_CurrentSkinmani;
		int64 m_InviolableTime;
		bool m_UseInviolable;
		bool m_Detached;
		int m_EmoteType;
		int m_EmoteStop;
		int m_DDNetVersion;
		bool m_IsAthClient;
		bool m_SentClientSuggestion;
		int m_ConverseID;
		int64 m_ConverseTick;

		void Reset()
		{
			mem_zero(this, sizeof(*this));
			m_CurrentKnockout = -1;
			m_CurrentGundesign = -1;
			m_CurrentSkinmani = -1;
			m_UseInviolable = true;
			m_EmoteStop = -1;
			m_ConverseID = -1;
			m_ConverseTick = 0;
		}
	};

	int Tick() const { return m_CurrentGameTick; }
	int TickSpeed() const { return m_TickSpeed; }

	virtual int MaxClients() const = 0;
	virtual const char *ClientName(int ClientID) = 0;
	virtual const char *ClientClan(int ClientID) = 0;
	virtual int ClientCountry(int ClientID) = 0;
	virtual bool ClientIngame(int ClientID) = 0;
	virtual CClientInfo *GetClientInfo(int ClientID) = 0;
	virtual void GetClientAddr(int ClientID, char *pAddrStr, int Size) = 0;
	virtual int GetClientAuthed(int ClientID) = 0;
	virtual int GetClientLatency(int ClientID) = 0;
	virtual bool CompClientAddr(int C1, int C2) = 0;

	virtual class CMap *FindMap(const char *pName) = 0;
	virtual bool MovePlayer(int ClientID, const char *pMapName) = 0;
	virtual bool MovePlayer(int ClientID, class CMap *pMap) = 0;
	virtual bool MoveLobby(int ClientID) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID) = 0;

	template<class T>
	void SendPackMsg(T *pMsg, int Flags, int ClientID)
	{	
		if (ClientID == -1)
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (ClientIngame(i) == false)
					continue;

				T Buff;
				mem_copy(&Buff, pMsg, sizeof(T));
				SendPackMsgTranslate(&Buff, Flags, i);
			}
		}
		else
			SendPackMsgTranslate(pMsg, Flags, ClientID);
	}

	template<class T>
	void SendPackMsgTranslate(T *pMsg, int Flags, int ClientID)
	{
		SendMsgFinal(pMsg, Flags, ClientID);
	}

	void SendPackMsgTranslate(CNetMsg_Sv_Emoticon *pMsg, int Flags, int ClientID)
	{
		pMsg->m_ClientID = Translate(ClientID, pMsg->m_ClientID);
		if(pMsg->m_ClientID != -1)
			SendMsgFinal(pMsg, Flags, ClientID);
	}

	void SendPackMsgTranslate(CNetMsg_Sv_KillMsg *pMsg, int Flags, int ClientID)
	{
		pMsg->m_Killer = Translate(ClientID, pMsg->m_Killer);
		pMsg->m_Victim = Translate(ClientID, pMsg->m_Victim);
		if (pMsg->m_Killer != -1 && pMsg->m_Victim != -1)
			SendMsgFinal(pMsg, Flags, ClientID);
	}

	void SendPackMsgTranslate(CNetMsg_Sv_Chat *pMsg, int Flags, int ClientID)
	{
		if (pMsg->m_ClientID == -1)
		{
			SendMsgFinal(pMsg, Flags, ClientID);
			return;
		}

		int TranslatedID = Translate(ClientID, pMsg->m_ClientID);
		if (TranslatedID != -1)
		{
			pMsg->m_ClientID = TranslatedID;
			SendMsgFinal(pMsg, Flags, ClientID);
			return;
		}

		char aChatBuffer[1024];
		str_format(aChatBuffer, sizeof(aChatBuffer), "%s: %s", ClientName(pMsg->m_ClientID), pMsg->m_pMessage);
		pMsg->m_ClientID = UsingMapItems(ClientID)-1;
		pMsg->m_pMessage = aChatBuffer;
		SendMsgFinal(pMsg, Flags, ClientID);
	}

	template<class T>
	void SendMsgFinal(T *pMsg, int Flags, int ClientID)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if (pMsg->Pack(&Packer))
			return;
		SendMsg(&Packer, Flags, ClientID);
	}

	virtual void SetClientName(int ClientID, char const *pName) = 0;
	virtual void SetClientCountry(int ClientID, int Country) = 0;

	virtual int SnapNewID() = 0;
	virtual void SnapFreeID(int ID) = 0;
	virtual void *SnapNewItem(int Type, int ID, int Size) = 0;

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	virtual void Shutdown() = 0;

	enum
	{
		RCON_CID_SERV=-1,
		RCON_CID_VOTE=-2,
	};
	virtual void SetRconCID(int ClientID) = 0;
	virtual void Kick(int ClientID, const char *pReason) = 0;
	virtual void DropClient(int ClientID, const char *pReason) = 0;
	virtual int RconClientID() = 0;

	virtual bool ClientOnline(int ClientID) = 0;
	virtual int UsingMapItems(int ClientID) = 0;
	virtual CGameMap *CurrentGameMap(int ClientID) = 0;
	virtual int64 GetJoinTick(int ClientID) = 0;
	virtual int GetNumMaps() = 0;
	virtual CGameMap *GetGameMap(int Index) = 0;

	virtual int64 IsMuted(int ClientID) = 0;
	virtual void MuteID(int ClientID, int64 Ticks) = 0;
	virtual bool UnmuteID(int ClientID) = 0;

	virtual int Translate(int For, int ClientID) = 0;
	virtual int ReverseTranslate(int For, int ClientID) = 0;
	virtual class  CStatisticsPerformance *StatisticsPerformance() = 0;
};

class IGameServer : public IInterface
{
	MACRO_INTERFACE("gameserver", 0)
protected:
public:
	virtual void OnInit() = 0;
	virtual void OnConsoleInit() = 0;
	virtual void OnShutdown() = 0;

	virtual void OnTick() = 0;
	virtual void OnPreSnap() = 0;
	virtual void OnSnap(int ClientID) = 0;
	virtual void OnPostSnap() = 0;

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) = 0;

	virtual void OnClientConnected(int ClientID) = 0;
	virtual void OnClientEnter(int ClientID, bool MapSwitching) = 0;
	virtual void OnClientLeave(int ClientID, const char *pReason) = 0;
	virtual void OnClientMapchange(int ClientID, CGameMap *pGameMapFrom, CGameMap *pGameMapTo) = 0;
	virtual void OnClientDrop(int ClientID, const char *pReason, CGameMap *pGameMap, bool MapSwitching) = 0;
	virtual void OnClientDirectInput(int ClientID, void *pInput) = 0;
	virtual void OnClientPredictedInput(int ClientID, void *pInput) = 0;

	virtual bool GameMapInit(CGameMap *pMap) = 0;

	virtual bool IsClientReady(int ClientID) = 0;
	virtual bool IsClientPlayer(int ClientID) = 0;
	virtual bool HideIDsFor(int ClientID) = 0;
	
	virtual bool CanShutdown() = 0;

	virtual const char *GameType() = 0;
	virtual const char *Version() = 0;
	virtual const char *FakeVersion() = 0;
	virtual const char *NetVersion() = 0;
};

extern IGameServer *CreateGameServer();
#endif
