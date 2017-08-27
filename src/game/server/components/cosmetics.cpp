
#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/generated/protocol.h>

#include "cosmetics.h"

char *CCosmeticsHandler::ms_KnockoutNames[NUM_KNOCKOUTS] = {
	"Explosion",
	"Hammerhit",
};

void CCosmeticsHandler::KnockoutExplosion(int ClientID, int Victim, vec2 Pos)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return;

	GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CCosmeticsHandler::KnockoutHammerhit(int ClientID, int Victim, vec2 Pos)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return;

	GameServer()->CreateHammerHit(pGameMap, Pos);
}

bool CCosmeticsHandler::HasKnockoutEffect(int ClientID, int Index)
{
	if (Index < 0 || Index >= NUM_KNOCKOUTS)
		return false;

	//check logged in

	return true;
	return Server()->GetClientInfo(ClientID)->m_AccountData.m_aKnockouts[Index] == '1';
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, int Victim, vec2 Pos)
{
	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentKnockout;
	if (HasKnockoutEffect(ClientID, Effect) == false)
		return;

	switch (Effect)
	{
	case KNOCKOUT_EXPLOSION: KnockoutExplosion(ClientID, Victim, Pos); break;
	case KNOCKOUT_HAMMERHIT: KnockoutHammerhit(ClientID, Victim, Pos);
	default: dbg_msg("cosmetics", "ERROR: Knockouteffect '%s' not implemented!", ms_KnockoutNames[Effect]);
	}
}

bool CCosmeticsHandler::ToggleKnockout(int ClientID, const char *pName)
{
	int Effect = -1;
	for (int i = 0; i < NUM_KNOCKOUTS; i++)
	{
		if (str_comp(ms_KnockoutNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	if (HasKnockoutEffect(ClientID, Effect) == false)
		return false;

	if (Server()->GetClientInfo(ClientID)->m_CurrentKnockout == Effect)
		Server()->GetClientInfo(ClientID)->m_CurrentKnockout = -1;
	else
		Server()->GetClientInfo(ClientID)->m_CurrentKnockout = Effect;
	return true;
}

void CCosmeticsHandler::FillKnockout(IServer::CAccountData *pFillingData, const char *pValue)
{
	int Length = str_length(pValue);
	for (int i = 0; i < CCosmeticsHandler::NUM_KNOCKOUTS; i++)
	{
		if (i >= Length)
			pFillingData->m_aKnockouts[i] = '0';
		else
			pFillingData->m_aKnockouts[i] = pValue[i];
	}

	pFillingData->m_aKnockouts[CCosmeticsHandler::NUM_KNOCKOUTS] = '\0';
}