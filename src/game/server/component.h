#pragma once

class CGameContext;
class IServer;
class CGameMap;

class CComponentGlobal
{
	friend class CGameContext;

private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;

protected:
	virtual void Init() { };
	virtual void Tick() { };

public:

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};

class CComponentMap
{
	friend class CGameMap;

private:
	CGameMap *m_pGameMap;
	CGameContext *m_pGameServer;
	IServer *m_pServer;

protected:

	virtual void Init() { };
	virtual void Tick() { };
	virtual void Snap(int SnappingClient) { };

public:

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	CGameMap *GameMap() const { return m_pGameMap; }
};