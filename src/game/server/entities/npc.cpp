
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>

#include "npc.h"

CNpc::CNpc(CGameWorld *pGameWorld, int ClientID)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_NPC)
{
	m_ClientID = ClientID;
	m_ProximityRadius = ms_PhysSize;
	Reset();
}

void CNpc::Spawn(vec2 Pos)
{
	m_Alive = true;
	GameWorld()->InsertEntity(this);

	m_Core.Reset();
	m_Core.Init(&GameWorld()->m_Core, GameMap()->Collision());
	m_Core.m_Pos = Pos;
}

void CNpc::Reset()
{
	m_Alive = false;
}

void CNpc::TickDefered()
{
	m_TranslateItem.m_ClientID = m_ClientID;
	m_TranslateItem.m_Pos = m_Pos;
	m_TranslateItem.m_Team = 0; // always ingame
}

void CNpc::Tick()
{
	m_Pos = m_Core.m_Pos;
	m_Core.Tick(false);
	m_Core.Move();
}

void CNpc::TickPaused()
{
	
}

void CNpc::Snap(int SnappingClient)
{
	if (m_Alive == false)
		return;

	int TranslatedID = Server()->Translate(SnappingClient, m_ClientID);
	if (TranslatedID == -1)
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, TranslatedID, sizeof(CNetObj_ClientInfo)));
	if (!pClientInfo)
		return;

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%i", m_ClientID);
	StrToInts(&pClientInfo->m_Name0, 4, aBuf);
	StrToInts(&pClientInfo->m_Clan0, 3, "Peter");
	pClientInfo->m_Country = -1;
	StrToInts(&pClientInfo->m_Skin0, 6, "");
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

	// set emote
	/*if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}*/

	pCharacter->m_Emote = EMOTE_NORMAL;//m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = WEAPON_HAMMER;//m_ActiveWeapon;
	pCharacter->m_AttackTick = 0;//m_AttackTick;

	pCharacter->m_Direction = 0;// m_Input.m_Direction;

	if (m_ClientID == SnappingClient || SnappingClient == -1 ||
		(GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID != -1 && m_ClientID == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = 10;//m_Health;
		pCharacter->m_Armor = 10;//m_Armor;
		//if (m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
		pCharacter->m_AmmoCount = 10;// m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	/*if (pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if (250 - ((Server()->Tick() - m_LastAction) % (250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}*/

	pCharacter->m_PlayerFlags = 0;// GetPlayer()->m_PlayerFlags;

	//translate hook
	if (pCharacter->m_HookedPlayer != -1)
	{
		int HookingID = Server()->Translate(SnappingClient, pCharacter->m_HookedPlayer);
		pCharacter->m_HookedPlayer = HookingID;
	}

}
