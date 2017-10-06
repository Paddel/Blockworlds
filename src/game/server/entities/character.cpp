

#include <new>
#include <engine/server/map.h>
#include <engine/shared/config.h>
#include <engine/shared/external_defines.h>
#include <game/mapitems.h>
#include <game/extras.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveWeapon = WEAPON_HAMMER;
	m_LastWeapon = WEAPON_GUN;
	m_QueuedWeapon = -1;
	m_ProtectionTime = 1;
	m_EndlessHook = false;
	m_SuperHammer = false;

	for (int i = 0; i < NUM_EXTRAIDS; i++)
		m_aExtraIDs[i] = -1;

	m_FreezeTime = 0;
	m_DeepFreeze = false;

	m_RaceStart = 0;

	m_ZoneSpawn = false;
	m_ZoneUntouchable = false;

	m_UnfreezeInput = false;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameWorld()->m_Core, GameMap()->Collision());
	m_Core.m_Pos = m_Pos;
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameWorld()->InsertEntity(this);
	m_Alive = true;

	mem_zero(&m_aGotWeapon, sizeof(m_aGotWeapon));
	GiveWeapon(WEAPON_HAMMER);
	if(GameMap()->IsShopMap() == false)
		GiveWeapon(WEAPON_GUN);

	m_SpawnTime = Server()->Tick();

	return true;
}

void CCharacter::Destroy()
{
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if(GameMap()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameMap()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

void CCharacter::Push(vec2 Force, int From)
{
	if (GameMap()->IsBlockMap() == false || IsInviolable() == true)
		return;

	if (From >= 0 && From < MAX_CLIENTS)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(From);
		if (pChr && pChr->IsInviolable() == true)
			return;
	}

	m_Core.m_Vel += Force;
	if(length(Force) > 10.0f && From != -1)
		GameServer()->BlockSystemAttack(From, GetPlayer()->GetCID(), false);
}

void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		m_aGotWeapon[WEAPON_NINJA] = false;
		m_ActiveWeapon = m_LastWeapon;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	int NinjaTime = m_Ninja.m_ActivationTick + (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000) - Server()->Tick();
	if (NinjaTime % Server()->TickSpeed() == 0 && NinjaTime / Server()->TickSpeed() <= 5)
		GameServer()->CreateDamageInd(GameWorld(), m_Pos, 0, NinjaTime / Server()->TickSpeed());

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameMap()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameWorld()->FindTees(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(GameWorld(), aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->Push(vec2(0, -10.0f), GetPlayer()->GetCID());
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aGotWeapon[WEAPON_NINJA])
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aGotWeapon[WantedWeapon])
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aGotWeapon[WantedWeapon])
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aGotWeapon[WantedWeapon])
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;

	if (GetPlayer()->GetPause())
		return;

	if (IsFreezed() && m_UnfreezeInput == false)
	{
		if (CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses && m_FreezeCryTick < Server()->Tick())
		{
			m_FreezeCryTick = Server()->Tick() + Server()->TickSpeed();
			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_PLAYER_PAIN_LONG);
		}

		return;
	}

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1))
		WillFire = true;

	if(!WillFire)
		return;

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius * 0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_HAMMER_FIRE);

			CEntity *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameWorld()->FindTees(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts, MAX_CLIENTS);

			for (int i = 0; i < Num; ++i)
			{
				CEntity *pTarget = apEnts[i];

				if (pTarget == this)
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(GameWorld(), pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(GameWorld(), ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				float Force = m_SuperHammer ? 15.0f : 10.0f;
				pTarget->Push(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * Force, GetPlayer()->GetCID());
				pTarget->Unfreeze();
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			new CProjectile(GameWorld(), WEAPON_GUN,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
				1, 0, 0, -1, WEAPON_GUN);

			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_GUN_FIRE);
		} break;

		case WEAPON_SHOTGUN:
		{
			new CLaser(GameWorld(), m_Pos, Direction, 800.0f, m_pPlayer->GetCID(), WEAPON_SHOTGUN);

			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			new CProjectile(GameWorld(), WEAPON_GRENADE,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), WEAPON_RIFLE);
			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	m_AttackTick = Server()->Tick();

	if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	if (m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS || m_aGotWeapon[m_ActiveWeapon] == false)
		m_ActiveWeapon = WEAPON_HAMMER;

	// fire Weapon, if wanted
	FireWeapon();
	return;
}

