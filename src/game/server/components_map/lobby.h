#pragma once

#include <base/tl/array.h>
#include <game/server/component.h>

struct CLobbyPlayercount
{
	vec2 m_Pos;
	char m_aMapName[128];
};

class CLobby : public CComponentMap
{
private:
	class CLayers *m_pLayers;
	array<CLobbyPlayercount *> m_lpLobbyMapcounts;

	CGameMap *FindGameMap(const char *pName);
public:
	~CLobby();
	virtual void Init();
	virtual void Tick();

	class CLayers *Layers() const { return m_pLayers; };
};