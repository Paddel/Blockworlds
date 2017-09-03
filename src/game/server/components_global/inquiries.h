#pragma once

#include <game/server/component.h>

#define INQUIRY_MAX_OPTIONS 8

class CGameContext;
class IServer;

#define INQUIRY_DATA_SIZE 512

class CInquiry
{
	friend class CInquiriesHandler;
	typedef void(*ResultFunction)(int OptionID, const unsigned char *pData, int ClientID, CGameContext *pGameServer);

private:
	int m_TimeLimit;
	char m_aaOptions[INQUIRY_MAX_OPTIONS][16];
	int m_NumOptions;
	ResultFunction m_fResultCallback;
	unsigned char m_aData[INQUIRY_DATA_SIZE];

public:
	CInquiry(ResultFunction fResultCallback, int TimeLimit, const unsigned char *pData);
	void AddOption(const char *pText);
};

class CInquiriesHandler : public CComponentGlobal
{
private:
	CInquiry *m_apInquiries[MAX_CLIENTS];

public:
	CInquiriesHandler();

	virtual void Tick();

	bool NewInquiryPossible(int ClientID);
	void NewInquiry(int ClientID, CInquiry *pInquiry, const char *pText);
	bool OnChatAnswer(int ClientID, const char *pText);
	void Clear(int ClientID);
};