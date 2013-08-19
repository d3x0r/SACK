#ifndef WORLD_SERVICE
#define WORLD_SERVICE
#endif
#define WORLD_SOURCE

#include <stdhdrs.h>
#include <deadstart.h>

#include <timers.h>

#ifdef WORLDSCAPE_SERVICE_PROGRAM
#include <systray.h>
#endif

#include <world.h>
#include <worldstrucs.h>
#include <msgclient.h>
#include "world_local.h"
//#include <msgserver.h>
#include "sector.h"
#include "global.h"

extern GLOBAL g;
//extern PWORLDSET worlds; // this is a member of global

typedef struct worldscape_client WORLDSCAPE_CLIENT, *PWORLDSCAPE_CLIENT;

typedef struct client_world_tracker CLIENT_WORLD_TRACKER, *PCLIENT_WORLD_TRACKER;

struct bitset
{
	_32 num;
	_32 *created;
	_32 *deleted;
	_32 *updated;
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
	_32 tracker_count;
	PCLIENT_WORLD_TRACKER world_trackers;
	SERVICE_ROUTE pid;
};

static struct worldscape_server_local
{
	_32 SrvrMsgBase;
	PLIST clients; // list of PWORLDSCAPE_CLIENTs
}l;
//#if 0

void ExpandClientTracking( PWORLDSCAPE_CLIENT client )
{
	if( client->tracker_count < client->worlds.num )
	{
		PCLIENT_WORLD_TRACKER new_trackers;

		new_trackers = NewArray( CLIENT_WORLD_TRACKER, client->worlds.num );
		MemSet( new_trackers + client->tracker_count
				, 0
				, ( client->worlds.num - client->tracker_count ) * sizeof( client->world_trackers[0] ) );
		if( client->tracker_count )
		{
			MemCpy( new_trackers, client->world_trackers
					, sizeof( client->world_trackers[0] ) * client->tracker_count );
			Release( client->world_trackers );
		}
		client->tracker_count = client->worlds.num;
		client->world_trackers = new_trackers;
	}

}

void ExpandFlagset( struct flagset *pbitset, INDEX least )
{
	/* should do something smarter here rather than looping every 32 increment. */
	if( least >= pbitset->num )
	{
		_32 old_count = pbitset->num;
		_32 *new_created_flags;
		pbitset->num += (32 > (least-pbitset->num))?32:((least+31)& (~31));

		new_created_flags = NewArray( _32, ( pbitset->num / 32 ) );

		MemSet( new_created_flags + ( old_count / 32 ), 0, sizeof( _32 ) );

		if( old_count )
		{
			MemCpy( new_created_flags
					, pbitset->flags
					, ( old_count / 32 ) );
			Release( pbitset->flags );
		}
		pbitset->flags = new_created_flags;
	}
}

void ExpandBitset( struct bitset *pbitset, INDEX least )
{
	/* should do something smarter here rather than looping every 32 increment. */
	if( least >= pbitset->num )
	{
		_32 old_count = pbitset->num;
		_32 *new_created_flags;
		_32 *new_updated_flags;
		_32 *new_deleted_flags;
		_32 *old_created_flags;
		_32 *old_updated_flags;
		_32 *old_deleted_flags;

		pbitset->num += (32 > (least-pbitset->num))?32:((least+31)& (~31));

		old_created_flags	  =  pbitset->created ;
		old_updated_flags	  =  pbitset->updated ;
		old_deleted_flags	  =  pbitset->deleted ;
		new_created_flags = NewArray( _32, ( pbitset->num / 32 ) );
		new_updated_flags = NewArray( _32, ( pbitset->num / 32 ) );
		new_deleted_flags = NewArray( _32, ( pbitset->num / 32 ) );

      // actually want the very next bit after oldcount to start.
		MemSet( new_created_flags + ( (old_count+1) / 32 ), 0, (pbitset->num-old_count)/8 );
		MemSet( new_updated_flags + ( (old_count+1) / 32 ), 0, (pbitset->num-old_count)/8 );
		MemSet( new_deleted_flags + ( (old_count+1) / 32 ), 0, (pbitset->num-old_count)/8 );

		if( old_count )
		{
			MemCpy( new_created_flags
					, pbitset->created
					, ( old_count / 8 ) );
			MemCpy( new_updated_flags
					, pbitset->updated
					, ( old_count / 8 ) );
			MemCpy( new_deleted_flags
					, pbitset->deleted
					, ( old_count / 8 ) );
		}
		pbitset->created = new_created_flags;
		pbitset->updated = new_updated_flags;
		pbitset->deleted = new_deleted_flags;
		if( old_count )
		{
			Release( old_created_flags );
			Release( old_updated_flags );
			Release( old_deleted_flags );
		}
	}
}

//---------------------------------------------------------------