bool CCharacter::GiveWeapon(int Weapon)
{
	if(!m_aGotWeapon[Weapon])
	{
		m_aGotWeapon[Weapon] = true;
		return true;
	}
	return false;
}

void CCharacter::GiveNinja(bool PlaySound)
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aGotWeapon[WEAPON_NINJA] = true;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	if(PlaySound)
		GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::Unfreeze()
{
	if (m_DeepFreeze == true || m_FreezeTime <= 0)
		return;

	if(m_ZoneFreeze == false)
		m_FreezeTime = 0;

	m_UnfreezeInput = true;

	if (m_ZoneProtection == false && m_ProtectionTime > 1)
		m_ProtectionTime = 0;
}

void CCharacter::Freeze(float Seconds)
{
	if (m_FreezeTime <= 0)
		m_FreezeTick = Server()->Tick();

	if (m_FreezeTime <= Server()->TickSpeed() * (Seconds - 1.0f) || Seconds < 1.0f)
		m_FreezeTime = Server()->TickSpeed() * Seconds;
}

bool CCharacter::IsFreezed()
{
	if (m_FreezeTime > 0)
		return true;
	if (m_DeepFreeze == true)
		return true;
	return false;
}

bool CCharacter::TakeWeapons()
{
	bool Found = false;
	for (int i = 0; i < NUM_WEAPONS; i++)
	{
		if (i == WEAPON_GUN || i == WEAPON_HAMMER)
			continue;

		if (m_aGotWeapon[i] == true)
		{
			m_aGotWeapon[i] = false;
			Found = true;
		}
	}

	if (Found)
		m_ActiveWeapon = WEAPON_HAMMER;

	return Found;
}

void CCharacter::RaceFinish()
{
	if (m_RaceStart == 0)
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s finished in: ", Server()->ClientName(m_pPlayer->GetCID()));
	GameServer()->StringTime(m_RaceStart, aBuf, sizeof(aBuf));

	//TODO: Save time
	GameMap()->SendChat(-1, aBuf);
	m_RaceStart = 0;
}

void CCharacter::SpeedUp(int Force, int MaxSpeed, int Angle)
{
	//Copied from DDNet Source: https://ddnet.tw/
	float AngleRad = Angle * (pi / 180.0f);
	vec2 Direction = vec2(cosf(AngleRad), sinf(AngleRad)), MaxVel, TempVel = m_Core.m_Vel;
	float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;

	if (Force == 255 && MaxSpeed)
	{
		m_Core.m_Vel = Direction * (MaxSpeed / 5);
	}
	else
	{
		if (MaxSpeed > 0 && MaxSpeed < 5)
			MaxSpeed = 5;

		if (MaxSpeed > 0)
		{
			if (Direction.x > 0.0000001f)
				SpeederAngle = -atan(Direction.y / Direction.x);
			else if (Direction.x < 0.0000001f)
				SpeederAngle = atan(Direction.y / Direction.x) + 2.0f * asin(1.0f);
			else if (Direction.y > 0.0000001f)
				SpeederAngle = asin(1.0f);
			else
				SpeederAngle = asin(-1.0f);

			if (SpeederAngle < 0)
				SpeederAngle = 4.0f * asin(1.0f) + SpeederAngle;

			if (TempVel.x > 0.0000001f)
				TeeAngle = -atan(TempVel.y / TempVel.x);
			else if (TempVel.x < 0.0000001f)
				TeeAngle = atan(TempVel.y / TempVel.x) + 2.0f * asin(1.0f);
			else if (TempVel.y > 0.0000001f)
				TeeAngle = asin(1.0f);
			else
				TeeAngle = asin(-1.0f);

			if (TeeAngle < 0)
				TeeAngle = 4.0f * asin(1.0f) + TeeAngle;

			TeeSpeed = sqrt(pow(TempVel.x, 2) + pow(TempVel.y, 2));

			DiffAngle = SpeederAngle - TeeAngle;
			SpeedLeft = MaxSpeed / 5.0f - cos(DiffAngle) * TeeSpeed;
			if (abs((int)SpeedLeft) > Force && SpeedLeft > 0.0000001f)
				TempVel += Direction * Force;
			else if (abs((int)SpeedLeft) > Force)
				TempVel += Direction * -Force;
			else
				TempVel += Direction * SpeedLeft;
		}
		else
			TempVel += Direction * Force;

		m_Core.m_Vel = TempVel;
	}
}

