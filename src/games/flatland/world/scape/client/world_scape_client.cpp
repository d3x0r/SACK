#define WORLD_SOURCE
#ifndef WORLD_CLIENT_LIBRARY
#define WORLD_CLIENT_LIBRARY
#endif
#define WORLD_SCAPE_CLIENT
#define WORLDSCAPE_INTERFACE_USED

#include <stdhdrs.h>
#include <deadstart.h>
#include <sack_types.h>
#include <sharemem.h>
#include <procreg.h>

#define BASE_MESSAGE_ID g.MsgBase
#define MyInterface world_scape_interface
#include <msgclient.h>

#include <world.h>
#include "../service.h"

// This defines the client interface structure for use with
// msgsvr...

#include "global.h"

extern GLOBAL g;

static struct worldscape_client_local
{
	_32 MsgBase;
	//PWORLDSET worlds; // and therefore contains all other data...
} l;


void InvokeCallbacks( void )
{
	INDEX idx;
	struct client_global_UpdateCallback_tag *proc;
	lprintf( "invoking callback..." );
	LIST_FORALL( g.callbacks, idx, struct client_global_UpdateCallback_tag*, proc )
	{
		lprintf( "%p %p", proc->proc, proc->psv );
		proc->proc( proc->psv );
	}
}


int CPROC EventHandler( MSGIDTYPE MsgID, _32 *params, size_t paramlen )
{
   int result = EVENT_HANDLED;
	_32 *job_block = (P_32)NewArray( _8, paramlen + sizeof( MsgID ) );
	job_block[0] = MsgID;
	MemCpy( job_block + 1, params, paramlen );
	//lprintf( "Save message to block %p", job_block );
	//LogBinary( params, paramlen );
	switch( MsgID )
	{
	case MSG_DispatchPending:
		lprintf( "Idle callback..." );
		if( g.flags.changes_done )
			InvokeCallbacks();
		Release( job_block );
		break;
		//POINTER ptr;
	case MSG_EVENT_CHANGES_DONE:
      lprintf( "change list completed callback..." );
		// all changes have been received(?)
		g.flags.changes_done = 1;
		Release( job_block );
      result = EVENT_WAIT_DISPATCH;
		break;
	default:
		// a new change has started, this is now an incomplete list.
		while( g.flags.accepting_changes )
			Relinquish();// wait to finish prior chain.
		g.flags.changes_done = 0;
		EnqueLink( &g.pending_changes, job_block );
		break;
	}
	return result;//EVENT_WAIT_DISPATCH; // EVENT_HANDLED
}

