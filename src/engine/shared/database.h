#pragma once

#include <base/tl/array.h>

#define QUERY_MAX_STR_LEN 1024
#define QUERY_MAX_RESULT_LEN 256

class CDatabase
{
	public:
		typedef void(*ResultFunction)(void *pQueryData, bool Error, void *pUserData);
		
		struct CResultRow
		{
			array<char *>m_lpResultFields;
		};
		
		struct CQueryData
		{
			//class writes
			char m_aAddr[48]; char m_aUserName[16]; char m_aPass[16]; char m_aSchema[16];
			char m_aCommand[QUERY_MAX_STR_LEN];
			ResultFunction m_fResultCallback;
			void *m_pUserData;
			void *m_pThread;
			
			//thread writes
			bool m_Error;
			array<CResultRow *>m_lpResultRows;
			int64 m_AffectedRows;
			
			//both write
			bool m_Working;
		};
	
	private:
		char m_aAddr[48]; char m_aUserName[16]; char m_aPass[16]; char m_aSchema[16];
		bool m_Connected;
		int m_ReconnectVal;
		int m_CreateTables;
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
		
		void Query(const char *command, ResultFunction fResultCallback, void *pUserData);
		void QueryOrderly(const char *pCommand, ResultFunction fResultCallback, void *pUserData);

		int NumRunningThreads();

		bool CreateTables();
		
		static const char *GetDatabaseValue(char *pStr);

		static void PreventInjectionAppend(char *pDst, const char *pStr, int DstSize);
		static void AddQueryStr(char *pDst, const char *pStr, int DstSize);
		static void AddQueryInt(char *pDst, int Val, int DstSize);
		
		static void Reconnect();
		static void DeleteThreadData(CQueryData *pData);

		bool GetConnected() const { return m_Connected; }
};