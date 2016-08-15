#ifndef WORLD_STRUCTURES_DEFINED

#include <sack_types.h>
#include <vectlib.h>
#include <colordef.h>
#include <virtuality.h>

#define WORLD_STRUCTURES_DEFINED
//--------------------------------------------

typedef struct nameflag_tag {
	unsigned int bVertical : 1;
}NAMEFLAGS;

struct name_data {
	uint16_t length;
	TEXTSTR name;
};

typedef struct name_tag {
	NAMEFLAGS flags;
	uint16_t lines;
	struct name_data *name;
	int refcount; // number of times this name is referenced...
} NAME, *PNAME;

#define MAXNAMESPERSET 32
DeclareSet( NAME );

//--------------------------------------------

typedef struct textureflags_tag {
	unsigned int bColor;
} TEXTUREFLAGS, *PTEXTUREFLAGS;

typedef struct flatland_texture_tag {
	TEXTUREFLAGS flags;
	union {
		CDATA color;
	} data;
	INDEX iName;
	//PNAME name;
	int refcount;
} FLATLAND_TEXTURE, *PFLATLAND_TEXTURE;

#define MAXFLATLAND_TEXTURESPERSET 32
DeclareSet( FLATLAND_TEXTURE );

//--------------------------------------------

typedef struct FLATLAND_lineseg_tag {
	union {
		struct {
			RAY r; // direction/slope
			RCOORD dFrom, dTo;
		};
#if !defined( _MSC_VER ) && ( !defined( __WATCOMC__ ) && defined( __cplusplus ) )
		LINESEG seg;
#endif
	};
#ifdef OUTPUT_TO_VIRTUALITY
	PMYLINESEG pfl;
#endif
	int refcount; // count of walls which reference this (more important in 3d)
	struct {
		BIT_FIELD bUpdated : 1;
	} flags;
} FLATLAND_MYLINESEG, *PFLATLAND_MYLINESEG;

#define MAXFLATLAND_MYLINESEGSPERSET 256
DeclareSet( FLATLAND_MYLINESEG );

//--------------------------------------------

typedef struct wallflags_tag
{
	unsigned int bUpdating : 1; // set this while updating to prevent recursion...
	unsigned int wall_start_end : 1; // wall_at_start links from wall_at_end
   unsigned int wall_end_end : 1;   // wall_at_end links from wall_at_end
   unsigned int detached : 1; // joined by joined FAR
   unsigned int bSkipMate: 1; // used when updating sets of lines - avoid double update mating walls
} WALLFLAGS;

typedef struct wall_tag {
	WALLFLAGS          flags;
   INDEX iWorld;
	INDEX iSector;
   INDEX iName;
   INDEX iLine;
   INDEX iWallInto;
   INDEX iWallStart, iWallEnd;
   int refcount;
} WALL, *PWALL;

#define MAXWALLSPERSET 256
DeclareSet( WALL );

//--------------------------------------------

typedef struct sectorflags_tag {
	BIT_FIELD bBody : 1;
	BIT_FIELD bStaticBody : 1;
	BIT_FIELD bUnused : 1;
	BIT_FIELD bOpenShape : 1; // should be drawn with lines not filled polygons (line/curve)
} SECTORFLAGS;

typedef struct sector_tag {
	SECTORFLAGS 		  flags;
   INDEX iName;
   INDEX iWall; // one of the walls, circular link to follow walls
	RAY 					  r;  		// origin offset and orientation differential...
   INDEX iTexture;
   INDEX iWorld;
	// point list is dyanmically built by the client
   // it is determined by following the wall links.
	int 		 			  npoints;
	_POINT 				 *pointlist;
	PFACET facet;
	//PSPACENODE 			  spacenode;
} SECTOR, *PSECTOR;

#define MAXSECTORSPERSET 128
DeclareSet( SECTOR );
//--------------------------------------------

typedef struct body_tag {
   // list of methods which are called when body is stepped in time.
	PLIST peripheral_tick;
	PTRANSFORM motion; // my current inertial frame;
   INDEX iSector; // sector in world which is me.
} FLATLANDER_BODY, *PFLATLANDER_BODY;

#define MAXFLATLANDER_BODYSPERSET 128
DeclareSet( FLATLANDER_BODY );

//--------------------------------------------


