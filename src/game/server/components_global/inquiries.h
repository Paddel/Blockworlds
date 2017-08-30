#pragma once

#include <game/server/component.h>

#define INQUIERY_MAX_OPTIONS 8

class CGameContext;
class IServer;

#define INQUIERY_DATA_SIZE 512

class CInquiery
{
	friend class CInquieriesHandler;
	typedef void(*ResultFunction)(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);

private:
	int m_TimeLimit;
	char m_aaOptions[INQUIERY_MAX_OPTIONS][16];
	int m_NumOptions;
	ResultFunction m_fResultCallback;
	unsigned char m_aData[INQUIERY_DATA_SIZE];

public:
	CInquiery(ResultFunction fResultCallback, int TimeLimit, const unsigned char *pData);
	void AddOption(const char *pText);
};

class CInquieriesHandler : public CComponentGlobal
{
private:
	CInquiery *m_apInquieries[MAX_CLIENTS];

public:
	CInquieriesHandler();

	virtual void Tick();

	bool NewInquieryPossible(int ClientID);
	void NewInquiery(int ClientID, CInquiery *pInquiery, const char *pText);
	bool OnChatAnswer(int ClientID, const char *pText);
};