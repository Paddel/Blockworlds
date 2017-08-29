#pragma once

#include <base/tl/array.h>
#include <engine/shared/protocol.h>
#include <game/voting.h>
#include <game/server/component.h>

struct CVoteMenu
{
	array <CVoteOptionServer *> m_lpKnockoutEffectsOptions;
	array <CVoteOptionServer *> m_lpGundesignOptions;
	array <CVoteOptionServer *> m_lpSkinmaniOptions;
	array <CVoteOptionServer *> m_lpExtrasOptions;

	void Reset();
	bool Compare(CVoteMenu *pCompare);
};

class CVoteMenuHandler : public CComponent
{
private:
	CVoteMenu m_Menus[MAX_CLIENTS];
	array <CVoteOptionServer *> m_lpServerOptions;

	void CreateStripline(char *pDst, int DstSize, const char *pTitle);
	void UpdateEveryone();
	void Construct(CVoteMenu *pFilling, int ClientID);
public:

	virtual void Init();
	virtual void Tick();

	void OnClientJoin(int ClientID);

	void AddVote(const char *pDescription, const char *pCommand);
	void RemoveVote(const char *pDescription);
	void ForceVote(int ClientID, const char *pDescription, const char *pReason);
	void ClearVotes();

	void CallVote(int ClientID, const char *pDescription, const char *pReason);
	void UpdateMenu(int ClientID);

};