
#include <engine/server.h>
#include <game/server/gamemap.h>
#include <game/server/player.h>

#include "mapvoting.h"

CMapVoting::CMapVote CMapVoting::m_aMapVotes[CMapVoting::NUM_MAPVOTES];
static int s_UpdateNumMapVotes = 0;

CMapVoting::CMapVoting()
{
	m_VoteCloseTime = 0;

	mem_zero(&m_aMapVoteAcitve, sizeof(m_aMapVoteAcitve));
	m_NumActiveMapVotes = 0;
	m_UpdateNumMapVotes = s_UpdateNumMapVotes;
}

void CMapVoting::UpdateVote()
{
	// update voting
	if (m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if (m_VoteCloseTime == -1)
		{
			GameMap()->SendChat(-1, "Vote aborted");
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
					if (GameMap()->m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = { 0 };
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!GameMap()->m_apPlayers[i] || GameMap()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i])	// don't count in votes by spectators
						continue;

					int ActVote = GameMap()->m_apPlayers[i]->m_Vote;
					int ActVotePos = GameMap()->m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for (int j = i + 1; j < MAX_CLIENTS; ++j)
					{
						if (!GameMap()->m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if (GameMap()->m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > GameMap()->m_apPlayers[j]->m_VotePos))
						{
							ActVote = GameMap()->m_apPlayers[j]->m_Vote;
							ActVotePos = GameMap()->m_apPlayers[j]->m_VotePos;
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
				if (m_aVoteCommand[0] == '%')
					ExecuteMapVote(m_aVoteDescription);
				else
				{
					Server()->SetRconCID(IServer::RCON_CID_VOTE);
					GameServer()->Console()->ExecuteLine(m_aVoteCommand);
					Server()->SetRconCID(IServer::RCON_CID_SERV);
				}
				EndVote();
				GameMap()->SendChat(-1, "Vote passed");

				if (GameMap()->m_apPlayers[m_VoteCreator])
					GameMap()->m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				GameMap()->SendChat(-1, "Vote failed");
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}
}

void CMapVoting::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if (m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameMap()->m_apPlayers[i])
		{
			GameMap()->m_apPlayers[i]->m_Vote = 0;
			GameMap()->m_apPlayers[i]->m_VotePos = 0;
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


void CMapVoting::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CMapVoting::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	if (ClientID == -1)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
			SendVoteStatus(i, Total, Yes, No);
	}
	else
	{
		if (GameMap()->m_apPlayers[ClientID] == 0x0)
			return;

		CNetMsg_Sv_VoteStatus Msg = { 0 };
		Msg.m_Total = Total;
		Msg.m_Yes = Yes;
		Msg.m_No = No;
		Msg.m_Pass = Total - (Yes + No);
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
}

void CMapVoting::VoteEnforce(const char *pVote)
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
	GameMap()->SendChat(-1, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pVote);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CMapVoting::SendVoteSet(int ClientID)
{
	if (ClientID == -1)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
			SendVoteSet(i);
	}
	else
	{
		if (GameMap()->m_apPlayers[ClientID] == 0x0)
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

void CMapVoting::Tick()
{
	UpdateVote();

	if (m_UpdateNumMapVotes != s_UpdateNumMapVotes)
	{
		m_NumActiveMapVotes = 0;
		for (int i = 0; i < NUM_MAPVOTES; i++)
			if (MapvoteActive(i))
				m_NumActiveMapVotes++;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameMap()->m_apPlayers[i] != 0x0)
				GameServer()->VoteMenuHandler()->UpdateMenu(i);
		}

		m_UpdateNumMapVotes = s_UpdateNumMapVotes;
	}
}

bool CMapVoting::ExecuteMapVote(const char *pDesc)
{
	int Index = -1;
	for (int i = 0; i < NUM_MAPVOTES; i++)
	{
		if (str_comp(pDesc, m_aMapVotes[i].m_aDescription) != 0)
			continue;
		Index = i;
		break;
	}

	if (Index == -1 || MapvoteActive(Index) == false)
		return false;

	if (Index == MAPVOTE_EVENT)
		GameMap()->StartRandomEvent();


	return true;
}

bool CMapVoting::OnMapVote(int ClientID, const char *pDesc, CVoteOptionServer **ppOption)
{
	int Index = -1;
	for (int i = 0; i < NUM_MAPVOTES; i++)
	{
		if (str_comp(pDesc, m_aMapVotes[i].m_aDescription) != 0)
			continue;
		Index = i;
		break;
	}

	if (Index == -1 || MapvoteActive(Index) == false)
		return false;

	if (Index == MAPVOTE_EVENT)
	{
		if (GameMap()->TryVoteRandomEvent(ClientID) == true)
		{
			*ppOption = &m_aMapVotes[Index];
		}
		return true;
	}

	return false;
}

bool CMapVoting::MapvoteActive(int Index)
{
	return m_aMapVoteAcitve[Index] == true && m_aMapVotes[Index].m_Active == true;
}

void CMapVoting::Init()
{
	if (GameMap()->IsBlockMap() == true && GameMap()->HasArena())
		m_aMapVoteAcitve[MAPVOTE_EVENT] = true;



	for (int i = 0; i < NUM_MAPVOTES; i++)
		if (MapvoteActive(i))
			m_NumActiveMapVotes++;
}

void CMapVoting::InitMapVotes()
{
	mem_zero(&m_aMapVotes, sizeof(m_aMapVotes));

	str_copy(m_aMapVotes[MAPVOTE_EVENT].m_aCommand, "%rand_event", sizeof(CMapVote::m_aCommand));
}

int CMapVoting::ActivateMapVoting(const char *pCommand, const char *pDesc)
{
	for (int i = 0; i < NUM_MAPVOTES; i++)
	{
		if (str_comp_nocase(pCommand, m_aMapVotes[i].m_aCommand) == 0)
		{
			if (m_aMapVotes[i].m_Active == true)
				return -1;
			m_aMapVotes[i].m_Active = true;
			str_copy(m_aMapVotes[i].m_aDescription, pDesc, sizeof(CMapVote::m_aDescription));
			s_UpdateNumMapVotes++;
			return 0;
		}
	}
	return -2;
}

bool CMapVoting::DeactivateMapVoting(const char *pDesc)
{
	for (int i = 0; i < NUM_MAPVOTES; i++)
	{
		if (str_comp_nocase(pDesc, m_aMapVotes[i].m_aDescription) == 0)
		{
			m_aMapVotes[i].m_Active = false;
			s_UpdateNumMapVotes++;
			return true;
		}
	}
	return false;
}


void CMapVoting::PrintMapVotes()
{
	for (int i = 0; i < NUM_MAPVOTES; i++)
		dbg_msg("mapvotes", "vote '%s' %s as '%s'", m_aMapVotes[i].m_aCommand, m_aMapVotes[i].m_Active ? "active" : "inactive", m_aMapVotes[i].m_aDescription);
}