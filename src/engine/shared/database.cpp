
#include <base/system.h>
#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
#include "winsock2.h"
#endif

#include <mysql.h>

#include "database.h"

#define MAX_THREADS 2000
#define USE_LOCK 0

static LOCK s_QueryLock = 0x0;
static int s_ReconnectVal = 0;

void CDatabase::ExecuteQuery(void *pData)
{
	MYSQL_RES *pResult = 0x0;
	MYSQL *pConn = 0x0;
	MYSQL_ROW Field;
	CQueryData *pThreadData = (CQueryData *)pData;

	pThreadData->m_Error = false;

	pConn = mysql_init(0x0);
	if (pConn == 0x0)
	{
		if (g_Config.m_DbgDatabase)
			dbg_msg("Database", "Initialing connection failed: '%s'", mysql_error(pConn));
		pThreadData->m_Error = true;
		pThreadData->m_Working = false;
		return;
	}

	mysql_options(pConn, MYSQL_SET_CHARSET_NAME, "utf8");

	if (mysql_real_connect(pConn, pThreadData->m_aAddr, pThreadData->m_aUserName, pThreadData->m_aPass, pThreadData->m_aSchema, 0, 0x0, 0) == 0x0)
	{
		if (g_Config.m_DbgDatabase)
			dbg_msg("Database", "Connecting failed: '%s'", mysql_error(pConn));
		pThreadData->m_Error = true;
		pThreadData->m_Working = false;
		mysql_close(pConn);
        return;
	}

	mysql_query(pConn, pThreadData->m_aCommand);//Main-cmd
	if(g_Config.m_DbgDatabase)
		dbg_msg("Database", pThreadData->m_aCommand);

	pThreadData->m_AffectedRows = mysql_affected_rows(pConn);

	{//error
		int err = mysql_errno(pConn);
		if(err)
		{
			if (g_Config.m_DbgDatabase)
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

	//pResult = mysql_store_result(pConn);

	//if (!pResult)//no Results. DONE
	//{
	//	mysql_close(pConn);
	//	pThreadData->m_Working = false;
	//	return;
	//}

	/*int count = (int)pResult->row_count;
	for (int i = 0; i < count; i++)
	{
		CResultRow *pNewRow = new CResultRow();

		Field = mysql_fetch_row(pResult);
		int affected = mysql_num_fields(pResult);

		for (int x = 0; x < affected; x++)
		{
			const char *pResult = CDatabase::GetDatabaseValue(Field[x]);
			int Length = str_length(pResult);
			char *pInsertion = new char[Length + 1];
			str_copy(pInsertion, pResult, Length + 1);
			pNewRow->m_lpResultFields.add(pInsertion);
		}

		pThreadData->m_lpResultRows.add(pNewRow);
	}*/
	mysql_close(pConn);

	pThreadData->m_Working = false;
}

void CDatabase::QueryThreadFunction(void *pData)
{
	CQueryData *pThreadData = (CQueryData *)pData;

	if(USE_LOCK)
		lock_wait(s_QueryLock);
	ExecuteQuery(pThreadData);
	if(USE_LOCK)
		lock_unlock(s_QueryLock);
}

void CDatabase::QueryTestConnection(void *pData)
{
	CDatabase *pThis = (CDatabase *)pData;

	//if (USE_LOCK)
		lock_wait(s_QueryLock);

	MYSQL *pConn = mysql_init(0x0);

	if (pConn == 0x0)
	{
		dbg_msg("Database", "Initialing connection to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		//if (USE_LOCK)
			lock_unlock(s_QueryLock);
		return;
	}

	if (mysql_real_connect(pConn, pThis->m_aAddr, pThis->m_aUserName, pThis->m_aPass, pThis->m_aSchema, 0, 0x0, 0) == 0x0)
	{
		dbg_msg("Database", "Connecting to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		mysql_close(pConn);
		//if (USE_LOCK)
			lock_unlock(s_QueryLock);
		return;
	}


	dbg_msg("Database", "Connected to %s*%s", pThis->m_aAddr, pThis->m_aSchema);
	pThis->m_Connected = true;
	mysql_close(pConn);

	//if (USE_LOCK)
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

	m_CreateTables = 0;
}

void CDatabase::Init(const char *pAddr, const char *pUserName, const char *pPass, const char *pSchema)
{
	if (s_QueryLock == 0x0)
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
	if (m_CreateTables == 0)
	{
		if (m_Connected)
			m_CreateTables = 1;
	}

	for (int i = 0; i < m_ThreadData.size(); i++)
	{
		CQueryData *pData = m_ThreadData[i]; 
		if (pData->m_Working == true)
			continue;

		if (pData->m_fResultCallback)
			pData->m_fResultCallback(pData, pData->m_Error, pData->m_pUserData);

		thread_detach(pData->m_pThread);
		DeleteThreadData(pData);
		m_ThreadData.remove_index(i);
		i--;
	}
}

void CDatabase::CheckReconnect()
{
	if (m_ReconnectVal == s_ReconnectVal)
		return;

	thread_init(QueryTestConnection, this);
	m_CreateTables = 0;
	m_ReconnectVal = s_ReconnectVal;
}

void CDatabase::Query(const char *pCommand, ResultFunction fResultCallback, void *pUserData)
{
	CheckReconnect();

	if (m_Connected == false)
	{
		if(fResultCallback != 0x0)
			fResultCallback(0x0, true, pUserData);
		return;
	}

	if (m_ThreadData.size() >= MAX_THREADS)
	{
		if (fResultCallback != 0x0)
			fResultCallback(0x0, true, pUserData);

		dbg_msg("Database", "ERROR: Maximum number of threads reachead %i", (int) MAX_THREADS);
		return;
	}

	CQueryData *pThreadData = new CQueryData();
	pThreadData->m_fResultCallback = fResultCallback;
	pThreadData->m_pUserData = pUserData;
	str_copy(pThreadData->m_aCommand, pCommand, sizeof(pThreadData->m_aCommand));
	str_copy(pThreadData->m_aAddr, m_aAddr, sizeof(pThreadData->m_aAddr));
	str_copy(pThreadData->m_aUserName, m_aUserName, sizeof(pThreadData->m_aUserName));
	str_copy(pThreadData->m_aPass, m_aPass, sizeof(pThreadData->m_aPass));
	str_copy(pThreadData->m_aSchema, m_aSchema, sizeof(pThreadData->m_aSchema));
	pThreadData->m_Working = true;

	pThreadData->m_pThread = thread_init(QueryThreadFunction, pThreadData);
	m_ThreadData.add(pThreadData);
	//thread_detach(pThread);
}

void CDatabase::QueryOrderly(const char *pCommand, ResultFunction fResultCallback, void *pUserData)
{
	if (m_Connected == false)
	{
		if (fResultCallback != 0x0)
			fResultCallback(0x0, true, pUserData);
		return;
	}

	CQueryData *pThreadData = new CQueryData();
	pThreadData->m_fResultCallback = fResultCallback;
	pThreadData->m_pUserData = pUserData;
	str_copy(pThreadData->m_aCommand, pCommand, sizeof(pThreadData->m_aCommand));
	str_copy(pThreadData->m_aAddr, m_aAddr, sizeof(pThreadData->m_aAddr));
	str_copy(pThreadData->m_aUserName, m_aUserName, sizeof(pThreadData->m_aUserName));
	str_copy(pThreadData->m_aPass, m_aPass, sizeof(pThreadData->m_aPass));
	str_copy(pThreadData->m_aSchema, m_aSchema, sizeof(pThreadData->m_aSchema));
	pThreadData->m_Working = true;

	ExecuteQuery(pThreadData);

	if(fResultCallback)
		fResultCallback(pThreadData, pThreadData->m_Error, pThreadData->m_pUserData);

	DeleteThreadData(pThreadData);
}

int CDatabase::NumRunningThreads()
{
	return m_ThreadData.size();
}

bool CDatabase::CreateTables()
{
	if (m_CreateTables == 1)
	{
		m_CreateTables = 2;
		return true;
	}
	return false;
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
		if (DstPos >= DstSize - 2)
		{
			if(g_Config.m_DbgDatabase == 1)
				dbg_msg("Database", "Error preventen injection Destination=%s/%i Source=%s/%i", pDst, DstSize, pStr, Len);
			break;
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

void CDatabase::DeleteThreadData(CQueryData *pData)
{
	for (int i = 0; i < pData->m_lpResultRows.size(); i++)
		pData->m_lpResultRows[i]->m_lpResultFields.delete_all();
	pData->m_lpResultRows.delete_all();

	delete pData;
}