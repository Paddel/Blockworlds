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
	{
		KNOCKOUT_EXPLOSION=0,
		KNOCKOUT_HAMMERHIT,
		KNOCKOUT_LOVE,
		KNOCKOUT_THUNDERSTORM,
		NUM_KNOCKOUTS,//Maximum sizeof(m_aKnockouts)/sizeof(char) = 256

		GUNDESIGN_HEART=0,
		GUNDESIGN_PEW,
		GUNDESIGN_1337,
		NUM_GUNDESIGNS,

		SKINMANI_RAINBOW=0,
		SKINMANI_RAINBOW_EPI,
		SKINMANI_COOLDOWN,
		SKINMANI_NIGHTBLUE,
		SKINMANI_1337,
		NUM_SKINMANIS,
	};


	static const char *ms_KnockoutNames[NUM_KNOCKOUTS];
	static const char *ms_GundesignNames[NUM_GUNDESIGNS];
	static const char *ms_SkinmaniNames[NUM_SKINMANIS];

private:

	void KnockoutExplosion(vec2 Pos, CGameMap *pGameMap);

public:

	int FindKnockoutEffect(const char *pName);
	bool HasKnockoutEffect(int ClientID, int Index);
	bool DoKnockoutEffect(int ClientID, vec2 Pos);
	void DoKnockoutEffectRaw(vec2 Pos, int Effect, CGameMap *pGameMap);
	bool ToggleKnockout(int ClientID, const char *pName);
	void FillKnockout(IServer::CAccountData *pFillingData, const char *pValue);

	int FindGundesign(const char *pName);
	bool HasGundesign(int ClientID, int Index);
	bool DoGundesign(int ClientID, vec2 Pos);
	bool DoGundesignRaw(vec2 Pos, int Effect, CGameMap *pGameMap);
	bool ToggleGundesign(int ClientID, const char *pName);

	bool SnapGundesign(int ClientID, vec2 Pos, int EntityID);
	bool SnapGundesignRaw(vec2 Pos, int Effect, int EntityID);
	void FillGundesign(IServer::CAccountData *pFillingData, const char *pValue);

	int FindSkinmani(const char *pName);
	bool HasSkinmani(int ClientID, int Index);
	bool ToggleSkinmani(int ClientID, const char *pName);
	void SnapSkinmani(int ClientID, int64 Tick, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo);
	void SnapSkinmaniRaw(int64 Tick, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo, int Effect);
	void FillSkinmani(IServer::CAccountData *pFillingData, const char *pValue);
};