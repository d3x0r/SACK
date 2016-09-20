//#define INTERFACE_USED

#ifndef WORLD_CLIENT_LIBRARY
#define DeclareMarkers( name, Name, iName ) \
void Mark##Name##Created( uint32_t from_client_id, INDEX iWorld, INDEX iName );  \
void Mark##Name##Updated( uint32_t from_client_id, INDEX iWorld, INDEX iName );

DeclareMarkers( sector, Sector, iSector );
DeclareMarkers( line, Line, iLine );
DeclareMarkers( name, Name, iName );
DeclareMarkers( texture, Texture, iTexture );
DeclareMarkers( wall, Wall, iWall );
#endif
/* this should be common local.h sorta stuff... */

INDEX SrvrCreateSquareSector( uint32_t client_id, INDEX iWorld, PC_POINT pOrigin, RCOORD size );
int SrvrLoadWorldFromFile( uint32_t client_id, FILE *pFile, INDEX iWorld );
//int CPROC CheckWallInRect( uint32_t client_id, PWALL wall, PGROUPWALLSELECTINFO psi );
INDEX SrvrOpenWorld( uint32_t client_id, CTEXTSTR name );

void SrvrMarkWorldUpdated( INDEX client_id, INDEX iWorld );
INDEX SrvrAddConnectedSector( uint32_t client_id, INDEX iWorld, INDEX iWall, RCOORD offset );
int SrvrMoveWalls( uint32_t client_id, INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );
int SrvrRemoveWall( uint32_t client_id, INDEX iWorld, INDEX iWall );



#if 0
#ifndef WORLD_STRUCTURES_DEFINED
#define WORLD_STRUCTURES_DEFINED


typedef uintptr_t PSECTORSET, PSECTOR
		 , PNAME, PNAMESET
		 , PWALLSET, PWALL
		 , PMYLINESEGSET, PMYLINESEG
		 , PTEXTURE, PTEXTURESET;

typedef struct local_world_tag {
	uintptr_t real; // the handle of this in the server side.
} *PWORLD;

typedef struct named_thing_tag {
   char *name;
   uintptr_t thing;
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
