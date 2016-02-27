#define WORLD_SOURCE
#define WORLDSCAPE_INTERFACE_USED
#ifndef WORLD_CLIENT_LIBRARY
#define WORLD_CLIENT_LIBRARY
#endif

#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include <math.h>
#include <msgclient.h>

#include "world.h"
#include "lines.h"

#include "global.h"
#include <world_proto.h>

extern GLOBAL g;

//extern WORLD world;
int SectorIDs;

//----------------------------------------------------------------------------


int SectorInSet( int nSize, INDEX *ppSector, INDEX pSector )
{
	int n;
	for( n = 0; n < nSize; n++ )
		if( ppSector[n] == pSector )
			return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------

int WallInSet( int nSize, PWALL *ppWall, PWALL pWall )
{
	int n;
	for( n = 0; n < nSize; n++ )
		if( ppWall[n] == pWall )
			return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------

void GetSectorMinMax( PSECTOR pSector, P_POINT min, P_POINT max )
{
	int i;
	SetPoint( min, pSector->pointlist[0] );
	SetPoint( max, pSector->pointlist[0] );

	for( i = 1; i < pSector->npoints; i++ )
	{
		if( pSector->pointlist[i][0] < min[0] )
			min[0] = pSector->pointlist[i][0];
		if( pSector->pointlist[i][1] < min[1] )
			min[1] = pSector->pointlist[i][1];
		if( pSector->pointlist[i][0] > max[0] )
			max[0] = pSector->pointlist[i][0];
		if( pSector->pointlist[i][1] > max[1] )
			max[1] = pSector->pointlist[i][1];
	}
	// min/max is needed for this only so - when we figure out how spacetree works
	// on the display side... move this line. (after calling create square sector)
	//pSector->spacenode = AddSpaceNode( &world->spacetree, pSector, min, max );
}

//----------------------------------------------------------------------------

// hmm this seems to be something like...
#ifdef USE_SPACETREE_INTERNAL
// this spot actually needs to move to display
void UpdateSpace( int nsectors, PSECTOR *sectors )
{
	int n;
	// update the space tree for this sector...
	for( n = 0; n < nsectors; n++ )
	{
		if( !RemoveSpaceNode( sectors[n]->spacenode ) )
		{
			Log( WIDE("Think we double removed this sector - removing from list") );
			sectors[n] = NULL;
		}
	}
	for( n = 0; n < nsectors; n++ )
	{   
		PSECTOR ps = sectors[n];
		if( ps )
		{
			_POINT min, max;
			GetSectorMinMax( ps, min, max );
			ReAddSpaceNode( &world.spacetree, ps->spacenode, min, max );
		}
	}
}
#endif

//----------------------------------------------------------------------------

void DumpWall( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PFLATLAND_MYLINESEG pLine = GetSetMember( FLATLAND_MYLINESEG
		, &world->lines
		, wall->iLine );
	DumpLine( pLine );

	Log5( WIDE("Wall(%08x): startend: %d (%08x) EndEnd: %d (%08x)")
		, wall
		, wall->flags.wall_start_end
		, wall->iWallStart
		, wall->flags.wall_end_end
		, wall->iWallEnd );
}


//----------------------------------------------------------------------------
INDEX CreateWallFromLine( INDEX iWorld
						 , INDEX iSector
						 // pStart == wall intended to be at the start part of
						 // this line...
						 // bFromStartEnd is a flag whether
						 // this new is at the end or start of the indicated line
						 , INDEX iStart, int bFromStartEnd
						 // pEnd == wall intended to be at the 'end' of this
						 // line...
						 // bFromEndEnd is a flag whether
						 // this new is at the end or start of the indicated line
						 , INDEX iEnd,   int bFromEndEnd
						 , INDEX iLine )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL pWall = GetFromSet( WALL, &world->walls );
	INDEX iWall = GetMemberIndex( WALL, &world->walls, pWall );
	pWall->flags.bUpdating = FALSE;
	pWall->iWallInto	= INVALID_INDEX;
	pWall->iSector	   = iSector;
	pWall->iLine		= iLine;
	{
		PFLATLAND_MYLINESEG pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine );
		pls->refcount++;
	}
	if( iStart != INVALID_INDEX )
	{
		PWALL pStart = GetSetMember( WALL, &world->walls, iStart );
		pWall->iWallStart = iStart;
		pWall->flags.wall_start_end = bFromStartEnd;
		if( bFromStartEnd )
		{
			pStart->flags.wall_end_end = FALSE;
			pStart->iWallEnd = iWall;
		}
		else
		{
			pStart->flags.wall_start_end = FALSE;
			pStart->iWallStart = iWall;
		}
		{
			PFLATLAND_MYLINESEG ps = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pStart->iLine );
			PFLATLAND_MYLINESEG pw = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWall->iLine );
			IntersectLines( ps, bFromStartEnd, pw, FALSE );
		}
		//UpdateClientWall( iWorld, iStart );
	}
	if( iEnd != INVALID_INDEX )
	{
		PWALL pEnd = GetSetMember( WALL, &world->walls, iEnd );
		pWall->iWallEnd = iEnd;
		pWall->flags.wall_end_end = bFromEndEnd;
		if( bFromEndEnd )
		{
			pEnd->flags.wall_end_end = TRUE;
			pEnd->iWallEnd = iWall;
		}
		else
		{
			pEnd->flags.wall_start_end = TRUE;
			pEnd->iWallStart = iWall;
		}
		{
			PFLATLAND_MYLINESEG pe = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pEnd->iLine );
			PFLATLAND_MYLINESEG pw = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWall->iLine );
			IntersectLines( pe, bFromEndEnd, pw, TRUE );
		}
	}
#ifdef WORLDSCAPE_SERVICE
	MarkWallUpdated( iWorld, iWall );
#endif
	return iWall;
}
//----------------------------------------------------------------------------

INDEX CreateWall( INDEX iWorld
				 , INDEX iSector
				 // pStart == wall intended to be at the start part of
				 // this line...
				 // bFromStartEnd is a flag whether
				 // this new is at the end or start of the indicated line
				 , INDEX iStart, int bFromStartEnd
				 // pEnd == wall intended to be at the 'end' of this
				 // line...
				 // bFromEndEnd is a flag whether
				 // this new is at the end or start of the indicated line
				 , INDEX iEnd,   int bFromEndEnd
				 , _POINT o, _POINT n )
{
	return CreateWallFromLine( iWorld, iSector
		, iStart, bFromStartEnd
		, iEnd, bFromEndEnd
		, CreateOpenLine( iWorld, o, n ) );
}

//----------------------------------------------------------------------------

