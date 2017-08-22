
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PickupPhysSize;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickup::Tick()
{
	// Check if a player intersected us
	CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				{
					if(m_pLastEntity != pChr)
						GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_HEALTH);
					pChr->Freeze(3.0f);
				}
				break;

			case POWERUP_ARMOR:
				{
					if(pChr->TakeWeapons())
						GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case POWERUP_WEAPON:
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS)
				{
					if(pChr->GiveWeapon(m_Subtype))
					{
						if(m_Subtype == WEAPON_GRENADE)
							GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_GRENADE);
						else if(m_Subtype == WEAPON_SHOTGUN)
							GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_SHOTGUN);
						else if(m_Subtype == WEAPON_RIFLE)
							GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_SHOTGUN);

						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Subtype);
					}
				}
				break;

			case POWERUP_NINJA:
				{
					// activate ninja on target player
					pChr->GiveNinja(m_pLastEntity != pChr);
					break;
				}

			default:
				break;
		};
	}

	m_pLastEntity = pChr;
}

void CPickup::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}
