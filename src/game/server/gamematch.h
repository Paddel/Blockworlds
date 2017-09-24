#pragma once

#include <game/server/player.h>

class CGameMap;
class CGameWorld;
class CGameContext;
class IServer;

class CGameMatch
{
private:
	CGameMap *m_pGameMap;
	CGameContext *m_pGameServer;
	IServer *m_pServer;
	CGameWorld *m_pGameWorld;
	bool m_aParticipants[MAX_CLIENTS];
	CCharState m_aCharStates[MAX_CLIENTS];
	int m_Scores[MAX_CLIENTS];
	int m_Blockpoints;
	int m_Pot;

	void DoScoreBroadcast();
	int NumParticipants();
	void UpdateParcitipants();
	void ResetMatchup();
	void ScorePlayer(int ClientID);
	void SendChat(const char *pMessage);
	void SetWinner(int ClientID);
public:
	CGameMatch(CGameMap *pGameMap, int Blockpoints);
	~CGameMatch();

	void PlayerBlocked(int ClientID, vec2 Pos);
	void PlayerKilled(int ClientID);

	void AddParticipant(int ClientID);
	void Tick();
	void Start();
	void ResumeClient(int ClientID);
	void End();

	CGameMap *GameMap() const { return m_pGameMap; }
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};