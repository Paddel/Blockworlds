
#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/server/map.h>
#include <game/extras.h>
#include <game/server/player.h>
#include <game/server/gamemap.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/dragger.h>
#include <game/server/entities/lasergun.h>

#include "race_components.h"

CRaceComponents::CRaceComponents()
{
	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;
}


CRaceComponents::~CRaceComponents()
{
	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (m_lDoorTiles[i]->m_Length > 0.0f)
			Server()->SnapFreeID(m_lDoorTiles[i]->m_SnapID);
	}

	m_lDoorTiles.delete_all();
	m_lTeleTo.delete_all();
}

float CRaceComponents::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameMap()->World()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for (; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if (pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void CRaceComponents::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for (int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameMap()->World()->FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };	// start, left, up, right, down
		int Result = -1;
		for (int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for (int c = 0; c < Num; ++c)
				if (GameMap()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i] + Positions[Index]) ||
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i] + Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if (Result == -1)
			continue;	// try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i] + Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if (!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CRaceComponents::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	if (Index == ENTITY_SPAWN)
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
	else if (Index == ENTITY_SPAWN_EVENT)
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
	else if (Index == ENTITY_SPAWN_1ON1)
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
	else if (Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if (Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if (Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
	}
	else if (Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	else if (Index == ENTITY_WEAPON_RIFLE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
	}
	else if (Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
	{
		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
	}
	else if (Index == ENTITY_DRAGGER_WEAK ||
		Index == ENTITY_DRAGGER_MEDIUM ||
		Index == ENTITY_DRAGGER_STRONG)
	{
		new CDragger(GameMap()->World(), Pos, float(Index - ENTITY_DRAGGER_WEAK + 1));
	}

	if (Type != -1)
	{
		CPickup *pPickup = new CPickup(GameMap()->World(), Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void CRaceComponents::InitDoorTile(CDoorTile *pTile)
{
	pTile->m_Delay = 0;
	pTile->m_Length = 0.0f;

	if (pTile->m_LaserDir[0] == '^' || pTile->m_LaserDir[0] == 'v' || pTile->m_LaserDir[0] == '|')
		pTile->m_Type = 1;
	else if (pTile->m_LaserDir[0] == '<' || pTile->m_LaserDir[0] == '>' || pTile->m_LaserDir[0] == '-')
		pTile->m_Type = 2;
	else
		pTile->m_Type = 0;

	if (pTile->m_LaserDir[0] == '^' || pTile->m_LaserDir[0] == 'v' ||
		pTile->m_LaserDir[0] == '<' || pTile->m_LaserDir[0] == '>')
	{
		vec2 Dir = vec2(0.0f, 0.0f);
		switch (pTile->m_LaserDir[0])
		{
		case '^': Dir = vec2(0, -1); break;
		case 'v': Dir = vec2(0, 1); break;
		case '<': Dir = vec2(-1, 0); break;
		case '>': Dir = vec2(1, 0); break;
		}

		pTile->m_Direction = Dir;
		pTile->m_SnapID = Server()->SnapNewID();

		pTile->m_Length = 16.0f;
		for (int i = 1; i < 64; i++)
		{
			if (GameMap()->Collision()->CheckPoint(pTile->m_Pos + Dir * 32.0f * i) == false)
				pTile->m_Length += 32;
			else
				break;
		}
	}
}

void CRaceComponents::InitEntities()
{
	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = GameMap()->Layers()->GameLayer();
	CTile *pTiles = (CTile *)GameMap()->Map()->EngineMap()->GetData(pTileMap->m_Data);


	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width + x].m_Index;

			if (Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x*32.0f + 16.0f, y*32.0f + 16.0f);
				OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}
}

void CRaceComponents::InitExtras()
{
	for (int l = 0; l < GameMap()->Layers()->GetNumExtrasLayer(); l++)
	{
		for (int x = 0; x < GameMap()->Layers()->GetExtrasWidth(l); x++)
		{
			for (int y = 0; y < GameMap()->Layers()->GetExtrasHeight(l); y++)
			{
				int Index = y * GameMap()->Layers()->GetExtrasWidth(l) + x;
				int Tile = GameMap()->Layers()->GetExtrasTile(l)[Index].m_Index;
				CExtrasData ExtrasData = GameMap()->Layers()->GetExtrasData(l)[Index];
				vec2 Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

				if (Tile == EXTRAS_DOOR)
				{
					CDoorTile *pDoorTile = new CDoorTile();
					pDoorTile->m_Index = Index;
					pDoorTile->m_Pos = Pos;
					const char *pData = ExtrasData.m_aData;
					pDoorTile->m_ID = str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][0];
					str_copy(pDoorTile->m_LaserDir, pData, sizeof(pDoorTile->m_LaserDir));
					pData += +gs_ExtrasSizes[Tile][1];
					pDoorTile->m_Default = (bool)str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][2];
					pDoorTile->m_Freeze = (bool)str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][3];
					InitDoorTile(pDoorTile);
					m_lDoorTiles.add(pDoorTile);
				}
				else if (Tile == EXTRAS_TELEPORT_TO)
				{
					CTeleTo *pTeleTo = new CTeleTo();
					pTeleTo->m_Pos = Pos;
					pTeleTo->m_ID = str_toint(ExtrasData.m_aData);
					m_lTeleTo.add(pTeleTo);
				}
				else if (Tile == EXTRAS_LASERGUN)
				{
					const char *pData = ExtrasData.m_aData;
					int ID = str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][0];
					int Type = str_toint(pData);
					pData += +gs_ExtrasSizes[Tile][1];

					new CLaserGun(GameMap()->World(), Pos, ID, Type);
				}
			}
		}
	}
}

void CRaceComponents::SnapDoors(int SnappingClient)
{
	CPlayer *pPlayer = GameMap()->m_apPlayers[SnappingClient];
	if (pPlayer == 0x0)
		return;

	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (DoorTileActive(m_lDoorTiles[i]) == false || m_lDoorTiles[i]->m_Length <= 0.0f)
			continue;

		float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x - m_lDoorTiles[i]->m_Pos.x;
		float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y - m_lDoorTiles[i]->m_Pos.y;

		if (absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
			continue;

		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_lDoorTiles[i]->m_SnapID, sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		vec2 From = m_lDoorTiles[i]->m_Pos + m_lDoorTiles[i]->m_Direction * m_lDoorTiles[i]->m_Length;

		pObj->m_X = (int)m_lDoorTiles[i]->m_Pos.x;
		pObj->m_Y = (int)m_lDoorTiles[i]->m_Pos.y;
		pObj->m_FromX = (int)From.x;
		pObj->m_FromY = (int)From.y;
		pObj->m_StartTick = Server()->Tick() - 1;
	}
}


bool CRaceComponents::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, Team);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

