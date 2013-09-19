
#ifndef WORLD_SERVICE
#define WORLD_SERVICE
#endif


#include <world.h>

typedef struct client_tag {
	DeclareLink( struct client_tag );
	struct {
		// if the appropraite create flag is set
		// said world does not get the create event
		_32 bCreateWorld : 1;
		_32 bCreateName : 1;
		_32 bCreateSector : 1;
		_32 bCreateTexture : 1;
		_32 bCreateLine : 1;
		_32 bCreateWall : 1;
	} flags;
	_32 pid;
} WORLD_CLIENT, *PWORLD_CLIENT;

typedef struct server_global_tag
{
	_32 MsgBase; // my message base?
	PWORLD_CLIENT clients;
	PWORLDSET worlds;
} SERVER_GLOBAL;

#define GLOBAL SERVER_GLOBAL
#define DoDefineMarkers( Name ) void Mark##Name##Updated( INDEX iWorld, INDEX iName ) ; void Mark##Name##Deleted( INDEX iWorld, INDEX iName );
DoDefineMarkers( Name );
DoDefineMarkers( Wall );
DoDefineMarkers( Sector );
DoDefineMarkers( Line );
DoDefineMarkers( Texture );
//DoDefineMarkers( Name );
void ClientCreateWorld_init( PSERVICE_ROUTE iClient, INDEX iWorld );
// defined in world.c for service.c to use
INDEX OpenWorld( _32 iClient, CTEXTSTR name );
// defined in service.c for world.c to use.
void MarkAllInWorldUpdated( PSERVICE_ROUTE pidClient, INDEX iWorld );

void ResetWorld( INDEX iWorld );
int MergeWalls( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall );
void GetLineData( INDEX iWorld, INDEX iLine, PFLATLAND_MYLINESEG *ppls );


INDEX CreateSquareSector( INDEX iWorld, PC_POINT pOrigin, RCOORD size );

