#pragma once

#include <base/vmath.h>
#include <base/tl/array.h>
#include <game/server/gamemap.h>
#include <game/server/component.h>

class CCosmeticsHandler : public CComponent
{
public:
	enum
	{
		KNOCKOUT_EXPLOSION=0,
		KNOCKOUT_HAMMERHIT,
		KNOCKOUT_LOVE,
		KNOCKOUT_THUNDERSTORM,
		LETTER_TEST,
		NUM_KNOCKOUTS,//Maximum sizeof(m_aKnockouts)/sizeof(char) = 256

		GUNDESIGN_HEART=0,
		GUNDESIGN_PEW,
		NUM_GUNDESIGNS,

		SKINMANI_RAINBOW=0,
		SKINMANI_RAINBOW_EPI,
		SKINMANI_COOLDOWN,
		SKINMANI_NIGHTBLUE,
		NUM_SKINMANIS,
	};


	static const char *ms_KnockoutNames[NUM_KNOCKOUTS];
	static const char *ms_GundesignNames[NUM_GUNDESIGNS];
	static const char *ms_SkinmaniNames[NUM_SKINMANIS];

	class CCosAnim
	{
	private:
		vec2 m_Pos;
		int64 m_Tick;
		CGameMap *m_pGameMap;

	public:
		CCosAnim(vec2 Pos, int64 Tick, CGameMap *pGameMap)
			: m_Pos(Pos), m_Tick(Tick), m_pGameMap(pGameMap) {}
		virtual ~CCosAnim() {}

		virtual void Tick() = 0;
		virtual void Snap(int SnappingClient) = 0;
		virtual bool Done() = 0;
		vec2 GetPos() const { return m_Pos; }
		int64 GetTick() const { return m_Tick; }
		CGameMap *GetGameMap() const { return m_pGameMap; }
		IServer *Server() { return m_pGameMap->Server(); }
		CGameContext *GameServer() { return m_pGameMap->GameServer(); }
	};

private:
	array<CCosAnim *> m_lpAnimations;

	void KnockoutExplosion(int ClientID, vec2 Pos, CGameMap *pGameMap);
	void KnockoutHammerhit(int ClientID, vec2 Pos, CGameMap *pGameMap);

public:

	virtual void Init();
	virtual void Tick();
	void Snap(int SnappingClient);

	void Laserwrite(const char *pText, vec2 StartPos, float Size, int Ticks, CGameMap *pGameMap);

	bool HasKnockoutEffect(int ClientID, int Index);
	bool DoKnockoutEffect(int ClientID, vec2 Pos);
	bool DoKnockoutEffect(int ClientID, vec2 Pos, const char *pName);
	bool DoKnockoutEffect(int ClientID, vec2 Pos, int Effect);
	bool ToggleKnockout(int ClientID, const char *pName);
	void FillKnockout(IServer::CAccountData *pFillingData, const char *pValue);


	bool HasGundesign(int ClientID, int Index);
	bool DoGundesign(int ClientID, vec2 Pos);
	bool DoGundesign(int ClientID, vec2 Pos, const char *pName);
	bool DoGundesign(int ClientID, vec2 Pos, int Effect);
	bool ToggleGundesign(int ClientID, const char *pName);
	bool SnapGundesign(int ClientID, vec2 Pos, int EntityID);
	void FillGundesign(IServer::CAccountData *pFillingData, const char *pValue);

	bool HasSkinmani(int ClientID, int Index);
	bool ToggleSkinmani(int ClientID, const char *pName);
	void SnapSkinmani(int ClientID, int64 Tick, CNetObj_ClientInfo *pClientInfo);
	void FillSkinmani(IServer::CAccountData *pFillingData, const char *pValue);
};