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

inline int NeededClanCreateLevel() { return 35; }

inline int NeededClanJoinLevel() { return 10; }

inline bool ShopInfoSkinmani(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::SKINMANI_RAINBOW)
	{
		Price = 100;
		Level = 50;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoGundesign(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::GUNDESIGN_HEART)
	{
		Price = 100;
		Level = 50;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_PEW)
	{
		Price = 500;
		Level = 1000;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoKnockout(int Index, int& Price, int& Level)
{
	if (Index == CCosmeticsHandler::KNOCKOUT_LOVE)
	{
		Price = 200;
		Level = 100;
		return true;
	}
	else
		return false;
}

inline bool ShopInfoExtra(const char *pName, int& Price, int& Level)
{
	if (str_comp_nocase(pName, "pages") == 0)
	{
		Price = 30;
		Level = 1;
		return true;
	}
	else
		return false;
}