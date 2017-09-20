
#include "statistics_performance.h"

CStatisticsPerformance::CStatisticsPerformance()
{
	//m_BaselineMem = 0;
	m_GameTickSpeed = 0;

	m_CurTicks = -1;
	m_ResultTimer = 0;
	m_LastTickResult = -1;

	m_WarningCPU = -1;
	m_WarningRAM = -1;
}

void CStatisticsPerformance::Init(int GameTickSpeed)
{
	//m_BaselineMem = mem_stats()->allocated;
	m_GameTickSpeed = GameTickSpeed;
}

void CStatisticsPerformance::OnTick()
{
	m_CurTicks++;

	if (m_ResultTimer < time_get())
	{
		m_ResultTimer = time_get() + time_freq();
		m_LastTickResult = m_CurTicks;

		if (m_CurTicks < m_GameTickSpeed)
			m_WarningCPU = m_CurTicks;

		m_CurTicks = 0;
	}

	//int NewAllocations = m_LastMemState - mem_stats()->allocated;
	m_LastMemState = mem_stats()->allocated;

	if (mem_stats()->allocated > m_HighestRam)
	{
		
		float GB = (mem_stats()->allocated - m_HighestRam) / 1024.0f / 1024.0f;
		if (GB > 50.0f)//every 50 mb a warning
		{
			m_WarningRAM = mem_stats()->allocated / 1024 / 1024;
			m_HighestRam = mem_stats()->allocated;
		}
	}
}

int CStatisticsPerformance::AdminWarningCPU()
{
	int Result = m_WarningCPU;
	if (m_WarningCPU != -1)
		m_WarningCPU = -1;
	return Result;
}

int CStatisticsPerformance::AdminWarningRAM()
{
	int Result = m_WarningRAM;
	if (m_WarningRAM != -1)
		m_WarningRAM = -1;
	return Result;
}