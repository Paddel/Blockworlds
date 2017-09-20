#pragma once

#include <base/system.h>

class CStatisticsPerformance
{
private:
	//int64 m_BaselineMem;
	int m_GameTickSpeed;

	int m_CurTicks;
	int64 m_ResultTimer;
	int m_LastTickResult;

	int64 m_LastMemState;
	int64 m_HighestRam;

	int m_WarningCPU;
	int m_WarningRAM;
public:
	CStatisticsPerformance();

	void Init(int GameTickSpeed);
	void OnTick();

	int AdminWarningCPU();
	int AdminWarningRAM();

	int LastTickResult() const { return m_LastTickResult; }

};