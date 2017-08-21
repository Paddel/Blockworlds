
#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/npc.h>
#include <game/server/entities/pickup.h>

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
}

CGameMap::~CGameMap()
{
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
	else if (Index == ENTITY_SPAWN_RED)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if (Index == ENTITY_SPAWN_BLUE)
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
			SendChat(-1, CGameContext::CHAT_ALL, "Vote aborted");
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
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote passed");

				if (m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote failed");
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
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

	CNpc *pNpc = new CNpc(&m_World, FreeNpcSlot());
	pNpc->Spawn(vec2(300, 300));

	pNpc = new CNpc(&m_World, FreeNpcSlot());
	pNpc->Spawn(vec2(300, 300));

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

	// send active vote
	if (m_VoteCloseTime)
		SendVoteSet(ClientID);
	return true;
}

void CGameMap::PlayerLeave(int ClientID)
{
	m_apPlayers[ClientID] = 0x0;

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
	{
		if (m_apPlayers[i] != 0x0)
		{
			mem_copy(&pTranslateItems[Num++], m_apPlayers[i]->GetTranslateItem(), sizeof(CTranslateItem));
		}
	}

	for (CNpc *pNpc = (CNpc *)m_World.FindFirst(CGameWorld::ENTTYPE_NPC); pNpc; pNpc = (CNpc *)pNpc->TypeNext())
		mem_copy(&pTranslateItems[Num++], pNpc->GetTranslateItem(), sizeof(CTranslateItem));
}

bool CGameMap::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, 0);
	EvaluateSpawnType(&Eval, 1);
	EvaluateSpawnType(&Eval, 2);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
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
	m_World.Snap(SnappingClient);
	SnapGameInfo(SnappingClient);
	m_Events.Snap(SnappingClient);
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
	SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pVote);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameMap::Tick()
{
	mem_copy(&m_World.m_Core.m_Tuning, GameServer()->Tuning(), sizeof(m_World.m_Core.m_Tuning));
	m_World.Tick();
	UpdateVote();
}

void CGameMap::SendChat(int ChatterClientID, int Team, const char *pText)
{
	if (g_Config.m_Debug)
	{
		char aBuf[256];
		if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
			str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
		else
			str_format(aBuf, sizeof(aBuf), "*** %s", pText);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team != CGameContext::CHAT_ALL ? "teamchat" : "chat", aBuf);
	}

	if (Team == CGameContext::CHAT_ALL)
	{
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
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// send to the clients
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, m_apPlayers[i]->GetCID());
		}
	}
}

void CGameMap::OnClientEnter(int ClientID)
{

	m_VoteUpdate = true;
}