#define DefineMarkers( name, Name, iName ) \
void Mark##Name##Updated( INDEX iWorld, INDEX iName )   \
{  \
	INDEX iClient;                     \
	PWORLDSCAPE_CLIENT client;         \
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )  \
	{ \
		if( client->world_trackers && TESTFLAG( client->worlds.created, iWorld ) )  \
		{  \
	PCLIENT_WORLD_TRACKER world = client->world_trackers + iWorld;  \
			ExpandBitset( &world->name##s, iName );  \
			SETFLAG( world->name##s.updated, iName);   \
			SETFLAG( client->worlds.updated, iWorld );\
		}  \
	}  \
	}  \
   void Mark##Name##UpdatedEx( PSERVICE_ROUTE notpid, INDEX iWorld, INDEX iName )   \
{  \
	INDEX iClient;                     \
	PWORLDSCAPE_CLIENT client;         \
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )  \
	{ \
	if( IsSameMsgDest( &client->pid, notpid )   \
	  && client->world_trackers && TESTFLAG( client->worlds.created, iWorld ) )  \
		{  \
	PCLIENT_WORLD_TRACKER world = client->world_trackers + iWorld;  \
			ExpandBitset( &world->name##s, iName );  \
			SETFLAG( world->name##s.updated, iName);   \
			SETFLAG( client->worlds.updated, iWorld );\
		}  \
	}  \
} \
void Mark##Name##Deleted( INDEX iWorld, INDEX iName )   \
{  \
	INDEX iClient;                     \
	PWORLDSCAPE_CLIENT client;         \
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld ); \
	EnterCriticalSec( &world->csDeletions ); \
	ExpandFlagset( &world->deletions.name##s, iName ); \
	SETFLAG( world->deletions.name##s.flags, iName ); \
	LeaveCriticalSec( &world->csDeletions ); \
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )  \
	{ \
		if( client->world_trackers && TESTFLAG( client->worlds.created, iWorld ) )  \
		{  \
			PCLIENT_WORLD_TRACKER cworld = client->world_trackers + iWorld;  \
			lprintf (WIDE("aadffzzf") );\
			ExpandBitset( &cworld->name##s, iName );  \
			SETFLAG( cworld->name##s.deleted, iName);   \
			SETFLAG( cworld->name##s.updated, iName);   \
			SETFLAG( client->worlds.updated, iWorld );\
		}  \
	}  \
	}  \


DefineMarkers( sector, Sector, iSector );
DefineMarkers( line, Line, iLine );
DefineMarkers( name, Name, iName );
DefineMarkers( texture, Texture, iTexture );
DefineMarkers( wall, Wall, iWall );

//---------------------------------------------------------------

void delete_MarkWorldCreated( INDEX iWorld )
{
	INDEX iClient;
	PWORLDSCAPE_CLIENT client;
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )
	{
		_32 old_count = client->worlds.num;
		ExpandBitset( &client->worlds, iWorld );
		SETFLAG( client->worlds.created, iWorld );
	}
}

//---------------------------------------------------------------

enum update_type {
   UpdateSector, UpdateName, UpdateWall, UpdateLine, UpdateTexture
};

struct set_active_tag
{
   enum update_type type;
   PCLIENT_WORLD_TRACKER world;
};

PTRSZVAL CPROC MarkUpdated( INDEX p, PTRSZVAL psv )
{
	struct set_active_tag *sat = (struct set_active_tag *)psv;
	lprintf( WIDE("Mark %d updated"), sat->type );
	switch( sat->type )
	{
	case UpdateSector:
		ExpandBitset( &sat->world->sectors, p );
		SETFLAG( sat->world->sectors.updated, p );
		break;
	case UpdateName:
		ExpandBitset( &sat->world->names, p );
		SETFLAG( sat->world->names.updated, p );
		break;
	case UpdateWall:
		ExpandBitset( &sat->world->walls, p );
		SETFLAG( sat->world->walls.updated, p );
		break;
	case UpdateLine:
		ExpandBitset( &sat->world->lines, p );
		SETFLAG( sat->world->lines.updated, p );
		break;
	case UpdateTexture:
		ExpandBitset( &sat->world->textures, p );
		SETFLAG( sat->world->textures.updated, p );
		break;
	}
   return 0;
}

//---------------------------------------------------------------

void MarkAllInWorldUpdated( PSERVICE_ROUTE pidClient, INDEX iWorld )
{
	INDEX iClient;
	PWORLDSCAPE_CLIENT client;
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	struct set_active_tag params;

	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )
	{
		if( IsSameMsgDest( &client->pid, pidClient ) )
		{
			_32 old_count = client->worlds.num;
			ExpandBitset( &client->worlds, iWorld );
			SETFLAG( client->worlds.created, iWorld );
			ExpandClientTracking( client );
			SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATEWORLD, 2
										, &iWorld, sizeof( INDEX )
										, &world->name, sizeof( INDEX )
										);

			params.world = client->world_trackers + iWorld;

			params.type = UpdateName;
			ForEachSetMember( NAME, world->names, MarkUpdated, (PTRSZVAL)&params );
			params.type = UpdateWall;
			lprintf( WIDE("Mark the walls updated?") );
			ForEachSetMember( WALL, world->walls, MarkUpdated, (PTRSZVAL)&params );
			params.type = UpdateLine;
			ForEachSetMember( FLATLAND_MYLINESEG, world->lines, MarkUpdated, (PTRSZVAL)&params );
			params.type = UpdateTexture;
			ForEachSetMember( FLATLAND_TEXTURE, world->textures, MarkUpdated, (PTRSZVAL)&params );
			params.type = UpdateSector;
			ForEachSetMember( SECTOR, world->sectors, MarkUpdated, (PTRSZVAL)&params );

			SETFLAG( client->worlds.updated, iWorld );
		}
	}
}

//---------------------------------------------------------------

//---------------------------------------------------------------

void ClientCreateWorld( PWORLDSCAPE_CLIENT client, INDEX iWorld )
{
	/* magic routine, so only clients that have actually requested this world get it.*/
	{
		if( !TESTFLAG( client->worlds.created, iWorld ) )
		{
			PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
			SETFLAG( client->worlds.created, iWorld );
			SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATEWORLD, 2
									, &iWorld, sizeof( INDEX )
									, &world->name, sizeof( INDEX )
									);
			//MarkNameUpdated( iWorld, world->name );
		}
	}
}

void ClientCreateWorld_init( PSERVICE_ROUTE iClient, INDEX iWorld )
{
	PWORLDSCAPE_CLIENT client;
	INDEX idx;
	LIST_FORALL( l.clients, idx, PWORLDSCAPE_CLIENT, client )
	{
		if( IsSameMsgDest( &client->pid, iClient ) )
		{
			_32 old_count = client->worlds.num;
			ExpandBitset( &client->worlds, iWorld );
			SETFLAG( client->worlds.created, iWorld );
			SETFLAG( client->worlds.updated, iWorld );
			/* only update those worlds that this client has 'created' */
			ExpandClientTracking( client );
			{
				PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
				//SETFLAG( client->worlds.created, iWorld );
				SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATEWORLD, 2
											, &iWorld, sizeof( INDEX )
											, &world->name, sizeof( INDEX )
											);
			}
		}
	}
}