bool CCharacter::IsInviolable()
{
	if (m_ProtectionTime > 1 && m_ProtectionTime > Server()->Tick() &&
		Server()->GetClientInfo(GetPlayer()->GetCID())->m_UseInviolable == true && Server()->GetClientInfo(GetPlayer()->GetCID())->m_InviolableTime > Server()->Tick())
		return true;
	if (m_ZoneUntouchable)
		return true;
	return false;
}

void CCharacter::SetActiveWeapon(int Weapon)
{
	if (m_aGotWeapon[Weapon] == true)
		m_ActiveWeapon = Weapon;
}

void CCharacter::SetExtraCollision()
{
	int Level = 0;
	if(Server()->GetClientInfo(GetPlayer()->GetCID())->m_LoggedIn == true)
		Level = Server()->GetClientInfo(GetPlayer()->GetCID())->m_AccountData.m_Level;
	if(Level < 1)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_1);
	if (Level < 50)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_50);
	if (Level < 100)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_100);
	if (Level < 200)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_200);
	if (Level < 300)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_300);
	if (Level < 400)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_400);
	if (Level < 500)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_500);
	if (Level < 600)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_600);
	if (Level < 700)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_700);
	if (Level < 800)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_800);
	if (Level < 900)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_900);
	if (Level < 999)
		GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_LEVEL_999);
}

void CCharacter::ReleaseExtraCollision()
{
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_1);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_50);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_100);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_200);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_300);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_400);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_500);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_600);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_700);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_800);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_900);
	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_LEVEL_999);
}

int CCharacter::GetCurrentEmote()
{
	int Emote = -1;

	if (IsFreezed())
	{
		if (m_DeepFreeze == true)
			Emote = EMOTE_PAIN;
		else
			Emote = EMOTE_BLINK;
	}

	if (Server()->GetClientInfo(GetPlayer()->GetCID())->m_EmoteType != EMOTE_NORMAL && Server()->GetClientInfo(GetPlayer()->GetCID())->m_EmoteStop > Server()->Tick())
		Emote = Server()->GetClientInfo(GetPlayer()->GetCID())->m_EmoteType;

	if (m_EmoteType != EMOTE_NORMAL)
		Emote = m_EmoteType;

	if (GetPlayer()->GetPause() == true)
		Emote = EMOTE_BLINK;

	if (GetPlayer()->InEvent() == true)
		Emote = EMOTE_NORMAL;

	if (Emote == EMOTE_NORMAL)
	{
		if (250 - ((Server()->Tick() - m_LastAction) % (250)) < 5)
			Emote = EMOTE_BLINK;
	}

	return Emote;
}

