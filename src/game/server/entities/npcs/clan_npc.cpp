
#include <game/server/gamecontext.h>

#include "clan_npc.h"

CClanNpc::CClanNpc(CGameWorld *pGameWorld, int ClientID)
: CNpc(pGameWorld, ClientID)
{
	str_copy(m_aName, "ERROR", sizeof(m_aName));
}

void CClanNpc::Tick()
{
	int Level = 0;
	int Experience = 0;
	for (int i = 0; i < GameServer()->AccountsHandler()->m_lpClans.size(); i++)
	{
		IServer::CClanData *pClan = GameServer()->AccountsHandler()->m_lpClans[i];
		if (pClan->m_Level > Level || (pClan->m_Level == Level && pClan->m_Experience > Experience))
		{
			str_copy(m_aName, pClan->m_aName, sizeof(m_aName));
			m_Level = Level;
			Level = pClan->m_Level;
			Experience = pClan->m_Experience;
		}
	}
}

void CClanNpc::Snap(int SnappingClient)
{
	if (m_Alive && GameServer()->m_apPlayers[SnappingClient] == 0x0)
		return;

	int TranslatedID = Server()->Translate(SnappingClient, m_ClientID);
	if (TranslatedID == -1)
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, TranslatedID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	char aBuf[32];
	if ((Server()->Tick() / int(Server()->TickSpeed() * 1.75f)) % 2)
		str_copy(aBuf, m_aName, sizeof(aBuf));
	else
		str_format(aBuf, sizeof(aBuf), "Level: %i", m_Level);

	StrToInts(&pClientInfo->m_Name0, 4, aBuf);
	StrToInts(&pClientInfo->m_Clan0, 3, "");
	pClientInfo->m_Country = -1;
	StrToInts(&pClientInfo->m_Skin0, 6, "default");
	pClientInfo->m_UseCustomColor = 0;
	pClientInfo->m_ColorBody = 0;
	pClientInfo->m_ColorFeet = 0;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, TranslatedID, sizeof(CNetObj_PlayerInfo)));
	if (!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 5;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = TranslatedID;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Team = 1;

	if (NetworkClipped(SnappingClient))
		return;


	//snap character
	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, TranslatedID, sizeof(CNetObj_Character)));
	if (!pCharacter)
		return;

	pCharacter->m_Tick = 0;
	m_Core.Write(pCharacter);

	pCharacter->m_Emote = EMOTE_NORMAL;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = WEAPON_HAMMER;
	pCharacter->m_AttackTick = 0;

	pCharacter->m_Direction = 0;

	if (m_ClientID == SnappingClient || SnappingClient == -1 ||
		(GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID != -1 && m_ClientID == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = 10;
		pCharacter->m_Armor = 10;
		pCharacter->m_AmmoCount = 10;
	}

	pCharacter->m_PlayerFlags = 0;

	//translate hook
	if (pCharacter->m_HookedPlayer != -1)
	{
		int HookingID = Server()->Translate(SnappingClient, pCharacter->m_HookedPlayer);
		pCharacter->m_HookedPlayer = HookingID;
	}
}