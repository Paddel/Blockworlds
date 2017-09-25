

#ifndef GAME_SERVER_GAMEWORLD_H
#define GAME_SERVER_GAMEWORLD_H

#include <game/server/srv_gamecore.h>
#include <game/server/animations.h>

#include "eventhandler.h"

class CEntity;
class CCharacter;
class CGameMap;
class CGameContext;
class IServer;

/*
	Class: Game World
		Tracks all entities in the game. Propagates tick and
		snap calls to all entities.
*/
class CGameWorld
{
public:
	enum
	{
		ENTTYPE_PROJECTILE = 0,
		ENTTYPE_LASER,
		ENTTYPE_PICKUP,
		ENTTYPE_FLAG,
		ENTTYPE_CHARACTER,
		ENTTYPE_NPC,
		ENTTYPE_DRAGGER,
		ENTTYPE_LASERGUN,
		NUM_ENTTYPES,

		WORLDTYPE_MAIN = 0,
		WORLDTYPE_EVENT,
		WORLDTYPE_GAMEMATCH,//aka 1n1
	};

private:
	void Reset();
	void RemoveEntities();

	CEventHandler m_EventHandler;
	CAnimationHandler m_AnimationHandler;
	int m_WorldType;
	CGameMap *m_pGameMap;
	CGameContext *m_pGameServer;
	IServer *m_pServer;

	CEntity *m_pNextTraverseEntity;
	CEntity *m_apFirstEntityTypes[NUM_ENTTYPES];

public:
	bool m_ResetRequested;
	CSrvWorldCore m_Core;

	CGameWorld(int WorldType, CGameMap *pGameMap);
	virtual ~CGameWorld();//compiler detects this class as a base class

	CEntity *FindFirst(int Type);

	/*
		Function: find_entities
			Finds entities close to a position and returns them in a list.

		Arguments:
			pos - Position.
			radius - How close the entities have to be.
			ents - Pointer to a list that should be filled with the pointers
				to the entities.
			max - Number of entities that fits into the ents array.
			type - Type of the entities to find.

		Returns:
			Number of entities found and added to the ents array.
	*/
	int FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type);
	int FindTees(vec2 Pos, float Radius, CEntity **ppEnts, int Max);

	/*
		Function: interserct_CCharacter
			Finds the closest CCharacter that intersects the line.

		Arguments:
			pos0 - Start position
			pos2 - End position
			radius - How for from the line the CCharacter is allowed to be.
			new_pos - Intersection position
			notthis - Entity to ignore intersecting with

		Returns:
			Returns a pointer to the closest hit or NULL of there is no intersection.
	*/
	class CCharacter *IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, class CEntity *pNotThis = 0);
	class CEntity *IntersectTee(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, class CEntity *pNotThis = 0);

	/*
		Function: closest_CCharacter
			Finds the closest CCharacter to a specific point.

		Arguments:
			pos - The center position.
			radius - How far off the CCharacter is allowed to be
			notthis - Entity to ignore

		Returns:
			Returns a pointer to the closest CCharacter or NULL if no CCharacter is close enough.
	*/
	class CCharacter *ClosestCharacter(vec2 Pos, float Radius, CEntity *ppNotThis, bool IntersectCollision = false);
	class CEntity *ClosestTee(vec2 Pos, float Radius, CEntity *ppNotThis, bool IntersectCollision = false);

	/*
		Function: insert_entity
			Adds an entity to the world.

		Arguments:
			entity - Entity to add
	*/
	void InsertEntity(CEntity *pEntity);

	/*
		Function: remove_entity
			Removes an entity from the world.

		Arguments:
			entity - Entity to remove
	*/
	void RemoveEntity(CEntity *pEntity);

	/*
		Function: destroy_entity
			Destroys an entity in the world.

		Arguments:
			entity - Entity to destroy
	*/
	void DestroyEntity(CEntity *pEntity);

	/*
		Function: snap
			Calls snap on all the entities in the world to create
			the snapshot.

		Arguments:
			snapping_client - ID of the client which snapshot
			is being created.
	*/
	virtual void Snap(int SnappingClient);

	/*
		Function: tick
			Calls tick on all the entities in the world to progress
			the world to the next tick.

	*/
	virtual void Tick();

	CAnimationHandler *AnimationHandler() { return &m_AnimationHandler; }
	CEventHandler *Events() { return &m_EventHandler; }
	int GetWorldType() const { return m_WorldType; }
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	CGameMap *GameMap() const { return m_pGameMap; }
};

#endif
