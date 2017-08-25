#pragma once

//All important calculations for gameplayer balancing

inline int NeededExp(int Level)
{
	return int(Level * 2.5f);
}

inline int NeededClanExp(int Level)
{
	return int(Level * 25.0f);
}

inline void NewRankings(int& Winner, int& Looser)//Age of Empires III ranking inc.
{
	int Points = clamp((int)(16 + (Looser - Winner) * (16.0f / 400.0f)), 1, 31);
	Winner += Points;
	Looser -= Points;
}