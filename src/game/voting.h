

#ifndef GAME_VOTING_H
#define GAME_VOTING_H

enum
{
	VOTE_DESC_LENGTH=64,
	VOTE_CMD_LENGTH=512,
	VOTE_REASON_LENGTH=16,

	MAX_VOTE_OPTIONS=128,
};

struct CVoteOptionClient
{
	CVoteOptionClient *m_pNext;
	CVoteOptionClient *m_pPrev;
	char m_aDescription[VOTE_DESC_LENGTH];
};

struct CVoteOptionServer
{
	char m_aDescription[VOTE_DESC_LENGTH];
	char m_aCommand[VOTE_CMD_LENGTH];
};

#endif
