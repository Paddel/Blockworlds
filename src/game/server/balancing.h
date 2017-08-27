#pragma once

//All important calculations for gameplayer balancing

inline int NeededExp(int Level)
{
	return int(Level * 0.420 + 15);
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