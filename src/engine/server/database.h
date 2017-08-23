#pragma once

#include <base/tl/array.h>

#define QUERY_MAX_STR_LEN 512
#define QUERY_MAX_RESULT_LEN 256

class CDatabase
{
	typedef void(*ResultFunction)(void *pResultData, bool Error, void *pUserData);

	struct CThreadData
	{
		//class writes
		char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
		char m_aCommand[512];
		ResultFunction m_fResultCallback;
		void *m_pUserData;

		//thread writes
		void *m_pResultData;
		bool m_Error;

		//both write
		bool m_Working;
	};

private:
	char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
	bool m_Connected;
	int m_ReconnectVal;
	array <CThreadData *>m_ThreadData;

	static void QueryTestConnection(void *pData);
	bool InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema);

	void CheckReconnect();

	static void ExecuteQuery(void *pData);
	static void QueryThreadFunction(void *pData);

public:
	CDatabase();

	void Init(char *pAddr, char *pUserName, char *pPass, char *pTable);
	void Tick();

	void QueryThread(char *command, ResultFunction fResultCallback, void *pUserData);
	int Query(char *pCommand, ResultFunction fResultCallback, void *pUserData);

	static void PreventInjectionAppend(char *pDst, const char *pStr, int DstSize);
	static void AddQueryStr(char *pDst, const char *pStr, int DstSize);
	static void AddQueryInt(char *pDst, int Val, int DstSize);

	static char *GetDatabaseValue(char *pStr);

	static void Reconnect();
	static void DeleteThreadData(CThreadData *pData);


	bool GetConnected() const { return m_Connected; }
};