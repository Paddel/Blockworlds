
#include <engine/shared/config.h>//remove
#include <engine/server.h>
#include <game/server/gamecontext.h>

#include "broadcast.h"

inline float GetCharSize(int c)
{
	switch (c)
	{
	case ' ': return 3.56f; case '!': return 4.54f; case '"': return 5.51f;
	case '#': return 9.72f; case '$': return 7.45f; case '%': return 11.02f;
	case '&': return 9.07f; case '\'': return 3.24f; case '(': return 4.54f;
	case ')': return 4.54f; case '*': return 5.83f; case '+': return 9.72f;
	case ',': return 3.56f; case '-': return 4.21f; case '.': return 3.56f;
	case '/': return 3.89f; case '0': return 7.45f; case '1': return 7.45f;
	case '2': return 7.45f; case '3': return 7.45f; case '4': return 7.45f;
	case '5': return 7.45f; case '6': return 7.45f; case '7': return 7.45f;
	case '8': return 7.45f; case '9': return 7.45f; case ':': return 3.89f;
	case ';': return 3.89f; case '<': return 9.72f; case '=': return 9.72f;
	case '>': return 9.72f; case '?': return 6.16f; case '@': return 11.67f;
	case 'A': return 8.10f; case 'B': return 8.10f; case 'C': return 8.10f;
	case 'D': return 9.07f; case 'E': return 7.45f; case 'F': return 6.81f;
	case 'G': return 9.07f; case 'H': return 8.75f; case 'I': return 3.56f;
	case 'J': return 3.56f; case 'K': return 7.78f; case 'L': return 6.48f;
	case 'M': return 10.05f; case 'N': return 8.75f; case 'O': return 9.07f;
	case 'Q': return 9.07f; case 'R': return 8.10f; case 'S': return 7.45f;
	case 'U': return 8.43f; case 'V': return 8.10f; case 'W': return 11.67f;
	case 'X': return 7.78f; case 'Z': return 8.10f; case '[': return 4.54f;
	case '\\': return 3.89f; case ']': return 4.54f;case '^': return 9.72f;
	case '_': return 5.83f; case '`': return 5.83f; case 'b': return 7.45f;
	case 'c': return 6.48f; case 'd': return 7.45f; case 'f': return 4.21f;
	case 'g': return 7.45f; case 'h': return 7.45f; case 'i': return 3.24f;
	case 'j': return 3.24f; case 'k': return 6.81f; case 'l': return 3.24f;
	case 'm': return 11.34f; case 'n': return 7.45f; case 'p': return 7.45f;
	case 'q': return 7.45f; case 'r': return 4.86f; case 's': return 6.16f;
	case 't': return 4.54f; case 'u': return 7.45f; case 'v': return 6.81f;
	case 'w': return 9.40f; case 'x': return 6.81f; case 'y': return 6.81f;
	case 'z': return 6.16f; case '{': return 7.45f; case '|': return 3.89f;
	case '}': return 7.45f; case '~': return 9.72f;
	}
	return 7.13f;
}

inline float CalcTextSize(const char *pText)
{
	int Length = str_length(pText);
	float Size = 0.0f;
	for (int i = 0; i < Length; i++)
		Size += GetCharSize(pText[i]);
	return Size;
}

CBroadcastHandler::CMainCast::~CMainCast()
{
	delete m_pText;
}

CBroadcastHandler::CBroadState::~CBroadState()
{
	m_lpSideCasts.delete_all();
	m_lpMainCasts.delete_all();
}

CBroadcastHandler::CBroadcastHandler()
{
	mem_zero(&m_pCurrentState, sizeof(m_pCurrentState));
	mem_zero(&m_pLastState, sizeof(m_pLastState));
	mem_zero(&m_LastUpdate, sizeof(m_LastUpdate));
}

void CBroadcastHandler::AddWhitespace(char *pDst, int Size, const char *pSrc)
{
	float TextWidth = CalcTextSize(pSrc);
	int Num = 65 - round_to_int((TextWidth / GetCharSize(' ') * 0.5f));
	for (int i = 0; i < Num; i++)
		str_fcat(pDst, Size, " ");
}

bool CBroadcastHandler::StateEmpty(CBroadState *pState)
{
	return pState->m_lpSideCasts.size() == 0 && pState->m_lpMainCasts.size() == 0;
}

bool CBroadcastHandler::NeedRefresh(int ClientID)
{
	if (m_pCurrentState[ClientID] == 0x0)
		return false;

	if (m_pLastState[ClientID] == 0x0)
		return true; //joined

	if (m_LastUpdate[ClientID] + Server()->TickSpeed() * 3.0f < Server()->Tick() &&
		StateEmpty(m_pCurrentState[ClientID]) == false && StateEmpty(m_pLastState[ClientID]) == false)
		return true;

	return false;
}