typedef struct UndoRecord_tag 
{
	int type; // from enum
	INDEX iWorld; // ?? not sure...
	union {
		struct {
			char byte;
		} nothing;
		struct {
			INDEX pWall;
			FLATLAND_MYLINESEG original;
		} end_start;
		struct {
			int nwalls;
			INDEX *walls;
			FLATLAND_MYLINESEG *lines;
		} wallset;
		struct {
			int nsectors;
INDEX *sectors;
			_POINT origin;
			int bCompleted;
			_POINT endorigin;
		} sectorset;
	} data;
	struct UndoRecord_tag *next;
}UNDORECORD, *PUNDORECORD;

struct UpdateThingMsg {
	INDEX iWorld;
	INDEX iThing;
	POINTER pThing;
};


struct flagset
{
		uint32_t num;
		uint32_t *flags;
};

struct all_flagset
{
	struct flagset lines;
	struct flagset walls;
	struct flagset sectors;
	struct flagset names;
	struct flagset textures;
	FLAGSET( deleteset, 5 ); // count of sets to delete entirely.
};

typedef struct world_tag {
	PFLATLAND_MYLINESEGSET lines;
	PWALLSET    walls;
	PSECTORSET  sectors;
   // statics are bodies - but more like bodies for environment
	//PSTATICSET  statics; // destructable?
   // bodies
	PFLATLANDER_BODYSET  bodies;
	PNAMESET    names;
	PFLATLAND_TEXTURESET textures;

	/* are these client layer things? */
	PUNDORECORD firstundo;
	PUNDORECORD firstredo; // maybe - redo undos?
	INDEX		name; // index of name in nameset. // portable pointer.
	POBJECT object;

	PDATAQUEUE UpdatedLines;
	CRITICALSECTION csDeletions;
	struct all_flagset deletions;
} WORLD, *PWORLD;

#define MAXWORLDSPERSET 16
DeclareSet( WORLD );
#if !defined( WORLD_SOURCE )
/*
// PWORLD itself should not be needed ever by the
// application...
WORLD_PROC( PWALL, GetWall )(INDEX world,INDEX index);
WORLD_PROC( INDEX, GetWallIndex )(INDEX world, PWALL wall);

WORLD_PROC( PFLATLAND_MYLINESEG, GetLine )(INDEX world, INDEX index);
WORLD_PROC( INDEX, GetLineIndex )(INDEX world, PFLATLAND_MYLINESEG line);

WORLD_PROC( PNAME, GetName )(INDEX world, INDEX idx);

WORLD_PROC( PFLATLAND_TEXTURE, GetTexture )(INDEX world,INDEX idx);

WORLD_PROC( PSECTOR, GetSector )(INDEX world,INDEX idx);
*/
#else

#define GetWall(index) 				GetSetMember( WALL, &world->walls, (index) )
#define GetWallIndex(wall)       GetMemberIndex( WALL, &world->walls, (wall) )

#define GetLine(index)           GetSetMember( FLATLAND_MYLINESEG, &world->lines, index )
#define GetLineIndex(line)       GetMemberIndex( FLATLAND_MYLINESEG, &world->lines, line )

#define GetName(idx)             GetSetMember( NAME, &world->names, idx )
#define GetTexture(idx)          GetSetMember( FLATLAND_TEXTURE, &world->textures, idx )

#define GetSector(idx) 				GetSetMember( SECTOR, &world->sectors, idx )

#ifdef SERVICE_SOURCE
#define GETWORLD(index) PWORLD world = GetUsedSetMember( WORLD, &g.worlds, index )
#else
#ifdef WORLD_SCAPE_CLIENT
#define GETWORLD(index) PWORLD world = GetUsedSetMember( WORLD, &g.worlds, index )
#else
#define GETWORLD(index) PWORLD world = GetUsedSetMember( WORLD, &g.worlds, index )
#endif
#endif

#define GetWall(index) 				GetSetMember( WALL, &world->walls, (index) )
#define GetWallIndex(wall)       GetMemberIndex( WALL, &world->walls, (wall) )

#define GetLine(index)           GetSetMember( FLATLAND_MYLINESEG, &world->lines, index )
#define GetLineIndex(line)       GetMemberIndex( FLATLAND_MYLINESEG, &world->lines, line )

