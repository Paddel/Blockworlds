
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/generated/protocol.h>

#include "cosmetics.h"

const char *CCosmeticsHandler::ms_KnockoutNames[NUM_KNOCKOUTS] = {
	"Explosion",
	"Hammerhit",
	"Love",
	"Thunderstorm",
};

const char *CCosmeticsHandler::ms_GundesignNames[NUM_GUNDESIGNS] = {
	"Heart",
	"Pew",
};

const char *CCosmeticsHandler::ms_SkinmaniNames[NUM_SKINMANIS] = {
	"Rainbow",
	"Epi Rainbow (VIP)",
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

void CCosmeticsHandler::KnockoutExplosion(vec2 Pos, CGameMap *pGameMap)
{
	GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CCosmeticsHandler::Tick()
{
	
}

void CCosmeticsHandler::Snap(int SnappingClient)
{
	
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

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return false;

	DoKnockoutEffectRaw(Pos, Effect, pGameMap);
	return true;
}

bool CCosmeticsHandler::DoKnockoutEffect(vec2 Pos, const char *pName, CGameMap *pGameMap)
{
	int Effect = -1;
	for (int i = 0; i < NUM_KNOCKOUTS; i++)
	{
		if (str_comp_nocase(ms_KnockoutNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	if (Effect == -1)
		return false;

	DoKnockoutEffectRaw(Pos, Effect, pGameMap);
	return true;
}

void CCosmeticsHandler::DoKnockoutEffectRaw(vec2 Pos, int Effect, CGameMap *pGameMap)
{
	/*if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Knockouteffect %s", ms_KnockoutNames[Effect]);*/

	switch (Effect)
	{
	case KNOCKOUT_EXPLOSION: KnockoutExplosion(Pos, pGameMap); break;
	case KNOCKOUT_HAMMERHIT: GameServer()->CreateHammerHit(pGameMap, Pos); break;
	case KNOCKOUT_LOVE: pGameMap->AnimationHandler()->DoAnimation(Pos, CAnimationHandler::ANIMATION_LOVE); break;
	case KNOCKOUT_THUNDERSTORM: pGameMap->AnimationHandler()->DoAnimation(Pos, CAnimationHandler::ANIMATION_THUNDERSTORM); break;
	default: dbg_msg("cosmetics", "ERROR: Knockouteffect '%s' not implemented!", ms_KnockoutNames[Effect]);
	}
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

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return false;

	DoGundesignRaw(Pos, Effect, pGameMap);
	return true;
}

bool CCosmeticsHandler::DoGundesign(vec2 Pos, const char *pName, CGameMap *pGameMap)
{
	int Effect = -1;
	for (int i = 0; i < NUM_GUNDESIGNS; i++)
	{
		if (str_comp_nocase(ms_GundesignNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}
	if (Effect == -1)
		return false;

	DoGundesignRaw(Pos, Effect, pGameMap);
	return true;
}

void CCosmeticsHandler::DoGundesignRaw(vec2 Pos, int Effect, CGameMap *pGameMap)
{
	/*if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Gundesigneffect %s", ms_GundesignNames[Effect]);*/

	if (Effect == GUNDESIGN_PEW)
		pGameMap->AnimationHandler()->Laserwrite("PEW", Pos - vec2((5.0f * 15.0f + 5.0f) *0.5f, 5.0f * 3.5f), 5.0f, Server()->TickSpeed() * 0.2f, pGameMap);
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

	return SnapGundesignRaw(Pos, Effect, EntityID);
}

bool CCosmeticsHandler::SnapGundesign(vec2 Pos, const char *pName, int EntityID)
{
	int Effect = -1;
	for (int i = 0; i < NUM_GUNDESIGNS; i++)
	{
		if (str_comp_nocase(ms_GundesignNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}
	if (Effect == -1)
		return false;

	return SnapGundesignRaw(Pos, Effect, EntityID);
}

bool CCosmeticsHandler::SnapGundesignRaw(vec2 Pos, int Effect, int EntityID)
{
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

	SnapSkinmaniRaw(Tick, pClientInfo, Effect);
}

void CCosmeticsHandler::SnapSkinmani(int64 Tick, CNetObj_ClientInfo *pClientInfo, const char *pName)
{
	int Effect = -1;
	for (int i = 0; i < NUM_SKINMANIS; i++)
	{
		if (str_comp_nocase(ms_SkinmaniNames[i], pName) == 0)
		{
			Effect = i;
			break;
		}
	}

	if (Effect == -1)
		return;

	SnapSkinmaniRaw(Tick, pClientInfo, Effect);
}

void CCosmeticsHandler::SnapSkinmaniRaw(int64 Tick, CNetObj_ClientInfo *pClientInfo, int Effect)
{
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