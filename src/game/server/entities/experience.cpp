
#include <engine/server.h>
#include <game/server/gamecontext.h>

#include "experience.h"

CExperience::CExperience(CGameWorld *pGameWorld, vec2 Pos, int Amount, int TargetID)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Amount = Amount;
	m_TargetID = TargetID;

	GameWorld()->InsertEntity(this);
}

void CExperience::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CExperience::Tick()
{
	CCharacter *pChr = GameServer()->GetPlayerChar(m_TargetID);
	if (pChr == 0x0)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	float Distance = distance(m_Pos, pChr->m_Pos);
	if(Distance < 28.0f)
	{
		GameWorld()->DestroyEntity(this);
		GameServer()->CreateSound(GameMap(), m_Pos, SOUND_PICKUP_HEALTH, CmaskOne(m_TargetID));
		return;
	}

	vec2 Direction = normalize(pChr->m_Pos - m_Pos);

	//y = x * 0.035 + 11
	float Speed = Distance * 0.035f + 11.0f;
	m_Pos += Direction * Speed;
}

void CExperience::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient, m_Pos))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if (pProj == 0x0)
		return;

	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = 0;
	pProj->m_VelY = 0;
	pProj->m_StartTick = Server()->Tick()-1;
	pProj->m_Type = WEAPON_HAMMER;
	if(m_Amount == 0)
		pProj->m_Type = WEAPON_GRENADE;
}