int CountWalls( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PWALL pStart, pCur;
	int priorend = TRUE;
	int walls = 0;
	if( !wall )  // no walls.
		return 0; 
	pCur = pStart = wall;
	do
	{
		walls++;
		Log1( WIDE("Walls: %d"), walls );
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( pCur != pStart );
	return walls;
}

//----------------------------------------------------------------------------



void MarkThingChanged( PDATAQUEUE *ppdq, INDEX iWorld, INDEX iType, POINTER pType )
{
	if( !(*ppdq) )
		(*ppdq) = CreateDataQueue( sizeof( struct UpdateThingMsg ) );
	{
		//int n;
		//int success;
		//INDEX iLineCheck;
		struct UpdateThingMsg msg;
		//for( n = 0; success = PeekDataQueueEx( ppdq, struct UpdateLineMsg, (POINTER)&msg, n ); n++ )
		//{
		//	if( ( msg.iType == iType->idx ) && ( msg.iWorld == iWorld ) )
		//		break;
		//}
		if( iType > 10000000 )
			DebugBreak();
		//if( !success )
		{
			msg.iWorld = iWorld;
			msg.iThing = iType;
			msg.pThing = pType;
			EnqueData( ppdq, &msg );
		}
	}
}


void MarkLineChanged( INDEX iWorld, INDEX iLine )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEG pLine = GetSetMember( FLATLAND_MYLINESEG
		, &world->lines
		, iLine );	
	if( !pLine->flags.bUpdated )
	{
		pLine->flags.bUpdated = 1;
		MarkThingChanged( &world->UpdatedLines, iWorld, iLine, pLine );
	}
}

void SendLinesChanged( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	struct UpdateThingMsg line;
	while( DequeData( &world->UpdatedLines, (POINTER)&line ) )
	{
		//_32 result;
		if( line.iThing > 1000 )
			DebugBreak();
		// expect to wait until the server
		// accepts the change.
		TransactServerMultiMessage( MSG_ID( UpdateLine ), 3
			, NULL /*&result*/, NULL, NULL
			, &line.iWorld, sizeof( INDEX )
			, &line.iThing, sizeof( INDEX )
			, line.pThing, sizeof( FLATLAND_MYLINESEG ) 
			);
		((PFLATLAND_MYLINESEG)line.pThing)->flags.bUpdated = 0;
	}
}

void SendLineChanged( INDEX iWorld, INDEX iWall, INDEX iLine, LINESEG *lineseg, LOGICAL no_update_mate, LOGICAL lock_mating_slopes )
{
		//_32 result;
		// expect to wait until the server
		// accepts the change.
		lprintf( WIDE("Send.") );
		TransactServerMultiMessage( MSG_ID( SendLineChanged ), 6
			, NULL /*&result*/, NULL, NULL
			, &iWorld, sizeof( INDEX )
			, &iWall, sizeof( INDEX )
			, &iLine, sizeof( INDEX )
			, &no_update_mate, sizeof( _32 )
			, &lock_mating_slopes, sizeof( _32 )
			, lineseg, sizeof( LINESEG ) 
										  );
		lprintf( WIDE("Sent.") );

}

//----------------------------------------------------------------------------

#define UpdateResult(r) {				 \
	if( !r ) Log1( WIDE("Failing update at : %d"), __LINE__ ); \
	pWall->flags.bUpdating = FALSE;  \
	/*UpdateLevels--;*/				  \
	/*if( !UpdateLevels ) UpdateSpace( UpdateCount, UpdateList );*/\
	return r; }

