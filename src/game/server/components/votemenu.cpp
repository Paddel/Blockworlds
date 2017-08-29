
#include <engine/server.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemap.h>

#include "votemenu.h"

static char SignBoxChecked[] = { '\xE2','\x98', '\x91', 0 };
static char SignBoxUnchecked[] = { '\xE2','\x98', '\x90', 0 };

void CVoteMenu::Reset()
{
	m_lpKnockoutEffectsOptions.delete_all();
	m_lpExtrasOptions.delete_all();
	m_lpGundesignOptions.delete_all();
	m_lpSkinmaniOptions.delete_all();
}

bool CVoteMenu::Compare(CVoteMenu *pCompare)
{
	if (m_lpKnockoutEffectsOptions.size() != pCompare->m_lpKnockoutEffectsOptions.size())
		return true;

	if (m_lpExtrasOptions.size() != pCompare->m_lpExtrasOptions.size())
		return true;

	if (m_lpGundesignOptions.size() != pCompare->m_lpGundesignOptions.size())
		return true;

	if (m_lpSkinmaniOptions.size() != pCompare->m_lpSkinmaniOptions.size())
		return true;

	for (int i = 0; i < m_lpKnockoutEffectsOptions.size(); i++)
	{
		if (mem_comp(m_lpKnockoutEffectsOptions[i], pCompare->m_lpKnockoutEffectsOptions[i], sizeof(CVoteOptionServer)) != 0)
			return true;
	}

	for (int i = 0; i < m_lpExtrasOptions.size(); i++)
	{
		if (mem_comp(m_lpExtrasOptions[i], pCompare->m_lpExtrasOptions[i], sizeof(CVoteOptionServer)) != 0)
			return true;
	}

	for (int i = 0; i < m_lpGundesignOptions.size(); i++)
	{
		if (mem_comp(m_lpGundesignOptions[i], pCompare->m_lpGundesignOptions[i], sizeof(CVoteOptionServer)) != 0)
			return true;
	}

	for (int i = 0; i < m_lpSkinmaniOptions.size(); i++)
	{
		if (mem_comp(m_lpSkinmaniOptions[i], pCompare->m_lpSkinmaniOptions[i], sizeof(CVoteOptionServer)) != 0)
			return true;
	}

	return false;
}

void CVoteMenuHandler::CreateStripline(char *pDst, int DstSize, const char *pTitle)
{
	int StripSideLen = min(15, DstSize / 2 - str_length(pTitle) - 3);
	mem_zero(pDst, DstSize);
	for (int i = 0; i < StripSideLen; i++)
		str_append(pDst, "#", DstSize);

	str_fcat(pDst, DstSize, " %s ", pTitle);

	for (int i = 0; i < StripSideLen; i++)
		str_append(pDst, "#", DstSize);
}

void CVoteMenuHandler::UpdateEveryone()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false)
			continue;
		UpdateMenu(i);
	}
}

