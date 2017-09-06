#pragma once

#include <base/tl/array.h>
#include <game/server/component.h>

enum
{
	SHOPITEM_SKINMANI=0,
	SHOPITEM_GUNDESIGN,
	SHOPITEM_KNOCKOUT,
	SHOPITEM_EXTRA,
};

struct CShopItem
{
	CShopItem(int Type, vec2 Pos, int Effect, int Price, int Level)
	{
		m_Type = Type;
		m_Pos = Pos;
		m_Effect = Effect;
		m_Price = Price;
		m_Level = Level;
	}
	virtual ~CShopItem() {}

	int m_Type;
	vec2 m_Pos;
	int m_Effect;
	int m_Price;
	int m_Level;
};

struct CShopItemGundesign : public CShopItem
{
	CShopItemGundesign(vec2 Pos, int Effect, IServer *pServer, int Price, int Level);
	~CShopItemGundesign();
	int m_ID;
	IServer *m_pServer;
};

struct CShopItemSkinmani : public CShopItem
{
	CShopItemSkinmani(vec2 Pos, int Effect, CGameMap *pGameMap, int Price, int Level);
	~CShopItemSkinmani();

	class CShopNpc *m_Npc;
};

struct CShopItemExtra : public CShopItem
{
	CShopItemExtra(vec2 Pos, const char *pName, int Price, int Level);
	char m_aName[64];
};

class CShop : public CComponentMap
{
private:
	CLayers *m_pLayers;
	array<CShopItem *> m_lpShopItems;

	void TickGundesign(int Index);
	void TickKnockout(int Index);

	void TickPrice(CShopItem *pItem);

	void SnapGundesign(int SnappingClient, int Index);
public:
	~CShop();
	virtual void Init();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	CLayers *Layers() const { return m_pLayers; };
};