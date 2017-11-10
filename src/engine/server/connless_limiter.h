#pragma once

#include <engine/shared/database.h>

class CConnlessLimiter
{
private:
	CDatabase m_Database;
	class CServer *m_pServer;
	int m_InquiriesPerSecond;
	int64 m_LastMesurement;

	int64 m_FloodDetectionTime;

	static void ResultAddrCheck(void *pQueryData, bool Error, void *pUserData);

public:
	CConnlessLimiter();
	void Init(class CServer *pServer);
	void Tick();

	bool AllowInfo(const NETADDR *pAddr, int Token, class CMap *pMap, NETSOCKET Socket, bool Info64, int Offset);
};