CRaceComponents::CDoorTile *CRaceComponents::GetDoorTile(int Index)
{
	for (int i = 0; i < m_lDoorTiles.size(); i++)
		if (m_lDoorTiles[i]->m_Index == Index)
			return m_lDoorTiles[i];
	return 0x0;
}

bool CRaceComponents::DoorTileActive(CDoorTile *pDoorTile) const
{
	if (pDoorTile->m_Delay > Server()->Tick())
		return !pDoorTile->m_Default;
	return pDoorTile->m_Default;
}

void CRaceComponents::OnDoorHandle(int ID, int Delay, bool Activate)
{
	if (Delay <= 0)
		Delay = 1;

	for (int i = 0; i < m_lDoorTiles.size(); i++)
	{
		if (m_lDoorTiles[i]->m_ID != ID)
			continue;

		if (m_lDoorTiles[i]->m_Default == Activate)
			m_lDoorTiles[i]->m_Delay = 0;
		else
			m_lDoorTiles[i]->m_Delay = Server()->Tick() + Delay * Server()->TickSpeed() + 1;
	}
}

void CRaceComponents::OnLasergunTrigger(int ID, int Delay)
{
	CLaserGun *pFirst = (CLaserGun *)GameMap()->World()->FindFirst(CGameWorld::ENTTYPE_LASERGUN);
	for (CLaserGun *pLaserGun = pFirst; pLaserGun; pLaserGun = (CLaserGun *)pLaserGun->TypeNext())
		if (pLaserGun->GetID() == ID)
			pLaserGun->SetActive(Delay);
}

bool CRaceComponents::GetRandomTelePos(int ID, vec2 *Pos)
{
	int Num = 0;
	for (int i = 0; i < m_lTeleTo.size(); i++)
		if (m_lTeleTo[i]->m_ID == ID)
			Num++;
	if (Num == 0)
		return false;
	int Chosen = rand() % Num;
	for (int i = 0; i < m_lTeleTo.size(); i++)
	{
		if (m_lTeleTo[i]->m_ID != ID)
			continue;

		if (Chosen <= 0)
		{
			*Pos = m_lTeleTo[i]->m_Pos;
			return true;
		}
		Chosen--;
	}

	return false;
}

void CRaceComponents::Init()
{
	InitEntities();
	InitExtras();
}

void CRaceComponents::Tick()
{

}

void CRaceComponents::Snap(int SnappingClient)
{
	SnapDoors(SnappingClient);
}