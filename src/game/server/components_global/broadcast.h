#pragma once

#include <base/tl/array.h>
#include <engine/shared/protocol.h>
#include <game/server/component.h>

class CBroadcastHandler : public CComponentGlobal
{
	enum
	{
		MAX_MAINCASTS = 8,
	};

	struct CMainCast
	{
		char m_aText[64];
		int m_Ticks;
	};

	struct CBroadState
	{
		int m_NumMainCasts;
		CMainCast m_aMainCasts[MAX_MAINCASTS];
		char m_aSideCast[512];
		CBroadState() { m_NumMainCasts = 0; m_aSideCast[0] = '\0'; }
	};

private:
	//array<CMainCast *> m_lpGlobalCasts;
	CBroadState m_aCurrentState[MAX_CLIENTS];
	CBroadState m_aLastState[MAX_CLIENTS];
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