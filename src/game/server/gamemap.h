#pragma once

#include <base/tl/array.h>
#include <game/layers.h>
#include <game/collision.h>
#include <game/server/gameworld.h>
#include <game/server/gamematch.h>
#include <game/server/components_map/shop.h>
#include <game/server/components_map/lobby.h>
#include <game/server/components_map/race_components.h>
#include <game/server/components_map/mapvoting.h>

//TODO: remove includes + refactor

class CMap;
class IServer;
class CGameContext;
class CPlayer;
class CTranslateItem;
class IConsole;
class CGameEvent;

class CGameMap
{
private:
	CMap *m_pMap;
	IServer *m_pServer;
	CGameContext *m_pGameServer;
	IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CGameEvent *m_pGameEvent;//current map event
	CShop m_Shop;
	CLobby m_Lobby;
	CRaceComponents m_RaceComponents;
	CMapVoting m_MapVoting;
	class CComponentMap *m_apComponents[16];
	array <CGameWorld *> m_lpWorlds;
	array <CGameMatch *> m_lpMatches;
	int m_NumComponents;
	
	int m_RoundStartTick;
	bool m_ShopMap;
	bool m_BlockMap;
	bool m_HasArena;
	int m_NumPlayers;
	int64 m_RandomEventTime;

	
	void InitComponents();
	void FindMapType();

	void DoMapTunings();
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

	void SnapFakePlayer(int SnappingClient);
	void SnapGameInfo(int SnappingClient);
	void Snap(int SnappingClient);

	CGameEvent *CreateGameEvent(int Index);
	bool TryVoteRandomEvent(int ClientID);
	void StartRandomEvent();
	void EndEvent();
	void SetEventCooldown();
	void ClientSubscribeEvent(int ClientID);

	void CreateGameMatch(int Client0, int Client1, int Blockpoints);
	void DeleteGameMatch(CGameMatch *pGameMatch);

	void PlayerBlocked(int ClientID, vec2 Pos);
	void PlayerKilled(int ClientID);

	bool IsBlockMap() const { return m_BlockMap; }
	void SetBlockMap(bool Value) { m_BlockMap = Value; }
	bool IsShopMap() const { return m_ShopMap; }
	bool HasArena() const { return m_HasArena; }
	int NumPlayers() { return m_NumPlayers; }
	CGameEvent *GetEvent() { return m_pGameEvent; }

	void Tick();
	void SendChat(int ChatterClientID, const char *pText);
	void SendBroadcast(const char *pText);
	void OnClientEnter(int ClientID);

	CGameWorld *CreateNewWorld(int Type);
	void DeleteWorld(CGameWorld *pWorld);
	void ClearEvents();

	CMap *Map() const { return m_pMap; };
	CGameContext *GameServer() const { return m_pGameServer; };
	IConsole *Console() const { return m_pConsole; };
	IServer *Server() const { return m_pServer; }
	CLayers *Layers() { return &m_Layers; };
	CCollision *Collision() { return &m_Collision; };
	CGameWorld *MainWorld() { return m_lpWorlds[0]; };//main world
	CRaceComponents *RaceComponents() { return &m_RaceComponents; }
	CMapVoting *MapVoting() { return &m_MapVoting; }
};