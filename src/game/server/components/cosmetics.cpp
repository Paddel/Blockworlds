
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/generated/protocol.h>

#include "cosmetics_anims.h"
#include "cosmetics.h"


const char *CCosmeticsHandler::ms_KnockoutNames[NUM_KNOCKOUTS] = {
	"Explosion",
	"Hammerhit",
	"Love",
	"Thunderstorm",
};

void CCosmeticsHandler::KnockoutExplosion(int ClientID, int Victim, vec2 Pos, CGameMap *pGameMap)
{
	GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CCosmeticsHandler::KnockoutHammerhit(int ClientID, int Victim, vec2 Pos, CGameMap *pGameMap)
{
	GameServer()->CreateHammerHit(pGameMap, Pos);
}

void CCosmeticsHandler::Tick()
{
	for (int i = 0; i < m_lpAnimations.size(); i++)
	{
		if (m_lpAnimations[i]->Done() == false)
		{
			m_lpAnimations[i]->Tick();
		}
		else
		{
			CCosmeticsHandler::CCosAnim *pAnimation = m_lpAnimations[i];
			m_lpAnimations.remove_index(i);
			delete pAnimation;
			i--;
		}
	}
}

void CCosmeticsHandler::Snap(int SnappingClient)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(SnappingClient);
	if (pGameMap == 0x0 || GameServer()->m_apPlayers[SnappingClient] == 0x0)
		return;

	for (int i = 0; i < m_lpAnimations.size(); i++)
	{
		if (m_lpAnimations[i]->GetGameMap() != pGameMap)
			continue;

		if (within_reach(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, m_lpAnimations[i]->GetPos(), 1100.0f) == false)
			continue;

		m_lpAnimations[i]->Snap(SnappingClient);
	}
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

	DoKnockoutEffect(ClientID, Victim, Pos, Effect);
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, int Victim, vec2 Pos, const char *pName)
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

	DoKnockoutEffect(ClientID, Victim, Pos, Effect);
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, int Victim, vec2 Pos, int Effect)
{
	if (HasKnockoutEffect(ClientID, Effect) == false)
		return;

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return;

	if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Knockouteffect %s", ms_KnockoutNames[Effect]);

	switch (Effect)
	{
	case KNOCKOUT_EXPLOSION: KnockoutExplosion(ClientID, Victim, Pos, pGameMap); break;
	case KNOCKOUT_HAMMERHIT: KnockoutHammerhit(ClientID, Victim, Pos, pGameMap); break;
	case KNOCKOUT_LOVE: m_lpAnimations.add(new CLoveAnim(Pos, Server()->Tick(), pGameMap)); break;
	case KNOCKOUT_THUNDERSTORM: m_lpAnimations.add(new CThunderstorm(Pos, Server()->Tick(), pGameMap)); break;
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