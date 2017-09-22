#pragma once

#include <base/system.h>

class CStatisticsPerformance
{
private:
	int m_GameTickSpeed;

	int m_CurTicks;
	int64 m_ResultTimer;
	int m_LastTickResult;

	int64 m_HighestRam;

	int m_WarningCPU;
	int m_WarningRAM;

	int m_AutothrottleSleep;
	
	int m_TickResults[3];//[0] = wanted (best over wanted throttle), [1] = wanted - 1, [2] = wanted + 1
	int m_CurTickTesting;

	void SetTesting();
	void DoAutoThrottle();
public:
	CStatisticsPerformance();

	void Init(int GameTickSpeed);
	void OnTick();

	int AdminWarningCPU();
	int AdminWarningRAM();

	int LastTickResult() const { return m_LastTickResult; }

};