int UpdateMatingLines( INDEX iWorld, INDEX iWall
					  , int bLockSlope, int bErrorOK )
					  // return TRUE to allow the update
					  // return FALSE to invalidate the update... too many 
					  // intersections... or lines that get deleted...
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	INDEX tmp = bLockSlope;
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(UpdateMatingLines), 1
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( INDEX )
								, &iWall, sizeof( INDEX )
								, &bLockSlope, sizeof( int )
								, &bErrorOK, sizeof( int )
								)
		&& ( ResultID == (MSG_ID(UpdateMatingLines)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return FALSE;
#if LIBRARY_OR_SERVER
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL pWall = GetSetMember( WALL, &world->walls, iWall );
	PFLATLAND_MYLINESEG pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWall->iLine );
	int AdjustPass = 0;
	PWALL pStart, pEnd;
	_POINT ptStart, ptEnd;
	FLATLAND_MYLINESEG lsStartSave, lsEndSave;
	bErrorOK = TRUE; // DUH! 
	// this wall moved, so for all mating lines, update this beast.
	if( !pWall || pWall->flags.bUpdating )
		return TRUE; // this is okay - we're just looping backwards.

	pWall->flags.bUpdating = TRUE;
	Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
	if( pWall->iWallInto != INVALID_INDEX )
	{
		if( !UpdateMatingLines( iWorld, pWall->iWallInto, bLockSlope, bErrorOK ) )
			UpdateResult( FALSE );
	}
	Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );

	if( CountWalls( iWorld, iWall ) < 4 )
		bErrorOK = TRUE;

	Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );

	if( !bLockSlope )
	{
		PFLATLAND_MYLINESEG plsStart, plsEnd;
		addscaled( ptStart, pls->r.o
			, pls->r.n
			, pls->dFrom );
		addscaled( ptEnd, pls->r.o
			, pls->r.n
			, pls->dTo );
Readjust:   
		pStart = GetSetMember( WALL, &world->walls, pWall->iWallStart );
		pEnd = GetSetMember( WALL, &world->walls, pWall->iWallEnd );
		lsStartSave = *(plsStart=GetSetMember( FLATLAND_MYLINESEG, &world->lines, pStart->iLine ));
		lsEndSave = *(plsEnd=GetSetMember( FLATLAND_MYLINESEG, &world->lines, pEnd->iLine ));
		// check opposite any other walls other than those 
		// directly related for... intersection with this line
		// which I intended to move.
		if( !bErrorOK )
		{
			PWALL pCur = pWall;
			PFLATLAND_MYLINESEG plsCur;
			int priorend = TRUE;
			RCOORD T1, T2;
			do
			{
				plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
				if( pCur != pStart &&
					pCur != pWall &&
					pCur != pEnd )
				{
					if( FindIntersectionTime( &T1, pls->r.n, pls->r.o
						, &T2, plsCur->r.n, plsCur->r.o ) )
					{
						if( T1 >= pls->dFrom && T1 <= pls->dTo &&
							T2 >= plsCur->dFrom && T2 <= plsCur->dTo )
							UpdateResult( FALSE );
					}
				}
				if( priorend )
				{
					priorend = pCur->flags.wall_start_end;
					pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
				}
				else
				{
					priorend = pCur->flags.wall_end_end;
					pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
				}
			}while( pCur != pWall );
		}

		Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
		if( pWall->flags.wall_start_end )
		{
			_POINT ptOther;
			// compute start point...
			if( !pStart->flags.bUpdating )
			{
				// compute original end of this line
				addscaled( ptOther, plsStart->r.o
					, plsStart->r.n
					, plsStart->dTo );
				// if original end != new end 
				if( !Near( ptOther, ptStart ) ) 
				{
					INDEX iOtherWall = pStart->iWallStart;
					PWALL pOtherWall = GetSetMember( WALL, &world->walls, iOtherWall );
					PFLATLAND_MYLINESEG plsOther = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pOtherWall->iLine );
					/*
					Log7( WIDE("Line Different %d: <%g,%g,%g> <%g,%g,%g> ")
					, __LINE__
					, ptOther[0]
					, ptOther[1]
					, ptOther[2]
					, ptStart[0]
					, ptStart[1]
					, ptStart[2] );
					Log7( WIDE("Line Different %d: <%08x,%08x,%08x> <%08x,%08x,%08x> ")
					, __LINE__
					, RCOORDBITS(ptOther[0])
					, RCOORDBITS(ptOther[1])
					, RCOORDBITS(ptOther[2])
					, RCOORDBITS(ptStart[0])
					, RCOORDBITS(ptStart[1])
					, RCOORDBITS(ptStart[2]) );
					*/
					if( pStart->flags.wall_start_end )
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dTo );
					else
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dFrom );

					plsStart->dFrom = 0;
					plsStart->dTo = 1;
					SetPoint( plsStart->r.o, ptOther );
					sub( ptOther, ptStart, ptOther );
					SetPoint( plsStart->r.n, ptOther );
					//DrawLineSeg( plsStart, Color( 0, 0, 255 ) );
					if( pStart->iWallInto != INVALID_INDEX )
					{
						pStart->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pStart->iWallInto, bLockSlope, bErrorOK ) )
						{
							pStart->flags.bUpdating = FALSE;
							*plsStart = lsStartSave;
							UpdateResult( FALSE );
						}
						pStart->flags.bUpdating = FALSE;
					}
				}
			}
		}
		else  // ( !pWall->flags.wall_start_end )
		{
			_POINT ptOther;
			// compute end point...
			if( !pStart->flags.bUpdating )
			{
				// compute original end of this line
				addscaled( ptOther, plsStart->r.o
					, plsStart->r.n
					, plsStart->dFrom );
				// if original end != new end 
				if( !Near( ptOther, ptStart ) )
				{
					PWALL pOtherWall = GetSetMember( WALL, &world->walls, pStart->iWallEnd );
					PFLATLAND_MYLINESEG plsOther = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pOtherWall->iLine );
					/*
					Log7( WIDE("Line Different %d: <%g,%g,%g> <%g,%g,%g> ")
					, __LINE__
					, ptOther[0]
					, ptOther[1]
					, ptOther[2]
					, ptStart[0]
					, ptStart[1]
					, ptStart[2] );
					Log7( WIDE("Line Different %d: <%08x,%08x,%08x> <%08x,%08x,%08x> ")
					, __LINE__
					, RCOORDBITS(ptOther[0])
					, RCOORDBITS(ptOther[1])
					, RCOORDBITS(ptOther[2])
					, RCOORDBITS(ptStart[0])
					, RCOORDBITS(ptStart[1])
					, RCOORDBITS(ptStart[2]) );
					*/
					if( pStart->flags.wall_end_end )
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dTo );
					else
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dFrom );
					plsStart->dFrom = -1;
					plsStart->dTo = 0;
					SetPoint( plsStart->r.o, ptOther );
					sub( ptOther, ptOther, ptStart );
					SetPoint( plsStart->r.n, ptOther );
					MarkLineChanged( iWorld, pStart->iLine );
					//DrawLineSeg( plsStart, Color( 0, 0, 255 ) );
					if( pStart->iWallInto != INVALID_INDEX )
					{
						pStart->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pStart->iWallInto, bLockSlope, bErrorOK ) )
						{
							pStart->flags.bUpdating = FALSE;
							*plsStart = lsStartSave;
							UpdateResult( FALSE );
						}
						pStart->flags.bUpdating = FALSE;
					}
				}
				else
				{
					//Log( WIDE("Points were the same?!") );
				}
			}
		}
		Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
		if( pWall->flags.wall_end_end )
		{
			_POINT ptOther;
			if( !pEnd->flags.bUpdating )
			{
				// compute original end of this line
				addscaled( ptOther, plsEnd->r.o
					, plsEnd->r.n
					, plsEnd->dTo );
				// if original end != new end 
				if( !Near( ptOther, ptEnd ) )
				{
					PWALL pOtherWall = GetSetMember( WALL, &world->walls, pEnd->iWallStart );
					PFLATLAND_MYLINESEG plsOther = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pOtherWall->iLine );
					/*
					Log7( WIDE("Line Different %d: <%g,%g,%g> <%g,%g,%g> ")
					, __LINE__
					, ptOther[0]
					, ptOther[1]
					, ptOther[2]
					, ptEnd[0]
					, ptEnd[1]
					, ptEnd[2] );
					Log7( WIDE("Line Different %d: <%08x,%08x,%08x> <%08x,%08x,%08x> ")
					, __LINE__
					, RCOORDBITS(ptOther[0])
					, RCOORDBITS(ptOther[1])
					, RCOORDBITS(ptOther[2])
					, RCOORDBITS(ptEnd[0])
					, RCOORDBITS(ptEnd[1])
					, RCOORDBITS(ptEnd[2]) );
					*/
					if( pEnd->flags.wall_start_end )
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dTo );
					else
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dFrom );
					plsEnd->dFrom = 0;
					plsEnd->dTo = 1;
					SetPoint( plsEnd->r.o, ptOther );
					sub( ptOther, ptEnd, ptOther );
					SetPoint( plsEnd->r.n, ptOther );
					MarkLineChanged( iWorld, pEnd->iLine );
					//DrawLineSeg( plsEnd, Color( 0, 0, 255 ) );
					if( pEnd->iWallInto != INVALID_INDEX )
					{
						pEnd->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pEnd->iWallInto, bLockSlope, bErrorOK ) )
						{
							pEnd->flags.bUpdating = FALSE;
							*plsStart = lsStartSave;
							*plsEnd = lsEndSave;
							UpdateResult( FALSE );
						}   
						pEnd->flags.bUpdating = FALSE;
					}
				}
			}
		}
		else	//(!pWall->flags.wall_end_end)
		{
			_POINT ptOther;
			// compute end point
			if( !pEnd->flags.bUpdating )
			{

				// compute original end of this line
				addscaled( ptOther, plsEnd->r.o
					, plsEnd->r.n
					, plsEnd->dFrom );
				// if original end != new end 
				if( !Near( ptOther, ptEnd ) )
				{
					PWALL pOtherWall = GetSetMember( WALL, &world->walls, pEnd->iWallEnd );
					PFLATLAND_MYLINESEG plsOther = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pOtherWall->iLine );
					/*
					Log7( WIDE("Line Different %d: <%g,%g,%g> <%g,%g,%g> ")
					, __LINE__
					, ptOther[0]
					, ptOther[1]
					, ptOther[2]
					, ptEnd[0]
					, ptEnd[1]
					, ptEnd[2] );
					Log7( WIDE("Line Different %d: <%08x,%08x,%08x> <%08x,%08x,%08x> ")
					, __LINE__
					, RCOORDBITS(ptOther[0])
					, RCOORDBITS(ptOther[1])
					, RCOORDBITS(ptOther[2])
					, RCOORDBITS(ptEnd[0])
					, RCOORDBITS(ptEnd[1])
					, RCOORDBITS(ptEnd[2]) );
					*/
					if( pEnd->flags.wall_end_end )
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dTo );
					else
						addscaled( ptOther, plsOther->r.o
						, plsOther->r.n
						, plsOther->dFrom );
					plsEnd->dFrom = -1;
					plsEnd->dTo = 0;
					SetPoint( plsEnd->r.o, ptOther );
					sub( ptOther, ptOther, ptEnd );
					SetPoint( plsEnd->r.n, ptOther );
					MarkLineChanged( iWorld, pEnd->iLine );
					//DrawLineSeg( plsEnd, Color( 0, 0, 255 ) );
					if( pEnd->iWallInto != INVALID_INDEX )
					{
						pEnd->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pEnd->iWallInto, bLockSlope, bErrorOK ) )
						{
							pEnd->flags.bUpdating = FALSE;
							*plsStart = lsStartSave;
							*plsEnd = lsEndSave;
							UpdateResult( FALSE );
						}
						pEnd->flags.bUpdating = FALSE;
					}
				}
			}
		}
		// check to see if we crossed the mating lines...
		// if so - uncross them.
		Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
		if( !bErrorOK )
		{
			RCOORD t1, t2;
			if( FindIntersectionTime( &t1, plsStart->r.n, plsStart->r.o
				, &t2, plsEnd->r.n, plsEnd->r.o ) &&
				t1 >= plsStart->dFrom && t1 <= plsStart->dTo && 
				t2 >= plsEnd->dFrom && t2 <= plsEnd->dTo  )
			{
				int tmp;
				if( AdjustPass++ )
				{
					Log( WIDE("We're dying!") );
					UpdateResult( FALSE );
				}
				tmp = pWall->flags.wall_start_end;
				pWall->flags.wall_start_end = pWall->flags.wall_end_end;
				pWall->flags.wall_end_end = tmp;

				{
					INDEX i = pWall->iWallEnd;
					pWall->iWallEnd = pWall->iWallStart;
					pWall->iWallStart = i;
				}

				if( pEnd->iWallStart == iWall )
					pEnd->flags.wall_start_end = !pEnd->flags.wall_start_end;
				else
					pEnd->flags.wall_end_end = !pEnd->flags.wall_end_end;

				if( pStart->iWallStart == iWall )
					pStart->flags.wall_start_end = !pStart->flags.wall_start_end;
				else
					pStart->flags.wall_end_end = !pStart->flags.wall_end_end;
				goto Readjust;
			}
			// need to find some way to limit... what happens when
			// lines become concave... how do I detect that simply?
			//
			if( FindIntersectionTime( &t1, plsStart->r.n, plsStart->r.o
				, &t2, plsEnd->r.n, plsEnd->r.o ) &&
				( ( t2 >= plsEnd->dFrom && t2 <= plsEnd->dTo ) || 
				( t1 >= plsStart->dFrom && t1 <= plsStart->dTo ) )  )
			{
				// if either segment intersects the other during itself...
				// then this is an invalid update... 
				*plsStart = lsStartSave;
				*plsEnd = lsEndSave;
				UpdateResult( FALSE );
			}
			// this is still insufficient... and should continue to check
			// remaining segments...
		}
		Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
