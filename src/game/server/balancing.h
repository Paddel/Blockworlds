#pragma once

#include <game/server/components_global/cosmetics.h>

//All important calculations for gameplaye balancing

inline int NeededExp(int Level)
{
	return int(Level * 0.18 + 15);
}

inline int NeededClanExp(int Level)
{
	return int(Level * 2.10 + 30);
}

inline void NewRankings(int& Winner, int& Looser)//Age of Empires III ranking inc.
{
	int Points = clamp((int)(16 + (Looser - Winner) * (16.0f / 400.0f)), 1, 31);
	Winner += Points;
	Looser -= Points;
}

inline int NeededClanCreateLevel() { return 65; }

inline int NeededClanJoinLevel() { return 8; }

inline bool ShopInfoSkinmani(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::SKINMANI_FEET_FIRE ||
		Index == CCosmeticsHandler::SKINMANI_FEET_WATER ||
		Index == CCosmeticsHandler::SKINMANI_FEET_POISON)
	{
		Price = 130;
		Level = 25;
		return true;
	}
	else if (Index == CCosmeticsHandler::SKINMANI_FEET_BLACKWHITE)
	{
		Price = 250;
		Level = 55;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoGundesign(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::GUNDESIGN_CLOCKWISE ||
		Index == CCosmeticsHandler::GUNDESIGN_COUNTERCLOCK)
	{
		Price = 130;
		Level = 25;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_TWOCLOCK)
	{
		Price = 160;
		Level = 28;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_BLINKING)
	{
		Price = 250;
		Level = 55;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_REVERSE)
	{
		Price = 320;
		Level = 65;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_INVISBULLET)
	{
		Price = 400;
		Level = 75;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoKnockout(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::KNOCKOUT_EXPLOSION ||
		Index == CCosmeticsHandler::KNOCKOUT_HAMMERHIT ||
		Index == CCosmeticsHandler::KNOCKOUT_KOSTARS)
	{
		Price = 130;
		Level = 25;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_STARRING)
	{
		Price = 250;
		Level = 55;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_STAREXPLOSION)
	{
		Price = 320;
		Level = 65;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoExtra(const char *pName, int& Price, int& Level)
{
	if (str_comp_nocase(pName, "weaponkit") == 0)
	{
		Price = 15;
		Level = 3;
		return true;
	}
	else if (str_comp_nocase(pName, "page") == 0)
	{
		Price = 30;
		Level = 6;
		return true;
	}
	else
		return false;
}