//void BroadcastMultiServiceEvent( )

void ClientCreateName( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iName, LOGICAL bCreate )
{
	/* end up having to build the name before broadcast... */
	PWORLD world  = GetSetMember( WORLD, &g.worlds, iWorld );
	PNAME name = GetSetMember( NAME, &world->names, iName );
	TEXTSTR namedata;
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
			SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATENAME, 5
									, &iWorld, sizeof( INDEX )
									, &iName, sizeof( INDEX )
									, &name->lines, sizeof( name->lines )
									, &name->flags, sizeof( name->flags )
									, namedata, len );
		}
		else
		{
			if( TESTFLAG( pcwt->names.deleted, iName ) )
			{
				SendMultiServiceEvent( &client->pid, MSG_EVENT_DELETENAME, 2
											, &iWorld, sizeof( INDEX )
											, &iName, sizeof( INDEX )
											);
				RESETFLAG( pcwt->names.created, iName );
				RESETFLAG( pcwt->names.deleted, iName );
			}
			/*
			else
				SendMultiServiceEvent( client->pid, MSG_EVENT_UPDATENAME, 2
											, &iWorld, sizeof( INDEX )
											, &iName, sizeof( INDEX )
											//, line, sizeof( FLATLAND_MYLINESEG )// yes this is NOT sizeof( MYLINESEG )!!
											);
											*/
			/* names are tricky.*/
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
	//if( bCreate )
	{
		ExpandBitset( &pcwt->lines, iLine );
		if( !TESTFLAG( pcwt->lines.created, iLine ) )
		{
			if( !TESTFLAG( pcwt->lines.deleted, iLine ) )
			{
				SETFLAG( pcwt->lines.created, iLine );
				SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATELINE, 3
										, &iWorld, sizeof( INDEX )
										, &iLine, sizeof( INDEX )
										, line, sizeof( FLATLAND_MYLINESEG )// yes this is NOT sizeof( MYLINESEG )!!
										);
			}
			else
				DebugBreak();
		}
		else
		{
			if( TESTFLAG( pcwt->lines.deleted, iLine ) )
			{
				SendMultiServiceEvent( &client->pid, MSG_EVENT_DELETELINE, 2
											, &iWorld, sizeof( INDEX )
											, &iLine, sizeof( INDEX )
											);
				RESETFLAG( pcwt->lines.created, iLine );
				RESETFLAG( pcwt->lines.deleted, iLine );
			}
			else
				SendMultiServiceEvent( &client->pid, MSG_EVENT_UPDATELINE, 3
											, &iWorld, sizeof( INDEX )
											, &iLine, sizeof( INDEX )
											, line, sizeof( FLATLAND_MYLINESEG )// yes this is NOT sizeof( MYLINESEG )!!
											);
		}
	}
}

