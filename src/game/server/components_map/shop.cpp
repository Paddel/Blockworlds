
#include <engine/server.h>
#include <game/layers.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/npcs/shop_npc.h>

#include "shop.h"

CShopItemGundesign::CShopItemGundesign(vec2 Pos, const char *pName, IServer *pServer)
	: CShopItem(SHOPITEM_GUNDESIGN, Pos, pName)
{
	m_pServer = pServer;
	m_ID = pServer->SnapNewID();
}

CShopItemGundesign::~CShopItemGundesign()
{
	if(m_ID != -1)
		m_pServer->SnapFreeID(m_ID);
}

CShopItemSkinmani::CShopItemSkinmani(vec2 Pos, const char *pName, CGameMap *pGameMap)
	: CShopItem(SHOPITEM_SKINMANI, Pos, pName)
{
	m_Npc = new CShopNpc(pGameMap->World(), pGameMap->FreeNpcSlot(), pName);
	m_Npc->Spawn(Pos - vec2(0, 192));
}

CShopItemSkinmani::~CShopItemSkinmani()
{
	delete m_Npc;
}

CShop::~CShop()
{
	m_lpShopItems.delete_all();
}

void CShop::Init()
{
	m_pLayers = GameMap()->Layers();

	for (int l = 0; l < Layers()->GetNumExtrasLayer(); l++)
	{
		for (int x = 0; x < Layers()->GetExtrasWidth(l); x++)
		{
			for (int y = 0; y < Layers()->GetExtrasHeight(l); y++)
			{
				int Index = y * Layers()->GetExtrasWidth(l) + x;
				int Tile = Layers()->GetExtrasTile(l)[Index].m_Index;
				CExtrasData ExtrasData = Layers()->GetExtrasData(l)[Index];
				vec2 Pos = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

				if (Tile == EXTRAS_SELL_SKINMANI)
					m_lpShopItems.add(new CShopItemSkinmani(Pos, ExtrasData.m_aData, GameMap()));
				else if (Tile == EXTRAS_SELL_GUNDESIGN)
					m_lpShopItems.add(new CShopItemGundesign(Pos, ExtrasData.m_aData, Server()));
				else if (Tile == EXTRAS_SELL_KNOCKOUT)
					m_lpShopItems.add(new CShopItem(SHOPITEM_KNOCKOUT, Pos, ExtrasData.m_aData));
				else if (Tile == EXTRAS_SELL_EXTRAS)
					m_lpShopItems.add(new CShopItem(SHOPITEM_EXTRAS, Pos, ExtrasData.m_aData));
			}
		}
	}
}

void CShop::RemoveShopItem(int Index, const char *pReson)
{
	dbg_msg("shop", "Removed shopitem type=%i name=%s (%s)", m_lpShopItems[Index]->m_Type, m_lpShopItems[Index]->m_aName, pReson);
	delete m_lpShopItems[Index];
	m_lpShopItems.remove_index(Index);
}

void CShop::TickGundesign(int Index)
{
	if (Server()->Tick() % (Server()->TickSpeed() * 3) == 0)
		if (GameServer()->CosmeticsHandler()->DoGundesign(m_lpShopItems[Index]->m_Pos - vec2(-64, 192.0f), m_lpShopItems[Index]->m_aName, GameMap()) == false)
			RemoveShopItem(Index, "Visualisation not possible");
}

void CShop::TickKnockout(int Index)
{
	if (Server()->Tick() % (Server()->TickSpeed() * 3) == 0)
		if (GameServer()->CosmeticsHandler()->DoKnockoutEffect(m_lpShopItems[Index]->m_Pos - vec2(0, 192.0f), m_lpShopItems[Index]->m_aName, GameMap()) == false)
			RemoveShopItem(Index, "Visualisation not possible");
}

void CShop::Tick()
{
	for (int i = 0; i < m_lpShopItems.size(); i++)
	{
		switch (m_lpShopItems[i]->m_Type)
		{
		case SHOPITEM_KNOCKOUT: TickKnockout(i); break;
		case SHOPITEM_GUNDESIGN: TickGundesign(i); break;
		}
	}
}

void CShop::SnapGundesign(int SnappingClient, int Index)
{
	CShopItemGundesign *pItem = (CShopItemGundesign *)m_lpShopItems[Index];
	if (GameServer()->CosmeticsHandler()->SnapGundesign(pItem->m_Pos - vec2(64, 192.0f), pItem->m_aName, pItem->m_ID) == false && pItem->m_ID != -1)
	{
		Server()->SnapFreeID(pItem->m_ID);
		pItem->m_ID = -1;
	}
}

void CShop::Snap(int SnappingClient)
{
	for (int i = 0; i < m_lpShopItems.size(); i++)
	{
		switch (m_lpShopItems[i]->m_Type)
		{
		case SHOPITEM_GUNDESIGN: SnapGundesign(SnappingClient, i); break;
		}
	}
}