bool CBroadcastHandler::CompareStates(int ClientID)
{
	if (m_pCurrentState[ClientID] == 0x0 || m_pLastState[ClientID] == 0x0)
		return false;

	if (m_pCurrentState[ClientID]->m_lpSideCasts.size() != m_pLastState[ClientID]->m_lpSideCasts.size() ||
		m_pCurrentState[ClientID]->m_lpMainCasts.size() != m_pLastState[ClientID]->m_lpMainCasts.size())
		return true;

	for (int i = 0; i < m_pCurrentState[ClientID]->m_lpSideCasts.size(); i++)
		if (str_comp(m_pCurrentState[ClientID]->m_lpSideCasts[i], m_pLastState[ClientID]->m_lpSideCasts[i]) != 0)
			return true;

	for(int i = 0; i < m_pCurrentState[ClientID]->m_lpMainCasts.size(); i++)
		if (str_comp(m_pCurrentState[ClientID]->m_lpMainCasts[i]->m_pText, m_pLastState[ClientID]->m_lpMainCasts[i]->m_pText) != 0)
			return true;

	return false;
}

void CBroadcastHandler::UpdateClient(int ClientID)
{
	if (m_pCurrentState[ClientID] == 0x0)
		return;

	m_LastUpdate[ClientID] = Server()->Tick();
	char aText[1024];
	mem_zero(&aText, sizeof(aText));
	//build the text
	for (int i = 0; i < m_pCurrentState[ClientID]->m_lpSideCasts.size(); i++)
		str_fcat(aText, sizeof(aText), "%s\n", m_pCurrentState[ClientID]->m_lpSideCasts[i]);

	for (int i = 0; i < m_pCurrentState[ClientID]->m_lpMainCasts.size(); i++)
	{
		AddWhitespace(aText, sizeof(aText), m_pCurrentState[ClientID]->m_lpMainCasts[i]->m_pText);
		str_fcat(aText, sizeof(aText), "%s\n", m_pCurrentState[ClientID]->m_lpMainCasts[i]->m_pText);
	}

	str_fcat(aText, sizeof(aText), "                                                                           "
		"                                                                                                    ");
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = aText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CBroadcastHandler::TickClient(int ClientID)
{
	if (Server()->ClientIngame(ClientID) == false)
		return;

	if (CompareStates(ClientID) == true || NeedRefresh(ClientID) == true)
		UpdateClient(ClientID);

	if (m_pLastState[ClientID] != 0x0)
	{
		for (int i = 0; i < m_pLastState[ClientID]->m_lpMainCasts.size(); i++)
		{
			if (m_pLastState[ClientID]->m_lpMainCasts[i]->m_Ticks + 1 > Server()->Tick())
			{
				m_pLastState[ClientID]->m_lpMainCasts.remove_index(i);//do not delete
				i--;
			}
		}

		delete m_pLastState[ClientID];
	}

	m_pLastState[ClientID] = m_pCurrentState[ClientID];
	m_pCurrentState[ClientID] = new CBroadState();

	if (m_pLastState[ClientID] != 0x0)
	{
		for (int i = 0; i < m_pLastState[ClientID]->m_lpMainCasts.size(); i++)
		{
			if (m_pLastState[ClientID]->m_lpMainCasts[i]->m_Ticks > Server()->Tick())
				m_pCurrentState[ClientID]->m_lpMainCasts.add(m_pLastState[ClientID]->m_lpMainCasts[i]);
		}
	}
}

void CBroadcastHandler::Tick()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
		TickClient(i);
}

void CBroadcastHandler::AddMainCast(int ClientID, const char *pText, int Time)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || m_pCurrentState[ClientID] == 0x0)
		return;

	int Length = str_length(pText);
	char *pBuf = new char[Length + 1];
	str_copy(pBuf, pText, Length + 1);
	CMainCast *pMainCast = new CMainCast();
	pMainCast->m_pText = pBuf;
	pMainCast->m_Ticks = Server()->Tick() + Time;
	m_pCurrentState[ClientID]->m_lpMainCasts.add(pMainCast);
}

void CBroadcastHandler::AddSideCast(int ClientID, const char *pText)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || m_pCurrentState[ClientID] == 0x0)
		return;

	int Length = str_length(pText);
	char *pBuf = new char[Length + 1];
	str_copy(pBuf, pText, Length + 1);
	m_pCurrentState[ClientID]->m_lpSideCasts.add(pBuf);
}