vec2 CCharacter::CursorPos()
{
	return m_Pos + vec2(clamp(m_Input.m_TargetX, -900, 900), clamp(m_Input.m_TargetY, -900, 900));
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::ResetZones()
{
	m_ZoneTournament = false;
	m_ZoneSpawn = false;
	m_ZoneUntouchable = false;
	m_ZoneProtection = false;
	m_ZoneFreeze = false;
}

bool CCharacter::HandleExtrasLayer(int Layer)
{
	CLayers *pLayers = GameMap()->Layers();
	int Index = pLayers->ExtrasIndex(Layer, m_Pos.x, m_Pos.y);
	int LastIndex = pLayers->ExtrasIndex(Layer, m_LastPos.x, m_LastPos.y);
	int Tile = pLayers->GetExtrasTile(Layer)[Index].m_Index;
	int LastTile = pLayers->GetExtrasTile(Layer)[LastIndex].m_Index;
	int NewTile = Tile != LastTile ? Tile : 0;
	CExtrasData ExtrasData = pLayers->GetExtrasData(Layer)[Index];

	if (/*Tile <= 0 || */Tile >= NUM_EXTRAS)
		return false;

	if (NewTile == EXTRAS_TELEPORT_FROM)
	{
		vec2 Pos;
		int ID = str_toint(ExtrasData.m_aData);
		if (GameMap()->RaceComponents()->GetRandomTelePos(ID, &Pos) == true)
		{
			m_Core.m_Pos = Pos;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_HookedPlayer = -1;
		}
	}

	if (Tile == EXTRAS_DOOR_HANDLE)
	{
		const char *pData = ExtrasData.m_aData;
		int ID = str_toint(pData);
		pData += gs_ExtrasSizes[Tile][0];
		int Delay = str_toint(pData);
		pData += gs_ExtrasSizes[Tile][1];
		bool Activate = (bool)str_toint(pData);
		pData += gs_ExtrasSizes[Tile][2];
		GameMap()->RaceComponents()->OnDoorHandle(ID, Delay, Activate);
	}

	if (Tile == EXTRAS_DOOR)
	{
		CRaceComponents::CDoorTile *pDoorTile = GameMap()->RaceComponents()->GetDoorTile(Index);
		if (pDoorTile && pDoorTile->m_Freeze == true && GameMap()->RaceComponents()->DoorTileActive(pDoorTile))
			Freeze(3.0f);
	}

	if (Tile == EXTRAS_LASERGUN_TRIGGER)
	{
		const char *pData = ExtrasData.m_aData;
		int ID = str_toint(pData);
		pData += gs_ExtrasSizes[Tile][0];
		int Delay = str_toint(pData);
		pData += gs_ExtrasSizes[Tile][1];
		GameMap()->RaceComponents()->OnLasergunTrigger(ID, Delay);
	}

	if (Tile == EXTRAS_FREEZE)
	{
		Freeze(3.0f);
		m_ZoneFreeze = true;
	}

	if (Tile == EXTRAS_UNFREEZE)
		Unfreeze();

	if (Tile == EXTRAS_ZONE_TOURNAMENT)
		m_ZoneTournament = true;

	if (Tile == EXTRAS_ZONE_SPAWN)
		m_ZoneSpawn = true;

	if (Tile == EXTRAS_ZONE_UNTOUCHABLE)
		m_ZoneUntouchable = true;

	if (Tile == EXTRAS_ZONE_PROTECTION && Server()->GetClientInfo(GetPlayer()->GetCID())->m_UseInviolable == false)
		m_ProtectionTime = 0;//anti abuse

	if (m_ProtectionTime == 1 && Tile == EXTRAS_ZONE_PROTECTION && Server()->GetClientInfo(GetPlayer()->GetCID())->m_InviolableTime > Server()->Tick())
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Passive mode enabled");
	if(LastTile == EXTRAS_ZONE_PROTECTION && Tile != EXTRAS_ZONE_PROTECTION && m_ProtectionTime != 0 && Server()->GetClientInfo(GetPlayer()->GetCID())->m_InviolableTime > Server()->Tick())
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Passive mode will be disabled in 3 seconds");

	if (Tile == EXTRAS_ZONE_PROTECTION)
		m_ZoneProtection = true;

	if (Tile == EXTRAS_ZONE_PROTECTION && m_ProtectionTime != 0)//TODO: ugly
		m_ProtectionTime = Server()->Tick() + Server()->TickSpeed() * 3;
	else if (m_ProtectionTime > 1 && m_ProtectionTime < Server()->Tick())
		m_ProtectionTime = 0;

	if (NewTile == EXTRAS_MAP)
	{
		CMap *pMap = Server()->FindMap(ExtrasData.m_aData);
		if(pMap == 0x0)
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Map will be available soon");
		else if(pMap->HasFreePlayerSlot() == false)
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "No free playerslot on Map");
		else if (Server()->MovePlayer(GetPlayer()->GetCID(), pMap) == true)
			return true;
		else
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "An Error occured");
	}

	if (NewTile == EXTRAS_SELL_SKINMANI)
		GameServer()->OnBuySkinmani(GetPlayer()->GetCID(), ExtrasData.m_aData);
	if(NewTile == EXTRAS_SELL_GUNDESIGN)
		GameServer()->OnBuyGundesign(GetPlayer()->GetCID(), ExtrasData.m_aData);
	if(NewTile == EXTRAS_SELL_KNOCKOUT)
		GameServer()->OnBuyKnockout(GetPlayer()->GetCID(), ExtrasData.m_aData);
	if(NewTile == EXTRAS_SELL_EXTRAS)
		GameServer()->OnBuyExtra(GetPlayer()->GetCID(), ExtrasData.m_aData);

	if (NewTile == EXTRAS_INFO_LEVEL)
	{
		int Level = str_toint(ExtrasData.m_aData);
		if (Server()->GetClientInfo(GetPlayer()->GetCID())->m_LoggedIn == false)
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You must be logged in to enter this area");
		else if (Server()->GetClientInfo(GetPlayer()->GetCID())->m_AccountData.m_Level < Level)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "You must be level %d to enter this area", Level);
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), aBuf);
		}
	}

	return false;
}

