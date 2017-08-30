#pragma once

#include <base/tl/array.h>
#include <game/voting.h>
#include <game/layers.h>
#include <game/collision.h>
#include <game/server/components_map/eventhandler.h>
#include <game/server/components_map/gameworld.h>
#include <game/server/components_map/shop.h>
#include <game/server/components_map/animations.h>

class CMap;
class IServer;
class CGameContext;
class CPlayer;
class CTranslateItem;
class IConsole;
class CGameEvent;

//god class?

class CGameMap
{
public:// TODO: clean this into an extra class. esp. vote spawn tele and doors
	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100, 100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

	struct CDoorTile
	{
		int m_Index;
		vec2 m_Pos;
		int m_ID;
		bool m_Default;
		char m_LaserDir[2];

		int m_Delay;
		float m_Length;
		vec2 m_Direction;
		int m_SnapID;
		int m_Type;
	};

	struct CTeleTo
	{
		int m_ID;
		vec2 m_Pos;
	};

private:
	CMap *m_pMap;
	IServer *m_pServer;
	CGameContext *m_pGameServer;
	IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CGameWorld m_World;
	CEventHandler m_Events;
	CGameEvent *m_pGameEvent;//current map event
	CShop m_Shop;
	CAnimationHandler m_AnimationHandler;
	array<CDoorTile *> m_lDoorTiles;
	array<CTeleTo *> m_lTeleTo;
	class CComponentMap *m_apComponents[16];
	int m_NumComponents;
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];
	int m_RoundStartTick;
	bool m_BlockMap;
	int m_NumPlayers;
	int64 m_RandomEventTime;

	//Voting
	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN = 0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool OnEntity(int Index, vec2 Pos);

	void UpdateVote();
	void InitEntities();
	void InitDoorTile(CDoorTile *pTile);
	void InitExtras();
	void InitComponents();

	void DoMapTunings();
	void DoStoreAnimations();
public:
	CGameMap(CMap *pMap);
	~CGameMap();

	CPlayer *m_apPlayers[MAX_CLIENTS];

	bool Init(CGameContext *pGameServer);
	bool FreePlayerSlot();
	bool PlayerJoin(int ClientID);
	void PlayerLeave(int ClientID);
	bool PlayerOnMap(int ClientID);
	int FreeNpcSlot();
	CNpc *GetNpc(int ClientID);

	int NumTranslateItems();
	void FillTranslateItems(CTranslateItem *pTranslateItems);
	vec2 GetPlayerViewPos(int ClientID);

	bool CanSpawn(int Team, vec2 *pOutPos);
	CDoorTile *GetDoorTile(int Index);
	bool DoorTileActive(CDoorTile *pDoorTile) const;
	void OnDoorHandle(int ID, int Delay, bool Activate);
	bool GetRandomTelePos(int ID, vec2 *Pos);

	void SnapDoors(int SnappingClient);
	void SnapFakePlayer(int SnappingClient);
	void SnapGameInfo(int SnappingClient);
	void Snap(int SnappingClient);

	CGameEvent *CreateGameEvent(int Index);
	bool TryVoteRandomEvent(int ClientID);
	void StartRandomEvent();
	void EndEvent();
	void ClientSubscribeEvent(int ClientID);
	void PlayerBlocked(int ClientID, bool Dead, vec2 Pos);
	void PlayerKilled(int ClientID);

	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void VoteEnforce(const char *pVote);
	int64 GetVoteCloseTime() const { return m_VoteCloseTime; };
	void SetVoteCreator(int ClientID) { m_VoteCreator = ClientID; };
	void SetVotePos(int Pos) { m_VotePos = Pos; };
	int GetVotePos() const { return m_VotePos; };
	void UpdateVotes() { m_VoteUpdate = true; };
	bool IsBlockMap() const { return m_BlockMap; }
	void SetBlockMap(bool Value) { m_BlockMap = Value; }
	int NumPlayers() { return m_NumPlayers; }
	CGameEvent *GetEvent() { return m_pGameEvent; }

	void Tick();
	void SendChat(int ChatterClientID, const char *pText);
	void SendBroadcast(const char *pText);
	void OnClientEnter(int ClientID);

	CMap *Map() const { return m_pMap; };
	CGameContext *GameServer() const { return m_pGameServer; };
	IConsole *Console() const { return m_pConsole; };
	IServer *Server() const { return m_pServer; }
	CLayers *Layers() { return &m_Layers; };
	CCollision *Collision() { return &m_Collision; };
	CGameWorld *World() { return &m_World; };
	CEventHandler *Events() { return &m_Events; }
	CAnimationHandler *AnimationHandler() { return &m_AnimationHandler; }
};