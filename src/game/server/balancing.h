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
		Price = 250;
		Level = 50;
		return true;
	}
	else if (Index == CCosmeticsHandler::SKINMANI_FEET_BLACKWHITE)
	{
		Price = 550;
		Level = 105;
		return true;
	}
	else if (Index == CCosmeticsHandler::SKINMANI_FEET_RGB || 
		Index == CCosmeticsHandler::SKINMANI_FEET_CMY)
	{
		Price = 750;
		Level = 145;
		return true;
	}
	else if (Index == CCosmeticsHandler::SKINMANI_BODY_FIRE ||
		Index == CCosmeticsHandler::SKINMANI_BODY_WATER ||
		Index == CCosmeticsHandler::SKINMANI_BODY_POISON)
	{
		Price = 1600;
		Level = 250;
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
		Price = 250;
		Level = 50;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_TWOCLOCK)
	{
		Price = 300;
		Level = 60;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_BLINKING)
	{
		Price = 600;
		Level = 115;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_STARX)
	{
		Price = 750;
		Level = 145;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_REVERSE)
	{
		Price = 900;
		Level = 175;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_ARMOR)
	{
		Price = 1000;
		Level = 205;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_HEART)
	{
		Price = 1500;
		Level = 245;
		return true;
	}
	else if (Index == CCosmeticsHandler::GUNDESIGN_PEW)
	{
		Price = 2000;
		Level = 285;
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
		Price = 250;
		Level = 50;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_STARRING)
	{
		Price = 600;
		Level = 115;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_STAREXPLOSION)
	{
		Price = 700;
		Level = 135;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_THUNDERSTORM)
	{
		Price = 900;
		Level = 175;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_KORIP)
	{
		Price = 1300;
		Level = 225;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_LOVE)
	{
		Price = 1600;
		Level = 255;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_KOEZ)
	{
		Price = 2000;
		Level = 295;
		return true;
	}
	else if (Index == CCosmeticsHandler::KNOCKOUT_KONOOB)
	{
		Price = 2100;
		Level = 305;
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