void CCharacter::HandleExtras()
{
	for (int i = 0; i < GameMap()->Layers()->GetNumExtrasLayer(); i++)
		if (HandleExtrasLayer(i))
			break;
}

void CCharacter::HandleStops()
{
	for (int i = 0; i < GameMap()->Layers()->GetNumExtrasLayer(); i++)
	{
		CLayers *pLayers = GameMap()->Layers();
		int Index = -1;
		for (int j = 0; j < 5; j++)
		{
			vec2 Pos;
			switch (j)
			{
			case 0: Pos = m_Pos; break;
			case 1: Pos = vec2(m_Pos.x + m_ProximityRadius / 1.5f, m_Pos.y - m_ProximityRadius / 1.5f); break;
			case 2: Pos = vec2(m_Pos.x + m_ProximityRadius / 1.5f, m_Pos.y + m_ProximityRadius / 1.5f); break;
			case 3: Pos = vec2(m_Pos.x - m_ProximityRadius / 1.5f, m_Pos.y - m_ProximityRadius / 1.5f); break;
			case 4: Pos = vec2(m_Pos.x - m_ProximityRadius / 1.5f, m_Pos.y + m_ProximityRadius / 1.5f); break;
			}

			int TempIndex = pLayers->ExtrasIndex(i, Pos.x, Pos.y);
			if (pLayers->GetExtrasTile(i)[TempIndex].m_Index == EXTRAS_DOOR)
			{
				Index = TempIndex;
				break;
			}
		}

		if (Index < 0)
			continue;

		CRaceComponents::CDoorTile *pDoorTile = GameMap()->RaceComponents()->GetDoorTile(Index);
		if (pDoorTile && pDoorTile->m_Freeze == false && GameMap()->RaceComponents()->DoorTileActive(pDoorTile))
		{
			vec2 DoorPos = vec2(Index % pLayers->GetExtrasWidth(i) * 32.0f + 16.0f, Index / pLayers->GetExtrasWidth(i) * 32.0f + 16.0f);
			if (pDoorTile->m_Type == 1)
			{
				if((DoorPos.x > m_Pos.x && m_Core.m_Vel.x > 0.0f) ||
					(DoorPos.x < m_Pos.x && m_Core.m_Vel.x < 0.0f))
					m_Core.m_Vel.x = 0.0f;
			}
			else if (pDoorTile->m_Type == 2)
			{
				if(DoorPos.y > m_Pos.y && m_Core.m_Vel.y > 0.0f)
					m_Core.m_Jumped = 0;

				if ((DoorPos.y > m_Pos.y && m_Core.m_Vel.y > 0.0f) ||
					(DoorPos.y < m_Pos.y && m_Core.m_Vel.y < 0.0f))
					m_Core.m_Vel.y = 0.0f;
			}
		}
	}

	for (int i = 0; i < GameMap()->Layers()->GetNumExtrasLayer(); i++)
	{
		int Index = GameMap()->Layers()->ExtrasIndex(i, m_Pos.x, m_Pos.y);
		int Tile = GameMap()->Layers()->GetExtrasTile(i)[Index].m_Index;
		CExtrasData ExtrasData = GameMap()->Layers()->GetExtrasData(i)[Index];

		if (Tile == EXTRAS_SPEEDUP)
		{
			const char *pData = ExtrasData.m_aData;
			int Force = str_toint(pData);
			pData += +gs_ExtrasSizes[Tile][0];
			int MaxSpeed = str_toint(pData);
			pData += +gs_ExtrasSizes[Tile][1];
			int Angle = str_toint(pData);
			pData += +gs_ExtrasSizes[Tile][2];
			SpeedUp(Force, MaxSpeed, Angle);
		}
	}

	//tiles
	{
		//int TileM = GameMap()->Collision()->GetTileAt(m_Pos);
		int TileTR = GameMap()->Collision()->GetTileAt(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f));
		int TileTL = GameMap()->Collision()->GetTileAt(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f));
		int TileBR = GameMap()->Collision()->GetTileAt(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f));
		int TileBL = GameMap()->Collision()->GetTileAt(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f));

		if (TileTR == TILE_ONEWAY_RIGHT || TileTL == TILE_ONEWAY_RIGHT ||
			TileBR == TILE_ONEWAY_RIGHT || TileBL == TILE_ONEWAY_RIGHT)
			if (m_Core.m_Vel.x < 0.0f)
				m_Core.m_Vel.x = 0.0f;
				
		if (TileTR == TILE_ONEWAY_LEFT || TileTL == TILE_ONEWAY_LEFT ||
			TileBR == TILE_ONEWAY_LEFT || TileBL == TILE_ONEWAY_LEFT)
			if (m_Core.m_Vel.x > 0.0f)
				m_Core.m_Vel.x = 0.0f;

		if (TileTR == TILE_ONEWAY_UP || TileTL == TILE_ONEWAY_UP ||
			TileBR == TILE_ONEWAY_UP || TileBL == TILE_ONEWAY_UP)
			if (m_Core.m_Vel.y > 0.0f)
				m_Core.m_Vel.y = 0.0f;
		
		if (TileTR == TILE_ONEWAY_DOWN || TileTL == TILE_ONEWAY_DOWN ||
			TileBR == TILE_ONEWAY_DOWN || TileBL == TILE_ONEWAY_DOWN)
			if (m_Core.m_Vel.y < 0.0f)
				m_Core.m_Vel.y = 0.0f;
	}
}

