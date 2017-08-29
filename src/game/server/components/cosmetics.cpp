
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
	"Lettertest",
};

const char *CCosmeticsHandler::ms_GundesignNames[NUM_GUNDESIGNS] = {
	"Heart (VIP)",
	"Pew",
};

const char *CCosmeticsHandler::ms_SkinmaniNames[NUM_SKINMANIS] = {
	"Rainbow",
	"Epi Rainbow",
	"Cooldown",
	"Nightblue",
};

inline int HslToCc(vec3 HSL)
{
	return ((int)(HSL.h * 255) << 16) + ((int)(HSL.s * 255) << 8) + (HSL.l - 0.5f) * 255 * 2;
}

inline vec3 CcToHsl(int Cc)
{
	return vec3(((Cc >> 16) & 0xff) / 255.0f, ((Cc >> 8) & 0xff) / 255.0f, 0.5f + (Cc & 0xff) / 255.0f*0.5f);
}

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
	mem_copy(gs_LetterBits['|'], gs_LetterLV, sizeof(gs_LetterBits[0]));
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
		CCosChar *pChar = new CCosChar(Pos, Server()->Tick(), pGameMap, Ticks, gs_LetterBits[(unsigned char)pText[i]], Size);
		m_lpAnimations.add(pChar);
		Pos.x += pChar->Width() + Size;
	}
}

bool CCosmeticsHandler::HasKnockoutEffect(int ClientID, int Index)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if (Index < 0 || Index >= NUM_KNOCKOUTS)
		return false;

	if (Server()->GetClientAuthed(ClientID) == IServer::AUTHED_ADMIN)
		return true;
	
	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return false;

	return Server()->GetClientInfo(ClientID)->m_AccountData.m_aKnockouts[Index] == '1';
}

bool CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentKnockout;
	if (HasKnockoutEffect(ClientID, Effect) == false)
		return false;

	return DoKnockoutEffect(ClientID, Pos, Effect);
}

bool CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos, const char *pName)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = -1;
	for (int i = 0; i < NUM_KNOCKOUTS; i++)
	{
		if (str_comp(ms_KnockoutNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	return DoKnockoutEffect(ClientID, Pos, Effect);
}

bool CCosmeticsHandler::DoKnockoutEffect(int ClientID, vec2 Pos, int Effect)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if (HasKnockoutEffect(ClientID, Effect) == false)
		return false;

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return false;

	if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Knockouteffect %s", ms_KnockoutNames[Effect]);

	switch (Effect)
	{
	case KNOCKOUT_EXPLOSION: KnockoutExplosion(ClientID, Pos, pGameMap); return true;
	case KNOCKOUT_HAMMERHIT: KnockoutHammerhit(ClientID, Pos, pGameMap); return true;
	case KNOCKOUT_LOVE: m_lpAnimations.add(new CLoveAnim(Pos, Server()->Tick(), pGameMap)); return true;
	case KNOCKOUT_THUNDERSTORM: m_lpAnimations.add(new CThunderstorm(Pos, Server()->Tick(), pGameMap)); return true;
	case LETTER_TEST: 
	{
		char aBuf[257] = {};
		for(int i = 30; i < 256; i++)
			str_fcat(aBuf, 257, "%c", i);
		Laserwrite(aBuf, Pos, 14.0f, Server()->TickSpeed() * 30.0f, pGameMap);
	} return true;
	default: dbg_msg("cosmetics", "ERROR: Knockouteffect '%s' not implemented!", ms_KnockoutNames[Effect]);
	}
	return false;
}

bool CCosmeticsHandler::ToggleKnockout(int ClientID, const char *pName)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	
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


bool CCosmeticsHandler::HasGundesign(int ClientID, int Index)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if (Index < 0 || Index >= NUM_GUNDESIGNS)
		return false;

	if (Server()->GetClientAuthed(ClientID) == IServer::AUTHED_ADMIN)
		return true;

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return false;

	return Server()->GetClientInfo(ClientID)->m_AccountData.m_aGundesigns[Index] == '1';
}

bool CCosmeticsHandler::DoGundesign(int ClientID, vec2 Pos)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentGundesign;
	if (HasGundesign(ClientID, Effect) == false)
		return false;

	return DoGundesign(ClientID, Pos, Effect);
}

