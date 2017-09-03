
#include "extras.h"

const char *gs_ExtrasNames[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ "ID" },							//EXTRAS_TELEPORT_FROM
{ "ID" },							//EXTRAS_TELEPORT_TO
{ "Force", "Max Speed", "Angle"},	//EXTRAS_SPEEDUP
{},									//EXTRAS_FREEZE
{},									//EXTRAS_UNFREEZE
{ "ID", "Direction", "Default"},	//EXTRAS_DOOR
{ "ID", "Delay", "Activate"},		//EXTRAS_DOOR_HANDLE
{},									//EXTRAS_ZONE_BLOCK
{},									//EXTRAS_ZONE_PROTECTION
{},									//EXTRAS_ZONE_SPAWN
{},									//EXTRAS_ZONE_UNTOUCHABLE
{"Name"},							//EXTRAS_MAP
{ "Name"},							//EXTRAS_SELL_SKINMANI
{ "Name" },							//EXTRAS_SELL_GUNDESIGN
{ "Name" },							//EXTRAS_SELL_KNOCKOUT
{ "Name" },							//EXTRAS_SELL_EXTRAS
{ "Map" },							//EXTRAS_PLAYERCOUNT
};


int gs_ExtrasSizes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ 4 },			//EXTRAS_TELEPORT_FROM
{ 4 },			//EXTRAS_TELEPORT_TO
{ 4, 4, 5},		//EXTRAS_SPEEDUP
{},				//EXTRAS_FREEZE
{},				//EXTRAS_UNFREEZE
{ 4, 2, 3},		//EXTRAS_DOOR
{ 4, 3, 3},		//EXTRAS_DOOR_HANDLE
{},				//EXTRAS_ZONE_BLOCK
{},				//EXTRAS_ZONE_PROTECTION
{},				//EXTRAS_ZONE_SPAWN
{},				//EXTRAS_ZONE_UNTOUCHABLE
{ 19 },			//EXTRAS_MAP
{ 19 },			//EXTRAS_SELL_SKINMANI
{ 19 },			//EXTRAS_SELL_GUNDESIGN
{ 19 },			//EXTRAS_SELL_KNOCKOUT
{ 19 },			//EXTRAS_SELL_EXTRAS
{ 19 },			//EXTRAS_PLAYERCOUNT
};

//TODO add angles
int gs_ExtrasColumntypes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {}, // 1 = integer, 0 = string
{ 1 },			//EXTRAS_TELEPORT_FROM
{ 1 },			//EXTRAS_TELEPORT_TO
{ 1, 1, 1},		//EXTRAS_SPEEDUP
{},				//EXTRAS_FREEZE
{},				//EXTRAS_UNFREEZE
{ 1, 0, 1},		//EXTRAS_DOOR
{ 1, 1, 1},		//EXTRAS_DOOR_HANDLE
{},				//EXTRAS_ZONE_BLOCK
{},				//EXTRAS_ZONE_PROTECTION
{},				//EXTRAS_ZONE_SPAWN
{},				//EXTRAS_ZONE_UNTOUCHABLE
{ 0 },			//EXTRAS_MAP
{ 0 },			//EXTRAS_SELL_SKINMANI
{ 0 },			//EXTRAS_SELL_GUNDESIGN
{ 0 },			//EXTRAS_SELL_KNOCKOUT
{ 0 },			//EXTRAS_SELL_EXTRAS
{ 0 },			//EXTRAS_PLAYERCOUNT
};