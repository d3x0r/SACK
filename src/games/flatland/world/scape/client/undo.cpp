#define WORLD_SOURCE
#define WORLDSCAPE_INTERFACE_USED
#define WORLD_CLIENT_LIBRARY

#include <stdhdrs.h>
#include <stdio.h>
#include <stdarg.h>

#include <msgclient.h>
#include <sharemem.h>
#include <logging.h>

#include <world.h>
#include "global.h"
// UNDO_ENDMOVE - PWALL wall (save line on wall.)
// UNDO_STARTMOVE - PWALL wall (save line on wall)
// UNDO_SLIPEMOVE - PWALL wall (save line on wall)
// UNDO_WALLMOVE - int walls, PWALL *wall (save lines on walls)
// UNDO_SECTORMOVE(add) - int sectors, PSECTOR *sectors (save origins), P_POINT start
// UNDO_SECTORMOVE(end) - P_POINT end ...

char *UndoNames[] = {
	"UNDO_SECTORMOVE"
	,"UNDO_WALLMOVE"
	,"UNDO_SLOPEMOVE"
	,"UNDO_STARTMOVE"
	,"UNDO_ENDMOVE"
	,"UNDO_SPLIT"
	,"UNDO_MERGE"
	,"UNDO_DELETESECTOR"
	,"UNDO_DELETEWALL"
};

extern GLOBAL g;

void ClearUndo( INDEX iWorld )
{
	if( ConnectToServer() )
		TransactServerMessage( g.MsgBase, MSG_OFFSET(ClearUndo)
									, &iWorld, sizeof( iWorld )// typically a world ID...
									, NULL, NULL, 0 );
}


void AddUndo( INDEX iWorld, int type, ... )
{
	va_list arglist;
	PUNDORECORD undo;
	if( ConnectToServer() )
		TransactServerMessage( g.MsgBase, MSG_OFFSET(AddUndo)
									, &iWorld, sizeof( iWorld )// typically a world ID...
									, NULL, NULL, 0 );
#if 0
   va_start( arglist, type );
	switch( type )
	{
	case UNDO_SECTORMOVE:
		{
			int nsectors = va_arg( arglist, int );
			INDEX *sectors = va_arg( arglist, INDEX* );
			P_POINT origin = va_arg( arglist, P_POINT );
			int n;
			undo->data.sectorset.nsectors = nsectors;
			undo->data.sectorset.sectors = (INDEX*)Allocate( sizeof( INDEX ) * nsectors );
         undo->data.sectorset.bCompleted = FALSE;
			SetPoint( undo->data.sectorset.origin, origin );
         for( n = 0; n < nsectors; n++ )
         {
         	undo->data.sectorset.sectors[n] = sectors[n];
         }
		}
		break;
	case UNDO_WALLMOVE:
		{
			int nwalls = va_arg( arglist, int );
			INDEX *walls = va_arg( arglist, INDEX* );
			int n;
			undo->data.wallset.nwalls = nwalls;
			undo->data.wallset.walls = (INDEX*)Allocate( sizeof( INDEX ) * nwalls );
			undo->data.wallset.lines = (FLATLAND_MYLINESEG*)Allocate( sizeof(FLATLAND_MYLINESEG) * nwalls );
			for( n = 0; n < nwalls; n++ )
			{
				undo->data.wallset.walls[n] = walls[n];
            undo->data.wallset.lines[n] = *(GetLine( GetWall( walls[n] )->iLine ));
			}
		}
		break;
	case UNDO_SLOPEMOVE:
	case UNDO_ENDMOVE:
	case UNDO_STARTMOVE:
		{
			INDEX wall = va_arg( arglist, INDEX );
			undo->data.end_start.pWall = wall;
			undo->data.end_start.original = *(GetLine( GetWall( wall )->iLine ) );
		}
		break;
	default: 
		Log1( "Cannot handle specified undo type...%s", UndoNames[type] );
	}
#endif
}

void EndUndo( INDEX iWorld, int type, ... )
{
	if( ConnectToServer() )
		TransactServerMessage( g.MsgBase, MSG_OFFSET(EndUndo)
									, &iWorld, sizeof( iWorld )// typically a world ID...
									, NULL, NULL, 0 );
#if 0
	GETWORLD( iWorld );
	if( world->firstundo->type != type )
	{
		Log( "Undo that was started is not the type we're ending..." );
		return;
	}
	else
	{
		va_list arglist;
		va_start( arglist, type );
		switch( type )
		{
		case UNDO_SECTORMOVE:
			{
				P_POINT endorigin = va_arg( arglist, P_POINT );
				world->firstundo->data.sectorset.bCompleted = TRUE;
				SetPoint( world->firstundo->data.sectorset.endorigin, endorigin );
			}
			break;
		default:
			Log( "Undo type does not need a continue?" );
			break;
		}
	}
#endif
}

void DoUndo( INDEX iWorld )
{
	if( ConnectToServer() )
		TransactServerMessage( g.MsgBase, MSG_OFFSET(DoUndo)
									, &iWorld, sizeof( iWorld )// typically a world ID...
									, NULL, NULL, 0 );
}
