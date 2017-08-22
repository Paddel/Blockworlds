
#include "mapitems.h"

static const char *gs_ExtrasNames[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ "ID" },							//EXTRAS_TELEPORT_FROM
{ "ID" },							//EXTRAS_TELEPORT_TO
{ "Force", "Max Speed", "Angle"},	//EXTRAS_SPEEDUP
{},									//EXTRAS_FREEZE
{},									//EXTRAS_UNFREEZE
{ "ID", "Direction", "Default"},	//EXTRAS_DOOR
{ "ID", "Delay", "Activate"},		//EXTRAS_DOOR_HANDLE
};

static const int gs_ExtrasSizes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ 4 },			//EXTRAS_TELEPORT_FROM
{ 4 },			//EXTRAS_TELEPORT_TO
{ 4, 4, 5},		//EXTRAS_SPEEDUP
{},				//EXTRAS_FREEZE
{},				//EXTRAS_UNFREEZE
{ 4, 2, 3},		//EXTRAS_DOOR
{ 4, 3, 3},		//EXTRAS_DOOR_HANDLE
};

//TODO add angles
static const int gs_ExtrasColumntypes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {}, // 1 = integer, 0 = string
{ 1 },			//EXTRAS_TELEPORT_FROM
{ 1 },			//EXTRAS_TELEPORT_TO
{ 1, 1, 1},		//EXTRAS_SPEEDUP
{},				//EXTRAS_FREEZE
{},				//EXTRAS_UNFREEZE
{ 1, 0, 1},		//EXTRAS_DOOR
{ 1, 1, 1},		//EXTRAS_DOOR_HANDLE
};