#include <vidlib.h>
#define PLUGIN_MODULE
#include "plugins.h"

#define RX_NORTHWEST 0
#define RX_NORTH 1
#define RX_NORTHEAST 2
#define RX_WEST 3
#define RX_HERE 4  // place holder for near consistant geometry of room
#define RX_EAST 5
#define RX_SOUTHWEST 6
#define RX_SOUTH 7
#define RX_SOUTHEAST 8
#define RX_UP 9
#define RX_DOWN 10
#define RX_MAXEXITS 11

// Room Graphic will look something like this....
// note clipped corners for north/west segments to connect from...
//    ____      
//   /    \     
//  |      |    
//  |      |    
//  |      |    
//   \____/     
//

// if I go with something like the brainboard method of mapping
// the connections, then all segments on the path must have location
// information... otherwise I can use simple lines to connect...
// would need at least point holders to be able to route lines in
// some cases around other rooms, and back down....
// which also means that a room may have multiple leads into a side...
// a room may have multiple exits also... an elevator room is just such
// a case - if there were a button in a room that one pressed, then
// the room would change what the exits from it were....
// wonder if there are muds that this is the case... in my limited
// experience I have found none...                             

typedef struct coord_tag {
	int x, y;
} COORDINATE, *PCOORDINATE;

typedef struct link_tag {
   struct {
      int used:1;
      int door:1;
      int oneway:1;
      int inonly:1; // not an output path...
   } flags;
   
   struct room_tag *from, *to;
	PLIST coords;	   
} LINK, *PLINK;

typedef struct room_tag {
   PTEXT pZone; // name of the zone this room is in....
   PTEXT pRoom;
   PTEXT pDescription;
      // a exit may be used but not referenced... this indicates
      // that a starting segment should be drawn, but does not connect
      // to anywhere...
   PLINK exits[RX_MAXEXITS];
   int x, y; // display location center
   int height, depth; // 0, 0 for this room only...
                      // height 1 = one room above ...
                      // depth 1 = one room below ...
}ROOM, *PROOM;

typedef struct zone_tag {

}ZONE, *PZONE;

typedef struct map_tag {
   // perhaps this is mostly just visual data... since the map SHOULD
   // be ... well hmm maybe a PLIST of starting points on the map...
   // should also index the room names and or descriptions for quick 
   // location.... 
   HVIDEO hDisplay;
} MAP, *PMAP;



HMENU hMapMenu;
PLIST pMapObjects;
PLIST pMapDatapaths;

PMAP CreateMap( void )
{
	PMAP pMap;
	pMap = Allocate( sizeof( MAP ) );
	pMap->hDisplay = InitVideo( "Map Display" );
	return pMap;
}

void DestroyMap( PMAP pMap )
{
	FreeVideo( pMap->hDisplay );
	Release( pMap );
}

PDATAPATH OpenMapMon( PSENTIENT ps, PENTITY pe )
{
	return NULL;
}

int InitMapObject( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	if( !hMapMenu )
	{
		hMapMenu = CreatePopup();
//		AppendPopupItem( hMapMenu, 
	}	   
	SetLink( pe->pPlugins, myTypeID, CreateMap() );
	AddLink( pMapObjects, pe );
   return TRUE;
}

void DestroyMapObjects( void )
{
	INDEX idx;
	PENTITY pe;
	FORALL( pMapObjects, idx, pe )
	{
		DestroyMap( GetLink( pe->pPlugins, myTypeID ) );		
		SetLink( pe->pPlugins, myTypeID, 0 );
		DestroyEntity( pe );
	}
}

void CloseMapDatapaths( void )
{
 	INDEX idx;
 	PDATAPATH pdp;
 	FORALL( pMapDatapaths, idx, pdp )
 	{
 		
 	}
}
// $Log: mapper.c,v $
// Revision 1.2  2003/03/25 08:59:02  panther
// Added CVS logging
//