#define GetName(idx)             GetSetMember( NAME, &world->names, idx )
#define GetTexture(idx)          GetSetMember( FLATLAND_TEXTURE, &world->textures, idx )

#define GetSector(idx) 				GetSetMember( SECTOR, &world->sectors, idx )
#define GetSectorIndex(idx)      GetMemberIndex( SECTOR, world->sectors, idx )

#define DeleteWall(pset, item)             DeleteFromSet( WALL, (pset), (item) )
#define DeleteIWall(pset, item)             DeleteSetMember( WALL, (pset), (item) )


#define GetLinearWallArray(set, nCount)    (PWALL*)GetLinearSetArray( WALL, set, nCount )
#define GetLinearSectorArray(set, nCount)    (PSECTOR*)GetLinearSetArray( SECTOR, set, nCount )
#define GetLinearNameArray(set, pLines)    (PNAME*)GetLinearSetArray( NAME, set, pLines )
#define GetLinearTextureArray(set, nCount)    (PFLATLAND_TEXTURE*)GetLinearSetArray( FLATLAND_TEXTURE, set, nCount )
#define GetLinearLineArray(set, nCount )      (PFLATLAND_MYLINESEG*)GetLinearSetArray( FLATLAND_MYLINESEG, set, nCount )
#define ForAllWalls(set, f, psv)           ForAllInSet( WALL, (set), (FAISCallback)(f), (uintptr_t)(psv) )

#define DeleteSector(pset, item) 				DeleteFromSet( SECTOR, (pset), (item) )

#define DoForAllSectors(set, f, psv)           ForEachSetMember( SECTOR, (set), (FESMCallback)(f), (uintptr_t)(psv) )
#define DoForAllWalls(set, f, psv)           ForEachSetMember( WALL, (set), (FESMCallback)(f), (uintptr_t)(psv) )
#define DoForAllLines(set, f, psv)           ForEachSetMember( FLATLAND_MYLINESEG, (set), (FESMCallback)(f), (uintptr_t)(psv) )


#define DeleteLine(set,line)  DeleteFromSet( FLATLAND_MYLINESEG, (set), (line) )
#define CountUsedLines(set)   CountUsedInSet( FLATLAND_MYLINESEG, (set) )
#define FindLineInArray(array, size, line) FindInArray( array, size, line )

#ifndef WORLD_SERVICE
#define SetLine( isetthis, pline ) { 				\
	if( isetthis != INVALID_INDEX )      			\
	{                              				\
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, isetthis ); \
			if( !(--(psetthis)->refcount))   		\
				DeleteLine( world->lines, (psetthis) ); \
		}                              				\
		if( pline != INVALID_INDEX )              \
		{                                         \
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pline ); \
			(isetthis) = (pline);                  \
			(psetthis)->refcount++;                \
		}                                         \
	}
#endif

#define SetLineWorld( world, isetthis, pline ) { 				\
	if( isetthis != INVALID_INDEX )      			\
	{                              				\
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, isetthis ); \
			(psetthis)[0] = (pline)[0];                  \
	}}


//#define ForAllTextures(set,f,psv) ForAllInSet( TEXTURE, (set), (FAISCallback)(f), (uintptr_t)(psv) )
//#define SetTexture( isetthis, itexture ) (((isetthis!=INVALID_INDEX)?(GetSetMember(TEXTURE,&world->textures,isetthis))->refcount--:0),(itexture!=INVALID_INDEX)?((isetthis) = (itexture), (GetSetMember(TEXTURE,&world->textures,itexture))->refcount++):0)

#include <world.h>

#endif
#endif

//---------------------------------------------------------------
//
// $Log: worldstrucs.h,v $
// Revision 1.6  2005/01/30 22:48:25  panther
// Progress - udpateing to new world API
//
// Revision 1.5  2005/01/03 07:03:23  panther
// Compiles - I'm sure there's technical errors left
//
// Revision 1.4  2004/12/30 21:11:55  panther
// checkpoint
//
// Revision 1.3  2004/12/23 11:27:47  panther
// minor progress
//
// Revision 1.2  2004/12/22 20:13:36  panther
// Checkpoint
//
// Revision 1.1  2004/03/21 14:40:30  panther
// Initial commit
//
// Revision 1.1  2004/03/19 17:00:44  panther
// Seperating engine from display
//
//
