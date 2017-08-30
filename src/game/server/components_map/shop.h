#pragma once

#include <game/server/component.h>

enum
{
	SHOPITEM_SKINMANI=0,
	SHOPITEM_GUNDESIGN,
	SHOPITEM_KNOCKOUT,
	SHOPITEM_EXTRAS,
};

struct CShopItem
{
	CShopItem(int Type, vec2 Pos, const char *pName)
	{
		m_Type = Type;
		m_Pos = Pos;
		str_copy(m_aName, pName, sizeof(m_aName));
	}
	virtual ~CShopItem() {}

	int m_Type;
	vec2 m_Pos;
	char m_aName[64];
};

struct CShopItemGundesign : public CShopItem
{
	CShopItemGundesign(vec2 Pos, const char *pName, IServer *pServer);
	~CShopItemGundesign();
	int m_ID;
	IServer *m_pServer;
};

struct CShopItemSkinmani : public CShopItem
{
	CShopItemSkinmani(vec2 Pos, const char *pName, CGameMap *pGameMap);
	~CShopItemSkinmani();

	class CShopNpc *m_Npc;
};

class CShop : public CComponentMap
{
private:
	CLayers *m_pLayers;
	array<CShopItem *> m_lpShopItems;

	void RemoveShopItem(int Index, const char *pReson);
	void TickGundesign(int Index);
	void TickKnockout(int Index);

	void SnapGundesign(int SnappingClient, int Index);
public:
	~CShop();
	virtual void Init();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	CLayers *Layers() const { return m_pLayers; };
};