#ifndef WORLD_SERVICE
		ComputeSectorPointList( iWorld, pWall->iSector, NULL );
#endif
		ComputeSectorOrigin( iWorld, pWall->iSector );
	}
	else
	{
		PFLATLAND_MYLINESEG plsStart, plsEnd;
		// handle updates but keep constant slope on mating lines....
		// check intersect of current with every other line in the
		// sector - prevents intersection of adjoining
		pStart = GetSetMember( WALL, &world->walls, pWall->iWallStart );
		pEnd = GetSetMember( WALL, &world->walls, pWall->iWallEnd );
		plsStart = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pStart->iLine );
		plsEnd = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pEnd->iLine );
		{
			PWALL pCur = pWall;
			PFLATLAND_MYLINESEG plsCur;
			int priorend = TRUE;
			RCOORD T1, T2;
			do
			{
				plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
				if( pCur != pStart &&
					pCur != pWall &&
					pCur != pEnd )
				{
					if( FindIntersectionTime( &T1, pls->r.n, pls->r.o
						, &T2, plsCur->r.n, plsCur->r.o ) )
					{
						if( T1 >= pls->dFrom && T1 <= pls->dTo &&
							T2 >= plsCur->dFrom && T2 <= plsCur->dTo )
							UpdateResult( FALSE );
					}
				}
				if( priorend )
				{
					priorend = pCur->flags.wall_start_end;
					pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
				}
				else
				{
					priorend = pCur->flags.wall_end_end;
					pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
				}
			}while( pCur != pWall );
		}

		{
			RCOORD T1, T2;
			// sigh - moved line... update end factors
			// of intersecting lines...
			if( FindIntersectionTime( &T1, pls->r.n, pls->r.o
				, &T2, plsStart->r.n, plsStart->r.o ) )
			{
				pls->dFrom = T1;
				if( pWall->flags.wall_start_end )
					plsStart->dTo = T2;
				else
					plsStart->dFrom = T2;
				UpdateMatingLines( iWorld, pWall->iWallStart, FALSE, TRUE );
			}
			else
			{
				Log2( WIDE("Failed to intersect wall with iWallStart %s(%d)"), __FILE__, __LINE__ );
				UpdateResult( FALSE );
			}

			if( FindIntersectionTime( &T1, pls->r.n, pls->r.o
				, &T2, plsEnd->r.n, plsEnd->r.o ) )
			{
				pls->dTo = T1;
				if( pWall->flags.wall_end_end )
					plsEnd->dTo = T2;
				else
					plsEnd->dFrom = T2;
				UpdateMatingLines( iWorld, pWall->iWallEnd, FALSE, TRUE );
			}
			else
			{
				Log2( WIDE("Failed to intersect wall with iWallStart %s(%d)"), __FILE__, __LINE__ );
				UpdateResult( FALSE );
			}

		}
	}
	UpdateResult( TRUE );
#endif
}

