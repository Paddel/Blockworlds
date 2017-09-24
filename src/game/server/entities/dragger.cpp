
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>

#include "dragger.h"

#define RANGE 700.0f

CDragger::CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_DRAGGER)
{
	m_Pos = Pos;
	m_Target = -1;
	m_Strength = Strength;
	m_DragPos = Pos;

	GameWorld()->InsertEntity(this);
}

void CDragger::UpdateTarget()
{
	if (m_Target != -1)
		return;


	CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, RANGE, 0x0, true);
	if(pChr != 0x0)
		m_Target = pChr->GetPlayer()->GetCID();
}

void CDragger::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CDragger::Tick()
{
	GameMap()->Collision()->SetExtraCollision(TILE_BARRIER_ENTITIES);

	UpdateTarget();
	
	if(m_Target != -1)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(m_Target);
		if (pChr == 0x0 || pChr->IsAlive() == false ||
			within_reach(m_Pos, pChr->m_Pos, RANGE) == false ||
			GameMap()->Collision()->IntersectLine(m_Pos, pChr->m_Pos, 0x0, 0x0) != 0)
		{
			m_Target = -1;
			m_DragPos = m_Pos;
		}
		else
		{
			m_DragPos = pChr->m_Pos;

			if (within_reach(m_Pos, pChr->m_Pos, 28.0f) == false)
				pChr->Core()->m_Vel += normalize(m_Pos - pChr->m_Pos) * m_Strength;
		}
	}

	GameMap()->Collision()->ReleaseExtraCollision(TILE_BARRIER_ENTITIES);
}

void CDragger::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_DragPos.x;
	pObj->m_FromY = (int)m_DragPos.y;
	pObj->m_StartTick = Server()->Tick() - 1;
}