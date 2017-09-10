
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

#define EXTRATILE_DATA 20
#define EXTRA_VERSION 0

// layer types
enum
{
	LAYERTYPE_INVALID = 0,
	LAYERTYPE_GAME, //not used anymore
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,
	LAYERTYPE_EXTRAS,

	MAPITEMTYPE_VERSION = 0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,


	CURVETYPE_STEP = 0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,

	// game layer tiles
	ENTITY_NULL = 0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_EVENT,
	ENTITY_SPAWN_1ON1,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_RIFLE,
	ENTITY_DRAGGER_WEAK,
	ENTITY_DRAGGER_MEDIUM,
	ENTITY_DRAGGER_STRONG,
	NUM_ENTITIES,

	TILE_AIR = 0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_FREEZE,
	TILE_UNFREEZE,
	TILE_FREEZE_DEEP,
	TILE_UNFREEZE_DEEP,
	TILE_RACE_START,
	TILE_RACE_FINISH,
	TILE_RESTART,
	TILE_ONEWAY_RIGHT,
	TILE_ONEWAY_UP,
	TILE_ONEWAY_LEFT,
	TILE_ONEWAY_DOWN,
	TILE_VIP,
	TILE_BESTCLAN,
	TILE_BARRIER_ENTITIES,
	TILE_ENDLESSHOOK,
	TILE_SUPERHAMMER,
	TILE_GIVEPASSIVE,
	TILE_BARRIER_LEVEL_1,
	TILE_BARRIER_LEVEL_50,
	TILE_BARRIER_LEVEL_100,
	TILE_BARRIER_LEVEL_200,
	TILE_BARRIER_LEVEL_300,
	TILE_BARRIER_LEVEL_400,
	TILE_BARRIER_LEVEL_500,
	TILE_BARRIER_LEVEL_600,
	TILE_BARRIER_LEVEL_700,
	TILE_BARRIER_LEVEL_800,
	TILE_BARRIER_LEVEL_900,
	TILE_BARRIER_LEVEL_999,
	TILE_EXTRAJUMP,
	NUM_TILES,

	EXTRAS_TELEPORT_FROM=1,
	EXTRAS_TELEPORT_TO,
	EXTRAS_SPEEDUP,
	EXTRAS_FREEZE,
	EXTRAS_UNFREEZE,
	EXTRAS_DOOR,
	EXTRAS_DOOR_HANDLE,
	EXTRAS_BUF1,//replace
	EXTRAS_ZONE_PROTECTION,
	EXTRAS_ZONE_SPAWN,
	EXTRAS_ZONE_UNTOUCHABLE,
	EXTRAS_MAP,
	EXTRAS_SELL_SKINMANI,
	EXTRAS_SELL_GUNDESIGN,
	EXTRAS_SELL_KNOCKOUT,
	EXTRAS_SELL_EXTRAS,
	EXTRAS_PLAYERCOUNT,
	EXTRAS_HOOKTHROUGH,
	EXTRAS_HOOKTHROUGH_TOP,
	EXTRAS_HOOKTHROUGH_BOTTOM,
	EXTRAS_HOOKTHROUGH_LEFT,
	EXTRAS_HOOKTHROUGH_RIGHT,
	EXTRAS_BUF0,//replace
	EXTRAS_INFO_LEVEL,
	EXTRAS_LASERGUN,
	EXTRAS_LASERGUN_TRIGGER,
	NUM_EXTRAS,

	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,

	LAYERFLAG_DETAIL=1,
	TILESLAYERFLAG_GAME=1,

	ENTITY_OFFSET=255-16*4,
};

struct CPoint
{
	int x, y; // 22.10 fixed point
};

struct CColor
{
	int r, g, b, a;
};

struct CQuad
{
	CPoint m_aPoints[5];
	CColor m_aColors[4];
	CPoint m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

class CExtrasData
{
public:
	char m_aData[EXTRATILE_DATA];
};


struct CMapItemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
} ;

struct CMapItemImage_v1
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemImage : public CMapItemImage_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Format;
};

struct CMapItemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
} ;


struct CMapItemGroup : public CMapItemGroup_v1
{
	enum { CURRENT_VERSION=3 };

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;

	int m_aName[3];

	int m_ExtrasData;
	unsigned char m_ExtraVersion;
} ;

struct CMapItemLayerQuads
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
} ;

struct CMapItemVersion
{
	int m_Version;
} ;

struct CEnvPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint &Other) { return m_Time < Other.m_Time; }
} ;

struct CMapItemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

struct CMapItemEnvelope : public CMapItemEnvelope_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Synchronized;
};

#endif
