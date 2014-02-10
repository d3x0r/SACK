
#define WORLDSCAPE_INTERFACE_USED

#include <sack_types.h>
#include <deadstart.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#include <timers.h>
#include <world.h>
#include <worldstrucs.h>
#include <msgclient.h>
//#include <msgserver.h>

#include "global.h"

#include "world.h"
#include "service.h"
#include "names.h"
#include "sector.h"
#include "texture.h"
extern GLOBAL g;
//extern PWORLDSET worlds; // this is a member of global

typedef struct worldscape_client WORLDSCAPE_CLIENT, *PWORLDSCAPE_CLIENT;

typedef struct client_world_tracker CLIENT_WORLD_TRACKER, *PCLIENT_WORLD_TRACKER;

struct bitset
{
	_32 num;
	FLAGSETTYPE *used; // dynamic array of bits indicating that this line is known?
	FLAGSETTYPE *created; // created status to client... for each client... 
	FLAGSETTYPE *updated;
	FLAGSETTYPE *allow_update; // to be used - check worlds.created usage that indicates sourced update allow
};

struct client_world_tracker
{
	struct bitset names;
	struct bitset textures;
	struct bitset lines;
	struct bitset walls;
	struct bitset sectors;
};

struct worldscape_client
{
	struct bitset worlds;

	//_32 tracker_count; // same as worlds->num
	/*
	* this is an array of bitsets... one for each world.  
	*/
	PCLIENT_WORLD_TRACKER world_trackers;
	_32 pid;
};

static struct worldscape_server_local
{
	_32 SrvrMsgBase;
	PLIST clients; // list of PWORLDSCAPE_CLIENTs
}l;
//#if 0

void ExpandBitset( struct bitset *pbitset, INDEX least )
{
	/* should do something smarter here rather than looping every 32 increment. */
	if( least >= pbitset->num )
	{
		_32 old_count = pbitset->num;
		FLAGSETTYPE *new_created_flags;
		FLAGSETTYPE *new_updated_flags;
		FLAGSETTYPE *new_used_flags;
		pbitset->num += (32 > (least-pbitset->num))?32:((least+31)& (~31));

		new_created_flags = NewArray( _32, ( pbitset->num / 32 ) );
		new_updated_flags = NewArray( _32, ( pbitset->num / 32 ) );
		new_used_flags = NewArray( _32, ( pbitset->num / 32 ) );
		MemSet( new_created_flags + ( old_count / 32 ), 0, sizeof( _32 ) );
		MemSet( new_updated_flags + ( old_count / 32 ), 0, sizeof( _32 ) );
		MemSet( new_used_flags + ( old_count / 32 ), 0, sizeof( _32 ) );
		if( old_count )
		{
			MemCpy( new_created_flags
					, pbitset->created
					, ( old_count / 32 ) );
			MemCpy( new_updated_flags
					, pbitset->created
					, ( old_count / 32 ) );
			MemCpy( new_used_flags
					, pbitset->used
					, ( old_count / 32 ) );
			Release( pbitset->created );
			Release( pbitset->updated );
			Release( pbitset->used );
		}
		pbitset->created = new_created_flags;
		pbitset->updated = new_updated_flags;
		pbitset->used = new_used_flags;
	}
}

//---------------------------------------------------------------

