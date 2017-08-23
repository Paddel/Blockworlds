
#include <base/system.h>
#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
#include "winsock2.h"
#endif

#include <mysql.h>

#include "database.h"

static LOCK s_QueryLock = 0x0;
static int s_ReconnectVal = 0;

void CDatabase::ExecuteQuery(void *pData)
{
	MYSQL_RES *pResult = 0x0;
	MYSQL *pConn = 0x0;
	CThreadData *pThreadData = (CThreadData *)pData;

	pThreadData->m_pResultData = 0x0;
	pThreadData->m_Error = false;

	pConn = mysql_init(0x0);
	if (pConn == 0x0)
	{
		if (g_Config.m_Debug)
			dbg_msg("Database", "Initialing connection failed: '%s'", mysql_error(pConn));
		pThreadData->m_Error = true;
		pThreadData->m_Working = false;
		return;
	}

	mysql_options(pConn, MYSQL_SET_CHARSET_NAME, "utf8");

	if (mysql_real_connect(pConn, pThreadData->m_aAddr, pThreadData->m_aUserName, pThreadData->m_aPass, pThreadData->m_aSchema, 0, 0x0, 0) == 0x0)
	{
		if (g_Config.m_Debug)
			dbg_msg("Database", "Connecting failed: '%s'", mysql_error(pConn));
		pThreadData->m_Error = true;
		pThreadData->m_Working = false;
        return;
	}

	mysql_query(pConn, pThreadData->m_aCommand);//Main-cmd
	if(g_Config.m_Debug)
		dbg_msg("Database", pThreadData->m_aCommand);

	{//error
		int err = mysql_errno(pConn);
		if(err)
		{
			if (g_Config.m_Debug)
				dbg_msg("Database", "Query failed: '%s", mysql_error(pConn));
			mysql_close(pConn);
			pThreadData->m_Error = true;
			pThreadData->m_Working = false;
			return;
		}
	}

	if(pThreadData->m_fResultCallback == 0x0)
	{
		mysql_close(pConn);
		pThreadData->m_Working = false;
		return;
	}

	pResult = mysql_store_result(pConn);

	if(!pResult)//no Results. DONE
	{
		//*pFeed->m_pResult = -1;
		mysql_close(pConn);
		pThreadData->m_Error = true;
		pThreadData->m_Working = false;
		return;
	}

	MYSQL_RES *pNewResult = new MYSQL_RES();
	mem_copy(pNewResult, pResult, sizeof(MYSQL_RES));
	pThreadData->m_pResultData = pNewResult;

	mysql_close(pConn);

	pThreadData->m_Working = false;
}

void CDatabase::QueryThreadFunction(void *pData)
{
	CThreadData *pThreadData = (CThreadData *)pData;

	lock_wait(s_QueryLock);
	ExecuteQuery(pThreadData);
	lock_unlock(s_QueryLock);
}

