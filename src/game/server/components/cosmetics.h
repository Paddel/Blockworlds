#pragma once

#include <base/vmath.h>
#include <game/server/component.h>

class CCosmeticsHandler : public CComponent
{
public:
	enum
	{
		KNOCKOUT_EXPLOSION=0,
		KNOCKOUT_HAMMERHIT,
		NUM_KNOCKOUTS,//Maximum sizeof(m_aKnockouts)/sizeof(char) = 256
	};


	static char *ms_KnockoutNames[NUM_KNOCKOUTS];

private:
	void KnockoutExplosion(int ClientID, int Victim, vec2 Pos);
	void KnockoutHammerhit(int ClientID, int Victim, vec2 Pos);

public:

	bool HasKnockoutEffect(int ClientID, int Index);
	void DoKnockoutEffect(int ClientID, int Victim, vec2 Pos);
	bool ToggleKnockout(int ClientID, const char *pName);

	void FillKnockout(IServer::CAccountData *pFillingData, const char *pValue);
};