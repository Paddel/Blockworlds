
#include <base/math.h>
#include <engine/shared/config.h>

#include "performance.h"

CStatisticsPerformance::CStatisticsPerformance()
{
	m_GameTickSpeed = 0;

	m_CurTicks = 0;
	m_ResultTimer = 0;
	m_LastTickResult = -1;
	m_CurTickTesting = 0;

	m_WarningCPU = -1;
	m_WarningRAM = -1;

	m_AutothrottleSleep = 0;
	mem_zero(&m_TickResults, sizeof(m_TickResults));
	m_CurTickTesting = -1;
}

void CStatisticsPerformance::Init(int GameTickSpeed)
{
	m_GameTickSpeed = GameTickSpeed;
	m_HighestRam = mem_stats()->allocated;//Baseline
	m_CurTicks = 1000;
}

void CStatisticsPerformance::SetTesting()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_TickResults[i] != 0)
			continue;
		m_CurTickTesting = i;
		break;
	}

	if (m_CurTickTesting == 1)
		m_AutothrottleSleep++;
	else if (m_CurTickTesting == 2)
		m_AutothrottleSleep--;
}

void CStatisticsPerformance::DoAutoThrottle()
{
	int WantedTicks = max(g_Config.m_SrvAutoThrottle, m_GameTickSpeed + 20);

	if (m_CurTickTesting != -1)
	{
		if (m_TickResults[m_CurTickTesting] == 0)
			m_TickResults[m_CurTickTesting] = m_CurTicks;

		if (m_CurTickTesting == 1)
			m_AutothrottleSleep--;
		else if (m_CurTickTesting == 2)
			m_AutothrottleSleep++;

		m_CurTickTesting = -1;

		SetTesting();
	}
	else
		SetTesting();

	//finished testing
	if (m_TickResults[0] && m_TickResults[1] && m_TickResults[2])
	{
		bool Retest = false;
		if (m_TickResults[0] < m_TickResults[1] || m_TickResults[0] > m_TickResults[2])
			Retest = true; // impossible results
		else
		{
			if (m_CurTicks >= WantedTicks)
			{
				if (m_CurTicks > m_TickResults[2] || m_TickResults[1] > WantedTicks)
				{
					m_AutothrottleSleep++;
					Retest = true;
				}
			}
			else if (m_AutothrottleSleep > 0)
			{
				m_AutothrottleSleep--;
				Retest = true;
			}
		}

		//panic
		/*if (m_CurTicks <= m_GameTickSpeed)
		{
			m_AutothrottleSleep = 0;
			Retest = true;
		}*/

		if(Retest)
			mem_zero(&m_TickResults, sizeof(m_TickResults));
	}
}

void CStatisticsPerformance::OnTick()
{
	m_CurTicks++;

	if (m_ResultTimer < time_get())
	{
		m_ResultTimer = time_get() + time_freq();

		if (m_CurTicks < m_GameTickSpeed)
			m_WarningCPU = m_CurTicks;

		if (g_Config.m_SrvAutoThrottle > 0)
			DoAutoThrottle();

		m_LastTickResult = m_CurTicks;
		m_CurTicks = 0;
	}

	if (g_Config.m_SrvAutoThrottle > 0 && m_AutothrottleSleep > 0)
		thread_sleep(m_AutothrottleSleep);

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