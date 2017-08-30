

#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

#include <game/server/component.h>

//
class CEventHandler : public CComponentMap
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*64;

	int m_aTypes[MAX_EVENTS]; // TODO: remove some of these arrays
	int m_aOffsets[MAX_EVENTS];
	int m_aSizes[MAX_EVENTS];
	int m_aClientMasks[MAX_EVENTS];
	char m_aData[MAX_DATASIZE];

	int m_CurrentOffset;
	int m_NumEvents;
public:

	CEventHandler();
	void *Create(int Type, int Size, int Mask = -1);
	void Clear();
	virtual void Snap(int SnappingClient);
};

#endif
