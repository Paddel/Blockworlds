
#include <game/mapitems.h>
#include <game/server/entities/npc.h>

#include "srv_gamecore.h"

#define LOOP_CORES(NCORE, NCID) CSrvCharacterCore *NCORE = 0x0; int NCID = -1; \
CNpc *pNpc = m_pWorld->m_ppFirst!=0x0?*m_pWorld->m_ppFirst:0x0; \
m_pWorld->NextCoreItem(&NCORE, &NCID, &pNpc); for(; NCORE; m_pWorld->NextCoreItem(&NCORE, &NCID, &pNpc))

void CSrvWorldCore::NextCoreItem(CSrvCharacterCore **ppCore, int *pClientID, CNpc **ppNpc)
{
	*pClientID = *pClientID + 1;

	if (*pClientID < MAX_CLIENTS)//Is a player
	{
		if (m_apCharacters[*pClientID] == 0x0)
			NextCoreItem(ppCore, pClientID, ppNpc);
		else
			*ppCore = m_apCharacters[*pClientID];

	}
	else//Is a npc
	{
		if (ppNpc != 0x0 && *ppNpc != 0x0)
		{
			*ppCore = (*ppNpc)->Core();
			*pClientID = (*ppNpc)->GetCID();
			*ppNpc = (CNpc *)(*ppNpc)->TypeNext();
		}
		else
		{
			*ppCore = 0x0;
			*pClientID = -1;
		}

	}
}

CSrvCharacterCore *CSrvWorldCore::SearchCore(int ClientID)
{
	if (ClientID < 0)
		return 0x0;
	if (ClientID < MAX_CLIENTS)
		return m_apCharacters[ClientID];
	if(m_ppFirst != 0x0)
		for (CNpc *pNpc = *m_ppFirst; pNpc; pNpc = (CNpc *)pNpc->TypeNext())
			if (pNpc->GetCID() == ClientID)
				return pNpc->Core();
	return 0x0;
}

void CSrvCharacterCore::Init(CSrvWorldCore *pWorld, CCollision *pCollision)
{
	m_pWorld = pWorld;
	m_pCollision = pCollision;
}

void CSrvCharacterCore::Reset()
{
	m_Pos = vec2(0, 0);
	m_Vel = vec2(0, 0);
	m_HookPos = vec2(0, 0);
	m_HookDir = vec2(0, 0);
	m_HookTick = 0;
	m_HookState = HOOK_IDLE;
	m_HookedPlayer = -1;
	m_Jumped = 0;
	m_TriggeredEvents = 0;
	m_Inviolable = false;
}

