
#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/extras.h>
#include <game/server/gamecontext.h>
#include <game/server/gameevent.h>
#include <game/server/entities/npc.h>
#include <game/server/entities/pickup.h>
#include <game/server/gameevents/lastmanblocking.h>

#include "gamemap.h"

CGameMap::CGameMap(CMap *pMap)
{
	m_pMap = pMap;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0x0;

	m_VoteCloseTime = 0;
	m_BlockMap = false;

	m_RandomEventTime = 0;
	m_pGameEvent = 0x0;
}

CGameMap::~CGameMap()
{
	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (m_lDoorTiles[i]->m_Length > 0.0f)
			Server()->SnapFreeID(m_lDoorTiles[i]->m_SnapID);
	}

	m_lDoorTiles.delete_all();
	m_lTeleTo.delete_all();
}

float CGameMap::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for (; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if (pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void CGameMap::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for (int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for (int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for (int c = 0; c < Num; ++c)
				if (m_Collision.CheckPoint(m_aaSpawnPoints[Type][i] + Positions[Index]) ||
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i] + Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if (Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i] + Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if (!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CGameMap::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	if (Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if (Index == ENTITY_SPAWN_EVENT)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if (Index == ENTITY_SPAWN_1ON1)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
	else if (Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if (Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if (Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
	}
	else if (Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	else if (Index == ENTITY_WEAPON_RIFLE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
	}
	else if (Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
	{
		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
	}

	if (Type != -1)
	{
		CPickup *pPickup = new CPickup(&m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void CGameMap::UpdateVote()
{
	// update voting
	if (m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if (m_VoteCloseTime == -1)
		{
			SendChat(-1, "Vote aborted");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if (m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = { { 0 } };
				for (int i = 0; i < MAX_CLIENTS; i++)
					if (m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = { 0 };
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i])	// don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for (int j = i + 1; j < MAX_CLIENTS; ++j)
					{
						if (!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if (m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if (ActVote > 0)
						Yes++;
					else if (ActVote < 0)
						No++;
				}

				if (Yes >= Total / 2 + 1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if (No >= (Total + 1) / 2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if (m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				if (str_comp(m_aVoteCommand, "%rand_event") == 0)
				{
					StartRandomEvent();
				}
				else
				{
					Server()->SetRconCID(IServer::RCON_CID_VOTE);
					Console()->ExecuteLine(m_aVoteCommand);
					Server()->SetRconCID(IServer::RCON_CID_SERV);
				}
				EndVote();
				SendChat(-1, "Vote passed");

				if (m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, "Vote failed");
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}
}

void CGameMap::InitEntities()
{
	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Map()->EngineMap()->GetData(pTileMap->m_Data);


	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width + x].m_Index;

			if (Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f + 16.0f, y*32.0f + 16.0f);
				OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}
}

void CGameMap::InitDoorTile(CDoorTile *pTile)
{
	pTile->m_Delay = 0;
	pTile->m_Length = 0.0f;

	if (pTile->m_LaserDir[0] == '^' || pTile->m_LaserDir[0] == 'v' || pTile->m_LaserDir[0] == '|')
		pTile->m_Type = 1;
	else if (pTile->m_LaserDir[0] == '<' || pTile->m_LaserDir[0] == '>' || pTile->m_LaserDir[0] == '-')
		pTile->m_Type = 2;
	else
		pTile->m_Type = 0;

	if (pTile->m_LaserDir[0] == '^' || pTile->m_LaserDir[0] == 'v' ||
		pTile->m_LaserDir[0] == '<' || pTile->m_LaserDir[0] == '>')
	{
		vec2 Dir = vec2(0.0f, 0.0f);
		switch (pTile->m_LaserDir[0])
		{
		case '^': Dir = vec2(0, -1); break;
		case 'v': Dir = vec2(0, 1); break;
		case '<': Dir = vec2(-1, 0); break;
		case '>': Dir = vec2(1, 0); break;
		}

		pTile->m_Direction = Dir;
		pTile->m_SnapID = Server()->SnapNewID();

		pTile->m_Length = 16.0f;
		for (int i = 1; i < 16; i++)
		{
			if (Collision()->CheckPoint(pTile->m_Pos + Dir * 32.0f * i) == false)
				pTile->m_Length += 32;
			else
				break;
		}
	}
}

void CGameMap::InitExtras()
{
	for (int l = 0; l < Layers()->GetNumExtrasLayer(); l++)
	{
		for (int x = 0; x < Layers()->GetExtrasWidth(l); x++)
		{
			for (int y = 0; y < Layers()->GetExtrasHeight(l); y++)
			{
				int Index = y * Layers()->GetExtrasWidth(l) + x;
				int Tile = Layers()->GetExtrasTile(l)[Index].m_Index;
				CExtrasData ExtrasData = Layers()->GetExtrasData(l)[Index];

				if (Tile == EXTRAS_DOOR)
				{
					CDoorTile *pDoorTile = new CDoorTile();
					pDoorTile->m_Index = Index;
					pDoorTile->m_Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
					const char *pData = ExtrasData.m_aData;
					pDoorTile->m_ID = str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][0];
					str_copy(pDoorTile->m_LaserDir, pData, sizeof(pDoorTile->m_LaserDir));
					pData += +gs_ExtrasSizes[Tile][1];
					pDoorTile->m_Default = (bool)str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][2];
					InitDoorTile(pDoorTile);
					m_lDoorTiles.add(pDoorTile);
				}
				else if (Tile == EXTRAS_TELEPORT_TO)
				{
					CTeleTo *pTeleTo = new CTeleTo();
					pTeleTo->m_Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
					pTeleTo->m_ID = str_toint(ExtrasData.m_aData);
					m_lTeleTo.add(pTeleTo);
				}
				else if (Tile == EXTRAS_ZONE_BLOCK)
					m_BlockMap = true;
			}
		}
	}
}

void CGameMap::DoMapTunings()
{
	if (m_BlockMap == false)
	{
		m_World.m_Core.m_Tuning.m_PlayerCollision = 0;
		m_World.m_Core.m_Tuning.m_PlayerHooking = 0;
	}
}

bool CGameMap::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pConsole = pGameServer->Console();

	m_Layers.Init(Map()->EngineMap());
	m_Collision.Init(&m_Layers);

	m_Events.SetGameServer(pGameServer);
	m_World.SetGameMap(this);

	m_RoundStartTick = Server()->Tick();

	InitEntities();
	InitExtras();

	//CNpc *pNpc = new CNpc(&m_World, FreeNpcSlot());
	//pNpc->Spawn(vec2(300, 300));

	//pNpc = new CNpc(&m_World, FreeNpcSlot());
	//pNpc->Spawn(vec2(300, 300));

	return true;
}

bool CGameMap::FreePlayerSlot()
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			Num++;

	return Num < g_Config.m_SvMaxClientsPerMap;
}

bool CGameMap::PlayerJoin(int ClientID)
{
	if (FreePlayerSlot() == false)
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	m_apPlayers[ClientID] = pPlayer;

	m_NumPlayers++;

	// send active vote
	if (m_VoteCloseTime)
		SendVoteSet(ClientID);
	return true;
}

void CGameMap::PlayerLeave(int ClientID)
{
	m_apPlayers[ClientID] = 0x0;
	m_NumPlayers--;
	m_VoteUpdate = true;
}

bool CGameMap::PlayerOnMap(int ClientID)
{
	return m_apPlayers[ClientID] != 0x0;
}

int CGameMap::FreeNpcSlot()
{
	int ClientID = MAX_CLIENTS;
	CNpc *pFirst = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC);
	for (CNpc *pNpc = pFirst; pNpc; pNpc = (CNpc *)pNpc->TypeNext())
	{
		if (ClientID != pNpc->GetCID())
			continue;

		ClientID++;
		pNpc = pFirst;
	}

	return ClientID;
}

CNpc *CGameMap::GetNpc(int ClientID)
{
	for (CNpc *pNpc = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pNpc; pNpc = (CNpc *)pNpc->TypeNext())
		if (pNpc->GetCID() == ClientID)
			return pNpc;
	return 0x0;
}

int CGameMap::NumTranslateItems()
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			Num++;

	for (CEntity *pEnt = m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pEnt; pEnt = pEnt->TypeNext())
		Num++;

	return Num;
}

void CGameMap::FillTranslateItems(CTranslateItem *pTranslateItems)
{
	int Num = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] != 0x0)
			mem_copy(&pTranslateItems[Num++], m_apPlayers[i]->GetTranslateItem(), sizeof(CTranslateItem));

	for (CNpc *pNpc = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pNpc; pNpc = (CNpc *)pNpc->TypeNext())
		mem_copy(&pTranslateItems[Num++], pNpc->GetTranslateItem(), sizeof(CTranslateItem));
}

vec2 CGameMap::GetPlayerViewPos(int ClientID)
{
	if (m_apPlayers[ClientID] == 0x0)
		return vec2(0.0f, 0.0f);
	return m_apPlayers[ClientID]->m_ViewPos;
}

bool CGameMap::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, Team);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

CGameMap::CDoorTile *CGameMap::GetDoorTile(int Index)
{
	for (int i = 0; i < m_lDoorTiles.size(); i++)
		if (m_lDoorTiles[i]->m_Index == Index)
			return m_lDoorTiles[i];
	return 0x0;
}

bool CGameMap::DoorTileActive(CDoorTile *pDoorTile) const
{
	if (pDoorTile->m_Delay > Server()->Tick())
		return !pDoorTile->m_Default;
	return pDoorTile->m_Default;
}

void CGameMap::OnDoorHandle(int ID, int Delay, bool Activate)
{
	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (m_lDoorTiles[i]->m_ID != ID)
			continue;

		if (m_lDoorTiles[i]->m_Default == Activate)
			m_lDoorTiles[i]->m_Delay = 0;
		else
			m_lDoorTiles[i]->m_Delay = Server()->Tick() + Delay * Server()->TickSpeed() + 1;
	}
}

bool CGameMap::GetRandomTelePos(int ID, vec2 *Pos)
{
	int Num = 0;
	for (int i = 0; i < m_lTeleTo.size(); i++)
		if (m_lTeleTo[i]->m_ID == ID)
			Num++;
	if (Num == 0)
		return false;
	int Chosen = rand() % Num;
	for (int i = 0; i < m_lTeleTo.size(); i++)
	{
		if (m_lTeleTo[i]->m_ID != ID)
			continue;

		if (Chosen <= 0)
		{
			*Pos = m_lTeleTo[i]->m_Pos;
			return true;
		}
		Chosen--;
	}

	return false;
}

void CGameMap::SnapDoors(int SnappingClient)
{
	CPlayer *pPlayer = m_apPlayers[SnappingClient];
	if (pPlayer == 0x0)
		return;

	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (DoorTileActive(m_lDoorTiles[i]) == false || m_lDoorTiles[i]->m_Length <= 0.0f)
			continue;

		float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x - m_lDoorTiles[i]->m_Pos.x;
		float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y - m_lDoorTiles[i]->m_Pos.y;

		if (absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
			continue;

		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_lDoorTiles[i]->m_SnapID, sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		vec2 From = m_lDoorTiles[i]->m_Pos + m_lDoorTiles[i]->m_Direction * m_lDoorTiles[i]->m_Length;

		pObj->m_X = (int)m_lDoorTiles[i]->m_Pos.x;
		pObj->m_Y = (int)m_lDoorTiles[i]->m_Pos.y;
		pObj->m_FromX = (int)From.x;
		pObj->m_FromY = (int)From.y;
		pObj->m_StartTick = Server()->Tick() - 1;
	}
}

void CGameMap::SnapFakePlayer(int SnappingClient)
{
	int ClientID = Server()->UsingMapItems(SnappingClient) - 1;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, ClientID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, "");
	pClientInfo->m_Country = -1;// Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, "");
	pClientInfo->m_UseCustomColor = 0;// m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = 0;// m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = 0;// m_TeeInfos.m_ColorFeet;

	/*CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, ClientID, sizeof(CNetObj_PlayerInfo)));
	if (!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = ClientID;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Team = 0;*/
}

void CGameMap::SnapGameInfo(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if (!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = GAMEFLAG_FLAGS;
	pGameInfoObj->m_GameStateFlags = 0;
	/*if(m_GameOverTick != -1)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;*/
	/*if(m_SuddenDeath)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;*/
	/*if(GameServer()->m_World.m_Paused)
	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;*/
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = 0;

	pGameInfoObj->m_ScoreLimit = 0;
	pGameInfoObj->m_TimeLimit = 0;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = 0;

	SnapFakePlayer(SnappingClient);
}

void CGameMap::Snap(int SnappingClient)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->Snap(SnappingClient);
	m_World.Snap(SnappingClient);
	SnapDoors(SnappingClient);
	SnapGameInfo(SnappingClient);
	m_Events.Snap(SnappingClient);
}

CGameEvent *CGameMap::CreateGameEvent(int Index)
{
	switch (Index)
	{
	case CGameEvent::EVENT_LASTMANBLOCKING: return new CLastManBlocking(this);
	};

	return 0x0;
}

bool CGameMap::TryVoteRandomEvent(int ClientID)
{
	if (m_pGameEvent != 0x0)
	{
		GameServer()->SendChatTarget(ClientID, "You canno start a event, when there is alreay one running");
		return false;
	}

	if (m_RandomEventTime + Server()->TickSpeed() * g_Config.m_SvEventCooldown > Server()->Tick())
	{
		char aBuf[128];
		str_copy(aBuf, "You have to wait ", sizeof(aBuf));
		GameServer()->StringTime(m_RandomEventTime + Server()->TickSpeed() * g_Config.m_SvEventCooldown, aBuf, sizeof(aBuf));
		str_append(aBuf, " until you can start this vote.", sizeof(aBuf));
		GameServer()->SendChatTarget(ClientID, aBuf);
		return false;
	}

	return true;
}

void CGameMap::StartRandomEvent()
{
	if (m_pGameEvent != 0x0)
		return;

	int Index = rand() % CGameEvent::NUM_EVENTS;
	m_pGameEvent = CreateGameEvent(Index);
}

void CGameMap::EndEvent()
{
	if (m_pGameEvent == 0x0)
		return;

	delete m_pGameEvent;
	m_pGameEvent = 0x0;

	m_RandomEventTime = Server()->Tick();
}

void CGameMap::ClientSubscribeEvent(int ClientID)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->ClientSubscribe(ClientID);
}

void CGameMap::PlayerBlocked(int ClientID, bool Dead, vec2 Pos)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->PlayerBlocked(ClientID, Dead, Pos);
}

void CGameMap::PlayerKilled(int ClientID)
{
	if (m_pGameEvent != 0x0)
		m_pGameEvent->PlayerKilled(ClientID);
}

void CGameMap::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if (m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq() * 25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}


void CGameMap::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameMap::SendVoteSet(int ClientID)
{
	if (ClientID == -1)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
			SendVoteSet(i);
	}
	else
	{
		if (m_apPlayers[ClientID] == 0x0)
			return;

		CNetMsg_Sv_VoteSet Msg;
		if (m_VoteCloseTime)
		{
			Msg.m_Timeout = (m_VoteCloseTime - time_get()) / time_freq();
			Msg.m_pDescription = m_aVoteDescription;
			Msg.m_pReason = m_aVoteReason;
		}
		else
		{
			Msg.m_Timeout = 0;
			Msg.m_pDescription = "";
			Msg.m_pReason = "";
		}
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
}

void CGameMap::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	if (ClientID == -1)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
			SendVoteStatus(i, Total, Yes, No);
	}
	else
	{
		if (m_apPlayers[ClientID] == 0x0)
			return;

		CNetMsg_Sv_VoteStatus Msg = { 0 };
		Msg.m_Total = Total;
		Msg.m_Yes = Yes;
		Msg.m_No = No;
		Msg.m_Pass = Total - (Yes + No);
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
}

void CGameMap::VoteEnforce(const char *pVote)
{
	// check if there is a vote running
	if (!m_VoteCloseTime)
		return;

	if (str_comp_nocase(pVote, "yes") == 0)
		m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if (str_comp_nocase(pVote, "no") == 0)
		m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "admin forced vote %s", pVote);
	SendChat(-1, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pVote);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameMap::Tick()
{
	mem_copy(&m_World.m_Core.m_Tuning, GameServer()->Tuning(), sizeof(m_World.m_Core.m_Tuning));
	DoMapTunings();

	if (m_pGameEvent != 0x0)
		m_pGameEvent->Tick();

	m_World.Tick();
	UpdateVote();
}

void CGameMap::SendChat(int ChatterClientID, const char *pText)
{
	if (g_Config.m_Debug)
	{
		char aBuf[256];
		if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
			str_format(aBuf, sizeof(aBuf), "%d:%s: %s", ChatterClientID, Server()->ClientName(ChatterClientID), pText);
		else
			str_format(aBuf, sizeof(aBuf), "*** %s", pText);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "chat", aBuf);
	}

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i] == 0x0)
			continue;

		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_apPlayers[i]->GetCID());
	}
}

void CGameMap::SendBroadcast(const char *pText)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i] == 0x0)
			continue;

		CNetMsg_Sv_Broadcast Msg;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_apPlayers[i]->GetCID());
	}
}

void CGameMap::OnClientEnter(int ClientID)
{

	m_VoteUpdate = true;
}