void ClientCreateWall( PWORLDSCAPE_CLIENT client, INDEX iWorld, INDEX iWall, LOGICAL bCreate )
{
	PWORLD world  = GetUsedSetMember( WORLD, &g.worlds, iWorld );
   lprintf( WIDE("...") );
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
				SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATEWALL, 3
								, &iWorld, sizeof( INDEX )
								, &iWall, sizeof( INDEX )
								, wall, sizeof( WALL ) );
			}
			else
			{
				if( TESTFLAG( pcwt->walls.deleted, iWall ) )
				{
					SendMultiServiceEvent( &client->pid, MSG_EVENT_DELETEWALL, 2
												, &iWorld, sizeof( INDEX )
												, &iWall, sizeof( INDEX )
												);
					RESETFLAG( pcwt->walls.created, iWall );
					RESETFLAG( pcwt->walls.deleted, iWall );
				}
				else
				{
					SendMultiServiceEvent( &client->pid, MSG_EVENT_UPDATEWALL, 3
									, &iWorld, sizeof( INDEX )
									, &iWall, sizeof( INDEX )
									, wall, sizeof( WALL ) );
				}
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
				SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATETEXTURE, 3
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
	if( !TESTFLAG( pcwt->sectors.created, iSector ) && !TESTFLAG( pcwt->sectors.deleted, iSector ) )
	{
		if( !TESTFLAG( pcwt->sectors.deleted, iSector ) )
		{
			SETFLAG( pcwt->sectors.created, iSector );
			SendMultiServiceEvent( &client->pid, MSG_EVENT_CREATESECTOR, 3
										, &iWorld, sizeof( INDEX )
										, &iSector, sizeof( INDEX )
										, sector, sizeof( SECTOR )
								);
				}
			}
	else
	{	
			if( TESTFLAG( pcwt->sectors.deleted, iSector ) )
			{
				SendMultiServiceEvent( &client->pid, MSG_EVENT_DELETESECTOR, 2
											, &iWorld, sizeof( INDEX )
											, &iSector, sizeof( INDEX )
											);
				RESETFLAG( pcwt->sectors.created, iSector );
				RESETFLAG( pcwt->sectors.deleted, iSector );
			}
			else
				SendMultiServiceEvent( &client->pid, MSG_EVENT_UPDATESECTOR, 3
											, &iWorld, sizeof( INDEX )
											, &iSector, sizeof( INDEX )
											, sector, sizeof( SECTOR )
											);
	}
}