void CCharacter::HandleTiles()
{
	int Tile = GameMap()->Collision()->GetTileAt(m_Pos);
	int LastTile = GameMap()->Collision()->GetTileAt(m_LastPos);
	int NewTile = Tile != LastTile ? Tile : TILE_AIR;

	if (Tile == TILE_FREEZE)
	{
		Freeze(3.0f);
		m_ZoneFreeze = true;
	}
	if (Tile == TILE_UNFREEZE)
		Unfreeze();
	if (Tile == TILE_FREEZE_DEEP)
		m_DeepFreeze = true;

	if (Tile == TILE_UNFREEZE_DEEP)
		m_DeepFreeze = false;
	if (Tile == TILE_RACE_START && m_RaceStart == 0)
		m_RaceStart = Server()->Tick();
	if (Tile == TILE_RACE_FINISH)
		RaceFinish();
	if (Tile == TILE_RESTART)
	{
		vec2 Pos;
		if (GameMap()->RaceComponents()->CanSpawn(&Pos, GameWorld()))
		{
			m_Core.m_Pos = Pos;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_HookedPlayer = -1;
			m_ProtectionTime = 1;
		}
	}

	if (Tile == TILE_VIP)
	{
		if (Server()->GetClientInfo(GetPlayer()->GetCID())->m_LoggedIn == false || Server()->GetClientInfo(GetPlayer()->GetCID())->m_AccountData.m_Vip == false)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Only VIP can access!");
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
			return;
		}
	}

	if (Tile == TILE_ENDLESSHOOK && m_EndlessHook == false)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Endless-Hook has been activated!");
		m_EndlessHook = true;
	}

	if (Tile == TILE_SUPERHAMMER && m_SuperHammer == false)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Superhammer has been activated!");
		m_SuperHammer = true;
	}

	if (Tile == TILE_GIVEPASSIVE && Server()->GetClientInfo(GetPlayer()->GetCID())->m_InviolableTime < Server()->Tick())
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Passive-Mode has been activated for two hours");
		Server()->GetClientInfo(GetPlayer()->GetCID())->m_InviolableTime = Server()->Tick() + Server()->TickSpeed() * 60 * 60 * 2;
	}

	if(NewTile == TILE_EXTRAJUMP)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "+1 Airjump");
		m_Core.m_MaxAirJumps++;
	}
}

