#pragma once

#include <base/vmath.h>
#include <engine/shared/protocol.h>

class CServer;

struct CTranslateItem // contains all info the thread needs to sort and idmap
{
	vec2 m_Pos;
	int m_ClientID;//0 - MAX_CLIENTS = player and above is npc
	int m_Team;
};

class CTranslator
{
private:
	struct CSortionData // contains all data for a thread
	{
		vec2 m_Pos; // clas writes
		CTranslateItem *m_aTranslateItems; // class writes
		int m_ClientID; // class writs
		int m_NumTranslateItems; // class writes
		int m_UsingMapItems; // class writes
		short m_aIDMap[MAX_CLIENTS]; // thread writes
		bool m_Working; // both write
	};

private:
	CServer *m_pServer;
	short m_aIDMap[MAX_CLIENTS * MAX_CLIENTS];
	CSortionData m_Sortions[MAX_CLIENTS];
	int *m_aNumItems;

	static void SortMap(void *pData);

	void UpdateMapInfos();
	void UpdatePlayerMap(int ClientID);
public:
	CTranslator(CServer *pServer);

	void Tick();

	int Translate(int For, int ClientID);
	int ReverseTranslate(int For, int ClientID);

	CServer *Server() const { return m_pServer; };
};