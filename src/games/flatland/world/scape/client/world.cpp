#define WORLD_SOURCE
#define WORLDSCAPE_INTERFACE_USED
#define WORLD_CLIENT_LIBRARY
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h> // debugbreak
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <msgclient.h>
#include <sharemem.h>
#include <logging.h>
#include "world.h"
#include "lines.h"
#include "sector.h"
//#include "names.h"
#include "walls.h"

#include "global.h"
#include <world_proto.h>
//#define LOG_SAVETIMING

extern GLOBAL g;
//----------------------------------------------------------------------------

INDEX  CPROC CreateSquareSector( INDEX iWorld, PC_POINT pOrigin, RCOORD size )
{
	MSGIDTYPE ResultID;
	uint32_t Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(CreateSquareSector), 3
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, pOrigin, DIMENSIONS * sizeof( RCOORD )
								, &size, sizeof( size )
								)
		&& ( ResultID == (MSG_ID(CreateSquareSector)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

#if 0
uintptr_t CPROC CompareWorldName( POINTER p, uintptr_t psv )
{
	CTEXTSTR name = (CTEXTSTR)psv;
	PWORLD world = (PWORLD)p;
	TEXTCHAR buffer[256];
   /* was working on fixing this when I learned it's not even used */
   PNAME GetName( world->name, buffer, sizeof( buffer )/sizeof(TEXTCHAR) )
	if( StrCmp( world->name, name ) == 0 )
		return (uintptr_t)world;
	return 0;
}
#endif


INDEX OpenWorld( CTEXTSTR name )
{
	MSGIDTYPE ResultID;
	INDEX Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMessage( g.MsgBase, MSG_OFFSET(OpenWorld), name, (StrLen( name )+1)*sizeof(TEXTCHAR)
								, &ResultID, Result, &ResultLen )
		&& ( ResultID == (MSG_ID(OpenWorld)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX CreateBasicWorld( void )
{
	INDEX world = OpenWorld( WIDE("Default world") );
	CreateSquareSector( world, VectorConst_0, 50 );
	return world;
}

//----------------------------------------------------------------------------
WORLD_PROC( PSECTORSET, GetSectors )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->sectors;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( PWALLSET, GetWalls )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->walls;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( PFLATLAND_MYLINESEGSET, GetLines )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->lines;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetSectorCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return CountUsedInSet( SECTOR, world->sectors );
	return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetWallCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
   if( world )
		return CountUsedInSet( WALL, world->walls );
   return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetLineCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return CountUsedInSet( FLATLAND_MYLINESEG, world->lines );
	return 0;
}
//----------------------------------------------------------------------------

WORLD_PROC( INDEX, GetWallSector )( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	if( wall )
		return wall->iSector;
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

void ResetWorld( INDEX iWorld )
{
	if( ConnectToServer() )
	{
		// even with wait, it still returns before events are sent to the client.
		MSGIDTYPE wait;
		TransactServerMessage( g.MsgBase, MSG_OFFSET(ResetWorld)
									, &iWorld, ParamLength( iWorld, iWorld )
									, &wait, NULL, 0 );
		WakeableSleep( 100 );
		// and then wait until there's no more events... ?
	}
}

//----------------------------------------------------------------------------

#define WriteSize( name ) {                                            \
			name##pos = ftell( pFile );                                   \
			name##size = 0;                                               \
			sz += fwrite( &name##size, 1, sizeof( name##size ), pFile );  \
			}

#define UpdateSize(name) { int endpos;                          \
			endpos = ftell( pFile );                               \
			fseek( pFile, name##pos, SEEK_SET );                   \
			fwrite( &name##size, 1, sizeof( name##size ), pFile ); \
			fseek( pFile, endpos, SEEK_SET );                      \
			sz += name##size; }                                    


int SaveWorldToFile( INDEX iWorld )
{
	MSGIDTYPE ResultID;
	uint32_t Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(SaveWorldToFile), 1
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								)
		&& ( ResultID == (MSG_OFFSET(SaveWorldToFile)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return FALSE;
}

//----------------------------------------------------------------------------

int LoadWorldFromFile( INDEX iWorld )
{
	MSGIDTYPE ResultID;
	uint32_t Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(LoadWorldFromFile), 1
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								)
		&& ( ResultID == (MSG_ID(LoadWorldFromFile)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return FALSE;
}

//----------------------------------------------------------------------------

typedef struct CollisionFind_tag {
	RAY r;
	INDEX walls[2];
	int nwalls;
	INDEX iWorld;
} WALLSELECTINFO, *PWALLSELECTINFO;

//----------------------------------------------------------------------------

INDEX CPROC CheckWallSelect( PWALL wall, PWALLSELECTINFO si )
{
	GETWORLD(si->iWorld);
	//PWALL wall = GetWall( iWall );
	PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
	RCOORD t1, t2;
	if( FindIntersectionTime( &t1, si->r.n, si->r.o
						         , &t2, line->r.n, line->r.o ) )
	{
		if( t1 >= 0 && t1 <= 1 &&
		    t2 >= line->dFrom && t2 <= line->dTo )
		{
			if( si->nwalls < 2 )
			{
				si->walls[si->nwalls++] = GetMemberIndex( WALL, &world->walls, wall );
			}
			else
				return 1; // breaks for_all loop
		}
		//Log4( WIDE("Results: %g %g (%g %g)"), t1, t2, line->start, line->dTo );
	}
	return 0;
}

//----------------------------------------------------------------------------

int MergeSelectedWalls( INDEX iWorld, INDEX iDefinite, PORTHOAREA rect )
{
	GETWORLD(iWorld );
   //PWALL pDefinite = GetWall( iDefinite );
	WALLSELECTINFO si;
   // normal 1
	si.r.n[0] = rect->w;
	si.r.n[1] = rect->h;
	si.r.n[2] = 0;
	si.r.o[0] = rect->x;
	si.r.o[1] = rect->y;
	si.r.o[2] = 0;
	si.iWorld = iWorld;
	si.nwalls = 0;
	
	if( !ForAllWalls( world->walls, CheckWallSelect, &si ) )
	{
		Log1( WIDE("Found %d walls to merge: %d"), si.nwalls );
		if( si.nwalls == 2 && 
			( GetWall( si.walls[0] )->iSector != GetWall( si.walls[1] )->iSector ) )
		{
			MergeWalls( iWorld, si.walls[0], si.walls[1] );
		}
		if( si.nwalls == 1 &&
			 iDefinite != INVALID_INDEX &&
		    si.walls[0] != iDefinite )
		{
			MergeWalls( iWorld, iDefinite, si.walls[0] );
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

typedef struct sectorselectinfo_tag {
   INDEX iWorld;
	PORTHOAREA rect;
	int nsectors;
	INDEX *ppsectors;
} SECTORSELECTINFO, *PSECTORSELECTINFO;

//----------------------------------------------------------------------------

INDEX CPROC CheckSectorInRect( INDEX sector, PSECTORSELECTINFO psi )
{
	PORTHOAREA rect = psi->rect;
	INDEX pStart, pCur;
	int priorend = TRUE;
	_POINT p;
   //GETWORLD( psi->iWorld );
	pCur = pStart = GetFirstWall( psi->iWorld, sector, &priorend );
	do
	{
		PFLATLAND_MYLINESEG line;
		GetLineData( psi->iWorld, GetWallLine( psi->iWorld, pCur ), &line );
		if( priorend )
		{
			addscaled( p, line->r.o, line->r.n, line->dTo );
		}
		else
		{
			addscaled( p, line->r.o, line->r.n, line->dFrom );
		}
		//Log7( WIDE("Checking (%g,%g) vs (%g,%g)-(%g,%g)"), 
		if( p[0] < (rect->x) ||
		    p[0] > (rect->x + rect->w) ||
          p[1] < (rect->y) ||
		    p[1] > (rect->y + rect->h) )
		{
			pCur = INVALID_INDEX;
			break;
		}
		pCur = GetNextWall( psi->iWorld
								, pCur, &priorend );
	}while( pCur != pStart );
	if( pCur == pStart )
	{
		if( psi->ppsectors )
			psi->ppsectors[psi->nsectors++] = sector;
		else
			psi->nsectors++;
	}
	return 0;
}

//----------------------------------------------------------------------------

int MarkSelectedSectors( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount )
{
	GETWORLD( iWorld );
	SECTORSELECTINFO si;
	si.rect = rect;
	si.nsectors = 0;		
	si.ppsectors = NULL;
	si.iWorld = iWorld;
	if( rect->w < 0 )
	{
		rect->x += rect->w;
		rect->w = -rect->w;
	}
	if( rect->h < 0 )
	{
		rect->y += rect->h;
		rect->h = -rect->h;
	}
	Log( WIDE("Marking Sectors") );
	DoForAllSectors( world->sectors, CheckSectorInRect, (uintptr_t)&si );
	if( si.nsectors )
	{
		Log1( WIDE("Found %d sectors in range"), si.nsectors );
		if( sectorcount )
			*sectorcount = si.nsectors;
		if( sectorarray )
		{
			*sectorarray = (INDEX*)Allocate( sizeof( INDEX ) * si.nsectors );
			si.ppsectors = *sectorarray;
			si.nsectors = 0;
			DoForAllSectors( world->sectors, CheckSectorInRect, (uintptr_t)&si );
		}
		return TRUE;
	}
	else
	{
		if( sectorcount )
			*sectorcount = si.nsectors;
		if( sectorarray )
			*sectorarray = NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

typedef struct groupwallselectinfo_tag {
	INDEX iWorld;
	PORTHOAREA rect;
	int nwalls;
	INDEX *ppwalls;
} GROUPWALLSELECTINFO, *PGROUPWALLSELECTINFO;

//----------------------------------------------------------------------------

void DumpBinary( unsigned char *pc, int sz )
{
	TEXTCHAR msg[256];
	int n = 0;
	while( sz-- )
	{
		n += snprintf( msg + n, 256 - n, WIDE("%02x"), *pc++ );
		if( ( sz & 3 ) == 0 )
			n += snprintf( msg + n, 256 - n, WIDE(" ") );
	}
	Log( msg );
}

//----------------------------------------------------------------------------

int PointInRect( P_POINT point, PORTHOAREA rect )
{
	if( point[0] < (rect->x) ||
		 point[0] > (rect->x + rect->w) ||
	    point[1] < (rect->y) ||
	    point[1] > (rect->y + rect->h) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------

int CPROC CheckWallInRect( PWALL wall, PGROUPWALLSELECTINFO psi )
{
	GETWORLD(psi->iWorld);
	_POINT p1, p2;
	PORTHOAREA rect = psi->rect; // shorter pointer
	if( wall->iLine == INVALID_INDEX )
	{
		PSECTOR sector = GetSector( wall->iSector );
   		Log( WIDE("Line didn't exist...") );
   		if( sector )
		{
			PNAME name = GetName( sector->iName );
   			Log( WIDE("Sector exists...") );
   			if( name &&
				name[0].name )
		   		Log1( WIDE("Wall in Sector %s does not have a line"), name[0].name );
			else
				Log( WIDE("Sector referenced does not have a name") );
		}
		else
			Log( WIDE("Wall should not be active... WHY is it?!") );
   }
   else
   {
		PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
		addscaled( p1, line->r.o, line->r.n, line->dFrom );
		addscaled( p2, line->r.o, line->r.n, line->dTo );
		if( !PointInRect( p1, rect) ||
	   	 !PointInRect( p2, rect) )
		{
			return 0;
		}
   		else
		{
			if( psi->ppwalls )
			{
				psi->ppwalls[psi->nwalls++] = GetWallIndex( wall );
				lprintf( WIDE("Client side... nto sure if we need a balance here? "));
				//BalanceALine( psi->iWorld, psi->ppwalls[psi->nwalls++], wall->iLine, line );
			}
			else
				psi->nwalls++;
		}
	}
	return 0; // abort immediate...
}

//----------------------------------------------------------------------------

void MergeOverlappingWalls( INDEX iWorld, PORTHOAREA rect )
{
	// for all walls - find a wall without a mate in the rect area...
	// then for all remaining walls - find another wall that is the 
	// same line as this one....
	GETWORLD(iWorld);

	int nwalls, n;
	PWALL *wallarray;
	wallarray = GetLinearWallArray( world->walls, &nwalls );
	for( n = 0; n < nwalls; n++ )
	{
		_POINT start, end;
		PWALL wall = wallarray[n];
		if( wall->iWallInto == INVALID_INDEX )
		{
			PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
			addscaled( start, line->r.o, line->r.n, line->dFrom );
			addscaled( end, line->r.o, line->r.n, line->dTo );
			if( PointInRect( start, rect ) &&
			    PointInRect( end, rect ) )
			{
				int m;
				for( m = n+1; m < nwalls; m++ )
				{
					PWALL wall2 = wallarray[m];
					if( wall2->iWallInto == INVALID_INDEX )
					{
						_POINT start2, end2;
						PFLATLAND_MYLINESEG line = GetLine( wall2->iLine );
						addscaled( start2, line->r.o, line->r.n, line->dFrom );
						addscaled( end2, line->r.o, line->r.n, line->dTo );
						/*
						if( PointInRect( start2, rect ) && 
							 PointInRect( end2, rect ) )
						{
							Log4( WIDE("starts: (%12.12g,%12.12g) vs (%12.12g,%12.12g)") 
										,start[0], start[1]
										,start2[0], start2[1] );
							Log4( WIDE("ends  : (%12.12g,%12.12g) vs (%12.12g,%12.12g)") 
										,end[0], end[1]
										,end2[0], end2[1] );
						}
						*/
						if( ( Near( start2, start ) &&
						      Near( end2, end ) ) 
						  ||( Near( start2, end ) &&
						      Near( end2, start ) ) )
						{
							MergeWalls( iWorld, GetWallIndex( wall ), GetWallIndex( wall2 ) );
							break;
						}

					}
				}
			}
		}	
	}
	Release( wallarray );
}

//----------------------------------------------------------------------------

int MarkSelectedWalls( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount )
{
	GETWORLD( iWorld );
	GROUPWALLSELECTINFO si;
	si.iWorld = iWorld;
	si.rect = rect;
	si.nwalls = 0;
	si.ppwalls = NULL;

	ForAllWalls( world->walls, CheckWallInRect, &si );
	if( si.nwalls )
	{
		if( wallcount )
			*wallcount = si.nwalls;
		if( wallarray )
		{
			*wallarray = (INDEX*)Allocate( sizeof( INDEX ) * si.nwalls );
			si.ppwalls = *wallarray;
			si.nwalls = 0;
			ForAllWalls( world->walls, CheckWallInRect, &si );
		}
		return TRUE;
	}
	else
	{
		if( wallcount )
			*wallcount = si.nwalls;
		if( wallarray )
			*wallarray = NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

int ValidateWorldLinks( INDEX iWorld )
{
	GETWORLD(iWorld);
	int status = TRUE;
	int nLines;
	PFLATLAND_MYLINESEG *pLines;
	int nWalls;
	PWALL *pWalls;
	int nSectors;
	PSECTOR *pSectors;
	int nNames;
	PNAME *pNames;
	pSectors = GetLinearSectorArray( world->sectors, &nSectors );
	pWalls = GetLinearWallArray( world->walls, &nWalls );
	pLines = GetLinearLineArray( world->lines, &nLines );
	pNames = GetLinearNameArray( world->names, &nNames );
	{
   	int n, m, refcount;
		for( n = 0; n < nLines; n++ )
		{
			refcount = 0;
			for( m = 0; m < nWalls; m++ )
			{
				if( GetLine( pWalls[m]->iLine ) == pLines[n] )
				{
					if( pWalls[m]->iWallInto != INVALID_INDEX )
					{
						refcount++;
						if( refcount == 2 )
						{
							//pLines[n] = NULL;
							break;
						}
					}
				}
				else
				{
					//pLines[n] = NULL;
					break;
				}
			}
			if( m == nWalls )
			{
				status = FALSE;
				Log1( WIDE("Line %08x is unreferenced... deleting now."), pLines[n] );
				DeleteLine( world->lines, pLines[n] );
				pLines[n] = NULL;
			}
		}
		for( n = 0; n < nWalls; n++ )
		{
			if( !pWalls[n] )
				continue;
			for( m = 0; m < nLines; m++ )
			{
				if( GetLine( pWalls[n]->iLine ) == pLines[m] )
				{
					// if this line is shared - remove the other reference to it.
					/*
					if( pWalls[n]->wall_into )
					{
						int i;
						for( i = 0; i < nWalls; i++ )
						{
							if( pWalls[i] == pWalls[n]->wall_into )
								pWalls[i] = NULL;
						}
					}
					pLines[m] = NULL; // clear line reference...
					*/
					break;
				}
			}
			if( m == nLines )
			{
				status = FALSE;
				Log3( WIDE("Wall %08x in Sector %d referenced line %08x that does not exist"), 
							pWalls[n], GetSector( pWalls[n]->iSector )->iName, pWalls[n]->iLine );
			}
		}
		for( n = 0; n < nLines; n++ )
		{
			int count = 0;
			if( !pLines[n]->refcount )
				Log( WIDE("Line exists with no reference count") );
			for( m = 0; m < nWalls; m++ )
			{
				if( GetLine( pWalls[m]->iLine ) == pLines[n] )
				{
					count++;
				}
			}
			if( count != pLines[n]->refcount )
			{
				Log2( WIDE("Line reference count of %d does not match actual %d")
							, pLines[n]->refcount
							, count );
			}
		}
		for( n = 0; n < nSectors; n++ )
		{
			PWALL pStart, pCur;
			int priorend = TRUE;
			if( pSectors[n]->iName != INVALID_INDEX )
			{
				int i;
				for( i = 0; i < nNames; i++ )
				{
					if( GetName( pSectors[n]->iName ) == pNames[i] )
						break;
				}
				if( i == nNames )
				{
					Log2( WIDE("Name %08x referenced by Sector %d does not exist"), pSectors[n]->iName, n );
				}
			}

			pCur = pStart = GetWall( pSectors[n]->iWall );
			do
			{
				if( pCur->iLine == INVALID_INDEX )
				{
					Log1( WIDE("Wall in sector %d has an invalid line def"), pSectors[n]->iName );
				}
				
				for( m = 0; m < nWalls; m++ )
				{
					if( pWalls[m] == pCur )
						break;
				}
				if( m == nWalls )
				{
					status = FALSE;
					Log4( WIDE("Sector %*.*s referenced wall %08x that does not exist"),
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].name, pCur );
				}
				// code goes here....
				if( priorend )
				{
					priorend = pCur->flags.wall_start_end;
					pCur = GetWall( pCur->iWallStart );
				}
				else
				{
					priorend = pCur->flags.wall_end_end;
					pCur = GetWall( pCur->iWallEnd );
				}
			}while( pCur != pStart );
		}
	}
	Release( pLines );
	Release( pWalls );
	Release( pSectors );
	Release( pNames );
	return status;
}

//----------------------------------------------------------------------------

// this really needs to use ForAllSectors ... 
// but then the callback needs to return a value to be able
// to stop that looping...

INDEX FindSectorAroundPoint( INDEX iWorld, P_POINT p )
{
	GETWORLD( iWorld );
	struct {
		INDEX iWorld;
		P_POINT p;
	} data;
	data.iWorld = iWorld;
	data.p = p;
	// hmm no spacetree now?
	//return FindPointInSpace( world->spacetree, p, PointWithinSingle );
   if( world )
		return DoForAllSectors( world->sectors, FlatlandPointWithinLoopSingle, &data ) - 1;
   return INVALID_INDEX;
}

//----------------------------------------------------------------------------

