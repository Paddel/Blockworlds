#pragma once

#include <base/system.h>
#include <game/voting.h>
#include <game/server/component.h>

class CMapVoting : public CComponentMap
{
public:
	struct CMapVote : public CVoteOptionServer//starts with %
	{
		bool m_Active;
	};

	enum
	{
		MAPVOTE_EVENT=0,
		NUM_MAPVOTES,
	};

private:
	//Voting
	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_VoteEnforce;
	static CMapVote m_aMapVotes[NUM_MAPVOTES];
	bool m_aMapVoteAcitve[NUM_MAPVOTES];
	int m_NumActiveMapVotes;
	int m_UpdateNumMapVotes;

	enum
	{
		VOTE_ENFORCE_UNKNOWN = 0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};

	void UpdateVote();

public:
	CMapVoting();

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

	bool ExecuteMapVote(const char *pDesc);
	bool OnMapVote(int ClientID, const char *pDesc, CVoteOptionServer **ppOption);
	bool MapvoteActive(int Index);

	int GetNumActiveMapVotes() const { return m_NumActiveMapVotes; }
	const char *GetMapVoteDesc(int Index) const { return m_aMapVotes[Index].m_aDescription; }

	virtual void Tick();
	virtual void Init();

	static void InitMapVotes();
	static int ActivateMapVoting(const char *pCommand, const char *pDesc);
	static bool DeactivateMapVoting(const char *pDesc);
	static void PrintMapVotes();
};