PTRSZVAL CPROC DeleteDeletions( void *pointer, PTRSZVAL psv )
{
	PWORLD world = (PWORLD)pointer;
	_32 n;
	EnterCriticalSec( &world->csDeletions );

	for( n = 0; n < world->deletions.lines.num; n++ )
		if( TESTFLAG( world->deletions.lines.flags, n ) )
		{
			RESETFLAG( world->deletions.lines.flags, n );
			DeleteSetMember( FLATLAND_MYLINESEG, world->lines, n );
		}
	if( TESTFLAG( world->deletions.deleteset, 0 ) )
		DeleteSetEx( FLATLAND_MYLINESEG, &world->lines );
	world->deletions.lines.flags = (FLAGSETTYPE*)Release( world->deletions.lines.flags );
	world->deletions.lines.num = 0;

	for( n = 0; n < world->deletions.walls.num; n++ )
		if( TESTFLAG( world->deletions.walls.flags, n ) )
		{
			RESETFLAG( world->deletions.walls.flags, n );
			DeleteSetMember( WALL, world->walls, n );
		}
	if( TESTFLAG( world->deletions.deleteset, 1 ) )
		DeleteSetEx( WALL, &world->walls );
	world->deletions.walls.flags = (FLAGSETTYPE*)Release( world->deletions.walls.flags );
	world->deletions.walls.num = 0;

	for( n = 0; n < world->deletions.sectors.num; n++ )
		if( TESTFLAG( world->deletions.sectors.flags, n ) )
		{
			RESETFLAG( world->deletions.sectors.flags, n );
			DeleteSetMember( SECTOR, world->sectors, n );
		}
	if( TESTFLAG( world->deletions.deleteset, 2 ) )
	{
		DeleteSetEx( SECTOR, &world->sectors );
	}
	world->deletions.sectors.flags = (FLAGSETTYPE*)Release( world->deletions.sectors.flags );
	world->deletions.sectors.num = 0;

	for( n = 0; n < world->deletions.textures.num; n++ )
		if( TESTFLAG( world->deletions.textures.flags, n ) )
		{
			RESETFLAG( world->deletions.textures.flags, n );
			DeleteSetMember( FLATLAND_TEXTURE, world->textures, n );
		}
	if( TESTFLAG( world->deletions.deleteset, 3 ) )
		DeleteSetEx( FLATLAND_TEXTURE, &world->textures );
	world->deletions.textures.flags = (FLAGSETTYPE*)Release( world->deletions.textures.flags );
	world->deletions.textures.num = 0;

	for( n = 0; n < world->deletions.names.num; n++ )
		if( TESTFLAG( world->deletions.names.flags, n ) )
		{
			RESETFLAG( world->deletions.names.flags, n );
			DeleteSetMember( NAME, world->names, n );
		}
	if( TESTFLAG( world->deletions.deleteset, 4 ) )
		DeleteSetEx( NAME, &world->names );
	world->deletions.names.flags = (FLAGSETTYPE*)Release( world->deletions.names.flags );
	world->deletions.names.num = 0;

	world->deletions.deleteset[0] = 0; // clear all 32 bits... cheaply.
	LeaveCriticalSec( &world->csDeletions );

	return 0;
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
			if( !TESTFLAG( client->worlds.created, iWorld ) )
				continue; // nexxt world, this client doesn't have this world open.
			if( TESTFLAG( client->worlds.updated, iWorld ) )
			{
				RESETFLAG( client->worlds.updated, iWorld );
				{
					INDEX idx;
					PCLIENT_WORLD_TRACKER pcwt = client->world_trackers + iWorld;

#define UPDATE_THING( name, Name ) \
					for( idx = 0; idx < pcwt->name##s.num; idx++ )  \
					{  \
						if( TESTFLAG( pcwt->name##s.updated, idx ) )  \
						{  \
							RESETFLAG( pcwt->name##s.updated, idx );  \
							ClientCreate##Name( client, iWorld, idx, FALSE );/* send event modified (implied create to client?) */  \
						}  \
					}
					UPDATE_THING( name, Name );
					UPDATE_THING( texture, Texture );
					UPDATE_THING( line, Line );
					lprintf( WIDE("Walls here...") );
					UPDATE_THING( wall, Wall );
					lprintf( WIDE("done with walls...") );
					UPDATE_THING( sector, Sector );
				}
			}
	lprintf( "SENT CHANGES DONE" );
			SendMultiServiceEvent( &client->pid, MSG_EVENT_CHANGES_DONE, 1
							, &iWorld, sizeof( INDEX )
							);

		}
	}
	ForAllInSet( WORLD, g.worlds, DeleteDeletions, 0 );
}



int CPROC ServerCreateWorld(  PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	extern INDEX ServerSideOpenWorld( PSERVICE_ROUTE iClient, CTEXTSTR name );
	INDEX iWorld;

	iWorld = ServerSideOpenWorld( route, (CTEXTSTR)params );

	UpdateClients();
	(*(INDEX*)result) = iWorld;
	(*result_length) = sizeof( INDEX );
	return 1;
}

int CPROC ServerSaveWorld( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	result[0] = SaveWorld( ((INDEX*)params)[0] );
	return 1;
}

int CPROC ServerLoadWorld( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	result[0] = LoadWorld( ((INDEX*)params)[0] );
	UpdateClients();
   	return 1;
}

/* create basic world */

int CPROC ServerDestroyWorld( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerResetWorld( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	ResetWorld( ((INDEX*)params)[0] );
	UpdateClients();
	(*result_length) = 0;
	return 1;
}

void MarkWorldUpdated( INDEX iWorld )
{
	INDEX iClient;
	PWORLDSCAPE_CLIENT client;
	LIST_FORALL( l.clients, iClient, PWORLDSCAPE_CLIENT, client )
	{
		_32 old_count = client->worlds.num;
		ExpandBitset( &client->worlds, iWorld );
		/* only update those worlds that this client has 'created' */
		if( old_count < client->worlds.num )
		{
			PCLIENT_WORLD_TRACKER new_trackers;

			new_trackers = NewArray( CLIENT_WORLD_TRACKER, client->worlds.num );
			MemSet( new_trackers + old_count
				, 0
				, ( client->worlds.num - old_count ) * sizeof( client->world_trackers[0] ) );
			if( old_count )
			{
				MemCpy( new_trackers, client->world_trackers
					, sizeof( client->world_trackers[0] ) * old_count );
				Release( client->world_trackers );
			}
			client->tracker_count = client->worlds.num;
			client->world_trackers = new_trackers;
		}
		if( TESTFLAG( client->worlds.created, iWorld ) )
		{
			SETFLAG( client->worlds.updated, iWorld );
		}
	}
}

int CPROC ServerFindSectorAroundPoint( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
												 )
{

   return 1;
}

int CPROC ServerCreateSquareSector( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	INDEX iWorld = params[0];
	INDEX iSector = CreateSquareSector( iWorld
					, (PC_POINT)params+1
					, *(RCOORD*)( params + (3*sizeof(RCOORD)/sizeof( _32)) + 1)
					);
	((INDEX*)result)[0] = iSector;

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
			MarkWallUpdated( iWorld, iCur );
			iCur = iNext;
			pCur = pNext;
		}while( pCur != pFirst );
	}
	//MarkWorldUpdated( iWorld );
	UpdateClients(); // result will happen after all other elemtns are created...
	(*result_length) = sizeof( iSector );
	return TRUE;
}

int CPROC ServerAddConnectedSector( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
											 )
{
	INDEX iWall;
	if( ( iWall = AddConnectedSector( ((INDEX*)params)[0], ((INDEX*)params)[1], *(RCOORD*)(params + 2*sizeof(INDEX)/sizeof(params[0])) ) ) != INVALID_INDEX )
	{
		((INDEX*)result)[0] = iWall;
		result_length[0] = sizeof( INDEX );
		UpdateClients();
		return 1;
	}
	return 0;
}

int CPROC ServerMoveWalls( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
								 )
{
	int result_code;
	lprintf( WIDE(" >... %d %d "), sizeof(_POINT), ( sizeof( _POINT ) / sizeof params[0] ) );
	result_code = MoveWalls( ((INDEX*)params)[0]
								, ((int*)(((INDEX*)params)+1))[0]
								, (INDEX*)(params + 2 + 1 * sizeof( INDEX)/sizeof(params[0]) + sizeof(_POINT)/sizeof(params[0]) )
								, (P_POINT)(params + 2 + 1 * sizeof( INDEX)/sizeof(params[0]) )
								, ((int*)(((INDEX*)params)+1))[1]
								);
	if( result_code )
		UpdateClients();
	return result_code;
}

int CPROC ReceiveLineChanged( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
										  )
{
	INDEX iWorld = ((INDEX*)params)[0];
	INDEX iWall = ((INDEX*)params)[1];
	INDEX iLine = ((INDEX*)params)[2];
	_32 no_update = ((_32*)(((INDEX*)params)+3))[0];
	_32 lock_mating_slopes = ((_32*)(((INDEX*)params)+3))[1];
	PLINESEG pls_new = (PLINESEG)(params + 2 + 3 * (sizeof(INDEX)/sizeof(params[0])) );

	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEG pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine );

	pls->r = pls_new->r;
	pls->dFrom = pls_new->dFrom;
	pls->dTo = pls_new->dTo;
	MarkLineUpdatedEx( GetServiceRoute( params ), iWorld, iLine );
	UpdateMatingLines( iWorld, iWall, lock_mating_slopes, 0 );
	UpdateClients();
	result_length[0] = -1;
	return 1;
}

