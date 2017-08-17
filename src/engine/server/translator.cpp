
#include <float.h>
#include <algorithm>

#include <engine/shared/config.h>//remove

#include <base/tl/algorithm.h>
#include <game/server/gamemap.h>

#include "server.h"
#include "translator.h"

#define THREADING 1

typedef std::pair<long double, int> CSortItem;

CTranslator::CTranslator(CServer *pServer)
{
	m_pServer = pServer;
	m_aNumItems = 0x0;
	//for(int i = 0; i < MAX_CLIENTS; i++)
	mem_zero(&m_Sortions, sizeof(m_Sortions));
}

void CTranslator::SortMap(void *pData)
{
	CSortionData *pSortionData = (CSortionData *)pData;

	//find own Translate item
	CTranslateItem *pOwnTranslateItem = 0x0;
	for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
	{
		CTranslateItem *pTranslateItem = &pSortionData->m_aTranslateItems[i];
		if (pTranslateItem->m_ClientID != pSortionData->m_ClientID)
			continue;
		pOwnTranslateItem = pTranslateItem;
		break;
	}

	//Check if we can do easy filling

	{
		if (pOwnTranslateItem == 0x0)// cannot calculate distances if we are not in liste
		{
			pSortionData->m_Working = false;
			return;
		}

		CSortItem *aSortItems = new CSortItem[pSortionData->m_NumTranslateItems];
		//GetDistances
		vec2 OwnPos = pOwnTranslateItem->m_Pos;
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			long double Distance;
			CTranslateItem *pTranslateItem = &pSortionData->m_aTranslateItems[i];
			if (pOwnTranslateItem == pTranslateItem)
				Distance = 0.0f;
			else if (pTranslateItem->m_Team == TEAM_SPECTATORS)
				Distance = LDBL_MAX - 1;
			else
			{
				vec2 DeltaPos = OwnPos - pTranslateItem->m_Pos;
				Distance = DeltaPos.x*DeltaPos.x + DeltaPos.y*DeltaPos.y;//no sqrt = way better performance
			}

			aSortItems[i].first = Distance;
			aSortItems[i].second = pTranslateItem->m_ClientID;
		}

		short OldMap[MAX_CLIENTS];
		mem_copy(OldMap, pSortionData->m_aIDMap, sizeof(OldMap));

		if (g_Config.m_Password[0] == '1')
			for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
				dbg_msg(0, "b[%i]", aSortItems[i].second);

		//std::sort(aSortItems, aSortItems + pSortionData->m_NumTranslateItems);
		bubblesort(aSortItems, pSortionData->m_NumTranslateItems);

		if (g_Config.m_Password[0] == '1')
			for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
				dbg_msg(0, "b[%i]", aSortItems[i].second);

		g_Config.m_Password[0] = '\0';

		for (int i = 0; i < MAX_CLIENTS; i++)
			pSortionData->m_aIDMap[i] = -1;

		int Used = min(pSortionData->m_UsingMapItems, pSortionData->m_NumTranslateItems);

		//insert already existing first
		for (int i = 0; i < Used; i++)
		{
			int InsertingID = aSortItems[i].second;
			if (aSortItems[i].first == LDBL_MAX)
				continue;

			for (int j = 0; j < Used; j++)
			{
				if (OldMap[j] == InsertingID)
				{
					pSortionData->m_aIDMap[j] = InsertingID;
					aSortItems[i].second = -1;//doesnt have to be inserted anymore
					break;
				}
			}
		}

		//new insert new to list
		for (int i = 0; i < Used; i++)
		{
			int InsertingID = aSortItems[i].second;
			if (InsertingID == -1 || aSortItems[i].first == LDBL_MAX)
				continue;

			for (int j = 0; j < Used; j++)
			{
				if (pSortionData->m_aIDMap[j] != -1)
					continue;

				pSortionData->m_aIDMap[j] = InsertingID;
				break;
			}
		}

		pSortionData->m_aIDMap[pSortionData->m_UsingMapItems - 1] = -1;//last slot must be free

		delete aSortItems;
	}

	pSortionData->m_Working = false;
}

void CTranslator::UpdateMapInfos()
{
	if (m_aNumItems != 0x0)
		delete m_aNumItems;

	m_aNumItems = new int[Server()->GetNumMaps()];
	for (int i = 0; i < Server()->GetNumMaps(); i++)
		m_aNumItems[i] = Server()->GetGameMap(i)->NumTranslateItems();
}

void CTranslator::UpdatePlayerMap(int ClientID)
{
	if (m_aNumItems == 0x0)
		return;

	//check if thread is done
	if (m_Sortions[ClientID].m_Working == false)
		mem_copy(m_aIDMap + ClientID * MAX_CLIENTS, m_Sortions[ClientID].m_aIDMap, sizeof(m_Sortions[ClientID].m_aIDMap));
	else
		return;//Wait until thread is done before starting a new one


	int MapIndex = Server()->CurrentMapIndex(ClientID);
	if (m_aNumItems[MapIndex] == 0)
		return;//nothing to do here

	m_Sortions[ClientID].m_Working = false;
	m_Sortions[ClientID].m_NumTranslateItems = m_aNumItems[MapIndex];
	m_Sortions[ClientID].m_UsingMapItems = Server()->UsingMapItems(ClientID);
	if (m_Sortions[ClientID].m_aTranslateItems != 0x0)
		delete m_Sortions[ClientID].m_aTranslateItems;
	m_Sortions[ClientID].m_aTranslateItems = new CTranslateItem[m_aNumItems[MapIndex]];
	Server()->GetGameMap(MapIndex)->FillTranslateItems(m_Sortions[ClientID].m_aTranslateItems);
	m_Sortions[ClientID].m_ClientID = ClientID;

	if (THREADING)
		thread_init(SortMap, &m_Sortions[ClientID]);
	else
		SortMap(&m_Sortions[ClientID]);
}

void CTranslator::Tick()
{
	UpdateMapInfos(); // always call this at first

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false)
			continue;

		UpdatePlayerMap(i);
	}
}

int CTranslator::Translate(int For, int ClientID)
{
	int Using = Server()->UsingMapItems(ClientID);
	for (int i = 0; i < Using; i++)
		if (m_aIDMap[For * MAX_CLIENTS + i] == ClientID)
			return i;
	return -1;
}

int CTranslator::ReverseTranslate(int For, int ClientID)
{
	int Using = Server()->UsingMapItems(ClientID);
	if (ClientID < 0 || ClientID >= Using)
		return -1;
	return m_aIDMap[ClientID];
}