void CPROC AcceptChanges( void )
{
	INDEX idx;
	P_32 msg;
	if( !g.flags.changes_done )
		return;
	g.flags.accepting_changes = 1;
	while( msg = (P_32)DequeLink( &g.pending_changes ) )
	{
		_32 MsgID = ((P_32)msg)[0];
		_32 *params = msg + 1;
		switch( MsgID )
		{
		case MSG_DispatchPending:
			// all changes have been received(?)
			break;

		case MSG_EVENT_CREATEWORLD:
			// just getting the member will invoke it's validity
			// hmm not sure I care - world spaces are allocated as needed...
			// maybe some kinda aliasing so that server1-world1 and server2-world1 do not overlap.
			{
				PWORLD world = GetSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
				lprintf( WIDE("Create a world... %d"), ((INDEX*)params)[0] );
				world->name = ((INDEX*)params)[1];
			}
			break;

		case MSG_EVENT_CREATEWALL:
			{
				// pWorld, and wall info
				// ((INDEX*)params)[0] = world_idx
				// ((INDEX*)params)[1] = wall_idx;
				GETWORLD( ((INDEX*)params)[0] );
				//PWORLD pWorld = GetSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
				PWALL wall = GetWall( ((INDEX*)params)[1] );
				PWALL copy_from = (PWALL)(((INDEX*)params)+2);
				lprintf( WIDE("Create a wall... %d"), ((INDEX*)params)[1] );
				wall[0] = copy_from[0];
			}
			// client event notice refresh...
			// maybe wait for end_set
			break;
		case MSG_EVENT_DELETEWALL:
			{
				lprintf( WIDE("Destroy a wall... %d"), ((INDEX*)params)[1] );
				DestroyWall( ((INDEX*)params)[0], ((INDEX*)params)[1] );
			}
			break;
		case MSG_EVENT_CREATENAME:
			{
				GETWORLD( ((INDEX*)params)[0] );
				PNAME name = GetSetMember( NAME, &world->names, ((INDEX*)params)[1] );
				lprintf( WIDE("Create a name... %d"), ((INDEX*)params)[1] );
				MemCpy( name, ((INDEX*)params)+2, offsetof( NAME, name ) );
				name->name = NewArray( struct name_data, name->lines );
				{
					int n;
					TEXTSTR line = (TEXTSTR)((PTRSZVAL)(((INDEX*)params)+ 2)+offsetof( NAME, name ) );
					for( n = 0; n < name->lines; n++ )
					{
						_32 len = name->name[n].length = (_16)StrLen( line );
						name->name[n].name = NewArray( TEXTCHAR, len + 1 );
						MemCpy( name->name[n].name, line, len + 1 );
						line += len + 1;
					}
				}
				name->refcount = 1;
			}
			break;
		case MSG_EVENT_CREATESECTOR:
			{
				PWORLD world;
				PSECTOR sector;
				lprintf( WIDE("Create a sector... %d"), ((INDEX*)params)[1] );
				world = GetSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
				sector = GetSetMember( SECTOR, &world->sectors, ((INDEX*)params)[1] );
				if(0)
				{
		case MSG_EVENT_UPDATESECTOR:
					lprintf( WIDE("update a sector... %d"), ((INDEX*)params)[1] );
					world = GetSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
					sector = GetUsedSetMember( SECTOR, &world->sectors, ((INDEX*)params)[1] );
				}
				{
					PSECTOR copy_from = (PSECTOR)( ((INDEX*)params) + 2 );
					sector[0] = copy_from[0];
					sector->pointlist = NULL;
					sector->pointlist = ComputeSectorPointList( ((INDEX*)params)[0], ((INDEX*)params)[1], &sector->npoints );
					ComputeSectorOrigin( ((INDEX*)params)[0], ((INDEX*)params)[1] );
				}
			}
			break;
		case MSG_EVENT_DELETESECTOR:
			{
				lprintf( WIDE("delete sector %d:%d"), ((INDEX*)params)[0], ((INDEX*)params)[1] );
				DestroySector( ((INDEX*)params)[0], ((INDEX*)params)[1] );
			}
			break;
		case MSG_EVENT_CREATETEXTURE:
			{
				GETWORLD( ((INDEX*)params)[0] );
				PFLATLAND_TEXTURE texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, ((INDEX*)params)[1] );
				PFLATLAND_TEXTURE copy_from = (PFLATLAND_TEXTURE)(((INDEX*)params)+2);
				texture[0] = copy_from[0];
			}
			break;
		case MSG_EVENT_CREATELINE:
			{
				PWORLD world;
				PFLATLAND_MYLINESEG line;
				//lprintf( WIDE("create line: %d"), ((INDEX*)params)[1] );
				lprintf( WIDE("Create a line... %p in %p"), ((INDEX*)params)[1], ((INDEX*)params)[0] );
				world = GetUsedSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
				line = GetSetMember( FLATLAND_MYLINESEG, &world->lines, ((INDEX*)params)[1] );
				if( 0 ){
		case MSG_EVENT_UPDATELINE:
					lprintf( WIDE("update line: %p in %p"), ((INDEX*)params)[1], ((INDEX*)params)[1] );
					world = GetUsedSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
					line = GetUsedSetMember( FLATLAND_MYLINESEG, &world->lines, ((INDEX*)params)[1] );
				}
				if( line )
				{
					PFLATLAND_MYLINESEG copy_from = (PFLATLAND_MYLINESEG)(((INDEX*)params) + 2);
					MemCpy( line, copy_from, sizeof( line[0] ) );
					lprintf( WIDE("Create a line... %d %g %g %g"), ((INDEX*)params)[1], line->r.n[0], line->r.n[1], line->r.n[2] );
				}
				else
					lprintf( WIDE("No Line?") );
			}
			break;
		case MSG_EVENT_DELETELINE:
			{
				PWORLD world;
				//PFLATLAND_MYLINESEG line;
				//lprintf( WIDE("create line: %d"), ((INDEX*)params)[1] );
				lprintf( WIDE("Delete a line... %d"), ((INDEX*)params)[1] );
				world = GetUsedSetMember( WORLD, &g.worlds, ((INDEX*)params)[0] );
				DeleteSetMember( FLATLAND_MYLINESEG, world->lines, ((INDEX*)params)[1] );
			}
			break;
		}
		Deallocate( P_32, msg );
	}
	g.flags.changes_done = 0; // no changes pending
	g.flags.accepting_changes = 0;
}