//----------------------------------------------------------------------------
// for now this is unused - but it is possible that 
// several walls can link to this one...and not match from count...
int UnlinkWall( INDEX iWorld, INDEX iWall, INDEX findwall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL pWall = GetSetMember( WALL, &world->walls, iWall );

	if( pWall->iWallInto == findwall )
	{
		pWall->iWallInto = INVALID_INDEX;
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------


int DestroyWall( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	//int shared;
	//shared = ForAllWalls( wall->sector->world->walls, UnlinkWall, wall );
	//ValidateSpaceTree( world.spacetree );
	if( wall->iWallInto != INVALID_INDEX )
	{
		PWALL pTmpWall = GetUsedSetMember( WALL, &world->walls, wall->iWallInto );
		if( pTmpWall && pTmpWall->iWallInto == iWall )
			pTmpWall->iWallInto = INVALID_INDEX;
	}
	

	if( wall->iWallStart != INVALID_INDEX )
	{
		PWALL pTmpWall = GetSetMember( WALL, &world->walls, wall->iWallStart );
		if( wall->flags.wall_start_end )
			pTmpWall->iWallEnd = INVALID_INDEX;
		else
			pTmpWall->iWallStart = INVALID_INDEX;
	}
	if( wall->iWallEnd != INVALID_INDEX )
	{
		PWALL pTmpWall = GetSetMember( WALL, &world->walls, wall->iWallEnd );
		if( wall->flags.wall_end_end )
			pTmpWall->iWallEnd = INVALID_INDEX;
		else
			pTmpWall->iWallStart = INVALID_INDEX;
	}
	SetLine( wall->iLine, INVALID_INDEX );
	DeleteIWall( world->walls, iWall );
	return 0;
}

//----------------------------------------------------------------------------

int DestroySector( INDEX iWorld, INDEX iSector )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR ps = GetSetMember( SECTOR, &world->sectors, iSector );
	/*
	PWALL pCur, pNext;
	INDEX iCur, iNext;
	int priorend = TRUE;
	//Log( WIDE("Destroy Sector(")STRSYM(__LINE__)WIDE(")"));
	//DeleteSpaceNode( RemoveSpaceNode( ps->spacenode ) );
	//Log( WIDE("Destroy Sector(")STRSYM(__LINE__)WIDE(")"));
	//ValidateSpaceTree( world.spacetree );
	//ps->spacenode = NULL;
	pCur = GetSetMember( WALL, &world->walls, iCur = ps->iWall );
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
		DestroyWall( iWorld, iCur );
		iCur = iNext;
		pCur = pNext;
	}while( pCur );
	*/
	//ps->iWall = INVALID_INDEX;
	Release( ps->pointlist );
	// can't clear sector->world... ahh well...
	DeleteSector( world->sectors, ps );
	return 0;
}

//----------------------------------------------------------------------------

void DeleteSectors( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	//PSPACENODE node;
	//Log( WIDE("Deleting Sectors...") );
	//while( node = FindDeepestNode( world.spacetree, 0 ) )
	//{
	//Log1( WIDE("%08x arg!"), node );
	//  DestroySector( (PSECTOR)GetNodeData( node ) );
	//}
	//ForAllSectors( *ppsectors, DestroySector, 0 );

	// for all sectors - dont' count on space tree for knowing where things are...
	// the tree scheduling alogrithm is a higher level construct is really
	// although needing positive hooks bi-directionally, has no bearing with
	// internal logic which knows nothing about the needs of display...
	// it's all a vast tree of nearly connected nodes that needs to be
	// traversed as such internally anyhow.....  there is no 'beyond here'
	DeleteSet( (struct genericset_tag**)&world->sectors );
}

//----------------------------------------------------------------------------

int MergeWalls( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL CurWall = GetSetMember( WALL, &world->walls, iCurWall );
	PWALL MarkedWall = GetSetMember( WALL, &world->walls, iMarkedWall );
	PFLATLAND_MYLINESEG plsCur;
	PFLATLAND_MYLINESEG plsMark;
	RCOORD c;
	if( !CurWall || !MarkedWall
		|| MarkedWall == CurWall
		|| MarkedWall->iWallInto != INVALID_INDEX 
		|| CurWall->iWallInto != INVALID_INDEX )
		return FALSE;
	plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, CurWall->iLine );
	plsMark = GetSetMember( FLATLAND_MYLINESEG, &world->lines, MarkedWall->iLine );
	c = CosAngle( plsCur->r.n, plsMark->r.n );
	if( fabs(c) < 0.5 ) // if lines are not parrallel enough...
		return FALSE;

	SetLine( MarkedWall->iLine, CurWall->iLine );

	MarkedWall->iWallInto = iCurWall;
	CurWall->iWallInto = iMarkedWall;

	MarkedWall->flags.detached = FALSE;
	CurWall->flags.detached = FALSE;

	// since the marked wall is the one that changes to the
	// curwall - I suppose we should update him instead of
	// curwall - which can result in mating lines to marked
	// not being updated - of course this is an error, somehow
	// but I know not why.  Now I know why... it seems that the 
	// line I replaced is opposite to the one I expected...
	// therefore the flags on this wall are backwards - or rather
	// the flags on the mating lines are backwards.

	// if the line is heading in the opposite direction...
	if( c < 0 )
	{
		INDEX tmp;
		int tflag;
		PWALL pTmpWall;
		tmp = MarkedWall->iWallStart;
		MarkedWall->iWallStart = MarkedWall->iWallEnd;
		MarkedWall->iWallEnd = tmp;
		tflag = MarkedWall->flags.wall_start_end;
		MarkedWall->flags.wall_start_end = MarkedWall->flags.wall_end_end;
		MarkedWall->flags.wall_end_end = tflag;

		pTmpWall = GetSetMember( WALL, &world->walls, MarkedWall->iWallStart );
		if( pTmpWall->iWallStart == iMarkedWall )
			pTmpWall->flags.wall_start_end ^= 1;
		else
			pTmpWall->flags.wall_end_end ^= 1;

		pTmpWall = GetSetMember( WALL, &world->walls, MarkedWall->iWallEnd );
		if( pTmpWall->iWallStart == iMarkedWall )
			pTmpWall->flags.wall_start_end ^= 1;
		else
			pTmpWall->flags.wall_end_end ^= 1;
	}
	UpdateMatingLines( iWorld, iCurWall, FALSE, FALSE );
	return TRUE;
}

//----------------------------------------------------------------------------

void SplitWall( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PWALL other;
	if( !wall )
		return;
	other = GetSetMember( WALL, &world->walls, wall->iWallInto );
	if( !other )
		return;

	SetLine( other->iLine, DuplicateLine( iWorld, wall->iLine ) );

	other->iWallInto = INVALID_INDEX;
	wall->iWallInto = INVALID_INDEX;
}

//----------------------------------------------------------------------------

