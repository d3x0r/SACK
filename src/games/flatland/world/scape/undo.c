#define WORLD_SOURCE

#include <stdhdrs.h>
#include <stdarg.h>

#include <sharemem.h>
#include <logging.h>

#include <world.h>
#include "global.h"

#include "lines.h"
#include "sector.h"
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
	GETWORLD( iWorld );
	if( world )
		while( world->firstundo )
		{
			PUNDORECORD next;
			next = world->firstundo->next;
			Release( world->firstundo );
			world->firstundo = next;
		}
}


void AddUndo( INDEX iWorld, int type, ... )
{
	va_list arglist;
	PUNDORECORD undo;
   GETWORLD( iWorld );
	//Log1( "Adding undo type:  %s", UndoNames[type] );
	undo = (PUNDORECORD)Allocate( sizeof( UNDORECORD ) );
	undo->next = world->firstundo;
	world->firstundo = undo;
	undo->type = type;
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
}

void SrvrEndUndo( _32 client_id, INDEX iWorld, int type, ... )
{
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
}

void SrvrDoUndo( _32 client_id, INDEX iWorld )
{
   GETWORLD( iWorld );
	PUNDORECORD next;
	if( world->firstundo )
	{
		next = world->firstundo->next;
		switch( world->firstundo->type )
		{
		case UNDO_SECTORMOVE:
			if( world->firstundo->data.sectorset.bCompleted )
			{
				_POINT del;
				sub( del, world->firstundo->data.sectorset.origin
							, world->firstundo->data.sectorset.endorigin );
				SrvrMoveSectors( client_id, iWorld
							  , world->firstundo->data.sectorset.nsectors
							  , world->firstundo->data.sectorset.sectors
							  , del );
				Release( world->firstundo->data.sectorset.sectors );
			}
			else
			{
				Log( "Sector move was never completed?!" );
			}
			break;
		case UNDO_WALLMOVE:
			{
				int n;
				for( n = 0; n < world->firstundo->data.wallset.nwalls; n++ )
				{
               PWALL pWall = GetWall( world->firstundo->data.wallset.walls[n] );
					PFLATLAND_MYLINESEG pLine = GetLine( pWall->iLine);
               *pLine = world->firstundo->data.wallset.lines[n];
				}
				for( n = 0; n < world->firstundo->data.wallset.nwalls; n++ )
				{
					SrvrUpdateMatingLines( client_id, iWorld
										  , world->firstundo->data.wallset.walls[n], FALSE, TRUE );
				}
				Release( world->firstundo->data.wallset.walls );
				Release( world->firstundo->data.wallset.lines );
			}
			break;
		case UNDO_SLOPEMOVE:
		case UNDO_ENDMOVE:
		case UNDO_STARTMOVE:
			{
				PWALL pWall = GetWall( world->firstundo->data.end_start.pWall );
				PFLATLAND_MYLINESEG pLine = GetLine( pWall->iLine );
				*pLine =
					world->firstundo->data.end_start.original;
				SrvrUpdateMatingLines( client_id, iWorld
									  , world->firstundo->data.end_start.pWall, FALSE, TRUE );
			}
			break;
		default:
			Log1( "Unknown undo operation %s",UndoNames[world->firstundo->type] );
			break;
		}
		Release( world->firstundo );
		world->firstundo = next;
	}
	else
		Log( "Nothing to undo..." );
}