void CCharacter::HandleRace()
{
	if (m_Alive == false)
		return;

	if (m_DeepFreeze)
		Freeze(3.0f);

	//Freeze
	if (m_FreezeTime > 0)
		m_FreezeTime--;

	if (GetPlayer()->GetPause() || GetPlayer()->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		m_Input.m_Jump = 0;
		m_Input.m_Direction = 0;
		m_Input.m_Hook = m_HookLock;
	}
	else
		m_HookLock = m_Input.m_Hook;

	//ignore input
	if (IsFreezed())
	{
		if (m_UnfreezeInput == false)//allow one input on unfreeze
		{
			m_Input.m_Jump = 0;
			m_Input.m_Direction = 0;
		}

		m_Input.m_Hook = 0;

		m_LastUnfreeze = Server()->Tick();
	}

	//counting stars
	if (IsFreezed() && m_FreezeTime % Server()->TickSpeed() == Server()->TickSpeed() - 1)
	{
		int NumStars = (m_FreezeTime + 1) / Server()->TickSpeed();
		if (m_DeepFreeze)
			NumStars = 3;

		GameServer()->CreateDamageInd(GameWorld(), m_Pos, 0, NumStars);
	}

	//gamecore inviolable
	m_Core.m_Inviolable = IsInviolable();
	m_Core.m_EndlessHook = m_EndlessHook;

	if(m_ZoneUntouchable && m_Core.m_HookState == HOOK_GRABBED && m_Core.m_HookTick >= Server()->TickSpeed() * 10.0f)
		m_Core.m_HookState = HOOK_RETRACTED;

	if ((GetPlayer()->InEvent() || GetPlayer()->InGameMatch()) && m_Core.m_HookState == HOOK_GRABBED
		&& m_Core.m_HookedPlayer == -1 && m_Core.m_HookTick >= Server()->TickSpeed() * 2.5f)
		Freeze(1.0f);
}

