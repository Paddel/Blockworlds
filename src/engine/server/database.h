#pragma once

#include <base/tl/array.h>

#define QUERY_MAX_STR_LEN 512
#define QUERY_MAX_RESULT_LEN 256

class CDatabase
{
public:
	typedef void(*ResultFunction)(void *pQueryData, bool Error, void *pUserData);

	/*struct CResultField
	{
		char *m_pValue;
	};*/

	struct CResultRow
	{
		array<char *>m_lpResultFields;
	};

	struct CQueryData
	{
		//class writes
		char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
		char m_aCommand[512];
		ResultFunction m_fResultCallback;
		void *m_pUserData;

		//thread writes
		bool m_Error;
		array<CResultRow *>m_lpResultRows;

		//both write
		bool m_Working;
	};

private:
	char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
	bool m_Connected;
	int m_ReconnectVal;
	array <CQueryData *>m_ThreadData;

	static void QueryTestConnection(void *pData);
	bool InitConnection(const char *pAddr, const char *pUserName, const char *pPass, const char *pSchema);

	void CheckReconnect();

	static void ExecuteQuery(void *pData);
	static void QueryThreadFunction(void *pData);

public:
	CDatabase();

	void Init(const char *pAddr, const char *pUserName, const char *pPass, const char *pTable);
	void Tick();

	void QueryThread(const char *command, ResultFunction fResultCallback, void *pUserData);
	int Query(const char *pCommand, ResultFunction fResultCallback, void *pUserData);

	static void PreventInjectionAppend(char *pDst, const char *pStr, int DstSize);
	static void AddQueryStr(char *pDst, const char *pStr, int DstSize);
	static void AddQueryInt(char *pDst, int Val, int DstSize);

	static const char *GetDatabaseValue(char *pStr);

	static void Reconnect();
	static void DeleteThreadData(CQueryData *pData);


	bool GetConnected() const { return m_Connected; }
};