#define DefineMarkers( name, Name, iName ) \
void Mark##Name##UpdatedEx( _32 source_id, _32 client_id, INDEX iWorld, INDEX iName )   \
{  \
	INDEX iClient;                     \
	PWORLDSCAPE_CLIENT client;         \
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )  \
	{                                  \
		if( client->pid == source_id ) \
  			continue;                  \
		if( client->world_trackers \
			&& (TESTFLAG( client->worlds.created, iWorld )   \
				||TESTFLAG( client->worlds.updated, iWorld )) )  \
		{                                                                      \
			PCLIENT_WORLD_TRACKER world; \
			world = client->world_trackers + iWorld;  \
			ExpandBitset( &world->name##s, iName );  \
			/*lprintf( "Marking "#name" %d updated", iName );*/\
			SETFLAG( world->name##s.used, iName);   \
			SETFLAG( world->name##s.updated, iName);   \
			SETFLAG( client->worlds.updated, iWorld );  \
		}  \
	}  \
}\
	void Mark##Name##Updated( _32 client_id, INDEX iWorld, INDEX iName )  { Mark##Name##UpdatedEx( 0/*safe default, no pid will be 0, therefore skip none, send all*/, client_id, iWorld, iName ); } \

DefineMarkers( sector, Sector, iSector );
DefineMarkers( line, Line, iLine );
DefineMarkers( name, Name, iName );
DefineMarkers( texture, Texture, iTexture );
DefineMarkers( wall, Wall, iWall );

//---------------------------------------------------------------

static PTRSZVAL CPROC SetBitOn( INDEX idx, PTRSZVAL psv )
{
	struct bitset *setinbitset = (struct bitset *)psv;
	ExpandBitset( setinbitset, idx );
	/* created? */
	SETFLAG( setinbitset->updated, idx );
	SETFLAG( setinbitset->used, idx );
	return 0;
}

/* for each thing created, mark as updated and used */
static void SyncCreateUpdate( PWORLD world, PCLIENT_WORLD_TRACKER client )
{
	ForEachSetMember( SECTOR, world->sectors, SetBitOn, (PTRSZVAL)&client->sectors );
	ForEachSetMember( FLATLAND_TEXTURE, world->textures, SetBitOn, (PTRSZVAL)&client->textures );
	ForEachSetMember( NAME, world->names, SetBitOn, (PTRSZVAL)&client->names );
	ForEachSetMember( FLATLAND_MYLINESEG, world->lines, SetBitOn, (PTRSZVAL)&client->lines );
	ForEachSetMember( WALL, world->walls, SetBitOn, (PTRSZVAL)&client->walls );
}

//---------------------------------------------------------------

LOGICAL ClientCreateWorld( PWORLDSCAPE_CLIENT client, INDEX iWorld )
{
	/* magic routine, so only clients that have actually requested this world get it.*/
	{
		if( !TESTFLAG( client->worlds.created, iWorld ) )
		{
			PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
			if( world )
			{
				SETFLAG( client->worlds.updated, iWorld );
				SETFLAG( client->worlds.created, iWorld );
				SendMultiServiceEvent( client->pid, l.SrvrMsgBase + MSG_EVENT_CREATEWORLD, 2
										, &iWorld, sizeof( INDEX )
										, &world->name, sizeof( INDEX )
										);
				SyncCreateUpdate( world, client->world_trackers );
			}
			else
			{
				// not updated!?
				RESETFLAG( client->worlds.updated, iWorld );
				return FALSE;
			}
		}
	}
	return TRUE;
}


void MarkWorldUpdated( INDEX iWorld )
{
}
/* Mark on this client that this world has been created...
 * that this client has this world.  
 * this is really an exclusive operation for open/create world.
 * 
 */
void SrvrMarkWorldUpdated( INDEX client_id, INDEX iWorld )
{
	PWORLDSCAPE_CLIENT client;
	INDEX iClient;	
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )
		if( client->pid == client_id )
		{
	_32 old_count = client->worlds.num;


	/* only update those worlds that this client has 'created' */
	if( iWorld >= old_count )
	{
		PCLIENT_WORLD_TRACKER new_trackers;
		client->worlds.num = iWorld; 
		new_trackers = NewArray( CLIENT_WORLD_TRACKER, client->worlds.num+1 );
		MemSet( new_trackers + old_count
			, 0
			, ( client->worlds.num - old_count + 1 ) * sizeof( client->world_trackers[0] ) );
		if( old_count )
		{
			MemCpy( new_trackers, client->world_trackers
				, sizeof( client->world_trackers[0] ) * old_count );
			Release( client->world_trackers );
		}
		//client->tracker_count = client->worlds.num+1;
		client->world_trackers = new_trackers;
	}
	ExpandBitset( &client->worlds, iWorld );

	SETFLAG( client->worlds.updated, iWorld );
		}
}

//void BroadcastMultiServiceEvent( )

void ClientCreateName( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iName, LOGICAL bCreate )
{
	/* end up having to build the name before broadcast... */
	PWORLD world  = GetSetMember( WORLD, &g.worlds, iWorld );
	PNAME name = GetSetMember( NAME, &world->names, iName );
	TEXTCHAR *namedata;
	int n, len;
	len = 0;
	for( n = 0; n < name->lines; n++ )
		len += name->name[n].length + 1;

	namedata = NewArray( TEXTCHAR, len + 1 );
	namedata[len] = 0;
	len = 0;
	for( n = 0; n < name->lines; n++ )
	{
		MemCpy( namedata + len, name->name[n].name, name->name[n].length + 1 );
		len += name->name[n].length + 1;
	}
	{
			PCLIENT_WORLD_TRACKER pcwt;
			pcwt = client->world_trackers + iWorld;

			ExpandBitset( &pcwt->names, iName );
			if( !TESTFLAG( pcwt->names.created, iName ) )
			{
				SETFLAG( pcwt->names.created, iName );
				SendMultiServiceEvent( client->pid, MSG_EVENT_CREATENAME + l.SrvrMsgBase, 5
										, &iWorld, sizeof( INDEX )
										, &iName, sizeof( INDEX )
										, &name->flags, offsetof( NAME, name )
										, namedata, len );
			}
	}
	Release( namedata );
}

void ClientCreateLine( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iLine, LOGICAL bCreate )
{
	PWORLD world  = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEG line = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine );
	PCLIENT_WORLD_TRACKER pcwt;
	pcwt = client->world_trackers + iWorld;
			ExpandBitset( &pcwt->lines, iLine );
			if( !TESTFLAG( pcwt->lines.created, iLine ) )
			{
				SETFLAG( pcwt->lines.created, iLine );
				SendMultiServiceEvent( client->pid, MSG_EVENT_CREATELINE + l.SrvrMsgBase, 3
								, &iWorld, sizeof( INDEX )
								, &iLine, sizeof( INDEX )
								, line, sizeof( FLATLAND_MYLINESEG )// yes this is NOT sizeof( MYLINESEG )!!
								);
			}
			else if( TESTFLAG( pcwt->lines.updated, iLine ) )
			{
				RESETFLAG( pcwt->lines.updated, iLine );
				SendMultiServiceEvent( client->pid, MSG_EVENT_UPDATELINE + l.SrvrMsgBase, 3
								, &iWorld, sizeof( INDEX )
								, &iLine, sizeof( INDEX )
								, line, sizeof( FLATLAND_MYLINESEG )// yes this is NOT sizeof( MYLINESEG )!!
								);
			}
}