void CVoteMenuHandler::Construct(CVoteMenu *pFilling, int ClientID)
{
	for (int i = 0; i < CCosmeticsHandler::NUM_KNOCKOUTS; i++)
	{
		if (GameServer()->CosmeticsHandler()->HasKnockoutEffect(ClientID, i) == false)
			continue;

		CVoteOptionServer *pOption = new CVoteOptionServer();
		str_format(pOption->m_aDescription, sizeof(pOption->m_aDescription), "%s %s",
			Server()->GetClientInfo(ClientID)->m_CurrentKnockout == i ? SignBoxChecked : SignBoxUnchecked,
			CCosmeticsHandler::ms_KnockoutNames[i]);
		str_copy(pOption->m_aCommand, CCosmeticsHandler::ms_KnockoutNames[i], sizeof(pOption->m_aCommand));
		pFilling->m_lpKnockoutEffectsOptions.add(pOption);
	}

	for (int i = 0; i < CCosmeticsHandler::NUM_SKINMANIS; i++)
	{
		if (GameServer()->CosmeticsHandler()->HasSkinmani(ClientID, i) == false)
			continue;

		CVoteOptionServer *pOption = new CVoteOptionServer();
		str_format(pOption->m_aDescription, sizeof(pOption->m_aDescription), "%s %s",
			Server()->GetClientInfo(ClientID)->m_CurrentSkinmani == i ? SignBoxChecked : SignBoxUnchecked,
			CCosmeticsHandler::ms_SkinmaniNames[i]);
		str_copy(pOption->m_aCommand, CCosmeticsHandler::ms_SkinmaniNames[i], sizeof(pOption->m_aCommand));
		pFilling->m_lpSkinmaniOptions.add(pOption);
	}

	for (int i = 0; i < CCosmeticsHandler::NUM_GUNDESIGNS; i++)
	{
		if (GameServer()->CosmeticsHandler()->HasGundesign(ClientID, i) == false)
			continue;

		CVoteOptionServer *pOption = new CVoteOptionServer();
		str_format(pOption->m_aDescription, sizeof(pOption->m_aDescription), "%s %s",
			Server()->GetClientInfo(ClientID)->m_CurrentGundesign == i ? SignBoxChecked : SignBoxUnchecked,
			CCosmeticsHandler::ms_GundesignNames[i]);
		str_copy(pOption->m_aCommand, CCosmeticsHandler::ms_GundesignNames[i], sizeof(pOption->m_aCommand));
		pFilling->m_lpGundesignOptions.add(pOption);
	}

	//extras
	if (Server()->GetClientInfo(ClientID)->m_InviolableTime > Server()->Tick())
	{
		CVoteOptionServer *pOption = new CVoteOptionServer();
		str_format(pOption->m_aDescription, sizeof(pOption->m_aDescription), "%s %s",
			Server()->GetClientInfo(ClientID)->m_UseInviolable == true ? SignBoxChecked : SignBoxUnchecked,
			"Anti Wayblock");
		str_copy(pOption->m_aCommand, "inviolable", sizeof(pOption->m_aCommand));
		pFilling->m_lpExtrasOptions.add(pOption);
	}
}

void CVoteMenuHandler::Init()
{

}

void CVoteMenuHandler::Tick()
{
	
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false || GameServer()->m_apPlayers[i] == 0x0 ||
			(GameServer()->m_apPlayers[i]->m_PlayerFlags&PLAYERFLAG_IN_MENU) == 0)
			continue;

		//TODO: rework.. very unefficient
		CVoteMenu OldMenu;
		Construct(&OldMenu, i);
		if (m_Menus[i].Compare(&OldMenu))
		{
			m_Menus[i].Reset();
			Construct(&m_Menus[i], i);
			UpdateMenu(i);
		}

		OldMenu.Reset();
	}
}

void CVoteMenuHandler::OnClientJoin(int ClientID)
{
	m_Menus[ClientID].Reset();
	UpdateMenu(ClientID);
}

void CVoteMenuHandler::AddVote(const char *pDescription, const char *pCommand)
{
	for (int i = 0; i < m_lpServerOptions.size(); i++)
	{
		if (str_comp_nocase(m_lpServerOptions[i]->m_aDescription, pDescription) != 0)
			continue;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	CVoteOptionServer *pNewOption = new CVoteOptionServer();
	str_copy(pNewOption->m_aDescription, pDescription, sizeof(pNewOption->m_aDescription));
	str_copy(pNewOption->m_aCommand, pCommand, sizeof(pNewOption->m_aCommand));
	m_lpServerOptions.add(pNewOption);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pDescription, pCommand);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	UpdateEveryone();
}