void SplitNearWalls( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	// this could very well be... a split of other near walls
	// which would hmm.. how to track down these elusive near walls...

	PWALL start;
	if( ( start = GetSetMember( WALL, &world->walls, wall->iWallStart ) ) &&
		start->iWallInto != INVALID_INDEX )
	{
		INDEX split;
		if( wall->flags.wall_start_end )
		{
			if( start->iWallEnd != iWall )
				Log1( WIDE("We're confused about linkings... %d"), __LINE__ );
			else
			{
				PWALL into;
				split = (GetSetMember( WALL, &world->walls, start->iWallInto ))->iWallEnd;
				into = GetSetMember( WALL, &world->walls, split );
				if( into && into->iWallInto != INVALID_INDEX )
					SplitWall( iWorld, split );
			}
		}
		else
		{
			if( start->iWallStart != iWall )
				Log1( WIDE("We're confused about linkings... %d"), __LINE__ );
			else
			{
				PWALL into;
				split = (GetSetMember( WALL, &world->walls, start->iWallInto ))->iWallStart;
				into = GetSetMember( WALL, &world->walls, split );
				if( into && into->iWallInto != INVALID_INDEX )
					SplitWall( iWorld, split );
			}
		}
	}
	if( ( start = GetSetMember( WALL, &world->walls, wall->iWallEnd ) ) &&
		start->iWallInto != INVALID_INDEX )
	{
		INDEX split;
		if( wall->flags.wall_end_end )
		{
			if( start->iWallEnd != iWall )
				Log1( WIDE("We're confused about linkings... %d"), __LINE__ );
			else
			{
				PWALL into;
				split = (GetSetMember( WALL, &world->walls, start->iWallInto ))->iWallEnd;
				into = GetSetMember( WALL, &world->walls, split );
				if( into && into->iWallInto != INVALID_INDEX )
					SplitWall( iWorld, split );
			}
		}
		else
		{
			if( start->iWallStart != iWall )
				Log1( WIDE("We're confused about linkings... %d"), __LINE__ );
			else
			{
				PWALL into;
				split = (GetSetMember( WALL, &world->walls, start->iWallInto ))->iWallStart;
				into = GetSetMember( WALL, &world->walls, split );
				if( into && into->iWallInto != INVALID_INDEX )
					SplitWall( iWorld, split );
			}
		}
	}
}

//----------------------------------------------------------------------------
#if 0
INDEX InsertConnectedSector( INDEX iWorld, INDEX iWall, RCOORD offset )
{
	INDEX ResultID;
	INDEX Result[1];
	size_t ResultLen = sizeof( INDEX );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(AddConnectedSector), 3
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, &iWall, sizeof( iWall )
								, &offset, sizeof( RCOORD )
								)
		&& ( ResultID == (MSG_ID(InsertConnectedSector)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
}
#endif
//----------------------------------------------------------------------------

INDEX AddConnectedSector( INDEX iWorld, INDEX iWall, RCOORD offset )
{
	MSGIDTYPE ResultID;
	INDEX Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(AddConnectedSector), 3
								, &ResultID, (_32*)Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, &iWall, sizeof( iWall )
								, &offset, sizeof( RCOORD )
								)
		&& ( ResultID == (MSG_ID(AddConnectedSector)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX FlatlandPointWithin( INDEX iWorld, int nSectors, INDEX *pSectors, P_POINT p )
{
	// this routine is perhaps a bit excessive - if one set of 
	// bounding lines is found ( break; ) we could probably return TRUE
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	int n;
	for( n = 0; n < nSectors; n++ )
	{
		PWALL pStart, pCur;
		_POINT norm;
		PFLATLAND_MYLINESEG plsCur;
		int even = 0, odd = 0;
		int priorend = TRUE;
		INDEX iSector = pSectors[n];
		PSECTOR pSector = GetSetMember( SECTOR, &world->sectors, iSector );
		pCur = pStart = GetSetMember( WALL, &world->walls, pSector->iWall );
		//lprintf( WIDE("------ SECTOR %d --------------"), n );
		if( Near( p, pSector->r.o ) )
		{
			//lprintf( WIDE("...") );
			return iSector;
		}
		sub( norm, p, pSector->r.o );
		do
		{
			RCOORD T1, T2;
			plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
			if( FindIntersectionTime( &T1, norm, pSector->r.o
				, &T2, plsCur->r.n, plsCur->r.o ) )
			{
				// if T1 < 1.0 or T1 > -1.0 then the intersection is not
				// beyond the end of this line...  which I guess if the origin is skewed
				// then one end or another may require success at more than the distance
				// from the origin to the line...
				//Log4( WIDE("Intersected at %g %g %g -> %g"), T1, T2,
				//	  plsCur->dFrom, plsCur->dTo );
				if( ( T2 >= plsCur->dFrom && T2 <= plsCur->dTo ) )
				{
					if( T1 > 1.0 )
						even = 1;
					// skew T1 by oigin
					else if( T1 < 0.0 ) // less than zero - that's the point of the sector origin...
						odd = 1;
					if( even && odd )
					{
						//Log( WIDE("Two successes is truth...") );
						return iSector;
					}
					//Log( WIDE("continuing truth (sector in list)") );
					//return iSector;
				}
			}
			//lprintf( WIDE("cur is %p start %p"), pCur, pStart );
			if( priorend )
			{
				priorend = pCur->flags.wall_start_end;
				pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
			}
			else
			{
				priorend = pCur->flags.wall_end_end;
				pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
			}
			//lprintf( WIDE("new cur is %p"), pCur );
		}while( pCur != pStart );
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX FlatlandPointWithinLoopSingle( INDEX iSector, PTRSZVAL psv )
{
	struct data_tag {
		INDEX iWorld;
		P_POINT p;
	} *data =(struct data_tag*) psv;
	if( iSector == INVALID_INDEX )
		return 0;
	return FlatlandPointWithin( data->iWorld, 1, &iSector, data->p ) + 1;
}

//----------------------------------------------------------------------------

INDEX FlatlandPointWithinSingle( INDEX iWorld, INDEX iSector, P_POINT p )
{
	if( iSector == INVALID_INDEX )
		return INVALID_INDEX;
	return FlatlandPointWithin( iWorld, 1, &iSector, p );
}

//----------------------------------------------------------------------------

void ComputeSectorOrigin( INDEX iWorld, INDEX iSector )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector  = GetSetMember( SECTOR, &world->sectors, iSector );
	_POINT temp, pt;
	PFLATLAND_MYLINESEG plsCur;
	int npoints;
	PWALL pStart, pCur;
	int priorend;

	if( !sector )
		return;

	npoints = 0;
	priorend = TRUE;
	if( pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall ) )
	{
		SetPoint( temp, VectorConst_0 );
		do
		{
			plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
			if( priorend )
				addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dTo );
			else
				addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dFrom );
			add( temp, temp, pt );
			npoints++;
			if( priorend )
			{
				priorend = pCur->flags.wall_start_end;
				pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
			}
			else
			{
				priorend = pCur->flags.wall_end_end;
				pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
			}
		}while( pCur && ( pCur != pStart ) );
	}
	scale( temp, temp, 1.0 / npoints );
	SetPoint( sector->r.o, temp );
}

void GetSectorOrigin( INDEX iWorld, INDEX iSector, P_POINT o )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );
	SetPoint( o, sector->r.o );
}

