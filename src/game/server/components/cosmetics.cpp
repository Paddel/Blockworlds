
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/generated/protocol.h>

#include "cosmetics_anims.h"
#include "cosmetics_chars.h"
#include "cosmetics.h"

static bool gs_LetterBits[256][5 * 7] = {};

const char *CCosmeticsHandler::ms_KnockoutNames[NUM_KNOCKOUTS] = {
	"Explosion",
	"Hammerhit",
	"Love",
	"Thunderstorm",
	"Lettesttest",
};

void CCosmeticsHandler::KnockoutExplosion(int ClientID, vec2 Pos, CGameMap *pGameMap)
{
	GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CCosmeticsHandler::KnockoutHammerhit(int ClientID, vec2 Pos, CGameMap *pGameMap)
{
	GameServer()->CreateHammerHit(pGameMap, Pos);
}

void CCosmeticsHandler::Init()
{
	mem_copy(gs_LetterBits['a'], gs_LetterA, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['b'], gs_LetterB, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['c'], gs_LetterC, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['d'], gs_LetterD, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['e'], gs_LetterE, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['f'], gs_LetterF, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['g'], gs_LetterG, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['h'], gs_LetterH, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['i'], gs_LetterI, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['j'], gs_LetterJ, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['k'], gs_LetterK, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['l'], gs_LetterL, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['m'], gs_LetterM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['n'], gs_LetterN, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['o'], gs_LetterO, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['p'], gs_LetterP, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['q'], gs_LetterQ, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['r'], gs_LetterR, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['s'], gs_LetterS, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['t'], gs_LetterT, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['u'], gs_LetterU, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['v'], gs_LetterV, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['w'], gs_LetterW, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['x'], gs_LetterX, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['y'], gs_LetterY, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['z'], gs_LetterZ, sizeof(gs_LetterBits[0]));

	mem_copy(gs_LetterBits['A'], gs_LetterA, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['B'], gs_LetterB, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['C'], gs_LetterC, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['D'], gs_LetterD, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['E'], gs_LetterE, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['F'], gs_LetterF, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['G'], gs_LetterG, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['H'], gs_LetterH, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['I'], gs_LetterI, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['J'], gs_LetterJ, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['K'], gs_LetterK, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['L'], gs_LetterL, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['M'], gs_LetterM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['N'], gs_LetterN, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['O'], gs_LetterO, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['P'], gs_LetterP, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['Q'], gs_LetterQ, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['R'], gs_LetterR, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['S'], gs_LetterS, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['T'], gs_LetterT, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['U'], gs_LetterU, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['V'], gs_LetterV, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['W'], gs_LetterW, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['X'], gs_LetterX, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['Y'], gs_LetterY, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['Z'], gs_LetterZ, sizeof(gs_LetterBits[0]));

	mem_copy(gs_LetterBits['0'], gs_Letter0, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['1'], gs_Letter1, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['2'], gs_Letter2, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['3'], gs_Letter3, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['4'], gs_Letter4, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['5'], gs_Letter5, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['6'], gs_Letter6, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['7'], gs_Letter7, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['8'], gs_Letter8, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['9'], gs_Letter9, sizeof(gs_LetterBits[0]));

	mem_copy(gs_LetterBits['+'], gs_LetterPL, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['-'], gs_LetterMN, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['!'], gs_LetterEM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['?'], gs_LetterQM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['%'], gs_LetterPC, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['$'], gs_LetterDL, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['('], gs_LetterBO, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits[')'], gs_LetterBC, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['.'], gs_LetterDT, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits[','], gs_LetterCM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits[':'], gs_LetterDD, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['<'], gs_LetterSM, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['>'], gs_LetterBI, sizeof(gs_LetterBits[0]));
	mem_copy(gs_LetterBits['='], gs_LetterEQ, sizeof(gs_LetterBits[0]));
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

void CCosmeticsHandler::Laserwrite(const char *pText, vec2 StartPos, float Size, int Ticks, CGameMap *pGameMap)
{
	int Length = str_length(pText);

	vec2 Pos = StartPos;
	for (int i = 0; i < Length; i++)
	{
		CCosChar *pChar = new CCosChar(Pos, Server()->Tick(), pGameMap, Ticks, gs_LetterBits[pText[i]], Size);
		m_lpAnimations.add(pChar);
		Pos.x += pChar->Width() + Size;
	}
}

bool CCosmeticsHandler::HasKnockoutEffect(int ClientID, int Index)
{
	if (Index < 0 || Index >= NUM_KNOCKOUTS)
		return false;

	if (Server()->GetClientAuthed(ClientID) == IServer::AUTHED_ADMIN)
		return true;
	
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return false;

	return Server()->GetClientInfo(ClientID)->m_AccountData.m_aKnockouts[Index] == '1';
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos)
{
	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentKnockout;
	if (HasKnockoutEffect(ClientID, Effect) == false)
		return;

	DoKnockoutEffect(ClientID, Pos, Effect);
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos, const char *pName)
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

	DoKnockoutEffect(ClientID, Pos, Effect);
}

void CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos, int Effect)
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
	case KNOCKOUT_EXPLOSION: KnockoutExplosion(ClientID, Pos, pGameMap); break;
	case KNOCKOUT_HAMMERHIT: KnockoutHammerhit(ClientID, Pos, pGameMap); break;
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