void CCharacter::Tick()
{
	ResetZones();
	HandleTiles();
	HandleExtras();
	HandleRace();

	if (m_Alive == false)
		return;

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	// handle death-tiles and leaving gamelayer
	if(GameMap()->Collision()->GetTileAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH ||
		GameMap()->Collision()->GetTileAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == TILE_DEATH ||
		GameMap()->Collision()->GetTileAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH ||
		GameMap()->Collision()->GetTileAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == TILE_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
		return;
	}

	// handle Weapons
	HandleWeapons();

	if (m_UnfreezeInput == true)
		m_UnfreezeInput = false;

	// Previnput
	m_PrevInput = m_Input;
	return;
}

void CCharacter::TickDefered()
{
	if (m_Alive == false)
		return;

	// advance the dummy
	{
		CSrvWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameMap()->Collision());
		m_ReckoningCore.TickPredict(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	HandleStops();

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameMap()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	SetExtraCollision();
	m_Core.Move();
	ReleaseExtraCollision();
	bool StuckAfterMove = GameMap()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameMap()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_LastPos = m_Pos;
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
	if (m_EmoteStop > -1)
		++m_EmoteStop;
}

void CCharacter::Die(int Killer, int Weapon)
{
	if (g_Config.m_Debug)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d",
			Killer, Server()->ClientName(Killer),
			m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	GameServer()->BlockSystemFinish(m_pPlayer->GetCID(), m_Pos, true);

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_pPlayer->GetCID();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = 0;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	// a nice sound
	GameServer()->CreateSound(GameWorld(), m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	for (int i = 0; i < NUM_EXTRAIDS; i++)
	{
		if (m_aExtraIDs[i] != -1)
			Server()->SnapFreeID(m_aExtraIDs[i]);
	}

	m_Alive = false;
	GameWorld()->RemoveEntity(this);
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(GameWorld(), m_Pos, m_pPlayer->GetCID());
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	int TranslatedID = Server()->Translate(SnappingClient, m_pPlayer->GetCID());
	if (TranslatedID == -1)
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, TranslatedID, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(!m_ReckoningTick)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;


	if(m_pPlayer->GetCID() == SnappingClient ||
		(GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID != -1 && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = 10;
		pCharacter->m_Armor = (IsFreezed()) ? 10 - (m_FreezeTime / 15) : 0;
		pCharacter->m_AmmoCount = 1;
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = GetCurrentEmote();

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	if(GetPlayer()->InEvent())
		pCharacter->m_PlayerFlags &= ~DDNET_PLAYERFLAG_AIM;//forbit hook coll

	if (IsFreezed())
		pCharacter->m_Weapon = WEAPON_NINJA;

	//translate hook
	if (pCharacter->m_HookedPlayer != -1)
	{
		int HookingID = Server()->Translate(SnappingClient, pCharacter->m_HookedPlayer);
		pCharacter->m_HookedPlayer = HookingID;
	}

	if (m_EndlessHook)
		pCharacter->m_HookTick = 0;

	SnapExtras(SnappingClient);
}


void CCharacter::SnapExtras(int SnappingClient)
{
	if (IsInviolable())
	{
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetExtraID(EXTRAID_INVIOLABLE), sizeof(CNetObj_Pickup)));
		if (pP)
		{
			pP->m_X = (int)m_Pos.x;
			pP->m_Y = (int)m_Pos.y - 48.0f;
			pP->m_Type = POWERUP_ARMOR;
			pP->m_Subtype = 0;
		}
	}
}

int CCharacter::GetExtraID(int Index)
{
	if (m_aExtraIDs[Index] == -1)
		m_aExtraIDs[Index] = Server()->SnapNewID();
	return m_aExtraIDs[Index];
}