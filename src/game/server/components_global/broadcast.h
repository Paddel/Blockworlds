#pragma once

#include <base/tl/array.h>
#include <engine/shared/protocol.h>
#include <game/server/component.h>

class CBroadcastHandler : public CComponentGlobal
{
	struct CMainCast
	{
		char *m_pText;
		int m_Ticks;
		~CMainCast();
	};

	struct CBroadState
	{
		array<char *> m_lpSideCasts;
		//array<CMainCast *> m_lpMainCasts;
		~CBroadState();
	};

private:
	//array<CMainCast *> m_lpGlobalCasts;
	CBroadState *m_pCurrentState[MAX_CLIENTS];
	CBroadState *m_pLastState[MAX_CLIENTS];
	int64 m_LastUpdate[MAX_CLIENTS];

	void AddWhitespace(char *pDst, int Size, const char *pSrc);
	bool StateEmpty(CBroadState *pState);
	bool NeedRefresh(int ClientID);
	bool CompareStates(int ClientID);
	void UpdateClient(int ClientID);
	void TickClient(int ClientID);
public:
	CBroadcastHandler();

	void AddMainCast(int ClientID, const char *pText, int Time);
	void AddSideCast(int ClientID, const char *pText);

	virtual void Tick();
};