void CVoteMenuHandler::RemoveVote(const char *pDescription)
{
	int Index = -1;
	for (int i = 0; i < m_lpServerOptions.size(); i++)
	{
		if (str_comp_nocase(m_lpServerOptions[i]->m_aDescription, pDescription) != 0)
			continue;

		Index = i;
		break;
	}

	if (Index == -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	CVoteOptionServer *pOption = m_lpServerOptions[Index];

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	m_lpServerOptions.remove_index(Index);
	delete pOption;

	UpdateEveryone();
}

void CVoteMenuHandler::ForceVote(int ClientID, const char *pDescription, const char *pReason)
{
	char aBuf[128];

	for (int i = 0; i < m_lpServerOptions.size(); i++)
	{
		if (str_comp_nocase(m_lpServerOptions[i]->m_aDescription, pDescription) != 0)
			continue;

		if (str_comp(m_lpServerOptions[i]->m_aCommand, "%rand_event") == 0)
		{
			CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
			if (pGameMap != 0x0)
				pGameMap->StartRandomEvent();
		}

		str_format(aBuf, sizeof(aBuf), "admin forced server option '%s' (%s)", pDescription, pReason);
		GameServer()->SendChat(-1, aBuf);
		GameServer()->Console()->ExecuteLine(m_lpServerOptions[i]->m_aCommand);
		return;
	}

	if (ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		for (int i = 0; i < m_Menus[ClientID].m_lpKnockoutEffectsOptions.size(); i++)
		{
			if (str_comp(pDescription, m_Menus[ClientID].m_lpKnockoutEffectsOptions[i]->m_aDescription) != 0)
				continue;

			CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
			if (pChr != 0x0 && pChr->IsAlive())
			{
				GameServer()->CosmeticsHandler()->DoKnockoutEffect(ClientID, pChr->m_Pos, m_Menus[ClientID].m_lpKnockoutEffectsOptions[i]->m_aCommand);
				pChr->GetPlayer()->m_LastVoteTry = 0;
			}
			return;
		}

		for (int i = 0; i < m_Menus[ClientID].m_lpGundesignOptions.size(); i++)
		{
			if (str_comp(pDescription, m_Menus[ClientID].m_lpGundesignOptions[i]->m_aDescription) != 0)
				continue;

			CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
			if (pChr != 0x0 && pChr->IsAlive())
			{
				GameServer()->CosmeticsHandler()->DoGundesign(ClientID, pChr->m_Pos, m_Menus[ClientID].m_lpGundesignOptions[i]->m_aCommand);
				pChr->GetPlayer()->m_LastVoteTry = 0;
			}
			return;
		}

		for (int i = 0; i < m_Menus[ClientID].m_lpSkinmaniOptions.size(); i++)
		{
			if (str_comp(pDescription, m_Menus[ClientID].m_lpSkinmaniOptions[i]->m_aDescription) != 0)
				continue;
			return;
		}
	}

	str_format(aBuf, sizeof(aBuf), "'%s' isn't an option on this server", pDescription);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CVoteMenuHandler::ClearVotes()
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	m_lpServerOptions.delete_all();
	UpdateEveryone();
}

void CVoteMenuHandler::CallVote(int ClientID, const char *pDescription, const char *pReason)
{
	CGameMap *pGameMap = Server()->CurrentGameMap(ClientID);
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];
	char aChatmsg[512];
	if (str_find(pDescription, "###") || pPlayer == 0x0)//stripline
		return;

	//check server option
	CVoteOptionServer *pOption = 0x0;
	for (int i = 0; i < m_lpServerOptions.size(); i++)
	{
		if (str_comp_nocase(m_lpServerOptions[i]->m_aDescription, pDescription) != 0)
			continue;
		pOption = m_lpServerOptions[i];
		break;
	}

	if (pOption != 0x0)
	{
		if (str_comp(pOption->m_aCommand, "%rand_event") == 0)
		{
			if(pGameMap->TryVoteRandomEvent(ClientID) == false)
				return;
		}

		str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s' (%s)", Server()->ClientName(ClientID),
			pOption->m_aDescription, pReason);

		pGameMap->SendChat(-1, aChatmsg);
		pGameMap->StartVote(pOption->m_aDescription, pOption->m_aCommand, pReason);
		pPlayer->m_Vote = 1;
		pPlayer->m_VotePos = 1;
		pGameMap->SetVotePos(1);
		pGameMap->SetVoteCreator(ClientID);
		pPlayer->m_LastVoteCall = Server()->Tick();

		return;
	}

	for (int i = 0; i < m_Menus[ClientID].m_lpKnockoutEffectsOptions.size(); i++)
	{
		if (str_comp(pDescription, m_Menus[ClientID].m_lpKnockoutEffectsOptions[i]->m_aDescription) != 0)
			continue;

		if (GameServer()->CosmeticsHandler()->ToggleKnockout(ClientID, m_Menus[ClientID].m_lpKnockoutEffectsOptions[i]->m_aCommand))
		{
			pPlayer->m_LastVoteTry = Server()->Tick() - Server()->TickSpeed() * 2.25f;
			return;
		}
	}

	for (int i = 0; i < m_Menus[ClientID].m_lpGundesignOptions.size(); i++)
	{
		if (str_comp(pDescription, m_Menus[ClientID].m_lpGundesignOptions[i]->m_aDescription) != 0)
			continue;

		if (GameServer()->CosmeticsHandler()->ToggleGundesign(ClientID, m_Menus[ClientID].m_lpGundesignOptions[i]->m_aCommand))
		{
			pPlayer->m_LastVoteTry = Server()->Tick() - Server()->TickSpeed() * 2.25f;
			return;
		}
	}

	for (int i = 0; i < m_Menus[ClientID].m_lpSkinmaniOptions.size(); i++)
	{
		if (str_comp(pDescription, m_Menus[ClientID].m_lpSkinmaniOptions[i]->m_aDescription) != 0)
			continue;

		if (GameServer()->CosmeticsHandler()->ToggleSkinmani(ClientID, m_Menus[ClientID].m_lpSkinmaniOptions[i]->m_aCommand))
		{
			pPlayer->m_LastVoteTry = Server()->Tick() - Server()->TickSpeed() * 2.25f;
			return;
		}
	}

	for (int i = 0; i < m_Menus[ClientID].m_lpExtrasOptions.size(); i++)
	{
		if (str_comp(pDescription, m_Menus[ClientID].m_lpExtrasOptions[i]->m_aDescription) != 0)
			continue;

		if(GameServer()->OnExtrasCallvote(ClientID, m_Menus[ClientID].m_lpExtrasOptions[i]->m_aCommand))
		{
			pPlayer->m_LastVoteTry = Server()->Tick() - Server()->TickSpeed() * 2.25f;
			return;
		}
	}

	str_format(aChatmsg, sizeof(aChatmsg), "'%s' isn't an option on this server", pDescription);
	GameServer()->SendChatTarget(ClientID, aChatmsg);
}