bool CCosmeticsHandler::DoGundesign(int ClientID, vec2 Pos, const char *pName)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = -1;
	for (int i = 0; i < NUM_GUNDESIGNS; i++)
	{
		if (str_comp(ms_GundesignNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	return DoGundesign(ClientID, Pos, Effect);
}

bool CCosmeticsHandler::DoGundesign(int ClientID, vec2 Pos, int Effect)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if (HasGundesign(ClientID, Effect) == false)
		return false;

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return false;

	/*if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Gundesigneffect %s", ms_GundesignNames[Effect]);*/

	if (Effect == GUNDESIGN_PEW)
		Laserwrite("PEW", Pos - vec2((5.0f * 15.0f + 5.0f) *0.5f, 5.0f * 3.5f), 5.0f, Server()->TickSpeed() * 0.2f, pGameMap);
	else
		return false;

	return true;
}

bool CCosmeticsHandler::ToggleGundesign(int ClientID, const char *pName)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = -1;
	for (int i = 0; i < NUM_GUNDESIGNS; i++)
	{
		if (str_comp(ms_GundesignNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	if (HasGundesign(ClientID, Effect) == false)
		return false;

	if (Server()->GetClientInfo(ClientID)->m_CurrentGundesign == Effect)
		Server()->GetClientInfo(ClientID)->m_CurrentGundesign = -1;
	else
		Server()->GetClientInfo(ClientID)->m_CurrentGundesign = Effect;
	return true;
}

bool CCosmeticsHandler::SnapGundesign(int ClientID, vec2 Pos, int EntityID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentGundesign;
	if (HasGundesign(ClientID, Effect) == false)
		return false;

	if (Effect == GUNDESIGN_HEART)
	{
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, EntityID, sizeof(CNetObj_Pickup)));
		if (pP != 0x0)
		{
			pP->m_X = (int)Pos.x;
			pP->m_Y = (int)Pos.y;
			pP->m_Type = POWERUP_HEALTH;
			pP->m_Subtype = 0;
		}

		return true;
	}

	return false;
}

void CCosmeticsHandler::FillGundesign(IServer::CAccountData *pFillingData, const char *pValue)
{
	int Length = str_length(pValue);
	for (int i = 0; i < CCosmeticsHandler::NUM_GUNDESIGNS; i++)
	{
		if (i >= Length)
			pFillingData->m_aGundesigns[i] = '0';
		else
			pFillingData->m_aGundesigns[i] = pValue[i];
	}

	pFillingData->m_aGundesigns[CCosmeticsHandler::NUM_GUNDESIGNS] = '\0';
}

bool CCosmeticsHandler::HasSkinmani(int ClientID, int Index)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if (Index < 0 || Index >= NUM_SKINMANIS)
		return false;

	if (Server()->GetClientAuthed(ClientID) == IServer::AUTHED_ADMIN)
		return true;

	if (Server()->GetClientInfo(ClientID)->m_LoggedIn == false)
		return false;

	return Server()->GetClientInfo(ClientID)->m_AccountData.m_aSkinmani[Index] == '1';
}

bool CCosmeticsHandler::ToggleSkinmani(int ClientID, const char *pName)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = -1;
	for (int i = 0; i < NUM_SKINMANIS; i++)
	{
		if (str_comp(ms_SkinmaniNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	if (HasSkinmani(ClientID, Effect) == false)
		return false;

	if (Server()->GetClientInfo(ClientID)->m_CurrentSkinmani == Effect)
		Server()->GetClientInfo(ClientID)->m_CurrentSkinmani = -1;
	else
		Server()->GetClientInfo(ClientID)->m_CurrentSkinmani = Effect;
	return true;
}

void CCosmeticsHandler::SnapSkinmani(int ClientID, int64 Tick, CNetObj_ClientInfo *pClientInfo)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;

	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentSkinmani;

	if (HasSkinmani(ClientID, Effect) == false)
		return;

	int64 TickDef = Server()->Tick() - Tick;//only work with Tickdef
	vec3 HSLBody = CcToHsl(pClientInfo->m_ColorBody);
	//vec3 HSLFeet = CcToHsl(pClientInfo->m_ColorFeet);

	if (Effect == SKINMANI_RAINBOW)
	{
		/*pClientInfo->m_ColorBody = ((int)((sinf(TickDef / 255.0f) + 1.0f) / 2.0f * 255) << 16) +
			((int)(((pClientInfo->m_ColorBody >> 8) & 0xff) * 255) << 8)
			+ ((0.5f + (pClientInfo->m_ColorBody & 0xff) / 255.0f*0.5f) - 0.5f) * 255 * 2;*/
		HSLBody.h = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLBody.s = 0.5f;
		HSLBody.l = 0.0f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
		pClientInfo->m_ColorFeet = HslToCc(HSLBody);
	}
	else if (Effect == SKINMANI_RAINBOW_EPI)
	{
		HSLBody.h = (sinf(TickDef / 2.0f) + 1.0f) / 2.0f;
		HSLBody.s = 1.0f;
		HSLBody.l = 0.0f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
		pClientInfo->m_ColorFeet = HslToCc(HSLBody);
	}
	else if (Effect == SKINMANI_COOLDOWN)
	{
		HSLBody.s *= clamp((sinf(TickDef / 50.0f) + 1.0f), 0.0f, HSLBody.s);
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
	}
	else if (Effect == SKINMANI_NIGHTBLUE)
	{
		HSLBody.h = 0.71f;
		HSLBody.s = 0.7f * clamp((sinf(TickDef / 50.0f) + 1.2f), 0.2f, 0.7f);
		HSLBody.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
	}
}

void CCosmeticsHandler::FillSkinmani(IServer::CAccountData *pFillingData, const char *pValue)
{
	int Length = str_length(pValue);
	for (int i = 0; i < CCosmeticsHandler::NUM_SKINMANIS; i++)
	{
		if (i >= Length)
			pFillingData->m_aSkinmani[i] = '0';
		else
			pFillingData->m_aSkinmani[i] = pValue[i];
	}

	pFillingData->m_aSkinmani[CCosmeticsHandler::NUM_SKINMANIS] = '\0';
}