void ClientCreateWall( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iWall, LOGICAL bCreate )
{
	PWORLD world  = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	if( world )
	{
	PCLIENT_WORLD_TRACKER pcwt;
	PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	pcwt = client->world_trackers + iWorld;
	if( wall )
	{
			ExpandBitset( &pcwt->walls, iWall );
			if( !TESTFLAG( pcwt->walls.created, iWall ) )
			{
				SETFLAG( pcwt->walls.created, iWall );
				SendMultiServiceEvent( client->pid, MSG_EVENT_CREATEWALL + l.SrvrMsgBase
					, 3
								, &iWorld, sizeof( INDEX )
								, &iWall, sizeof( INDEX )
								, wall, sizeof( WALL ) );
			}
	}
	}
}

void ClientCreateTexture( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iTexture, LOGICAL bCreate )
{
	PWORLD world  = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	PCLIENT_WORLD_TRACKER pcwt;
	pcwt = client->world_trackers + iWorld;

	ExpandBitset( &pcwt->textures, iTexture );
	if( !TESTFLAG( pcwt->textures.created, iTexture ) )
	{
		SETFLAG( pcwt->textures.created, iTexture );
		SendMultiServiceEvent( client->pid, MSG_EVENT_CREATETEXTURE + l.SrvrMsgBase, 3
						, &iWorld, sizeof( INDEX )
						, &iTexture, sizeof( INDEX )
						, texture, sizeof( FLATLAND_TEXTURE )// yes this is NOT sizeof( MYLINESEG )!!
						);
	}
}

