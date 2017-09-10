
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>
#include <game/generated/protocol.h>

#include "cosmetics.h"

const char *CCosmeticsHandler::ms_KnockoutNames[NUM_KNOCKOUTS] = {
	"Explosion",
	"Hammerhit",
	"KO Stars",
	"Star Ring",
	"Starexplosion",
	"Thunderstorm",
	"Love",
	"KO RIP",
	"KO EZ",
	"KO NOOB",

	"Sorry c: (Teemo)",
};

const char *CCosmeticsHandler::ms_GundesignNames[NUM_GUNDESIGNS] = {
	"Clockwise",
	"Counterclock",
	"TwoOClock",
	"Blinking Bullet",
	"Reverse",
	"StarX",
	"Invis Bullet",
	"Armorgun",
	"Heartgun",
	"Pew",
	"1337 gun",
};

const char *CCosmeticsHandler::ms_SkinmaniNames[NUM_SKINMANIS] = {
	"Feet Fire",
	"Feet Water",
	"Feet Poison",
	"Feet Blackwhite",
	"Feet RGB",
	"Feet CMY",
	"Body Fire",
	"Body Water",
	"Body Poison",
	"Nightblue",
	"Rainbow (VIP)",
	"Epi Rainbow (VIP)",
};

inline int HslToCc(vec3 HSL)
{
	return ((int)(HSL.h * 255) << 16) + ((int)(HSL.s * 255) << 8) + (HSL.l - 0.5f) * 255 * 2;
}

inline vec3 CcToHsl(int Cc)
{
	return vec3(((Cc >> 16) & 0xff) / 255.0f, ((Cc >> 8) & 0xff) / 255.0f, 0.5f + (Cc & 0xff) / 255.0f*0.5f);
}

int CCosmeticsHandler::FindKnockoutEffect(const char *pName)
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

	return Effect;
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

void CCosmeticsHandler::DoKnockoutEffectRaw(vec2 Pos, int Effect, CGameMap *pGameMap)
{
	/*if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Knockouteffect %s", ms_KnockoutNames[Effect]);*/

	if (Effect == KNOCKOUT_EXPLOSION)
	{
		GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

		CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}
	}
	else if (Effect == KNOCKOUT_HAMMERHIT)
		GameServer()->CreateHammerHit(pGameMap, Pos);
	else if (Effect == KNOCKOUT_KOSTARS)
	{
		for (float i = 0.1f; i < 2 * pi; i += pi / 4.0f)
			GameServer()->CreateDamageInd(pGameMap, Pos, i, 1);
	}
	else if (Effect == KNOCKOUT_STARRING)
	{
		for (float i = 0.0f; i < 2 * pi; i += pi / 20.0f)
			GameServer()->CreateDamageInd(pGameMap, Pos, i, 1);
	}
	else if (Effect == KNOCKOUT_STAREXPLOSION)
	{
		GameServer()->CreateSound(pGameMap, Pos, SOUND_GRENADE_EXPLODE);

		CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)pGameMap->Events()->Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
		}

		for (float i = 0.1f; i < 2 * pi; i += pi / 4.0f)
			GameServer()->CreateDamageInd(pGameMap, Pos, i, 1);
	}
	else if (Effect == KNOCKOUT_LOVE)
		pGameMap->AnimationHandler()->DoAnimation(Pos, CAnimationHandler::ANIMATION_LOVE);
	else if (Effect == KNOCKOUT_THUNDERSTORM)
		pGameMap->AnimationHandler()->DoAnimation(Pos, CAnimationHandler::ANIMATION_THUNDERSTORM);
	else if (Effect == KNOCKOUT_KORIP)
		pGameMap->AnimationHandler()->Laserwrite("RIP", Pos + vec2(0, 30.0f), 10.0f, Server()->TickSpeed() * 0.7f);
	else if (Effect == KNOCKOUT_KOEZ)
		pGameMap->AnimationHandler()->Laserwrite("EZ", Pos + vec2(0, 30.0f), 10.0f, Server()->TickSpeed() * 0.7f);
	else if (Effect == KNOCKOUT_KONOOB)
		pGameMap->AnimationHandler()->Laserwrite("NOOB", Pos + vec2(0, 30.0f), 10.0f, Server()->TickSpeed() * 0.7f);
	else if (Effect == KNOCKOUT_SORRY)
		pGameMap->AnimationHandler()->Laserwrite("SORRY c:", Pos + vec2(0, 30.0f), 10.0f, Server()->TickSpeed() * 0.7f);
	else
		dbg_msg("cosmetics", "ERROR: Knockouteffect '%s' not implemented!", ms_KnockoutNames[Effect]);
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

int CCosmeticsHandler::FindGundesign(const char *pName)
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

	return Effect;
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

bool CCosmeticsHandler::DoGundesign(int ClientID, vec2 Pos, vec2 Direction)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	int Effect = Server()->GetClientInfo(ClientID)->m_CurrentGundesign;
	if (HasGundesign(ClientID, Effect) == false)
		return false;

	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	if (pGameMap == 0x0)
		return false;

	return DoGundesignRaw(Pos, Effect, pGameMap, Direction);
}