void CVoteMenuHandler::UpdateMenu(int ClientID)
{
	char aBuf[VOTE_DESC_LENGTH];
	CNetMsg_Sv_VoteOptionAdd OptionMsg;

	//clear old options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

	if (m_lpServerOptions.size() > 0)
	{
		CreateStripline(aBuf, VOTE_DESC_LENGTH, "Server Votes");
		OptionMsg.m_pDescription = aBuf;
		Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

		for (int i = 0; i < m_lpServerOptions.size(); i++)
		{
			OptionMsg.m_pDescription = m_lpServerOptions[i]->m_aDescription;
			Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
		}
	}

	if (m_Menus[ClientID].m_lpExtrasOptions.size() > 0)
	{
		CreateStripline(aBuf, VOTE_DESC_LENGTH, "Extras");
		OptionMsg.m_pDescription = aBuf;
		Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

		for (int i = 0; i < m_Menus[ClientID].m_lpExtrasOptions.size(); i++)
		{
			OptionMsg.m_pDescription = m_Menus[ClientID].m_lpExtrasOptions[i]->m_aDescription;
			Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
		}

	}

	if (m_Menus[ClientID].m_lpSkinmaniOptions.size() > 0)
	{
		CreateStripline(aBuf, VOTE_DESC_LENGTH, " Skinmanipulation ");
		OptionMsg.m_pDescription = aBuf;
		Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

		for (int i = 0; i < m_Menus[ClientID].m_lpSkinmaniOptions.size(); i++)
		{
			OptionMsg.m_pDescription = m_Menus[ClientID].m_lpSkinmaniOptions[i]->m_aDescription;
			Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
		}
	}

	if (m_Menus[ClientID].m_lpGundesignOptions.size() > 0)
	{
		CreateStripline(aBuf, VOTE_DESC_LENGTH, " Gundesigns ");
		OptionMsg.m_pDescription = aBuf;
		Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

		for (int i = 0; i < m_Menus[ClientID].m_lpGundesignOptions.size(); i++)
		{
			OptionMsg.m_pDescription = m_Menus[ClientID].m_lpGundesignOptions[i]->m_aDescription;
			Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
		}
	}

	if (m_Menus[ClientID].m_lpKnockoutEffectsOptions.size() > 0)
	{
		CreateStripline(aBuf, VOTE_DESC_LENGTH, "Knockouteffects");
		OptionMsg.m_pDescription = aBuf;
		Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);

		for (int i = 0; i < m_Menus[ClientID].m_lpKnockoutEffectsOptions.size(); i++)
		{
			OptionMsg.m_pDescription = m_Menus[ClientID].m_lpKnockoutEffectsOptions[i]->m_aDescription;
			Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
		}

	}
}