void ClientCreateSector( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iSector, LOGICAL bCreate )
{
	PWORLD world  = GetSetMember( WORLD, &g.worlds, iWorld );
	PCLIENT_WORLD_TRACKER pcwt;

			PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );
			pcwt = client->world_trackers + iWorld;

			ExpandBitset( &pcwt->sectors, iSector );
			if( !TESTFLAG( pcwt->sectors.created, iSector ) )
			{
				SETFLAG( pcwt->sectors.created, iSector );
				SendMultiServiceEvent( client->pid, MSG_EVENT_CREATESECTOR + l.SrvrMsgBase, 3
								, &iWorld, sizeof( INDEX )
								, &iSector, sizeof( INDEX )
								, sector, sizeof( SECTOR )
								//, sector->pointlist, sizeof( sector->pointlist[0] ) * sector->npoints

							);
			}
			else if( TESTFLAG( pcwt->sectors.updated, iSector ) )
			{
				RESETFLAG( pcwt->sectors.updated, iSector );
				SendMultiServiceEvent( client->pid, MSG_EVENT_UPDATESECTOR + l.SrvrMsgBase, 3
								, &iWorld, sizeof( INDEX )
								, &iSector, sizeof( INDEX )
								, sector, sizeof( SECTOR )// yes this is NOT sizeof( MYLINESEG )!!
								);
			}
}



//---------------------------------------------------------------

