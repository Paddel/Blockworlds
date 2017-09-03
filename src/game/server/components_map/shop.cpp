
#include <engine/server.h>
#include <game/layers.h>
#include <game/server/gamemap.h>
#include <game/server/gamecontext.h>
#include <game/server/balancing.h>
#include <game/server/entities/npcs/shop_npc.h>

#include "shop.h"

CShopItemGundesign::CShopItemGundesign(vec2 Pos, int Effect, IServer *pServer, int Price, int Level)
	: CShopItem(SHOPITEM_GUNDESIGN, Pos, Effect, Price, Level)
{
	m_pServer = pServer;
	m_ID = pServer->SnapNewID();
}

CShopItemGundesign::~CShopItemGundesign()
{
	if(m_ID != -1)
		m_pServer->SnapFreeID(m_ID);
}

CShopItemSkinmani::CShopItemSkinmani(vec2 Pos, int Effect, CGameMap *pGameMap, int Price, int Level)
	: CShopItem(SHOPITEM_SKINMANI, Pos, Effect, Price, Level)
{
	m_Npc = new CShopNpc(pGameMap->World(), pGameMap->FreeNpcSlot(), Effect);
	m_Npc->Spawn(Pos - vec2(0, 192));
}

CShopItemSkinmani::~CShopItemSkinmani()
{
	delete m_Npc;
}

CShopItemExtra::CShopItemExtra(vec2 Pos, const char *pName, int Price, int Level)
	: CShopItem(SHOPITEM_EXTRA, Pos, -1, Price, Level)
{
	str_copy(m_aName, pName, sizeof(m_aName));
}

CShop::~CShop()
{
	m_lpShopItems.delete_all();
}

void CShop::Init()//TODO: methode to big
{
	m_pLayers = GameMap()->Layers();
	int Price = 0;
	int Level = 0;

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
				{
					int Effect = GameServer()->CosmeticsHandler()->FindSkinmani(ExtrasData.m_aData);
					if (Effect == -1)
					{
						dbg_msg("shop", "Could not find Skinmanipulation effect '%s'", ExtrasData.m_aData);
						continue;
					}
					if (ShopInfoSkinmani(Effect, Price, Level) == false)
					{
						dbg_msg("shop", "Could not find Shopinfo for Skinmanipulation effect '%s'", ExtrasData.m_aData);
						continue;
					}

					m_lpShopItems.add(new CShopItemSkinmani(Pos, Effect, GameMap(), Price, Level));
						
				}
				else if (Tile == EXTRAS_SELL_GUNDESIGN)
				{
					int Effect = GameServer()->CosmeticsHandler()->FindGundesign(ExtrasData.m_aData);
					if (Effect == -1)
					{
						dbg_msg("shop", "Could not find Gundesign effect '%s'", ExtrasData.m_aData);
						continue;
					}
					if (ShopInfoGundesign(Effect, Price, Level) == false)
					{
						dbg_msg("shop", "Could not find Shopinfo for Gundesign effect '%s'", ExtrasData.m_aData);
						continue;
					}

					m_lpShopItems.add(new CShopItemGundesign(Pos, Effect, Server(), Price, Level));
						
				}
				else if (Tile == EXTRAS_SELL_KNOCKOUT)
				{
					int Effect = GameServer()->CosmeticsHandler()->FindKnockoutEffect(ExtrasData.m_aData);
					if (Effect == -1)
					{
						dbg_msg("shop", "Could not find Knockout effect '%s'", ExtrasData.m_aData);
						continue;
					}
					if (ShopInfoKnockout(Effect, Price, Level) == false)
					{
						dbg_msg("shop", "Could not find Shopinfo for Knockout effect '%s'", ExtrasData.m_aData);
						continue;
					}

					m_lpShopItems.add(new CShopItem(SHOPITEM_KNOCKOUT, Pos, Effect, Price, Level));
					
				}
				else if (Tile == EXTRAS_SELL_EXTRAS)
				{
					if (ShopInfoExtra(ExtrasData.m_aData, Price, Level) == false)
					{
						dbg_msg("shop", "Could not find Shopinfo for Extra '%s'", ExtrasData.m_aData);
						continue;
					}
					
					m_lpShopItems.add(new CShopItemExtra(Pos, ExtrasData.m_aData, Price, Level));
				}
			}
		}
	}
}

void CShop::TickGundesign(int Index)
{
	if (Server()->Tick() % (Server()->TickSpeed()) == 0)
		if(GameServer()->CosmeticsHandler()->DoGundesignRaw(m_lpShopItems[Index]->m_Pos - vec2(-64, 192.0f), m_lpShopItems[Index]->m_Effect, GameMap()) == false)
			GameServer()->CreateDamageInd(GameMap(), m_lpShopItems[Index]->m_Pos - vec2(-64, 192.0f), -atan2(1, 0), 10);
		
}

void CShop::TickKnockout(int Index)
{
	if (Server()->Tick() % (Server()->TickSpeed() * 3) == 0)
		GameServer()->CosmeticsHandler()->DoKnockoutEffectRaw(m_lpShopItems[Index]->m_Pos - vec2(0, 192.0f), m_lpShopItems[Index]->m_Effect, GameMap());
}

void CShop::TickPrice(CShopItem *pItem)
{
	char aPrice[32];

	if (Server()->Tick() % (Server()->TickSpeed() * 3) == 0)
	{
		if (Server()->Tick() % (Server()->TickSpeed() * 6) >= Server()->TickSpeed() * 3)
			str_format(aPrice, sizeof(aPrice), "BP:%i", pItem->m_Price);
		else
			str_format(aPrice, sizeof(aPrice), "LVL:%i", pItem->m_Level);

		GameMap()->AnimationHandler()->Laserwrite(aPrice, pItem->m_Pos - vec2(0, 96.0f - 8.0f), 5.0f, Server()->TickSpeed() * 3, true);
	}
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

		TickPrice(m_lpShopItems[i]);
	}
}

void CShop::SnapGundesign(int SnappingClient, int Index)
{
	CShopItemGundesign *pItem = (CShopItemGundesign *)m_lpShopItems[Index];
	if (GameServer()->CosmeticsHandler()->SnapGundesignRaw(pItem->m_Pos - vec2(64, 192.0f), pItem->m_Effect, pItem->m_ID) == false)
	{
		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, pItem->m_ID, sizeof(CNetObj_Projectile)));
		if (pProj)
		{
			pProj->m_X = (int)pItem->m_Pos.x - 64;
			pProj->m_Y = (int)pItem->m_Pos.y - 192;
			pProj->m_VelX = (int)(1);
			pProj->m_VelY = (int)(0);
			pProj->m_StartTick = Server()->Tick() - 5;
			pProj->m_Type = WEAPON_GUN;
		}
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