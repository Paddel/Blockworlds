
#include <game/server/gamecontext.h>

#include "eventhandler.h"

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
CEventHandler::CEventHandler()
{
	Clear();
}

void *CEventHandler::Create(int Type, int Size, int Mask)
{
	if(m_NumEvents == MAX_EVENTS)
		return 0;
	if(m_CurrentOffset+Size >= MAX_DATASIZE)
		return 0;

	void *p = &m_aData[m_CurrentOffset];
	m_aOffsets[m_NumEvents] = m_CurrentOffset;
	m_aTypes[m_NumEvents] = Type;
	m_aSizes[m_NumEvents] = Size;
	m_aClientMasks[m_NumEvents] = Mask;
	m_CurrentOffset += Size;
	m_NumEvents++;
	return p;
}

void CEventHandler::Clear()
{
	m_NumEvents = 0;
	m_CurrentOffset = 0;
}

void CEventHandler::Snap(int SnappingClient)
{
	for(int i = 0; i < m_NumEvents; i++)
	{
		if(CmaskIsSet(m_aClientMasks[i], SnappingClient))
		{
			CNetEvent_Common *ev = (CNetEvent_Common *)&m_aData[m_aOffsets[i]];
			if(distance(m_pGameServer->m_apPlayers[SnappingClient]->m_ViewPos, vec2(ev->m_X, ev->m_Y)) < 1500.0f)
			{
				void *d = m_pGameServer->Server()->SnapNewItem(m_aTypes[i], i, m_aSizes[i]);
				if(d)
					mem_copy(d, &m_aData[m_aOffsets[i]], m_aSizes[i]);
			}
		}
	}
}
