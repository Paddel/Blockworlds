#pragma once

#include <engine/server.h>

#include "animations.h"

class CAnimLetter : public CMapAnimation
{
protected:
	int m_Ticks;
	int m_NumIds;
	float m_Width;
	float m_Size;
	int *m_aIDs;
	bool *m_pBitField;
	bool m_Shotgun;

public:
	CAnimLetter(vec2 Pos, int64 Tick, CGameMap *pGameMap, int Ticks, bool *pBitField, float Size, bool Shotgun) : CMapAnimation(Pos, Tick, pGameMap)
	{
		m_Ticks = Ticks;
		m_pBitField = pBitField;
		m_Size = Size;
		m_Shotgun = Shotgun;

		m_Width = 0.0f;
		m_NumIds = 0;
		for (int i = 0; i < 5 * 7; i++)
			if (m_pBitField[i] == 1)
				m_NumIds++;

		if (m_NumIds > 0)
		{
			m_aIDs = new int[m_NumIds];
		}

		for (int i = 0; i < m_NumIds; i++)
			m_aIDs[i] = Server()->SnapNewID();

	}
	virtual ~CAnimLetter()
	{
		for (int i = 0; i < m_NumIds; i++)
			 Server()->SnapFreeID(m_aIDs[i]);

		if(m_NumIds > 0)
			delete m_aIDs;
	}

	virtual void Tick() { };
	virtual void Snap(int SnappingClient)
	{
		int IDIndex = 0;

		for (int x = 4; x >= 0; x--)
		{
			for (int y = 0; y < 7; y++)
			{
				if (m_pBitField[x + y * 5] == 0)
					continue;

				if (m_Shotgun == false)
				{
					CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[IDIndex], sizeof(CNetObj_Laser)));
					if (!pObj)
						return;

					vec2 Pos = GetPos() + vec2(x * m_Size, y * m_Size);
					pObj->m_X = (int)Pos.x;
					pObj->m_Y = (int)Pos.y;
					pObj->m_FromX = (int)Pos.x;
					pObj->m_FromY = (int)Pos.y;
					pObj->m_StartTick = Server()->Tick() - 5;
				}
				else
				{
					CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[IDIndex], sizeof(CNetObj_Projectile)));
					if (pProj)
					{
						vec2 Pos = GetPos() + vec2(x * m_Size, y * m_Size);
						pProj->m_X = (int)Pos.x;
						pProj->m_Y = (int)Pos.y;
						pProj->m_VelX = (int)(Pos.x*100.0f);
						pProj->m_VelY = (int)(Pos.y*100.0f);
						pProj->m_StartTick = Server()->Tick() - 5;
						pProj->m_Type = WEAPON_SHOTGUN;
					}
				}

				IDIndex++;
			}
		}
	}

	virtual bool Done() { return GetTick() + m_Ticks < Server()->Tick(); }

	float Width() const { return m_NumIds > 0 ? m_Size * 5 : 0.0f; }

};

static bool gs_LetterA[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterB[5 * 7] = {
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
};

static bool gs_LetterC[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterD[5 * 7] = {
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
};

static bool gs_LetterE[5 * 7] = {
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
};

static bool gs_LetterF[5 * 7] = {
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
};

static bool gs_LetterG[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 0,
	1, 0, 1, 1, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterH[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterI[5 * 7] = {
	1, 1, 1, 1, 1,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	1, 1, 1, 1, 1,
};

static bool gs_LetterJ[5 * 7] = {
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterK[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 1, 0,
	1, 0, 1, 0, 0,
	1, 1, 0, 0, 0,
	1, 0, 1, 0, 0,
	1, 0, 0, 1, 0,
	1, 0, 0, 0, 1,
};

static bool gs_LetterL[5 * 7] = {
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
};

static bool gs_LetterM[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 1, 0, 1, 1,
	1, 0, 1, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterN[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 0, 0, 1,
	1, 0, 1, 0, 1,
	1, 0, 0, 1, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterO[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterP[5 * 7] = {
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
};

static bool gs_LetterQ[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 1, 0, 1,
	1, 0, 0, 1, 0,
	0, 1, 1, 0, 1,
};

static bool gs_LetterR[5 * 7] = {
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
	1, 0, 1, 0, 0,
	1, 0, 0, 1, 0,
	1, 0, 0, 0, 1,
};

static bool gs_LetterS[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 0,
	0, 1, 1, 1, 0,
	0, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterT[5 * 7] = {
	1, 1, 1, 1, 1,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterU[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterV[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 0, 1, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterW[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 1, 0, 1,
	1, 1, 0, 1, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterX[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
};

static bool gs_LetterY[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterZ[5 * 7] = {
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
};

static bool gs_Letter0[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 1, 1,
	1, 0, 1, 0, 1,
	1, 1, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_Letter1[5 * 7] = {
	0, 0, 1, 0, 0,
	0, 1, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 1, 1, 1, 0,
};

static bool gs_Letter2[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 0, 1, 1, 0,
	0, 1, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
};

static bool gs_Letter3[5 * 7] = {
	1, 1, 1, 1, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 1, 0,
	0, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_Letter4[5 * 7] = {
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
};

static bool gs_Letter5[5 * 7] = {
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 0,
	0, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_Letter6[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 0,
	1, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};


static bool gs_Letter7[5 * 7] = {
	1, 1, 1, 1, 1,
	1, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_Letter8[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_Letter9[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 1,
	0, 0, 0, 0, 1,
	1, 0, 0, 0, 1,
	0, 1, 1, 1, 0,
};

static bool gs_LetterPL[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	1, 1, 1, 1, 1,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
};

static bool gs_LetterMN[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};


static bool gs_LetterEM[5 * 7] = {
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterQM[5 * 7] = {
	0, 1, 1, 1, 0,
	1, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterPC[5 * 7] = {
	1, 1, 0, 0, 0,
	1, 1, 0, 0, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 0, 0,
	1, 0, 0, 1, 1,
	0, 0, 0, 1, 1,
};

static bool gs_LetterDL[5 * 7] = {
	0, 0, 1, 0, 0,
	0, 1, 1, 1, 1,
	1, 0, 0, 0, 0,
	0, 1, 1, 1, 0,
	0, 0, 0, 0, 1,
	1, 1, 1, 1, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterBO[5 * 7] = {
	0, 1, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	1, 0, 0, 0, 0,
	0, 1, 0, 0, 0,
};

static bool gs_LetterBC[5 * 7] = {
	0, 0, 0, 1, 0,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 0, 1,
	0, 0, 0, 1, 0,
};

static bool gs_LetterDT[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
};

static bool gs_LetterCM[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 0, 0,
};

static bool gs_LetterDD[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};

static bool gs_LetterSM[5 * 7] = {
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 0, 0,
	1, 0, 0, 0, 0,
	0, 1, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 1, 0,
};

static bool gs_LetterBI[5 * 7] = {
	0, 1, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 1, 0,
	0, 0, 0, 0, 1,
	0, 0, 0, 1, 0,
	0, 0, 1, 0, 0,
	0, 1, 0, 0, 0,
};

static bool gs_LetterEQ[5 * 7] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 0,
	1, 1, 1, 1, 1,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};

static bool gs_LetterLV[5 * 7] = {
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 1, 0, 0,
};