void UpdateClients( void )
{
	int locked = 0;
	PWORLDSCAPE_CLIENT client;
	INDEX iClient;

	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )
	{
		INDEX iWorld;
		for( iWorld = 0; iWorld < client->worlds.num; iWorld++ )
		{
			if( TESTFLAG( client->worlds.updated, iWorld ) )
			{
				if( !TESTFLAG( client->worlds.created, iWorld ) )
				{
					if( !ClientCreateWorld( client, iWorld ) )
						continue; // next world please, this one's a failure.
				}
				RESETFLAG( client->worlds.updated, iWorld );
				{
					INDEX idx;
					PCLIENT_WORLD_TRACKER pcwt = client->world_trackers + iWorld;
#define UPDATE_THING( name, Name ) \
					for( idx = 0; idx < pcwt->name##s.num; idx++ )  \
					{  \
						if( TESTFLAG( pcwt->name##s.used, idx ) )  \
						{  \
						if( TESTFLAG( pcwt->name##s.updated, idx ) )  \
						{  \
							if( TESTFLAG( pcwt->name##s.created, idx ) )  \
							{  \
								ClientCreate##Name( client, iWorld, idx, FALSE );/* send event modified (implied create to client?) */  \
							}  \
							else  \
							{  \
								ClientCreate##Name( client, iWorld, idx, TRUE );  \
							}  \
							RESETFLAG( pcwt->name##s.updated, idx );  \
						}  \
						}  \
					}
					UPDATE_THING( name, Name );
					UPDATE_THING( texture, Texture );
					UPDATE_THING( line, Line );
					UPDATE_THING( wall, Wall );
					UPDATE_THING( sector, Sector );
				}
			}
		}
	}
}



int CPROC ServerCreateWorld(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	INDEX iWorld;
	PWORLD world;
	INDEX idx;
	PWORLDSCAPE_CLIENT client;
	iWorld = SrvrOpenWorld( params[-1], (CTEXTSTR)params );
	world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	LIST_FORALL( l.clients, idx, PWORLDSCAPE_CLIENT, client )
	{
		if( client->pid == params[-1] )
		{
			// post a create world event to the client that requested the created world.
		}
	}
	UpdateClients();
	(*(INDEX*)result) = iWorld;
	(*result_length) = sizeof( INDEX );
	return 1;
}

/* create basic world */

int CPROC ServerDestroyWorld(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerResetWorld(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

//--------------------------------------

int CPROC ServerMakeName(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	INDEX iWorld = params[0];
    ((INDEX*)result)[0] = SrvrMakeName( params[-1], iWorld, (CTEXTSTR)(params + 1) );
	(*result_length) = sizeof( INDEX );
	UpdateClients();
	return 1;	
}

int CPROC ServerSetSectorName(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	SrvrSetSectorName( params[-1], params[0], params[1], params[2] );
	(*result_length) = INVALID_INDEX;
	UpdateClients();
	return 1;
}
//--------------------------------------


int CPROC ServerCreateSquareSector(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	INDEX iWorld = params[0];
	INDEX iSector = SrvrCreateSquareSector( params[-1], iWorld
					, (PC_POINT)params+1
					, *(RCOORD*)( params + (3*sizeof(RCOORD)/sizeof( _32)) + 1)
					);
    result[0] = iSector;
	//MarkSectorUpdated( iWorld, iSector );
	{
		PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
		PSECTOR ps = GetSetMember( SECTOR, &world->sectors, iSector );
		PWALL pCur, pNext, pFirst;
		INDEX iCur, iNext;
		int priorend = TRUE;
		pFirst = pCur = GetSetMember( WALL, &world->walls, iCur = ps->iWall );
		do
		{
			if( priorend )
			{
				priorend = pCur->flags.wall_start_end;
				pNext = GetSetMember( WALL, &world->walls, iNext = pCur->iWallStart );
			}
			else
			{
				priorend = pCur->flags.wall_end_end;
				pNext = GetSetMember( WALL, &world->walls, iNext = pCur->iWallEnd );
			}
			MarkWallUpdated( params[-1], iWorld, iCur );
			iCur = iNext;
			pCur = pNext;
		}while( pCur != pFirst );
	}
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( iSector );
	return 1;
}


int CPROC ServerAddConnectedSector( _32 *params, _32 param_length
					  , _32 *result, _32 *result_length )
{
	((int*)result)[0] = SrvrAddConnectedSector( params[-1], params[0], params[1], ((RCOORD*)(params + 2))[0] );
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( int );
	return 1;
}

int CPROC ServerMoveSectors( _32 *params, _32 param_length
					  , _32 *result, _32 *result_length )
{
	PTRSZVAL param_idx = (PTRSZVAL)params;

	((int*)result)[0] = SrvrMoveSectors( params[-1], params[0]
		, ( param_length - sizeof( params[0] ) - sizeof( _POINT ) ) / sizeof( INDEX )
		, (INDEX*)(((P_POINT)(params + 1))+1)
		, (P_POINT)(params + 1) 
		);
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( int );
	return 1;
}

int CPROC ServerMoveWalls( _32 *params, _32 param_length
					  , _32 *result, _32 *result_length )
{
	PTRSZVAL param_idx = (PTRSZVAL)params;

	((int*)result)[0] = SrvrMoveWalls( params[-1], params[0]
		, ( param_length - (2*sizeof( params[0] )) - sizeof( _POINT ) ) / sizeof( INDEX )
		, (INDEX*)(((_POINT*)(params + 2))+1)
		, (P_POINT)(params + 2) 
		, (params[1])
		);
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( int );
	return 1;
}

int CPROC ServerUpdateMatingLines( _32 *params, _32 param_length
					  , _32 *result, _32 *result_length )
{
	PTRSZVAL param_idx = (PTRSZVAL)params;

	((int*)result)[0] = SrvrUpdateMatingLines( params[-1], params[0], params[1], params[2], params[3] );
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( int );
	return 1;
}


int CPROC UpdateLine( _32 *params, _32 param_length
					  , _32 *result, _32 *result_length )
{
	INDEX iWorld = params[0];
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	INDEX iLine;
	PFLATLAND_MYLINESEG line = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine = params[1] );
	PFLATLAND_MYLINESEG copy_from = (PFLATLAND_MYLINESEG)(params+2);
	line[0] = copy_from[0];
	(*result_length) = INVALID_INDEX;
	MarkLineUpdatedEx( params[0], params[0], iWorld, iLine );
	UpdateClients();
	(*result_length) = 0;
	return 1;
}


int CPROC ServerClearUndo(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerSaveWorld(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
   result[0] = SaveWorldToFile( params[0] );
	//(*result_length);
	return 1;
}

int CPROC ServerLoadeWorld(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
   result[0] = LoadWorldFromFile( params[0] );
	//(*result_length);
	return 1;
}

int CPROC ServerAddUndo(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}


int CPROC ServerEndUndo(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}


int CPROC ClientConnect(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	PWORLDSCAPE_CLIENT client = New( WORLDSCAPE_CLIENT );
	client->pid = params[-1];
	client->world_trackers = NULL;
	client->worlds.num = 0;
	AddLink( &l.clients, client );
   DebugBreak();
	result[0] = (offsetof( WORLD_SCAPE_INTERFACE, ___LastServiceFunction)
					/sizeof(void(*)(void)))
					+ MSG_EventUser;
		; // create world,sector,blah...
	return TRUE;
}

int CPROC ClientDisconnect(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	INDEX idx;
	PWORLDSCAPE_CLIENT client;
	LIST_FORALL( l.clients, idx, PWORLDSCAPE_CLIENT, client )
	{
		if( client->pid == params[-1] )
		{
			SetLink( &l.clients, idx, NULL );
			Release( client );
			break;
		}
	}
	return TRUE;
}

int CPROC ServerMakeTexture(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	((INDEX*)result)[0] = SrvrMakeTexture( params[-1], params[0], params[1] );
	(*result_length) = sizeof( INDEX );
	return 1;
}


int CPROC ServerSetTexture(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	SrvrSetTexture( params[-1], params[0], params[1], params[2] );	
	(*result_length) = INVALID_INDEX;
	return 1;
}


int CPROC FlushUpdates(  _32 *params, _32 param_length
							, _32 *result, _32 *result_length
							)
{
	UpdateClients();
	(*result_length) = INVALID_INDEX;
	return 1;
}

SERVER_FUNCTION functions[] = {
	 ServerFunctionEntry(ClientDisconnect) // disocnnect
	, ServerFunctionEntry(ClientConnect)   // connect
	, ServerFunctionEntry(NULL) // undefined
	, ServerFunctionEntry(NULL) // unedefined
	, ServerFunctionEntry( ServerCreateWorld )
	  , ServerFunctionEntry( ServerDestroyWorld )
	  , ServerFunctionEntry( ServerResetWorld )
	  , ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetSectorCount )( INDEX iWorld );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetWallCount )( INDEX iWorld );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetLineCount )( INDEX iWorld );
	  , ServerFunctionEntry( ServerSaveWorld )//, SaveWorldToFile
	  , ServerFunctionEntry( ServerLoadWorld )//, LoadWorldFromFile


	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, MergeSelectedWalls )( INDEX iWorld, INDEX iDefiniteWall, PORTHOAREA rect );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, MarkSelectedSectors )( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, MarkSelectedWalls )( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MergeOverlappingWalls )( INDEX iWorld, PORTHOAREA rect );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, ValidateWorldLinks )( INDEX iWorld );

//------ Sectors ----------
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, FindSectorAroundPoint )( INDEX iWorld, P_POINT p );

//	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteSectors )( PSECTORSET *psectors );

	  , ServerFunctionEntry( ServerCreateSquareSector )
	  , ServerFunctionEntry( ServerAddConnectedSector )//WORLD_PROC( INDEX, AddConnectedSector )( INDEX iWorld, INDEX iWall, RCOORD offset );
	  , ServerFunctionEntry( ServerMoveWalls )//WORLD_PROC( int, MoveWalls )( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );
// send current line, get all updates from world-scape server
	  , ServerFunctionEntry( ServerUpdateMatingLines )//WORLD_PROC( int, UpdateMatingLines )( INDEX iWorld, INDEX iWall, int bLockSlopes, int bErrorOK );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, WallInSector )( INDEX iWorld, INDEX iSector, INDEX iWall );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, GetSectorOrigin )( INDEX iWorld, INDEX sector, P_POINT o );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, LineInCur )( INDEX iWorld
									// , INDEX *SectorList, int nSectors
									// , INDEX *WallList, int nWalls
									// , INDEX pLine );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, FindIntersectingWall )( INDEX iWorld, INDEX iSector, P_POINT n, P_POINT o );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, DestroySector )( INDEX iWorld, INDEX iSector );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, FlatlandPointWithin )( INDEX iWorld, int nSectors, INDEX *piSectors, P_POINT p );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, FlatlandPointWithinSingle )( INDEX world, INDEX iSector, P_POINT p );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, FlatlandPointWithinLoopSingle )(  PTRSZVAL psv, INDEX iSector );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, ComputeSectorOrigin )( INDEX iWorld, INDEX iSector );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, ComputeSectorSetOrigin )( INDEX iWorld, int nSectors, INDEX *sectors, P_POINT origin );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, ComputeWallSetOrigin )( INDEX iWorld, int nWalls, INDEX *walls, P_POINT origin );

	  , ServerFunctionEntry( ServerMoveSectors )//WORLD_PROC( int, MoveSectors )( INDEX iWorld
										//, int nSectors
										//, INDEX *pSectors, P_POINT del );


	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, CreateWall )( INDEX world, INDEX iSector
										// , INDEX pStart, int bFromStartEnd
										// , INDEX pEnd,   int bFromEndEnd
										// , _POINT o, _POINT n );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, MergeWalls )( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, SplitWall )( INDEX iWorld, INDEX iWall );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( int, RemoveWall )( INDEX iWorld, INDEX iWall ); // remove wall, and relink mating...


	  , ServerFunctionEntry( NULL )//WORLD_PROC( _POINT*, CheckPointOrder )( PC_POINT normal, _POINT *plist, int npoints );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( _POINT*, ComputeSectorPointList )( INDEX iWorld, INDEX sector, int *pnpoints );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, BreakWall )( INDEX iWorld, INDEX wall );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, GetNameText )( INDEX iWorld, INDEX name, char *text, int maxlen );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX wall );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetWallSector )( INDEX iWorld, INDEX iSector );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetMatedWall )( INDEX iWorld, INDEX wall );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX wall, int *priorend );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetSectorName )( INDEX iWorld, INDEX iSector );

	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, GetLineData )( INDEX iWorld, INDEX iLine, PFLATLAND_MYLINESEG *line );
	  //, ServerFunctionEntry( NULL )//WORLD_PROC( void, BalanceALine )( INDEX iWorld, INDEX iLine );
//	  , ServerFunctionEntry( NULL )//WORLD_PROC( PFLATLAND_TEXTURE, GetSectorData )( INDEX iWorld, INDEX iTexture );

	 // , ServerFunctionEntry( NULL )//WORLD_PROC( void, ForAllSectors )( INDEX iWorld, PTRSZVAL(CPROC *f)(PTRSZVAL,INDEX),PTRSZVAL);

// internal only function use
// BalanceALine and pass world and line index
  //   , ServerFunctionEntry( NULL )//WORLD_PROC( void, BalanceLine )( PFLATLAND_MYLINESEG pls );


	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, SetSolidColor )( INDEX iWorld, INDEX iTexture, CDATA color );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTexture )( INDEX iWorld, INDEX iTexture );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTextures )( INDEX iWorld );