void CSrvCharacterCore::Tick(bool UseInput)
{
	float PhysSize = 28.0f;
	m_TriggeredEvents = 0;

	// get ground state
	bool Grounded = false;
	if (m_pCollision->CheckPoint(m_Pos.x + PhysSize / 2, m_Pos.y + PhysSize / 2 + 5))
		Grounded = true;
	if (m_pCollision->CheckPoint(m_Pos.x - PhysSize / 2, m_Pos.y + PhysSize / 2 + 5))
		Grounded = true;

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	m_Vel.y += m_pWorld->m_Tuning.m_Gravity;

	float MaxSpeed = Grounded ? m_pWorld->m_Tuning.m_GroundControlSpeed : m_pWorld->m_Tuning.m_AirControlSpeed;
	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;

	// handle input
	if (UseInput)
	{
		m_Direction = m_Input.m_Direction;

		// setup angle
		float a = 0;
		if (m_Input.m_TargetX == 0)
			a = atanf((float)m_Input.m_TargetY);
		else
			a = atanf((float)m_Input.m_TargetY / (float)m_Input.m_TargetX);

		if (m_Input.m_TargetX < 0)
			a = a + pi;

		m_Angle = (int)(a*256.0f);

		// handle jump
		if (m_Input.m_Jump)
		{
			if (!(m_Jumped & 1))
			{
				if (Grounded)
				{
					m_TriggeredEvents |= COREEVENT_GROUND_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse;
					m_Jumped |= 1;
				}
				else if (!(m_Jumped & 2))
				{
					m_TriggeredEvents |= COREEVENT_AIR_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_AirJumpImpulse;
					m_Jumped |= 3;
				}
			}
		}
		else
			m_Jumped &= ~1;

		// handle hook
		if (m_Input.m_Hook)
		{
			if (m_HookState == HOOK_IDLE)
			{
				m_HookState = HOOK_FLYING;
				m_HookPos = m_Pos + TargetDirection*PhysSize*1.5f;
				m_HookDir = TargetDirection;
				m_HookedPlayer = -1;
				m_HookTick = 0;
				m_TriggeredEvents |= COREEVENT_HOOK_LAUNCH;
			}
		}
		else
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_IDLE;
			m_HookPos = m_Pos;
		}
	}

	// add the speed modification according to players wanted direction
	if (m_Direction < 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
	if (m_Direction > 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
	if (m_Direction == 0)
		m_Vel.x *= Friction;

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if (Grounded)
		m_Jumped &= ~2;

	// do hook
	if (m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if (m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if (m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_HookState = HOOK_RETRACTED;
	}
	else if (m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_HookPos + m_HookDir*m_pWorld->m_Tuning.m_HookFireSpeed;
		if (within_reach(m_Pos, NewPos, m_pWorld->m_Tuning.m_HookLength, true) == false)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos - m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if (Hit)
		{
			if (Hit == TILE_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if (m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			LOOP_CORES(pCharCore, i)
			{
				if (pCharCore == this || pCharCore->m_Inviolable == true || m_Inviolable == true)
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if (within_reach(pCharCore->m_Pos, ClosestPoint, PhysSize + 2.0f))
				{
					if (m_HookedPlayer == -1 || within_reach(m_HookPos, pCharCore->m_Pos, Distance))
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}
		}

		if (m_HookState == HOOK_FLYING)
		{
			// check against ground
			if (GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;
			}
			else if (GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if (m_HookState == HOOK_GRABBED)
	{
		if (m_HookedPlayer != -1)
		{
			CSrvCharacterCore *pCharCore = m_pWorld->SearchCore(m_HookedPlayer);
			if (pCharCore)
				m_HookPos = pCharCore->m_Pos;
			else
			{
				// release hook
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
			//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if (m_HookedPlayer == -1 && within_reach(m_HookPos, m_Pos, 46.0f, true) == false)
		{
			vec2 HookVel = normalize(m_HookPos - m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if (HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if ((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel + HookVel;

			// check if we are under the legal limit for the hook
			if (length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if (m_HookedPlayer != -1)
		{
			CSrvCharacterCore *pCore = m_pWorld->SearchCore(m_HookedPlayer);
			if (m_HookTick > SERVER_TICK_SPEED + SERVER_TICK_SPEED / 5 || pCore == 0x0 || pCore->m_Inviolable == true || m_Inviolable == true)
			{
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
			}
		}
	}

	if (m_pWorld)
	{
		LOOP_CORES(pCharCore, i)
		{
			//player *p = (player*)ent;
			if (pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

						  // handle player <-> player collision
			
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if (m_pWorld->m_Tuning.m_PlayerCollision && within_reach(m_Pos, pCharCore->m_Pos, PhysSize*1.25f) && m_Pos != pCharCore->m_Pos && pCharCore->m_Inviolable == false && m_Inviolable == false)
			{
				float Distance = distance(m_Pos, pCharCore->m_Pos);
				float a = (PhysSize*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if (length(m_Vel) > 0.0001)
					Velocity = 1 - (dot(normalize(m_Vel), Dir) + 1) / 2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
			}

			// handle hook influence
			if (m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking)
			{
				if (within_reach(m_Pos, pCharCore->m_Pos, PhysSize*1.50f, true) == false) // TODO: fix tweakable variable
				{
					float Distance = distance(m_Pos, pCharCore->m_Pos);
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance / m_pWorld->m_Tuning.m_HookLength);
					float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

					// add force to the hooked player
					pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
					pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
					m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
				}
			}
		}
	}

	// clamp the velocity to something sane
	if (length(m_Vel) > 350)
		m_Vel = normalize(m_Vel) * 350;
}

void CSrvCharacterCore::TickPredict(bool UseInput)
{
	float PhysSize = 28.0f;
	m_TriggeredEvents = 0;

	// get ground state
	bool Grounded = false;
	if (m_pCollision->CheckPoint(m_Pos.x + PhysSize / 2, m_Pos.y + PhysSize / 2 + 5))
		Grounded = true;
	if (m_pCollision->CheckPoint(m_Pos.x - PhysSize / 2, m_Pos.y + PhysSize / 2 + 5))
		Grounded = true;

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	m_Vel.y += m_pWorld->m_Tuning.m_Gravity;

	float MaxSpeed = Grounded ? m_pWorld->m_Tuning.m_GroundControlSpeed : m_pWorld->m_Tuning.m_AirControlSpeed;
	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;

	// handle input
	if (UseInput)
	{
		m_Direction = m_Input.m_Direction;

		// setup angle
		float a = 0;
		if (m_Input.m_TargetX == 0)
			a = atanf((float)m_Input.m_TargetY);
		else
			a = atanf((float)m_Input.m_TargetY / (float)m_Input.m_TargetX);

		if (m_Input.m_TargetX < 0)
			a = a + pi;

		m_Angle = (int)(a*256.0f);

		// handle jump
		if (m_Input.m_Jump)
		{
			if (!(m_Jumped & 1))
			{
				if (Grounded)
				{
					m_TriggeredEvents |= COREEVENT_GROUND_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse;
					m_Jumped |= 1;
				}
				else if (!(m_Jumped & 2))
				{
					m_TriggeredEvents |= COREEVENT_AIR_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_AirJumpImpulse;
					m_Jumped |= 3;
				}
			}
		}
		else
			m_Jumped &= ~1;

		// handle hook
		if (m_Input.m_Hook)
		{
			if (m_HookState == HOOK_IDLE)
			{
				m_HookState = HOOK_FLYING;
				m_HookPos = m_Pos + TargetDirection*PhysSize*1.5f;
				m_HookDir = TargetDirection;
				m_HookedPlayer = -1;
				m_HookTick = 0;
				m_TriggeredEvents |= COREEVENT_HOOK_LAUNCH;
			}
		}
		else
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_IDLE;
			m_HookPos = m_Pos;
		}
	}

	// add the speed modification according to players wanted direction
	if (m_Direction < 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
	if (m_Direction > 0)
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
	if (m_Direction == 0)
		m_Vel.x *= Friction;

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if (Grounded)
		m_Jumped &= ~2;

	// do hook
	if (m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if (m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if (m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_HookState = HOOK_RETRACTED;
	}
	else if (m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_HookPos + m_HookDir*m_pWorld->m_Tuning.m_HookFireSpeed;
		if (within_reach(m_Pos, NewPos, m_pWorld->m_Tuning.m_HookLength, true) == false)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos - m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if (Hit)
		{
			if (Hit == TILE_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if (m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			LOOP_CORES(pCharCore, i)
			{
				if (pCharCore == this)
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if (within_reach(pCharCore->m_Pos, ClosestPoint, PhysSize + 2.0f))
				{
					if (m_HookedPlayer == -1 || within_reach(m_HookPos, pCharCore->m_Pos, Distance))
					{
						m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}
		}

		if (m_HookState == HOOK_FLYING)
		{
			// check against ground
			if (GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;
			}
			else if (GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENT_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if (m_HookState == HOOK_GRABBED)
	{
		if (m_HookedPlayer != -1)
		{
			CSrvCharacterCore *pCharCore = m_pWorld->SearchCore(m_HookedPlayer);
			if (pCharCore)
				m_HookPos = pCharCore->m_Pos;
			else
			{
				// release hook
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
			//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if (m_HookedPlayer == -1 && within_reach(m_HookPos, m_Pos, 46.0f, true) == false)
		{
			vec2 HookVel = normalize(m_HookPos - m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if (HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if ((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel + HookVel;

			// check if we are under the legal limit for the hook
			if (length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if (m_HookedPlayer != -1 && (m_HookTick > SERVER_TICK_SPEED + SERVER_TICK_SPEED / 5 || !m_pWorld->SearchCore(m_HookedPlayer)))
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_RETRACTED;
			m_HookPos = m_Pos;
		}
	}

	if (m_pWorld)
	{
		LOOP_CORES(pCharCore, i)
		{
			//player *p = (player*)ent;
			if (pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

						  // handle player <-> player collision
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if (m_pWorld->m_Tuning.m_PlayerCollision && within_reach(m_Pos, pCharCore->m_Pos, PhysSize*1.25f) && m_Pos != pCharCore->m_Pos && pCharCore->m_Inviolable == false && m_Inviolable == false)
			{
				float Distance = distance(m_Pos, pCharCore->m_Pos);
				float a = (PhysSize*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if (length(m_Vel) > 0.0001)
					Velocity = 1 - (dot(normalize(m_Vel), Dir) + 1) / 2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
			}

			// handle hook influence
			if (m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking)
			{
				if (within_reach(m_Pos, pCharCore->m_Pos, PhysSize*1.50f, true) == false) // TODO: fix tweakable variable
				{
					float Distance = distance(m_Pos, pCharCore->m_Pos);
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance / m_pWorld->m_Tuning.m_HookLength);
					float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

					// add force to the hooked player
					pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
					pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
					m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
				}
			}
		}
	}

	// clamp the velocity to something sane
	if (length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CSrvCharacterCore::Move()
{
	float RampValue = VelocityRamp(length(m_Vel) * 50, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);

	m_Vel.x = m_Vel.x*RampValue;

	vec2 NewPos = m_Pos;
	m_pCollision->MoveBox(&NewPos, &m_Vel, vec2(28.0f, 28.0f), 0);

	m_Vel.x = m_Vel.x*(1.0f / RampValue);

	m_Pos = NewPos;
}

void CSrvCharacterCore::Write(CNetObj_CharacterCore *pObjCore)
{
	pObjCore->m_X = round_to_int(m_Pos.x);
	pObjCore->m_Y = round_to_int(m_Pos.y);

	pObjCore->m_VelX = round_to_int(m_Vel.x*256.0f);
	pObjCore->m_VelY = round_to_int(m_Vel.y*256.0f);
	pObjCore->m_HookState = m_HookState;
	pObjCore->m_HookTick = m_HookTick;
	pObjCore->m_HookX = round_to_int(m_HookPos.x);
	pObjCore->m_HookY = round_to_int(m_HookPos.y);
	pObjCore->m_HookDx = round_to_int(m_HookDir.x*256.0f);
	pObjCore->m_HookDy = round_to_int(m_HookDir.y*256.0f);
	pObjCore->m_HookedPlayer = m_HookedPlayer;
	pObjCore->m_Jumped = m_Jumped;
	pObjCore->m_Direction = m_Direction;
	pObjCore->m_Angle = m_Angle;
}

void CSrvCharacterCore::Read(const CNetObj_CharacterCore *pObjCore)
{
	m_Pos.x = pObjCore->m_X;
	m_Pos.y = pObjCore->m_Y;
	m_Vel.x = pObjCore->m_VelX / 256.0f;
	m_Vel.y = pObjCore->m_VelY / 256.0f;
	m_HookState = pObjCore->m_HookState;
	m_HookTick = pObjCore->m_HookTick;
	m_HookPos.x = pObjCore->m_HookX;
	m_HookPos.y = pObjCore->m_HookY;
	m_HookDir.x = pObjCore->m_HookDx / 256.0f;
	m_HookDir.y = pObjCore->m_HookDy / 256.0f;
	m_HookedPlayer = pObjCore->m_HookedPlayer;
	m_Jumped = pObjCore->m_Jumped;
	m_Direction = pObjCore->m_Direction;
	m_Angle = pObjCore->m_Angle;
}

void CSrvCharacterCore::Quantize()
{
	CNetObj_CharacterCore Core;
	Write(&Core);
	Read(&Core);
}