int CPROC ServerHandleUpdateLine( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
										  )
{
	// copy updated line information into the specified line...
   PWORLD world  = GetUsedSetMember( WORLD, &g.worlds, params[0] );
	if( world )
	{
		PFLATLAND_MYLINESEG pls = GetUsedSetMember( FLATLAND_MYLINESEG, &world->lines, params[1] );
		if( pls )
		{
			MemCpy( pls, params + 2, sizeof( FLATLAND_MYLINESEG ) );
         MarkLineUpdatedEx( GetServiceRoute( params ), params[0], params[1] );
		}
	}
   (*result_length) = INVALID_INDEX;
   return 1;
}

int CPROC ServerUpdateMatingLines( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
											)
{
   UpdateMatingLines( params[0], params[1], params[2], params[3] );
   return 1;
}

int CPROC HandleLineChanged( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
											)
{
	INDEX iWorld = ((INDEX*)params)[0];
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	if( world )
	{
		INDEX iWall = ((INDEX*)params)[1];
		INDEX iLine = ((INDEX*)params)[2];
		LOGICAL no_update = ((LOGICAL*)(((INDEX*)params)+3))[0];
		LOGICAL lockslope = ((LOGICAL*)(((INDEX*)params)+3))[1];
		PFLATLAND_MYLINESEG pls = GetUsedSetMember( FLATLAND_MYLINESEG
			, &world->lines
			, iLine );
		if( pls )
		{
			((LINESEG*)pls)[0] = ((LINESEG*)(((LOGICAL*)(((INDEX*)params)+3))+2))[0];
			lprintf( WIDE("this update has only unlocked slopes... ") );
			// should result in all affected lines getting marked as updated...
			if( !no_update ) // the boolean for no update mate..
			{
				if( UpdateMatingLines( iWorld, iWall, lockslope, FALSE ) )
					UpdateClients();
			}
			else
			{
				MarkLineUpdated( iWorld, iLine );
				UpdateClients();
			}
		}
	}
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerClearUndo( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerDoUndo( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerAddUndo( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}


int CPROC ServerEndUndo( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	(*result_length) = INVALID_INDEX;
	return 1;
}

int CPROC ServerMoveSectors( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	int result_code = MoveSectors( ((INDEX*)params)[0]
		, ( param_length - ( sizeof( INDEX ) + sizeof( _POINT ) ) ) / sizeof(INDEX)
		, (INDEX*)(params + (sizeof(INDEX)/sizeof(_32)) + ( sizeof( _POINT ) / sizeof( params[0] ) ))
		, (P_POINT)( params + (sizeof(INDEX)/sizeof(_32)) ) );
	UpdateClients();
	return result_code;
}

int CPROC ClientConnect( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	PWORLDSCAPE_CLIENT client = New( WORLDSCAPE_CLIENT );
	client->pid = route[0];
	client->tracker_count = 0;
	client->world_trackers = NULL;
	client->worlds.num = 0;
	AddLink( &l.clients, client );
	(*result_length) = 0; // don't return extra info...
	//result[0] = 20; // create world,sector,blah...
	return TRUE;
}

int CPROC ClientDisconnect( PSERVICE_ROUTE route, _32 *params, size_t param_length
							, _32 *result, size_t *result_length
							)
{
	INDEX idx;
	PWORLDSCAPE_CLIENT client;
	LIST_FORALL( l.clients, idx, PWORLDSCAPE_CLIENT, client )
	{
 		if( IsSameMsgDest( &client->pid, route ) )
		{
			SetLink( &l.clients, idx, NULL );
			Release( client );
			break;
		}
	}
	return TRUE;
}

SERVER_FUNCTION functions[] = {
	ServerFunctionEntry(ClientDisconnect) // disocnnect
										, ServerFunctionEntry(ClientConnect)   // connect
										, ServerFunctionEntry(NULL) // undefined
										, ServerFunctionEntry(NULL) // unedefined
										// 4
										, ServerFunctionEntry( ServerCreateWorld )
										, ServerFunctionEntry( ServerDestroyWorld )
										, ServerFunctionEntry( ServerResetWorld )
										, ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetSectorCount )( INDEX iWorld );
										, ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetWallCount )( INDEX iWorld );
										, ServerFunctionEntry( NULL )//WORLD_PROC( _32, GetLineCount )( INDEX iWorld );
										// 10

										, ServerFunctionEntry( ServerSaveWorld )//, SaveWorldToFile
										, ServerFunctionEntry( ServerLoadWorld )//, LoadWorldFromFile


										, ServerFunctionEntry( NULL )//WORLD_PROC( int, MergeSelectedWalls )( INDEX iWorld, INDEX iDefiniteWall, PORTHOAREA rect );
										, ServerFunctionEntry( NULL )//WORLD_PROC( int, MarkSelectedSectors )( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount );
										, ServerFunctionEntry( NULL )//WORLD_PROC( int, MarkSelectedWalls )( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, MergeOverlappingWalls )( INDEX iWorld, PORTHOAREA rect );

										, ServerFunctionEntry( NULL )//WORLD_PROC( int, ValidateWorldLinks )( INDEX iWorld );

										//------ Sectors ----------
										/// this function isn't actually given to server, search can be done client side.
										, ServerFunctionEntry( ServerFindSectorAroundPoint )//WORLD_PROC( INDEX, FindSectorAroundPoint )( INDEX iWorld, P_POINT p );

										//	  , ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteSectors )( PSECTORSET *psectors );

										, ServerFunctionEntry( ServerCreateSquareSector )
										, ServerFunctionEntry( ServerAddConnectedSector )//WORLD_PROC( INDEX, AddConnectedSector )( INDEX iWorld, INDEX iWall, RCOORD offset );
										// 20
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

										// 30
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
										, ServerFunctionEntry( NULL )//WORLD_PROC( int, DestroyWall )( INDEX iWorld, INDEX iWall );

										, ServerFunctionEntry( NULL )//WORLD_PROC( int, MergeWalls )( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, SplitWall )( INDEX iWorld, INDEX iWall );
										, ServerFunctionEntry( NULL )//WORLD_PROC( int, RemoveWall )( INDEX iWorld, INDEX iWall ); // remove wall, and relink mating...


										, ServerFunctionEntry( NULL )//WORLD_PROC( _POINT*, CheckPointOrder )( PC_POINT normal, _POINT *plist, int npoints );
										// 40 
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
										//50
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, BalanceALine )( INDEX iWorld, INDEX iLine );

										, ServerFunctionEntry( NULL )//WORLD_PROC( void, GetNameData )( INDEX iWorld, INDEX iName, PNAME *name );
										, ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, GetSectorTexture )( INDEX iWorld, INDEX iSector );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, GetTextureData )( INDEX iWorld, INDEX iTexture, PFLATLAND_TEXTURE *texture );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, GetSectorPoints )( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints );
										
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, ForAllSectors )( INDEX iWorld, PTRSZVAL(CPROC *f)(PTRSZVAL,INDEX),PTRSZVAL);

										// internal only function use
										// BalanceALine and pass world and line index
										//   , ServerFunctionEntry( NULL )//WORLD_PROC( void, BalanceLine )( PFLATLAND_MYLINESEG pls );


										, ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, MakeTexture )( INDEX iWorld, char *text );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, SetSolidColor )( INDEX iWorld, INDEX iTexture, CDATA color );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTexture )( INDEX iWorld, INDEX iTexture );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteTextures )( INDEX iWorld );

										// 60
										, ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, MakeName )( INDEX iWorld, char *text );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, SetName )( INDEX iWorld, INDEX iName, char *text );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteNames )( INDEX iWorld );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, DeleteName )( INDEX iWorld, INDEX iName );

										, ServerFunctionEntry( ServerClearUndo )
										, ServerFunctionEntry( ServerDoUndo )//WORLD_PROC( void, DoUndo )( INDEX iWorld );

										, ServerFunctionEntry( ServerAddUndo )//WORLD_PROC( INDEX, ForAllTextures )( INDEX iWorld, INDEX (CPROC*)(INDEX,PTRSZVAL), PTRSZVAL );
										, ServerFunctionEntry( ServerEndUndo )//WORLD_PROC( void, GetTextureNameText )( INDEX iWorld, INDEX iTexture, char *buf, int bufsize );


										, ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, ForAllTextures )( INDEX iWorld, INDEX (CPROC*)(INDEX,PTRSZVAL), PTRSZVAL );
										, ServerFunctionEntry( NULL )//WORLD_PROC( void, GetTextureNameText )( INDEX iWorld, INDEX iTexture, char *buf, int bufsize );
										// 70
										, ServerFunctionEntry( NULL )//WORLD_PROC( INDEX, SetTexture )( INDEX iWorld, INDEX iSector, INDEX iTexture );

										, ServerFunctionEntry( NULL ) //SetSectorName )( INDEX iWorld, INDEX iSector, INDEX iName );

										//-------------------------------------------------------
										/* intended for client application registration for update notifications */

										//typedef void (CPROC *WorldScapeUdpateProc)( PTRSZVAL );
										, ServerFunctionEntry( NULL ) //AddUpdateCallback )( WorldScapeUdpateProc, PTRSZVAL psv );

										//-------------------------------------------------------
										// meant for client->server communication
										, ServerFunctionEntry( ServerHandleUpdateLine ) //UpdateLine )( void );
										//, ServerFunctionEntry( NULL ) //UpdateSector )( void );
										, ServerFunctionEntry( NULL ) //UpdateWall )( void );
										, ServerFunctionEntry( NULL ) //UpdateName )( void );
										, ServerFunctionEntry( NULL ) //UpdateTexture )( void );
										, ServerFunctionEntry( NULL ) //FlushUpdates )( void ); // meant for client->server communication
										, ServerFunctionEntry( NULL ) //MarkLineChanged )( INDEX iWorld, INDEX iLine );
										, ServerFunctionEntry( NULL ) //SendLinesChanged )( INDEX iWorld );
										// 80
										, ServerFunctionEntry( ReceiveLineChanged )//HandleLineNormalsChanged ) //SendLineChanged )( INDEX iWorld );
										, ServerFunctionEntry( NULL ) // a blank one.
										, ServerFunctionEntry( HandleLineChanged ) //SendLineChanged )( INDEX iWorld );
};

