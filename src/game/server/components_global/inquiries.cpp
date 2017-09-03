
#include <base/system.h>
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>

#include "inquiries.h"

CInquiry::CInquiry(ResultFunction fResultCallback, int TimeLimit, const unsigned char *pData)
{
	m_fResultCallback = fResultCallback;
	m_TimeLimit = TimeLimit;
	if(pData != 0x0)
		mem_copy(m_aData, pData, sizeof(m_aData));
	m_NumOptions = 0;
}

void CInquiry::AddOption(const char *pText)
{
	str_copy(m_aaOptions[m_NumOptions], pText, sizeof(m_aaOptions[m_NumOptions]));
	m_NumOptions++;
}

CInquiriesHandler::CInquiriesHandler()
{
	mem_zero(m_apInquiries, sizeof(m_apInquiries));
}

void CInquiriesHandler::Tick()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false)
			continue;

		if (m_apInquiries[i] != 0x0 && Server()->Tick() > m_apInquiries[i]->m_TimeLimit)
		{
			m_apInquiries[i]->m_fResultCallback(-1, m_apInquiries[i]->m_aData, i, GameServer());
			delete m_apInquiries[i];
			m_apInquiries[i] = 0x0;
		}
	}
}

bool CInquiriesHandler::NewInquiryPossible(int ClientID)
{
	return m_apInquiries[ClientID] == 0x0;
}

void CInquiriesHandler::NewInquiry(int ClientID, CInquiry *pInquiry, const char *pText)
{
	if (NewInquiryPossible(ClientID) == false || pInquiry->m_NumOptions == 0)
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s (", pText);
	for (int i = 0; i < pInquiry->m_NumOptions; i++)
		str_fcat(aBuf, sizeof(aBuf), "/%s, ", pInquiry->m_aaOptions[i]);
	aBuf[str_length(aBuf) - 2] = '\0';
	str_append(aBuf, ")", sizeof(aBuf));
	GameServer()->SendChatTarget(ClientID, aBuf);

	m_apInquiries[ClientID] = pInquiry;
}

bool CInquiriesHandler::OnChatAnswer(int ClientID, const char *pText)
{
	if (m_apInquiries[ClientID] == 0x0)
		return false;

	for (int i = 0; i < m_apInquiries[ClientID]->m_NumOptions; i++)
	{
		if (str_comp_nocase(m_apInquiries[ClientID]->m_aaOptions[i], pText) == 0)
		{
			m_apInquiries[ClientID]->m_fResultCallback(i, m_apInquiries[ClientID]->m_aData, ClientID, GameServer());
			delete m_apInquiries[ClientID];
			m_apInquiries[ClientID] = 0x0;
			return true;
		}
	}

	return false;
}

void CInquiriesHandler::Clear(int ClientID)
{
	if (m_apInquiries[ClientID] != 0x0)
	{
		m_apInquiries[ClientID]->m_fResultCallback(-1, m_apInquiries[ClientID]->m_aData, ClientID, GameServer());
		delete m_apInquiries[ClientID];
		m_apInquiries[ClientID] = 0x0;
	}
}