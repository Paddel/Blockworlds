
#include <game/server/gamecontext.h>

#include "shop_npc.h"

CShopNpc::CShopNpc(CGameWorld *pGameWorld, int ClientID, int Effect)
: CNpc(pGameWorld, ClientID)
{
	m_Effect = Effect;
}

void CShopNpc::Snap(int SnappingClient)
{
	if (m_Alive && GameServer()->m_apPlayers[SnappingClient] == 0x0)
		return;

	int TranslatedID = Server()->Translate(SnappingClient, m_ClientID);

	if (TranslatedID == -1)
		return;

	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, TranslatedID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, "");
	StrToInts(&pClientInfo->m_Clan0, 3, "");
	pClientInfo->m_Country = -1;
	StrToInts(&pClientInfo->m_Skin0, 6, GameServer()->m_apPlayers[SnappingClient]->m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = 1;
	pClientInfo->m_ColorBody = GameServer()->m_apPlayers[SnappingClient]->m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = GameServer()->m_apPlayers[SnappingClient]->m_TeeInfos.m_ColorFeet;

	GameServer()->CosmeticsHandler()->SnapSkinmaniRaw(0, pClientInfo, m_Effect);

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, TranslatedID, sizeof(CNetObj_PlayerInfo)));
	if (!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 5;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = TranslatedID;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Team = 1;

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