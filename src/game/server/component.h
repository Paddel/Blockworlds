#pragma once

class CGameContext;
class IServer;

class CComponent
{
	friend class CGameContext;

private:
	CGameContext *m_pGameServer;
	IServer *m_pServer;

protected:

	virtual void Init() { };
	virtual void Tick() { };

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
};