// enable magic linking.
PRELOAD( RegisterFlatlandService )
{
	//PWSTR result;
	//PeerCreatePeerName( NULL, "test.singular", &result );
	if( !( l.SrvrMsgBase = RegisterService( WORLD_SCAPE_INTERFACE_NAME
						, functions
						, sizeof( functions ) / sizeof( functions[0] ) ) ) )

	{
		lprintf( WIDE("Service not available, attempt to load core server myself.... then register my service") );
		SuspendDeadstart();
		LoadFunction( WIDE("sack.msgsvr.service.plugin"), NULL );
		if( !( l.SrvrMsgBase = RegisterService( WORLD_SCAPE_INTERFACE_NAME
							, functions
							, sizeof( functions ) / sizeof( functions[0] ) ) ) )
		{
			lprintf( WIDE("Failed to register service, Exiting.") );
			return;                                             
		}
	}
}
//#endif

PUBLIC( void, BuildStupidExportLibraryThingAlready )( void )
{
}

#ifdef WORLDSCAPE_SERVICE_PROGRAM
#ifdef _MSC_VER
int STDCALL WinMain( HINSTANCE x, HINSTANCE y, LPSTR cmdline, int cmdshow )
#else
int main( void )
#endif
{
	RegisterIcon(NULL);
	while( 1 )
		WakeableSleep( 10000 );
	return 0;
}
#endif



