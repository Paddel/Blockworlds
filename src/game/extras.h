
#include "mapitems.h"

static const char *gs_ExtrasNames[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ "ID" }, //EXTRAS_TELEPORT_FROM
{ "ID" }, //EXTRAS_TELEPORT_TO
};

static const int gs_ExtrasSizes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
{ 3 }, //EXTRAS_TELEPORT_FROM
{ 3 }, //EXTRAS_TELEPORT_TO
};

static const int gs_ExtrasColumntypes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {}, // 1 = integer, 0 = string
{ 1 }, //EXTRAS_TELEPORT_FROM
{ 1 }, //EXTRAS_TELEPORT_TO
};