//----------------------------------------------------------------------------

void ComputeSectorSetOrigin( INDEX iWorld, int nSectors, INDEX *sectors, P_POINT origin )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR pSector;
	_POINT temp;
	int n;
	if( !origin ) // don't even bother if there's no place to output...
		return;
	SetPoint( temp, VectorConst_0 );
	for( n = 0; n < nSectors; n++ )
	{
		pSector = GetSetMember( SECTOR, &world->sectors, sectors[n] );
		add( temp, temp, pSector->r.o );
	}
	scale( temp, temp, 1.0 / (RCOORD)nSectors );
	SetPoint( origin, temp );
}

//----------------------------------------------------------------------------

void ComputeWallSetOrigin( INDEX iWorld, int nWalls, INDEX *walls, P_POINT origin )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	_POINT temp;
	PWALL pWall;
	PFLATLAND_MYLINESEG pls;
	int n;
	if( !origin ) // don't even bother if there's no place to output...
		return;
	SetPoint( temp, VectorConst_0 );
	for( n = 0; n < nWalls; n++ )
	{
		pWall = GetSetMember( WALL, &world->walls, walls[n] );
		pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWall->iLine );
		add( temp, temp, pls->r.o );
	}
	scale( temp, temp, 1.0 / (RCOORD)nWalls );
	SetPoint( origin, temp );
}

//----------------------------------------------------------------------------

_POINT* CheckPointOrder( PC_POINT normal, _POINT *plist, int npoints )
{
	_POINT v1, v2, cross;
	RCOORD cosine;
	sub( v1, plist[0], plist[1] );
	sub( v2, plist[1], plist[2] );
	crossproduct( cross, v1, v2 );
	cosine = CosAngle( cross, normal );
	// the corss product of any point list better be 
	// parallel or opposing to normal since all points are 
	// considered to be on the plane defined by the normal...
	if( cosine > 0 )
	{
		_POINT *newlist;
		int n;
		newlist = (_POINT*)Allocate( sizeof( _POINT ) * npoints );
		for( n = 0; n < npoints; n++ )
		{
			SetPoint( newlist[n], plist[(npoints-1)-n] );
		}
		Release( plist );
		return newlist;
	}
	else
		return plist;
}

//----------------------------------------------------------------------------

_POINT* ComputeSectorPointList( INDEX iWorld, INDEX iSector, int *pnpoints )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector  = GetSetMember( SECTOR, &world->sectors, iSector );
	PFLATLAND_MYLINESEG plsCur;
	int npoints;
	PWALL pStart, pCur;
	int priorend = TRUE;
	_POINT pt;
	_POINT *ptlist;
	if( !sector )
	{
		if( pnpoints )
			*pnpoints = 0;
		return NULL;
	}

	npoints = 0;

	// the wall definitions for the sector might not be in yet
	// though the sector has a wall.
	pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
	if( pCur ) do
	{
		//lprintf( WIDE("first wall %p %d %d"), pCur, pCur->iWallStart, pCur->iWallEnd );
		plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
		npoints++;
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
			//lprintf( WIDE("next wall %p"), pCur );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
			//lprintf( WIDE("next wall %p"), pCur );
		}
		if( !pCur )
		{
			lprintf( WIDE("Fell off the side of the list no more current...") );
			return NULL;
		}
	}while( pCur != pStart );

	ptlist = (_POINT*)Allocate( sizeof( _POINT ) * npoints );

	npoints = 0;
	// start again...
	if( pCur = pStart ) do
	{
		plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
		if( priorend )
			addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dTo );
		else
			addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dFrom );
		SetPoint( ptlist[npoints], pt );
		npoints++;
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( pCur != pStart );

	// consider resolving list into 'correct ordered' list
	ptlist = CheckPointOrder( VectorConst_Z, ptlist, npoints );

	if( sector->pointlist )
		Release( sector->pointlist );
	sector->pointlist = ptlist;
	sector->npoints = npoints;
	if( !ptlist && npoints )
		DebugBreak();

	if( pnpoints )
		*pnpoints = npoints;		
	return ptlist;
}

LOGICAL IsSectorClockwise( INDEX iWorld, INDEX iSector )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector  = GetUsedSetMember( SECTOR, &world->sectors, iSector );
	PFLATLAND_MYLINESEG plsCur;
	PWALL pStart, pCur;
	int priorend = TRUE;
	int normal_count;
	_POINT normals[2];
	_POINT pt;

	pCur = pStart = GetUsedSetMember( WALL, &world->walls, sector->iWall );

	// start again...
	pCur = pStart;
	normal_count = 0;
	do
	{
		plsCur = GetUsedSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
      SetPoint( pt, plsCur->r.n );
		if( priorend )
         //addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dTo );
         ;
		else
         Invert( pt );
		 //  addscaled( pt, plsCur->r.o, plsCur->r.n, plsCur->dFrom );
      // might have to invert n for purposes of this...
		SetPoint( normals[normal_count++], pt );
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( normal_count < 2 );
	{
		_POINT cross;
		crossproduct( cross, normals[0], normals[1] );
		if( dotproduct( cross, VectorConst_Y ) > 0 )
			return TRUE;
      return FALSE;
	}
}

//----------------------------------------------------------------------------

void GetSectorPoints( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector  = GetUsedSetMember( SECTOR, &world->sectors, iSector );
	*list = sector->pointlist;
	*npoints = sector->npoints;
}

//----------------------------------------------------------------------------

int MoveSectors( INDEX iWorld, int nSectors,INDEX *pSectors, P_POINT del )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = 0;
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(MoveSectors), 3
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, del, sizeof( _POINT )
								, pSectors, sizeof( INDEX ) * nSectors
								)
		&& ( ResultID == (MSG_ID(MoveSectors)|SERVER_SUCCESS)))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------


int MoveWalls( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	int tmp = bLockSlope;
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(MoveWalls), 5
											  , &ResultID, Result, &ResultLen
											  , &iWorld, sizeof( iWorld )
											  , &nWalls, sizeof( nWalls )
											  , &tmp, sizeof( tmp )
											  , del, sizeof( _POINT )
											  , WallList, sizeof( INDEX ) * nWalls
											  )
		&& ( ResultID == (MSG_ID(MoveWalls)|SERVER_SUCCESS)))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