void CDatabase::QueryTestConnection(void *pData)
{
	CDatabase *pThis = (CDatabase *)pData;

	lock_wait(s_QueryLock);

	MYSQL *pConn = mysql_init(0x0);

	if (pConn == 0x0)
	{
		dbg_msg("Database", "Initialing connection to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		lock_unlock(s_QueryLock);
		return;
	}

	if (mysql_real_connect(pConn, pThis->m_aAddr, pThis->m_aUserName, pThis->m_aPass, pThis->m_aSchema, 0, NULL, 0) == NULL)
	{
		dbg_msg("Database", "Connecting to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		lock_unlock(s_QueryLock);
		return;
	}


	dbg_msg("Database", "Connected to %s*%s", pThis->m_aAddr, pThis->m_aSchema);
	pThis->m_Connected = true;
	mysql_close(pConn);

	lock_unlock(s_QueryLock);
}

CDatabase::CDatabase()
{
	m_Connected = false;
	m_ReconnectVal = false;
	mem_zero(m_aAddr, sizeof(m_aAddr));
	mem_zero(m_aUserName, sizeof(m_aUserName));
	mem_zero(m_aPass, sizeof(m_aPass));
	mem_zero(m_aSchema, sizeof(m_aSchema));
}

void CDatabase::Init(const char *pAddr, const char *pUserName, const char *pPass, const char *pSchema)
{
	if (s_QueryLock == NULL)
		s_QueryLock = lock_create();

	InitConnection(pAddr, pUserName, pPass, pSchema);
}


bool CDatabase::InitConnection(const char *pAddr, const char *pUserName, const char *pPass, const char *pSchema)
{
	str_copy(m_aAddr, pAddr, sizeof(m_aAddr));
	str_copy(m_aUserName, pUserName, sizeof(m_aUserName));
	str_copy(m_aPass, pPass, sizeof(m_aPass));
	str_copy(m_aSchema, pSchema, sizeof(m_aSchema));

	thread_init(QueryTestConnection, this);
	return true;
}

void CDatabase::Tick()
{
	for (int i = 0; i < m_ThreadData.size(); i++)
	{
		CThreadData *pData = m_ThreadData[i];
		if (pData->m_Working == true)
			continue;

		if (pData->m_fResultCallback)
			pData->m_fResultCallback(pData->m_pResultData, pData->m_Error, pData->m_pUserData);

		DeleteThreadData(m_ThreadData[i]);
		m_ThreadData.remove_index(i);
	}
}

void CDatabase::CheckReconnect()
{
	if (m_ReconnectVal == s_ReconnectVal)
		return;

	thread_init(QueryTestConnection, this);

	m_ReconnectVal = s_ReconnectVal;
}

void CDatabase::QueryThread(char *pCommand, ResultFunction fResultCallback, void *pUserData)
{
	CheckReconnect();

	if (m_Connected == false)
	{
		if(fResultCallback != 0x0)
			fResultCallback(0x0, true, pUserData);
		return;
	}

	CThreadData *pThreadData = new CThreadData();
	pThreadData->m_fResultCallback = fResultCallback;
	pThreadData->m_pUserData = pUserData;
	str_copy(pThreadData->m_aCommand, pCommand, sizeof(pThreadData->m_aCommand));
	str_copy(pThreadData->m_aAddr, m_aAddr, sizeof(pThreadData->m_aAddr));
	str_copy(pThreadData->m_aUserName, m_aUserName, sizeof(pThreadData->m_aUserName));
	str_copy(pThreadData->m_aPass, m_aPass, sizeof(pThreadData->m_aPass));
	str_copy(pThreadData->m_aSchema, m_aSchema, sizeof(pThreadData->m_aSchema));
	pThreadData->m_Working = true;

	m_ThreadData.add(pThreadData);
	
	//QueryThreadFunction(pThreadData);
	thread_init(QueryThreadFunction, pThreadData);
}

int CDatabase::Query(char *pCommand, ResultFunction fResultCallback, void *pUserData)
{
	CheckReconnect();

	if(m_Connected == false)
		return -1;

	int Result = 0;

	/*CThreadFeed Feed;
	Feed.ResultCallback = ResultCallback;
	Feed.pResultData = pData;
	str_copy(Feed.m_aCommand, command, sizeof(Feed.m_aCommand));
	Feed.m_pResult = &Result;
	str_copy(Feed.m_aAddr, m_aAddr, sizeof(Feed.m_aAddr));
	str_copy(Feed.m_aUserName, m_aUserName, sizeof(Feed.m_aUserName));
	str_copy(Feed.m_aPass, m_aPass, sizeof(Feed.m_aPass));
	str_copy(Feed.m_aSchema, m_aSchema, sizeof(Feed.m_aSchema));

	ExecuteQuery(&Feed);*/
	return Result;
}

const char *CDatabase::GetDatabaseValue(char *pStr)
{
	if(pStr == 0)
		return "";
	return pStr;
}

void CDatabase::PreventInjectionAppend(char *pDst, const char *pStr, int DstSize)
{
	int Len = str_length(pStr);
	int DstPos = str_length(pDst);
	for(int i = 0; i < Len; i++)
	{
		if(DstPos >= DstSize - 3)
		{
			dbg_msg(0, "size");
			return;
		}

		if(pStr[i] == '\\' || pStr[i] == '\'' || pStr[i] == '\"')
			pDst[DstPos++] = '\\';

		pDst[DstPos++] = pStr[i];
	}
	pDst[DstPos] = '\0';
}

void CDatabase::AddQueryStr(char *pDst, const char *pStr, int DstSize)
{
	str_append(pDst, "\'", DstSize);
	PreventInjectionAppend(pDst, pStr, DstSize);
	str_append(pDst, "\'", DstSize);
}

void CDatabase::AddQueryInt(char *pDst, int Val, int DstSize)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%i", Val);
	str_append(pDst, "\'", DstSize);
	str_append(pDst, aBuf, DstSize);
	str_append(pDst, "\'", DstSize);
}

void CDatabase::Reconnect()
{
	s_ReconnectVal++;
}

void CDatabase::DeleteThreadData(CThreadData *pData)
{
	if (pData->m_pResultData != 0x0)
		delete (MYSQL_RES *)pData->m_pResultData;
	
	delete pData;
}