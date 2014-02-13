#define WORLD_SOURCE

#define DEKWARE_PLUGIN
#include "../../dekware/include/plugin.h"
#include "global.h"

typedef struct local_tag {
	INDEX iWorld;
} LOCAL;
static LOCAL l;

#if 0
GetVariableFunc GetSectorCount;
GetVariableFunc GetCurrentSector;
GetVariableFunc GetCurrentSectors;
GetVariableFunc GetWallCount;
GetVariableFunc GetCurrentWall;


volatile_variable_entry worldvars[] = { { DEFTEXT( "sectors" ), GetSectorCount }
												  , { DEFTEXT( "current_sector" ), GetCurrentSector }
												  , { DEFTEXT( "current_sectors" ), GetCurrentSectors }
												  , { DEFTEXT( "walls" ), GetWallCount }
												  , { DEFTEXT( "current_wall" ), GetCurrentWall }
};
#endif

static command_entry methods[] = { { DEFTEXT( "create" ), 1, 6, DEFTEXT( "create a thing... sector" ), NULL }
								  , { DEFTEXT( "select" ), 1, 6, DEFTEXT( "select a thing... <sector <origin>,wall <origin/left/right>>" ), NULL }
								  , { DEFTEXT( "mark" ), 1, 4, DEFTEXT( "mark walls with a stroke/sectors with a rect" ), NULL }
								  , { DEFTEXT( "break" ), 1, 5, DEFTEXT( "break selected wall, resulting walls remain selected" ), NULL }
};


static int CPROC CreateWorldEntity( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
   // optional parameter may be named world to load (file, database?)
	{
		INDEX iWorld = OpenWorld( GetText( pe->pName ) );
		SetLink( &pe->pPlugin, l.iWorld, iWorld );
		AddMethod( pe, methods );
		AddMethod( pe, methods + 1 );
		AddMethod( pe, methods + 2 );
		AddMethod( pe, methods + 3 );
		return 0;
	}
   //return 1; // failure - error code? message?
}

PUBLIC( char *, RegisterRoutines )( void )
{
   l.iWorld = RegisterExtension( "World" );
	// hmm Methods
	//   /make world world_scape [filename]
	//   /world/methods
	//     /world/select sector 0
	//     /world/select wall <1 0 0>
	//     /world/select origin
   //     /world/select point
	//     /world/create sector # default 1 square unitoffset away from this wall, and attached
	//     /world/mark %sector <1 0 0>
   //     /world/break
	//     /world/merge
   //
	//     /world/move [sector/wall/point]
   //     /world/rotate [wall/sector]
	//
	//   /make editor world_eidtor world
	//     #now the editor is freely attached to the world
	//     # the editor is able to get the updates to the world done by this script
	//     # this module could <I suppose> result with information about corrdinates, etc...
	//
	RegisterObject( "world_scape", "A server for a single world...", CreateWorldEntity );
   return DekVersion;
}
PUBLIC( void, UnloadPlugin )( void )
{
   // not much to do here yet...
}