int RemoveWall( INDEX iWorld, INDEX iWall )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(RemoveWall), 2
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, &iWall, sizeof( iWall )
								)
		&& ( ResultID == (MSG_ID(RemoveWall)|SERVER_SUCCESS)))
	{
		return TRUE;
	}
	return FALSE;
}
void BreakWall( INDEX iWorld, INDEX iWall )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(BreakWall), 2
								, &ResultID, Result, &ResultLen 
								, &iWorld, sizeof( iWorld )
								, &iWall, sizeof( iWall )
								)
		&& ( ResultID == (MSG_ID(BreakWall)|SERVER_SUCCESS)))
	{
	}
}

int WallInSector( INDEX iWorld, INDEX iSector, INDEX iWall )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	PSECTOR sector = GetUsedSetMember( SECTOR, &world->sectors, iSector );

	PWALL pStart, pCur;
	int priorend = TRUE;

	pCur = pStart = GetUsedSetMember( WALL, &world->walls, sector->iWall );
	do
	{
		if( wall == pCur )
			return TRUE;
		// code goes here....
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( pCur != pStart );

	return FALSE;
}

//--------------------------------------------------------------

int LineInCur( INDEX iWorld
			  , INDEX *SectorList, int nSectors
			  , INDEX *WallList, int nWalls
			  , INDEX iLine )
			  //int LineInCur( PDISPLAY display, PLINESEG pLine )
			  // returns 0 - not part of anything current
			  // returns 1 - in current sector
			  // returns 2 - is current wall
{
	int n;
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	//PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	//PSECTOR sector = GetUsedSetMember( SECTOR, &world->sectors, iSector );
	for( n = 0; n < nWalls; n++ )
	{
		PWALL wall = GetUsedSetMember( WALL, &world->walls, WallList[n] );
		if( wall && wall->iLine == iLine )
			return 2;
	}
	for( n = 0; n < nSectors; n++ )
	{
		PSECTOR sector = GetUsedSetMember( SECTOR, &world->sectors, SectorList[n] );
		PWALL pStart, pCur;
		int priorend = TRUE;
		pCur = pStart = GetUsedSetMember( WALL, &world->walls, sector->iWall );
		do
		{
			if( pCur->iLine == iLine )
				return 1;
			if( priorend )
			{
				priorend = pCur->flags.wall_start_end;
				pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallStart );
			}
			else
			{
				priorend = pCur->flags.wall_end_end;
				pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallEnd );
			}
		}while( pCur != pStart );
	}
	return 0;
}

//--------------------------------------------------------------

INDEX FindIntersectingWall( INDEX iWorld, INDEX iSector, P_POINT n, P_POINT o )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	//PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	PSECTOR sector = GetUsedSetMember( SECTOR, &world->sectors, iSector );
	PWALL pStart, pCur;
	int priorend = TRUE;
	if( !sector )
		return INVALID_INDEX;
	pCur = pStart = GetUsedSetMember( WALL, &world->walls, sector->iWall );
	//Log( WIDE("------- FindIntersectingWall ------------ ") );
	do
	{
		RCOORD T1, T2;
		PFLATLAND_MYLINESEG plsCur = GetUsedSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
		//Log1( WIDE("FIW Testing %08x"), pCur );
		if( FindIntersectionTime( &T1, n, o
			, &T2, plsCur->r.n, plsCur->r.o ) )
		{
			//Log6( WIDE("Intersects somewhere.... %g<%g<%g %g<%g<%g"), 0.0, T1, 1.0, plsCur->dFrom, T2, plsCur->dTo );
			if( (0 <= T1) && (T1 <= 1.0) &&
				(plsCur->dFrom <= T2) && (T2 <= plsCur->dTo) )
			{
				//Log( WIDE("Intersects within both segments...") );
				return GetMemberIndex( WALL, &world->walls, pCur );
			}
		}
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetUsedSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( pCur != pStart );
	return INVALID_INDEX;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	if( wall )
	{
		return wall->iLine;
	}
	return INVALID_INDEX;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX iWall, int *priorend )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetUsedSetMember( WALL, &world->walls, iWall );
	INDEX iLast = iWall;
	if( priorend )
	{
		if( *priorend )
		{
			//lprintf( WIDE("start_end"), iWall );
			*priorend = wall->flags.wall_start_end;
			wall = GetUsedSetMember( WALL, &world->walls, iLast = wall->iWallStart );
		}
		else
		{
			//lprintf( WIDE("end_end"), iWall );
			*priorend = wall->flags.wall_end_end;
			wall = GetUsedSetMember( WALL, &world->walls, iLast = wall->iWallEnd );
		}
	}
	//lprintf( WIDE("This wall is %d, next is %d"), iWall, iLast );
	return iLast;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	if( world )
	{
		PSECTOR sector = GetUsedSetMember( SECTOR, &world->sectors, iSector );
		if( sector )
		{

			PWALL wall = sector?GetUsedSetMember( WALL, &world->walls, sector->iWall ):NULL;
			if( wall )
			{
				if( priorend )
					(*priorend) = wall->flags.wall_start_end;
				return sector->iWall;
			}
		}
	}
	return INVALID_INDEX;

}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetMatedWall )( INDEX iWorld, INDEX iWall )
{
	GETWORLD( iWorld );
	PWALL wall = GetWall( iWall );
	if( wall ) return wall->iWallInto;
	return INVALID_INDEX;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetSectorName )( INDEX iWorld, INDEX iSector  )
{
	GETWORLD( iWorld );
	PSECTOR sector = GetSector( iSector );
	return sector->iName;
}

WORLD_PROC( void, SetSectorName )( INDEX iWorld, INDEX iSector, INDEX iName  )
{
	//INDEX ResultID;
	//_32 Result[1];
	//size_t ResultLen = 4;
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(SetSectorName), 3
								, NULL, NULL, NULL //, &ResultID, Result, &ResultLen
								, &iWorld, sizeof( iWorld )
								, &iSector, sizeof( iSector )
								, &iName, sizeof( iName )
								)
		/*&& ( ResultID == (MSG_ID(SetSectorName)|SERVER_SUCCESS))*/)
	{
		//return Result[0];
	}
	//return INVALID_INDEX;
}

//--------------------------------------------------------------

static void CPROC SectorForCallback( PTRSZVAL psv, INDEX iSector )
{

}

//--------------------------------------------------------------

WORLD_PROC( void, ForAllSectors )( INDEX iWorld
								  , FESMCallback f
								  , PTRSZVAL psv)
{
	GETWORLD( iWorld );
	if( world )
		DoForAllSectors( world->sectors, f, psv );
}


INDEX GetSectorTexture( INDEX iWorld, INDEX iSector )
{
	GETWORLD( iWorld );
	PSECTOR sector = GetSector( iSector );
	return sector->iTexture;
}

