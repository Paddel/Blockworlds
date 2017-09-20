
#include <float.h>
#include <algorithm>

#include <base/tl/algorithm.h>
#include <game/server/gamemap.h>//TODO: Create an interface to remove this include

#include "server.h"
#include "translator.h"

#define THREADING 0

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

	//Check if we can do easy filling
	if (pSortionData->m_NumTranslateItems < pSortionData->m_UsingMapItems)// <= if you dont need last id
	{
		short OldMap[MAX_CLIENTS];
		mem_copy(OldMap, pSortionData->m_aIDMap, sizeof(OldMap));
		short *aFillingList = new short[pSortionData->m_NumTranslateItems];
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			aFillingList[i] = pSortionData->m_aTranslateItems[i].m_ClientID;
			//dbg_msg(0, "%i %i", pSortionData->m_ClientID, aFillingList[i]);
		}

		for (int i = 0; i < MAX_CLIENTS; i++)
			pSortionData->m_aIDMap[i] = -1;

		//fill in correct IDs
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			if (aFillingList[i] >= pSortionData->m_UsingMapItems - 1)// no -1 if you dont need last id
				continue;

			pSortionData->m_aIDMap[aFillingList[i]] = aFillingList[i];
			aFillingList[i] = -1;//do not fill me anywhere else
		}

		//fill in already inserted IDs
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			if (aFillingList[i] == -1)
				continue;

			for (int j = pSortionData->m_UsingMapItems - 2; j >= 0; j--)// -1 if you dont need last id
			{
				if (OldMap[j] == aFillingList[i] && pSortionData->m_aIDMap[j] == -1)
				{
					pSortionData->m_aIDMap[j] = aFillingList[i];
					aFillingList[i] = -1;//do not fill me anywhere else
					break;
				}

			}
		}

		//fill in rest
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			if (aFillingList[i] == -1)
				continue;

			for (int j = pSortionData->m_UsingMapItems - 2; j >= 0; j--)// -1 if you dont need last id
			{
				if (pSortionData->m_aIDMap[j] == -1)
				{
					pSortionData->m_aIDMap[j] = aFillingList[i];
					break;
				}

			}
		}
		delete[] aFillingList;

		pSortionData->m_aIDMap[pSortionData->m_UsingMapItems - 1] = -1;//last slot must be free

	}
	else
	{
		CSortItem *aSortItems = new CSortItem[pSortionData->m_NumTranslateItems];
		//GetDistances
		for (int i = 0; i < pSortionData->m_NumTranslateItems; i++)
		{
			long double Distance;
			CTranslateItem *pTranslateItem = &pSortionData->m_aTranslateItems[i];
			if (pTranslateItem->m_ClientID == pSortionData->m_ClientID)
				Distance = 0.0f;
			else if (pTranslateItem->m_Team == TEAM_SPECTATORS)
				Distance = LDBL_MAX - 1;
			else
			{
				vec2 DeltaPos = pSortionData->m_Pos - pTranslateItem->m_Pos;
				Distance = DeltaPos.x*DeltaPos.x + DeltaPos.y*DeltaPos.y;//no sqrt = way better performance
			}

			aSortItems[i].first = Distance;
			aSortItems[i].second = pTranslateItem->m_ClientID;
		}

		short OldMap[MAX_CLIENTS];
		mem_copy(OldMap, pSortionData->m_aIDMap, sizeof(OldMap));

		bubblesort(aSortItems, pSortionData->m_NumTranslateItems);

		for (int i = 0; i < MAX_CLIENTS; i++)
			pSortionData->m_aIDMap[i] = -1;

		int Used = min(pSortionData->m_UsingMapItems - 1, pSortionData->m_NumTranslateItems);// no -1 if you dont need last spot

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

		delete[] aSortItems;
	}

	pSortionData->m_Working = false;
}

void CTranslator::UpdateMapInfos()
{
	if (m_aNumItems != 0x0)
		delete[] m_aNumItems;

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
		delete[] m_Sortions[ClientID].m_aTranslateItems;
	m_Sortions[ClientID].m_aTranslateItems = new CTranslateItem[m_aNumItems[MapIndex]];
	Server()->GetGameMap(MapIndex)->FillTranslateItems(m_Sortions[ClientID].m_aTranslateItems);
	m_Sortions[ClientID].m_ClientID = ClientID;
	m_Sortions[ClientID].m_Pos = Server()->GetGameMap(MapIndex)->GetPlayerViewPos(ClientID);

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
	if (ClientID < 0)
		return -1;
	int Using = Server()->UsingMapItems(For);
	for (int i = 0; i < Using; i++)
		if (m_aIDMap[For * MAX_CLIENTS + i] == ClientID)
			return i;
	return -1;
}

int CTranslator::ReverseTranslate(int For, int ClientID)
{
	int Using = Server()->UsingMapItems(For);
	if (ClientID < 0 || ClientID >= Using)
		return -1;
	return m_aIDMap[For * MAX_CLIENTS + ClientID];
}