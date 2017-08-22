
#include "mapitems.h"

static const char *gs_ExtrasNames[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ "ID" },							//EXTRAS_TELEPORT_FROM
{ "ID" },							//EXTRAS_TELEPORT_TO
{ "Force", "Max Speed", "Angle"},	//EXTRAS_SPEEDUP
};

static const int gs_ExtrasSizes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ 4 },			//EXTRAS_TELEPORT_FROM
{ 4 },			//EXTRAS_TELEPORT_TO
{ 4, 4, 5},		//EXTRAS_SPEEDUP
};

//TODO add angles
static const int gs_ExtrasColumntypes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {}, // 1 = integer, 0 = string
{ 1 },			//EXTRAS_TELEPORT_FROM
{ 1 },			//EXTRAS_TELEPORT_TO
{ 1, 1, 1},		//EXTRAS_SPEEDUP
};