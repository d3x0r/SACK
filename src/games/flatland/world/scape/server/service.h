//#define INTERFACE_USED

#define DeclareMarkers( name, Name, iName ) \
void Mark##Name##Created( INDEX iWorld, INDEX iName );  \
void Mark##Name##Updated( INDEX iWorld, INDEX iName );

void MarkWorldUpdated( INDEX iWorld );
DeclareMarkers( sector, Sector, iSector );
DeclareMarkers( line, Line, iLine );
DeclareMarkers( name, Name, iName );
DeclareMarkers( texture, Texture, iTexture );
DeclareMarkers( wall, Wall, iWall );

// have to allow certain things to flush events
// (such as the ResetWorld before LoadWorld()
void UpdateClients( void );


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
