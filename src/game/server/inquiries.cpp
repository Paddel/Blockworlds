
#include <base/system.h>
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>

#include "inquiries.h"

CInquiery::CInquiery(ResultFunction fResultCallback, int TimeLimit, const unsigned char *pData)
{
	m_fResultCallback = fResultCallback;
	m_TimeLimit = TimeLimit;
	if(pData != 0x0)
		mem_copy(m_aData, pData, sizeof(m_aData));
	m_NumOptions = 0;
}

void CInquiery::AddOption(const char *pText)
{
	str_copy(m_aaOptions[m_NumOptions], pText, sizeof(m_aaOptions[m_NumOptions]));
	m_NumOptions++;
}

CInquieriesHandler::CInquieriesHandler()
{
	mem_zero(m_apInquieries, sizeof(m_apInquieries));
}

void CInquieriesHandler::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
}

void CInquieriesHandler::Tick()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false)
			continue;

		if (m_apInquieries[i] != 0x0 && Server()->Tick() > m_apInquieries[i]->m_TimeLimit)
		{
			m_apInquieries[i]->m_fResultCallback(-1, m_apInquieries[i]->m_aData, i, m_pGameServer);
			delete m_apInquieries[i];
			m_apInquieries[i] = 0x0;
		}
	}
}

bool CInquieriesHandler::NewInquieryPossible(int ClientID)
{
	return m_apInquieries[ClientID] == 0x0;
}

void CInquieriesHandler::NewInquiery(int ClientID, CInquiery *pInquiery, const char *pText)
{
	if (NewInquieryPossible(ClientID) == false || pInquiery->m_NumOptions == 0)
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s (", pText);
	for (int i = 0; i < pInquiery->m_NumOptions; i++)
		str_fcat(aBuf, sizeof(aBuf), "/%s, ", pInquiery->m_aaOptions[i]);
	aBuf[str_length(aBuf) - 2] = '\0';
	str_append(aBuf, ")", sizeof(aBuf));
	GameServer()->SendChatTarget(ClientID, aBuf);

	m_apInquieries[ClientID] = pInquiery;
}

bool CInquieriesHandler::OnChatAnswer(int ClientID, const char *pText)
{
	if (m_apInquieries[ClientID] == 0x0)
		return false;

	for (int i = 0; i < m_apInquieries[ClientID]->m_NumOptions; i++)
	{
		if (str_comp_nocase(m_apInquieries[ClientID]->m_aaOptions[i], pText) == 0)
		{
			m_apInquieries[ClientID]->m_fResultCallback(i, m_apInquieries[ClientID]->m_aData, ClientID, m_pGameServer);
			delete m_apInquieries[ClientID];
			m_apInquieries[ClientID] = 0x0;
			return true;
		}
	}

	return false;
}