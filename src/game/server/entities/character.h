

#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/server/srv_gamecore.h>

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

	enum
	{
		EXTRAID_INVIOLABLE=0,
		NUM_EXTRAIDS,
	};
public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void Push(vec2 Force, int From);

	void SnapExtras(int SnappingClient);
	int GetExtraID(int Index);

	bool IsGrounded();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void ResetZones();
	bool HandleExtrasLayer(int Layer);
	void HandleExtras();
	void HandleStops();

	void HandleTiles();

	void HandleRace();

	void Die(int Killer, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool GiveWeapon(int Weapon);
	void GiveNinja(bool PlaySound);

	void SetEmote(int Emote, int Tick);

	virtual void Freeze(float Seconds);
	virtual void Unfreeze();
	bool IsFreezed();
	bool TakeWeapons();
	void RaceFinish();
	void SpeedUp(int Force, int MaxSpeed, int Angle);
	bool IsInviolable();
	void SetActiveWeapon(int Weapon);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }
	CSrvCharacterCore *Core() { return &m_Core; }
	int64 FreezeTick() const { return m_FreezeTick; }
	int FreezeTime() const { return m_FreezeTime; }
	bool InBlockZone() const { return m_ZoneBlock; }
	bool *Weapons() { return m_aGotWeapon; }
	int GetActiveWeapon() const { return m_ActiveWeapon; }

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	bool m_Alive;
	int m_FreezeTime;
	int64 m_FreezeTick;
	bool m_DeepFreeze;
	int64 m_RaceStart;
	int64 m_FreezeCryTick;
	int m_aExtraIDs[NUM_EXTRAIDS];

	//zones
	bool m_ZoneBlock;

	int64 m_ProtectionTime;

	vec2 m_LastPos;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	bool m_aGotWeapon[NUM_WEAPONS];

	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CSrvCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CSrvCharacterCore m_SendCore; // core that we should send
	CSrvCharacterCore m_ReckoningCore; // the dead reckoning core

};

#endif
