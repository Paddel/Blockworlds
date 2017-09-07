#pragma once

#include <base/vmath.h>
#include <base/tl/array.h>
#include <engine/server.h>
#include <game/server/gamemap.h>
#include <game/server/component.h>

class CCosmeticsHandler : public CComponentGlobal
{
public:
	enum
	{//Maximum sizeof(m_aKnockouts)/sizeof(char) = 256
		KNOCKOUT_EXPLOSION = 0,
		KNOCKOUT_HAMMERHIT,
		KNOCKOUT_KOSTARS,
		KNOCKOUT_STARRING,
		KNOCKOUT_STAREXPLOSION,
		KNOCKOUT_LOVE,
		KNOCKOUT_THUNDERSTORM,
		NUM_KNOCKOUTS,

		GUNDESIGN_CLOCKWISE = 0,
		GUNDESIGN_COUNTERCLOCK,
		GUNDESIGN_TWOCLOCK,
		GUNDESIGN_BLINKING,
		GUNDESIGN_REVERSE,
		GUNDESIGN_INVISBULLET,
		GUNDESIGN_HEART,
		GUNDESIGN_PEW,
		GUNDESIGN_1337,
		NUM_GUNDESIGNS,

		SKINMANI_FEET_FIRE = 0,
		SKINMANI_FEET_WATER,
		SKINMANI_FEET_POISON,
		SKINMANI_FEET_BLACKWHITE,
		SKINMANI_NIGHTBLUE,
		SKINMANI_VIP_RAINBOW,
		SKINMANI_VIP_RAINBOW_EPI,
		NUM_SKINMANIS,
	};


	static const char *ms_KnockoutNames[NUM_KNOCKOUTS];
	static const char *ms_GundesignNames[NUM_GUNDESIGNS];
	static const char *ms_SkinmaniNames[NUM_SKINMANIS];

public:

	int FindKnockoutEffect(const char *pName);
	bool HasKnockoutEffect(int ClientID, int Index);
	bool DoKnockoutEffect(int ClientID, vec2 Pos);
	void DoKnockoutEffectRaw(vec2 Pos, int Effect, CGameMap *pGameMap);
	bool ToggleKnockout(int ClientID, const char *pName);
	void FillKnockout(IServer::CAccountData *pFillingData, const char *pValue);

	int FindGundesign(const char *pName);
	bool HasGundesign(int ClientID, int Index);
	bool DoGundesign(int ClientID, vec2 Pos, vec2 Direction);
	bool DoGundesignRaw(vec2 Pos, int Effect, CGameMap *pGameMap, vec2 Direction);
	bool ToggleGundesign(int ClientID, const char *pName);

	bool SnapGundesign(int ClientID, vec2 Pos, int EntityID);
	bool SnapGundesignRaw(vec2 Pos, int Effect, int EntityID);
	void FillGundesign(IServer::CAccountData *pFillingData, const char *pValue);

	int FindSkinmani(const char *pName);
	bool HasSkinmani(int ClientID, int Index);
	bool ToggleSkinmani(int ClientID, const char *pName);
	void SnapSkinmani(int ClientID, int64 Tick, CNetObj_ClientInfo *pClientInfo);
	void SnapSkinmaniRaw(int64 Tick, CNetObj_ClientInfo *pClientInfo, int Effect);
	void FillSkinmani(IServer::CAccountData *pFillingData, const char *pValue);
};