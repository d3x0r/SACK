//#define INTERFACE_USED

#ifndef WORLD_CLIENT_LIBRARY
#define DeclareMarkers( name, Name, iName ) \
void Mark##Name##Created( _32 from_client_id, INDEX iWorld, INDEX iName );  \
void Mark##Name##Updated( _32 from_client_id, INDEX iWorld, INDEX iName );

DeclareMarkers( sector, Sector, iSector );
DeclareMarkers( line, Line, iLine );
DeclareMarkers( name, Name, iName );
DeclareMarkers( texture, Texture, iTexture );
DeclareMarkers( wall, Wall, iWall );
#endif
/* this should be common local.h sorta stuff... */

INDEX SrvrCreateSquareSector( _32 client_id, INDEX iWorld, PC_POINT pOrigin, RCOORD size );
int SrvrLoadWorldFromFile( _32 client_id, FILE *pFile, INDEX iWorld );
//int CPROC CheckWallInRect( _32 client_id, PWALL wall, PGROUPWALLSELECTINFO psi );
INDEX SrvrOpenWorld( _32 client_id, CTEXTSTR name );

void SrvrMarkWorldUpdated( INDEX client_id, INDEX iWorld );
INDEX SrvrAddConnectedSector( _32 client_id, INDEX iWorld, INDEX iWall, RCOORD offset );
int SrvrMoveWalls( _32 client_id, INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );
int SrvrRemoveWall( _32 client_id, INDEX iWorld, INDEX iWall );



#if 0
#ifndef WORLD_STRUCTURES_DEFINED
#define WORLD_STRUCTURES_DEFINED


typedef PTRSZVAL PSECTORSET, PSECTOR
		 , PNAME, PNAMESET
		 , PWALLSET, PWALL
		 , PMYLINESEGSET, PMYLINESEG
		 , PTEXTURE, PTEXTURESET;

typedef struct local_world_tag {
	PTRSZVAL real; // the handle of this in the server side.
} *PWORLD;

typedef struct named_thing_tag {
   char *name;
   PTRSZVAL thing;
} NAMED_THING;

typedef union texture_tag {
	NAMED_THING base;
	struct
	{
		char *name;
      PTEXTURE texture;
	};
} Texture;



#endif

#endif
