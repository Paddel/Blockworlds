#pragma once

#include <engine/shared/database.h>

static const unsigned char EXINFO_ACTIVATE[] = { 255, 255, 255, 255, 'e', 'x', 'i', 'a' };
static const unsigned char EXINFO_DEACTIVATE[] = { 255, 255, 255, 255, 'e', 'x', 'i', 'd' };
static const unsigned char EXINFO_INFO[] = { 255, 255, 255, 255, 'e', 'x', 'i', 'i' };
static const unsigned char EXINFO_INFO64[] = { 255, 255, 255, 255, 'e', 'x', 'i', 'e' };

class CConnlessLimiter
{

private:
	CDatabase m_Database;
	class CServer *m_pServer;
	int m_InquiriesPerSecond;
	int64 m_LastMesurement;
	int64 m_FloodDetectionTime;
	int m_LastUnfilteredResult;
	int m_LastFilteredResult;
	int m_FilteredInquiries;
	
	int64 m_LastExtInfoSent;
	int64 m_ExternInfoTime;
	NETADDR m_ExternalAddr;
	NETSOCKET m_Socket;

	static void ResultAddrCheck(void *pQueryData, bool Error, void *pUserData);

public:
	CConnlessLimiter();
	void Init(class CServer *pServer);
	void Tick();

	bool AllowInfo(const NETADDR *pAddr, int Token, class CMap *pMap, NETSOCKET Socket, bool Info64, int Offset);
	bool FilterActive();
	bool ExternInfoActive();

	int LastUnfilteredResult() const { return m_LastUnfilteredResult; }
	int LastFilteredResult() const { return m_LastFilteredResult; }
};