bool CCosmeticsHandler::DoGundesignRaw(vec2 Pos, int Effect, CGameMap *pGameMap, vec2 Direction)
{
	/*if (g_Config.m_Debug)
		dbg_msg("cosmetics", "Gundesigneffect %s", ms_GundesignNames[Effect]);*/

	if (Effect == GUNDESIGN_PEW)
	{
		pGameMap->AnimationHandler()->Laserwrite("PEW", Pos, 5.0f, Server()->TickSpeed() * 0.2f);
	}
	else if (Effect == GUNDESIGN_REVERSE)
	{
		float AngleFrom = GetAngle(Direction) +  5.1f;
		for (int i = 0; i < 10; i++)
		{
			float AngleTo = AngleFrom + ((i - 5) / 5.0f) * pi * 0.3f;
			GameServer()->CreateDamageInd(pGameMap, Pos + GetDir(AngleTo - 5.1f) * 85.0f, AngleTo + pi, 1);
		}
	}
	else if (Effect == GUNDESIGN_STARX)
	{
		GameServer()->CreateDamageInd(pGameMap, Pos + vec2(1, 1) * 28.0f, 2.7f, 1);
		GameServer()->CreateDamageInd(pGameMap, Pos + vec2(-1, 1) * 28.0f, 2.7f + pi * 0.5f, 1);
	}
	else if (Effect == GUNDESIGN_CLOCKWISE)
	{
		pGameMap->AnimationHandler()->DoAnimationGundesign(Pos, CAnimationHandler::ANIMATION_STARS_CW, Direction);
	}
	else if (Effect == GUNDESIGN_COUNTERCLOCK)
	{
		pGameMap->AnimationHandler()->DoAnimationGundesign(Pos, CAnimationHandler::ANIMATION_STARS_CCW, Direction);
	}
	else if (Effect == GUNDESIGN_TWOCLOCK)
	{
		pGameMap->AnimationHandler()->DoAnimationGundesign(Pos, CAnimationHandler::ANIMATION_STARS_TOC, Direction);
	}
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
	else if (Effect == GUNDESIGN_ARMOR)
	{
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, EntityID, sizeof(CNetObj_Pickup)));
		if (pP != 0x0)
		{
			pP->m_X = (int)Pos.x;
			pP->m_Y = (int)Pos.y;
			pP->m_Type = POWERUP_ARMOR;
			pP->m_Subtype = 0;
		}
		return true;
	}
	else if (Effect == GUNDESIGN_1337)
	{
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, EntityID, sizeof(CNetObj_Pickup)));
		if (pP != 0x0)
		{
			pP->m_X = (int)Pos.x;
			pP->m_Y = (int)Pos.y;
			pP->m_Type = 16;//53;
			pP->m_Subtype = 0;
		}
		return true;
	}
	else if (Effect == GUNDESIGN_BLINKING)
	{
		if ((Server()->Tick() / (Server()->TickSpeed() / 25)) % 2)
			return true;
	}
	else if (Effect == GUNDESIGN_INVISBULLET)
		return true;

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

int CCosmeticsHandler::FindSkinmani(const char *pName)
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
	return Effect;
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

	if (Server()->GetClientInfo(ClientID)->m_AccountData.m_Vip == true &&
		(Index == SKINMANI_VIP_RAINBOW || Index == SKINMANI_VIP_RAINBOW_EPI))
		return true;

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

void CCosmeticsHandler::SnapSkinmaniRaw(int64 Tick, CNetObj_ClientInfo *pClientInfo, int Effect)
{
	int64 TickDef = Server()->Tick() - Tick;//only work with Tickdef
	vec3 HSLBody = CcToHsl(pClientInfo->m_ColorBody);
	vec3 HSLFeet = CcToHsl(pClientInfo->m_ColorFeet);

	if (Effect == SKINMANI_FEET_FIRE)
	{
		HSLFeet.h = 0.0f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_FEET_WATER)
	{
		HSLFeet.h = 0.6f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_FEET_POISON)
	{
		HSLFeet.h = 0.3f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_FEET_BLACKWHITE)
	{
		HSLFeet.h = 0.0f;
		HSLFeet.s = 0.0f;
		HSLFeet.l = (sinf(TickDef / 255.0f) + 3.0f) / 4.0f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_FEET_RGB)
	{
		HSLFeet.h = 0.3f * ((int)(Server()->Tick() / (Server()->TickSpeed() * 2)) % 3);
		HSLFeet.s = 1.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_FEET_CMY)
	{
		HSLFeet.h = 0.15f + 0.3f * ((int)(Server()->Tick() / (Server()->TickSpeed() * 2)) % 3);
		HSLFeet.s = 1.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorFeet = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	if (Effect == SKINMANI_BODY_FIRE)
	{
		HSLFeet.h = 0.0f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_BODY_WATER)
	{
		HSLFeet.h = 0.6f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_BODY_POISON)
	{
		HSLFeet.h = 0.3f;
		HSLFeet.s = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLFeet.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLFeet);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_NIGHTBLUE)
	{
		HSLBody.h = 0.71f;
		HSLBody.s = 0.7f * clamp((sinf(TickDef / 50.0f) + 1.2f), 0.2f, 0.7f);
		HSLBody.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_VIP_RAINBOW)
	{
		HSLBody.h = (sinf(TickDef / 255.0f) + 1.0f) / 2.0f;
		HSLBody.s = 0.5f;
		HSLBody.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
		pClientInfo->m_ColorFeet = HslToCc(HSLBody);
		pClientInfo->m_UseCustomColor = 1;
	}
	else if (Effect == SKINMANI_VIP_RAINBOW_EPI)
	{
		HSLBody.h = (sinf(TickDef / 2.0f) + 1.0f) / 2.0f;
		HSLBody.s = 1.0f;
		HSLBody.l = 0.5f;
		pClientInfo->m_ColorBody = HslToCc(HSLBody);
		pClientInfo->m_ColorFeet = HslToCc(HSLBody);
		pClientInfo->m_UseCustomColor = 1;
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