//	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, SetName )( INDEX iWorld, INDEX iName, char *text );
	  , ServerFunctionEntry( ServerMakeTexture )//WORLD_PROC( INDEX, MakeTexture )( INDEX iWorld, char *text );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, SetSolidColor )( INDEX iWorld, INDEX iTexture, CDATA color );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTexture )( INDEX iWorld, INDEX iTexture );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTextures )( INDEX iWorld );

	  , ServerFunctionEntry( ServerMakeName )//WORLD_PROC( INDEX, MakeName )( INDEX iWorld, char *text );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, SetName )( INDEX iWorld, INDEX iName, CTEXTSTR text );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteNames )( INDEX iWorld );
/	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteName )( INDEX iWorld, INDEX iName );

	  , ServerFunctionEntry( ServerClearUndo )
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, GetSectorPoints )( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints );


	  , ServerFunctionEntry( ServerAddUndo )//WORLD_PROC( INDEX, ForAllTextures )( INDEX iWorld, INDEX (CPROC*)(INDEX,PTRSZVAL), PTRSZVAL );
	  , ServerFunctionEntry( ServerEndUndo )//WORLD_PROC( void, GetTextureNameText )( INDEX iWorld, INDEX iTexture, char *buf, int bufsize );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MarkTextureUpdated )( INDEX iWorld, INDEX iTexture );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MarkSectorUpdated )( INDEX iWorld, INDEX iSector );
	  , ServerFunctionEntry( ServerSetTexture )//WORLD_PROC( INDEX, SetTexture )( INDEX iWorld, INDEX iSector, INDEX iTexture );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MarkWorldUpdated )( INDEX iWorld );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MarkSectorUpdated )( INDEX iWorld, INDEX iSector );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, MarkWorldUpdated )( INDEX iWorld );

	  , ServerFunctionEntry( ServerSetSectorName )//WORLD_PROC( void, SetSectorName )( INDEX iWorld, INDEX iSector, INDEX iName );
	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, AddUpdateCallback )( WorldScapeUdpateProc, PTRSZVAL psv );
	  , ServerFunctionEntry( UpdateLine )
	  , ServerFunctionEntry( NULL )//ServerFunctionEntry( UpdateSector )
	  , ServerFunctionEntry( NULL )//ServerFunctionEntry( UpdateWall )
	  , ServerFunctionEntry( NULL )//ServerFunctionEntry( UpdateName )
	  , ServerFunctionEntry( NULL )//ServerFunctionEntry( UpdateTexture )
	  , ServerFunctionEntry( FlushUpdates )
};

// enable magic linking.
PRELOAD( RegisterFlatlandService )
{
	if( !( l.SrvrMsgBase = RegisterService( WORLD_SCAPE_INTERFACE_NAME
						, functions
						, sizeof( functions ) / sizeof( functions[0] ) ) ) )

	{
		LoadFunction( "msg.core.service", NULL );
		if( !( l.SrvrMsgBase = RegisterService( WORLD_SCAPE_INTERFACE_NAME
							, functions
							, sizeof( functions ) / sizeof( functions[0] ) ) ) )
		{
			lprintf( "Failed to register service, Exiting." );
			return;
		}
	}
}
//#endif

PUBLIC( void, BuildStupidExportLibraryThingAlready )( void )
{
}

#ifdef _MSC_VER
int WinMain( HINSTANCE x, HINSTANCE y, LPSTR cmdline, int cmdshow )
#else
int main( void )
#endif
{
	while( 1 ) 
		WakeableSleep( 0 );
	return 0;
}