int CPROC ConnectToServer( void )
#define ConnectToServer() ( g.flags.connected || ConnectToServer() )
{
	if( !g.flags.connected )
	{
		g.MsgBase = LoadService( WORLD_SCAPE_INTERFACE_NAME, EventHandler );
		Log1( WIDE("worldscape message base is %d"), g.MsgBase );
		if( g.MsgBase )
			g.flags.connected = 1;
		else
		{
			LoadFunction( WIDE("world_scape_msg_server.dll"), NULL );
			g.MsgBase = LoadService( WORLD_SCAPE_INTERFACE_NAME, EventHandler );
			Log1( WIDE("worldscape message base is %d"), g.MsgBase );
			if( g.MsgBase )
				g.flags.connected = 1;
		}
	}
	if( !g.flags.connected )
		Log( WIDE("Failed to connect") );
	return g.flags.connected;
}

static void DisconnectFromServer( void )
{
	if( g.flags.connected )
	{
		Log( WIDE("Disconnecting from service (worldscape)") );
		UnloadService( WORLD_SCAPE_INTERFACE_NAME );
		g.MsgBase = 0;
		g.flags.connected = 0;
	}
}

#define DefineMarkers( PNAME, name, Name, iName ) \
void Mark##Name##Updated( INDEX iWorld, INDEX iName )   \
{  \
	INDEX ResultID;\
	_32 Result[1];\
	_32 ResultLen = 0;\
	PNAME Name; \
	Get##Name##Data( iWorld, iName, &Name ); \
	if( ConnectToServer()\
	&& TransactServerMultiMessage( MSG_ID(Update##Name), 2    \
								, &ResultID, Result, &ResultLen \
								, &iWorld, ParamLength( iWorld, iName ) \
								, Name, sizeof( *Name ) \
)\
		&& ( ResultID == (MSG_ID(Update##Name)|SERVER_SUCCESS)))\
	{\
		return /*Result[0]*/;\
	}\
	return /*INVALID_INDEX*/;\
	/* not sure what to do here on the client side, yet */ \
}

void MarkLineUpdated( INDEX iWorld, INDEX iName )   
{  
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = 0;
	PFLATLAND_MYLINESEG Name; 
	GetLineData( iWorld, iName, &Name ); 
	if( ConnectToServer()
	&& TransactServerMultiMessage( MSG_ID(UpdateLine), 2
								, &ResultID, Result, &ResultLen 
								, &iWorld, ParamLength( iWorld, iName ) 
								, Name, sizeof( *Name ) 
)
		&& ( ResultID == (MSG_ID(UpdateLine)|SERVER_SUCCESS)))
	{
		return /*Result[0]*/;
	}
	return /*INVALID_INDEX*/;
	/* not sure what to do here on the client side, yet */ 
}

//DefineMarkers( sector, Sector, iSector );
//DefineMarkers( PFLATLAND_MYLINESEG, line, Line, iLine );  // we cheat on lines...
//DefineMarkers( name, Name, iName );
//DefineMarkers( texture, Texture, iTexture );
//DefineMarkers( wall, Wall, iWall );



//--------------------------------------------------------------------

void CPROC DestroyWorld( INDEX world )
{
	if( ConnectToServer() )
		TransactServerMessage( g.MsgBase, MSG_OFFSET( DestroyWorld )
									, &world, sizeof( world )
									, NULL, NULL, 0 );
}

//--------------------------------------------------------------------
 
 int  SaveWorldToFile ( INDEX pWorld );
 int  LoadWorldFromFile ( INDEX pWorld );


 int  MergeSelectedWalls ( INDEX iWorld, INDEX pDefinite, PORTHOAREA rect );
 int  MarkSelectedSectors ( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount );
 int  MarkSelectedWalls ( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount );
 void  MergeOverlappingWalls ( INDEX iWorld, PORTHOAREA rect );

 int  ValidateWorldLinks ( INDEX iWorld );

//------ Sectors ----------
 INDEX  FindSectorAroundPoint ( INDEX iWorld, P_POINT p );

// void  DeleteSectors ( INDEXSET *psectors );

void CPROC FlatlandForAllSectors( INDEX iWorld
			  , FESMCallback f, PTRSZVAL psv )
{
	GETWORLD( iWorld );
	if( world ) 
		DoForAllSectors( world->sectors, f, psv );
}

WORLD_PROC( void, AddUpdateCallback )( WorldScapeUdpateProc proc, PTRSZVAL psv )
{
	struct client_global_UpdateCallback_tag *blah;
	blah = New( struct client_global_UpdateCallback_tag );
	blah->proc = proc;
	blah->psv = psv;
	AddLink( &g.callbacks, blah );
}


static WORLD_SCAPE_INTERFACE ClientInterface
	= { OpenWorld
	  , DestroyWorld
	  , ResetWorld
	  , GetSectorCount
	  , GetWallCount
	  , GetLineCount
	  , SaveWorldToFile
	  , LoadWorldFromFile


	  , MergeSelectedWalls//WORLD_PROC( int, MergeSelectedWalls )( INDEX iWorld, INDEX iDefiniteWall, PORTHOAREA rect );
	  , NULL//WORLD_PROC( int, MarkSelectedSectors )( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount );
	  , NULL//WORLD_PROC( int, MarkSelectedWalls )( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount );
	  , NULL//WORLD_PROC( void, MergeOverlappingWalls )( INDEX iWorld, PORTHOAREA rect );

	  , NULL//WORLD_PROC( int, ValidateWorldLinks )( INDEX iWorld );

//------ Sectors ----------
	  , FindSectorAroundPoint//WORLD_PROC( INDEX, FindSectorAroundPoint )( INDEX iWorld, P_POINT p );

//	  , NULL//WORLD_PROC( void, DeleteSectors )( INDEXSET *psectors );

	  , CreateSquareSector
	  , AddConnectedSector //WORLD_PROC( INDEX, AddConnectedSector )( INDEX iWorld, INDEX iWall, RCOORD offset );
	  , MoveWalls//WORLD_PROC( int, MoveWalls )( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );
		// send current line, get all updates from world-scape server
	  , NULL // UpdateMatingLines//WORLD_PROC( int, UpdateMatingLines )( INDEX iWorld, INDEX iWall, int bLockSlopes, int bErrorOK );
	  , WallInSector//WORLD_PROC( int, WallInSector )( INDEX iWorld, INDEX iSector, INDEX iWall );

	  , GetSectorOrigin//WORLD_PROC( void, GetSectorOrigin )( INDEX iWorld, INDEX sector, P_POINT o );
	  , LineInCur//WORLD_PROC( int, LineInCur )( INDEX iWorld
									// , INDEX *SectorList, int nSectors
									// , INDEX *WallList, int nWalls
									// , INDEX pLine );
	  , FindIntersectingWall//WORLD_PROC( INDEX, FindIntersectingWall )( INDEX iWorld, INDEX iSector, P_POINT n, P_POINT o );

	  , DestroySector//WORLD_PROC( int, DestroySector )( INDEX iWorld, INDEX iSector );

	  , FlatlandPointWithin//WORLD_PROC( INDEX, FlatlandPointWithin )( INDEX iWorld, int nSectors, INDEX *piSectors, P_POINT p );
	  , FlatlandPointWithinSingle//WORLD_PROC( INDEX, FlatlandPointWithinSingle )( INDEX world, INDEX iSector, P_POINT p );
	  , FlatlandPointWithinLoopSingle//WORLD_PROC( INDEX, FlatlandPointWithinLoopSingle )(  PTRSZVAL psv, INDEX iSector );

	  , ComputeSectorOrigin//WORLD_PROC( void, ComputeSectorOrigin )( INDEX iWorld, INDEX iSector );
	  , ComputeSectorSetOrigin//WORLD_PROC( void, ComputeSectorSetOrigin )( INDEX iWorld, int nSectors, INDEX *sectors, P_POINT origin );
	  , ComputeWallSetOrigin//WORLD_PROC( void, ComputeWallSetOrigin )( INDEX iWorld, int nWalls, INDEX *walls, P_POINT origin );

	  , MoveSectors//WORLD_PROC( int, MoveSectors )( INDEX iWorld
										//, int nSectors
										//, INDEX *pSectors, P_POINT del );


	  , CreateWall//WORLD_PROC( INDEX, CreateWall )( INDEX world, INDEX iSector
										// , INDEX pStart, int bFromStartEnd
										// , INDEX pEnd,   int bFromEndEnd
										// , _POINT o, _POINT n );
	, DestroyWall //WORLD_PROC( int, DestroyWall )( INDEX iWorld, INDEX iWall );

	  , MergeWalls//WORLD_PROC( int, MergeWalls )( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall );
	  , SplitWall//WORLD_PROC( void, SplitWall )( INDEX iWorld, INDEX iWall );
	  , RemoveWall//WORLD_PROC( int, RemoveWall )( INDEX iWorld, INDEX iWall ); // remove wall, and relink mating...


	  , NULL//WORLD_PROC( _POINT*, CheckPointOrder )( PC_POINT normal, _POINT *plist, int npoints );
	  , ComputeSectorPointList//WORLD_PROC( _POINT*, ComputeSectorPointList )( INDEX iWorld, INDEX sector, int *pnpoints );
	  , NULL//WORLD_PROC( void, BreakWall )( INDEX iWorld, INDEX wall );

	  , NULL//WORLD_PROC( void, GetNameText )( INDEX iWorld, INDEX name, char *text, int maxlen );
	  , GetWallLine//WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX wall );
	  , GetWallSector//WORLD_PROC( INDEX, GetWallSector )( INDEX iWorld, INDEX iSector );
	  , GetMatedWall//WORLD_PROC( INDEX, GetMatedWall )( INDEX iWorld, INDEX wall );
	  , GetFirstWall//WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend );
	  , GetNextWall//WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX wall, int *priorend );
	  , GetSectorName//WORLD_PROC( INDEX, GetSectorName )( INDEX iWorld, INDEX iSector );

	  , GetLineData//WORLD_PROC( void, GetLineData )( INDEX iWorld, INDEX iLine, PFLATLAND_MYLINESEG *line );
	  , BalanceALine//WORLD_PROC( void, BalanceALine )( INDEX iWorld, INDEX iLine );
	  , GetNameData//WORLD_PROC( void, GetNameData )( INDEX iWorld, INDEX iName, PNAME *name );
	  , GetSectorTexture//WORLD_PROC( INDEX, GetSectorTexture )( INDEX iWorld, INDEX iSector );
	  , GetTextureData//WORLD_PROC( void, GetTextureData )( INDEX iWorld, INDEX iTexture, PFLATLAND_TEXTURE *texture );
	  , NULL//WORLD_PROC( void, GetSectorPoints )( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints );
//	  , NULL//WORLD_PROC( PFLATLAND_TEXTURE, GetSectorData )( INDEX iWorld, INDEX iTexture );

	  , FlatlandForAllSectors

// internal only function use
// BalanceALine and pass world and line index
	  //, NULL//WORLD_PROC( void, BalanceLine )( PFLATLAND_MYLINESEG pls );


	  , NULL//WORLD_PROC( INDEX, MakeTexture )( INDEX iWorld, char *text );
	  , NULL//WORLD_PROC( void, SetSolidColor )( INDEX iWorld, INDEX iTexture, CDATA color );
	  , NULL//WORLD_PROC( void, DeleteTexture )( INDEX iWorld, INDEX iTexture );
	  , NULL//WORLD_PROC( void, DeleteTextures )( INDEX iWorld );

	  , NULL//WORLD_PROC( INDEX, MakeName )( INDEX iWorld, char *text );
	  , NULL//WORLD_PROC( void, SetName )( INDEX iWorld, INDEX iName, char *text );
	  , NULL//WORLD_PROC( void, DeleteNames )( INDEX iWorld );
	  , NULL//WORLD_PROC( void, DeleteName )( INDEX iWorld, INDEX iName );

	  , ClearUndo
	  , DoUndo

	  , AddUndo
	  , EndUndo


	  , NULL//WORLD_PROC( INDEX, ForAllTextures )( INDEX iWorld, INDEX (CPROC*)(INDEX,PTRSZVAL), PTRSZVAL );
	  , NULL//WORLD_PROC( void, GetTextureNameText )( INDEX iWorld, INDEX iTexture, char *buf, int bufsize );
	  , NULL//WORLD_PROC( INDEX, SetTexture )( INDEX iWorld, INDEX iSector, INDEX iTexture );
	  , NULL //SetSectorName
	  , AddUpdateCallback
	  , NULL //UpdateLine
	  , NULL //UpdateSector
	  , NULL //UpdateWall
	  , NULL //UpdateName
	  , NULL //UpdateTexture
	  , NULL //FlushUpdates
	  , NULL //MarkLineChanges
	  , SendLinesChanged
	  , NULL //SendLineNormalsChanged
	  , SendLineChanged

     , IsSectorClockwise
     , AcceptChanges

	};

static int references;
static POINTER  CPROC GetMyInterface ( void )
{
	if( ConnectToServer() )
	{
		references++;
		return &ClientInterface;
	}
	return NULL;
}

static void CPROC DropMyInterface( POINTER p )
{
	if( references )
	{
		Log1( WIDE("Dropping 1 of %d display connections.."), references );
		references--;
		if( !references )
			DisconnectFromServer();
	}
}

ATEXIT( DropAllInterfaces )
{
	do
	{
		DropMyInterface( NULL );
	} while( references );
}

PRELOAD( Startup )
{
	RegisterInterface( WORLD_SCAPE_INTERFACE_NAME, GetMyInterface, DropMyInterface );
}

PUBLIC( void, DamnitBuildAExportLibrary )( void )
{
}


