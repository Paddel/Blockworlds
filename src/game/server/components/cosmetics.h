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
		NUM_KNOCKOUTS,//Maximum sizeof(m_aKnockouts)/sizeof(char) = 256
	};


	static const char *ms_KnockoutNames[NUM_KNOCKOUTS];

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

	void KnockoutExplosion(int ClientID, int Victim, vec2 Pos, CGameMap *pGameMap);
	void KnockoutHammerhit(int ClientID, int Victim, vec2 Pos, CGameMap *pGameMap);

public:

	virtual void Tick();
	void Snap(int SnappingClient);

	bool HasKnockoutEffect(int ClientID, int Index);
	void DoKnockoutEffect(int ClientID, int Victim, vec2 Pos);
	void DoKnockoutEffect(int ClientID, int Victim, vec2 Pos, const char *pName);
	void DoKnockoutEffect(int ClientID, int Victim, vec2 Pos, int Effect);
	bool ToggleKnockout(int ClientID, const char *pName);

	void FillKnockout(IServer::CAccountData *pFillingData, const char *pValue);
};