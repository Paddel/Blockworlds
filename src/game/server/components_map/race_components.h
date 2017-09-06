#pragma once

#include <base/vmath.h>
#include <base/tl/array.h>
#include <game/server/component.h>

class CRaceComponents : public CComponentMap
{
public:
	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100, 100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

	struct CDoorTile
	{
		int m_Index;
		vec2 m_Pos;
		int m_ID;
		bool m_Default;
		char m_LaserDir[2];
		bool m_Freeze;

		int m_Delay;
		float m_Length;
		vec2 m_Direction;
		int m_SnapID;
		int m_Type;
	};

	struct CTeleTo
	{
		int m_ID;
		vec2 m_Pos;
	};

private:
	array<CDoorTile *> m_lDoorTiles;
	array<CTeleTo *> m_lTeleTo;
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool OnEntity(int Index, vec2 Pos);
	void InitDoorTile(CDoorTile *pTile);

	void InitEntities();
	void InitExtras();
	void SnapDoors(int SnappingClient);

public:
	CRaceComponents();
	~CRaceComponents();

	bool CanSpawn(int Team, vec2 *pOutPos);
	CDoorTile *GetDoorTile(int Index);
	bool DoorTileActive(CDoorTile *pDoorTile) const;
	void OnDoorHandle(int ID, int Delay, bool Activate);
	void OnLasergunTrigger(int ID, int Delay);
	bool GetRandomTelePos(int ID, vec2 *Pos);

	virtual void Init();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};