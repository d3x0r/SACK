#define WORLD_SERVICE
#define WORLD_SOURCE
#define WORLD_SERVICE_SOURCE

#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include <math.h>

#define NEED_VECTLIB_COMPARE
#include "world.h"
#include "lines.h"
#include "texture.h" 
#include "global.h"

extern GLOBAL g;

//extern WORLD world;
int SectorIDs;
INDEX FlatlandPointWithin( INDEX iWorld, int nSectors, INDEX *pSectors, P_POINT p );
INDEX FlatlandPointWithinSingle( INDEX iWorld, INDEX iSector, P_POINT p );

//----------------------------------------------------------------------------
#define SetLine( isetthis, pline ) { 				\
	if( isetthis != INVALID_INDEX )      			\
	{                              				\
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, isetthis ); \
			if( !(--(psetthis)->refcount))   		\
				ServerDestroyLine( isetthis, iWorld ); \
		}                              				\
		if( pline != INVALID_INDEX )              \
		{                                         \
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pline ); \
			(isetthis) = (pline);                  \
			(psetthis)->refcount++;                \
		}                                         \
	}


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
   //lprintf( WIDE("WORLD Is %p(%d)"), world, iWorld );
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
			PFLATLAND_MYLINESEG ps;
			PFLATLAND_MYLINESEG pw;
			ps = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pStart->iLine );
			pw = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWall->iLine );
         //lprintf( WIDE("Lines here are %d %d"), pStart->iLine, pWall->iLine );
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
#ifdef WORLD_SERVICE
	lprintf( WIDE("Mark wall and line updated...") );
	MarkWallUpdated( iWorld, iWall );
	MarkLineUpdated( iWorld, iLine );
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
		//Log1( WIDE("Walls: %d"), walls );
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
	//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );

	if( CountWalls( iWorld, iWall ) < 4 )
	{
		// with a triangular configuration it looks like peers intersect
		bErrorOK = TRUE;
	}

	//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );

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

		//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
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
						if( !UpdateMatingLines( iWorld, pStart->iWallInto, FALSE/*bLockSlope*/, bErrorOK ) )
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
					//DrawLineSeg( plsStart, Color( 0, 0, 255 ) );
					if( pStart->iWallInto != INVALID_INDEX )
					{
						pStart->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pStart->iWallInto, FALSE/*bLockSlope*/, bErrorOK ) )
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
		//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
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
					//DrawLineSeg( plsEnd, Color( 0, 0, 255 ) );
					if( pEnd->iWallInto != INVALID_INDEX )
					{
						pEnd->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pEnd->iWallInto, FALSE/*bLockSlope*/, bErrorOK ) )
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
					//DrawLineSeg( plsEnd, Color( 0, 0, 255 ) );
					if( pEnd->iWallInto != INVALID_INDEX )
					{
						pEnd->flags.bUpdating = TRUE;
						if( !UpdateMatingLines( iWorld, pEnd->iWallInto, FALSE/*bLockSlope*/, bErrorOK ) )
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
		//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
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
		//Log( WIDE("UpdateMating(")STRSYM(__LINE__)WIDE(")") );
#ifndef WORLD_SERVICE
		ComputeSectorPointList( iWorld, pWall->iSector, NULL );
		ComputeSectorOrigin( iWorld, pWall->iSector );
#endif
		//MarkSectorUpdated( iWorld, pWall->iSector );
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
	if( pWall->iWallInto != INVALID_INDEX )
	{
		// only the orogiinal slopes can be locked...
		// the mating sector might have to move his wall slope.
		if( !UpdateMatingLines( iWorld, pWall->iWallInto, FALSE /*bLockSlope*/, bErrorOK ) )
			UpdateResult( FALSE );
	}
	// posts line update too.
	ServerBalanceALine( iWorld, pWall->iLine );
	ServerBalanceALine( iWorld, pStart->iLine );
	ServerBalanceALine( iWorld, pEnd->iLine );
	MarkSectorUpdated( iWorld, pWall->iSector );
	UpdateResult( TRUE );
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
// PARAMETERS  are backwards because of foreachmemberinset thing.
uintptr_t CPROC ServerDestroyLine( INDEX iLine, INDEX iWorld )
{
	MarkLineDeleted( iWorld, iLine );
	return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( void, ServerForAllLines )( INDEX iWorld
								  , uintptr_t(CPROC *f)(INDEX,INDEX) )
{
	GETWORLD( iWorld );
	DoForAllLines( world->lines, f, iWorld );
}

//----------------------------------------------------------------------------

uintptr_t CPROC ServerDestroyWall(  INDEX iWall, INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	//int shared;
	//shared = ForAllWalls( wall->sector->world->walls, UnlinkWall, wall );
	//ValidateSpaceTree( world.spacetree );
	PWALL pTmpWall = GetSetMember( WALL, &world->walls, wall->iWallInto );
	if( pTmpWall && pTmpWall->iWallInto == iWall )
		pTmpWall->iWallInto = INVALID_INDEX;
	if( wall->iWallStart != INVALID_INDEX )
	{
		pTmpWall = GetSetMember( WALL, &world->walls, wall->iWallStart );
		if( wall->flags.wall_start_end )
			pTmpWall->iWallEnd = INVALID_INDEX;
		else
			pTmpWall->iWallStart = INVALID_INDEX;
	}
	if( wall->iWallEnd != INVALID_INDEX )
	{
		pTmpWall = GetSetMember( WALL, &world->walls, wall->iWallEnd );
		if( wall->flags.wall_end_end )
			pTmpWall->iWallEnd = INVALID_INDEX;
		else
			pTmpWall->iWallStart = INVALID_INDEX;
	}
	SetLine( wall->iLine, INVALID_INDEX );
	// can't delete the line, cause it might have bene shared?
	// but I know everyone who I'm linked to...
	// is the game soemthing...
	MarkWallDeleted( iWorld, iWall );
	return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( void, ServerForAllWalls )( INDEX iWorld
								  , uintptr_t(CPROC *f)(INDEX,INDEX) )
{
	GETWORLD( iWorld );
	DoForAllWalls( world->walls, f, iWorld );
}
//----------------------------------------------------------------------------

uintptr_t CPROC ServerDestroySector( INDEX iSector, INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR ps = GetSetMember( SECTOR, &world->sectors, iSector );
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
		ServerDestroyWall( iCur, iWorld );
		iCur = iNext;
		pCur = pNext;
	}while( pCur );
	//ps->iWall = INVALID_INDEX;
	// this poitnlist is a temporary phenonmenon... remove from server.
	//Release( ps->pointlist );
	// can't clear sector->world... ahh well...
	MarkSectorDeleted( iWorld, iSector );
	// have to nofify clients of destruction..w hcih should then do this destroy.
	//DeleteSector( world->sectors, ps );
	return 0;
}

//----------------------------------------------------------------------------
WORLD_PROC( void, ServerForAllSectors )( INDEX iWorld
								  , uintptr_t(CPROC *f)(INDEX,INDEX) )
{
	GETWORLD( iWorld );
	DoForAllSectors( world->sectors, f, iWorld );
}

void DeleteSectors( INDEX iWorld )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	//PSPACENODE node;
	//Log( WIDE("Deleting Sectors...") );
	//while( node = FindDeepestNode( world.spacetree, 0 ) )
	//{
	//Log1( WIDE("%08x arg!"), node );
	//  DestroySector( (PSECTOR)GetNodeData( node ) );
	//}
	ServerForAllSectors( iWorld, ServerDestroySector );

	// for all sectors - dont' count on space tree for knowing where things are...
	// the tree scheduling alogrithm is a higher level construct is really
	// although needing positive hooks bi-directionally, has no bearing with
	// internal logic which knows nothing about the needs of display...
	// it's all a vast tree of nearly connected nodes that needs to be
	// traversed as such internally anyhow.....  there is no 'beyond here'
	//DeleteSet( (struct genericset_tag**)&world->sectors );
	SETFLAG( world->deletions.deleteset, 2 );
}

void DeleteWalls( INDEX iWorld )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	ServerForAllWalls( iWorld, ServerDestroyWall );
	SETFLAG( world->deletions.deleteset, 1 );
}

void DeleteLines( INDEX iWorld )
{
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	ServerForAllLines( iWorld, ServerDestroyLine );
	SETFLAG( world->deletions.deleteset, 0 );
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

INDEX InsertConnectedSector( INDEX iWorld, INDEX iWall, RCOORD offset )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );

	PWALL wall;
	PFLATLAND_MYLINESEG pls;
	INDEX iSector, iLine2;
	PWALL lastwall;
	INDEX new_wall, iLastWall, iOtherWall;
	PWALL other_wall;
	PWALL pNewWall;
	PSECTOR ps;
	_POINT o, n;

	SplitNearWalls( iWorld, iWall );
	wall = GetSetMember( WALL, &world->walls, iWall );
	pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
	iOtherWall = wall->iWallInto;
	other_wall = GetSetMember( WALL, &world->walls, iOtherWall );

	ps = GetFromSet( SECTOR, &world->sectors );
	iSector = GetMemberIndex( SECTOR, &world->sectors, ps );
	/*
	{
	char name[20];
	sprintf( name, WIDE("%d"), SectorIDs++ );
	ps->name = GetName( &world->names, name );
	}
	*/
	ps->iName = INVALID_INDEX;
	ps->iWorld = iWorld;
	pls->refcount--; // gets incrementd in - but it's not really new...
	new_wall  = CreateWallFromLine( iWorld, iSector
		, INVALID_INDEX, FALSE
		, INVALID_INDEX, FALSE
		, wall->iLine );
	ps->iWall  = new_wall;
	{
		PSECTOR pTmpSector = GetSetMember(SECTOR, &world->sectors, wall->iSector );
		SetTexture( iWorld, iSector, pTmpSector->iTexture );
	}
	pNewWall = GetSetMember(WALL,&world->walls, new_wall);
	pNewWall->iWallInto = iWall;
	wall->iWallInto	 = new_wall;

	crossproduct( n, VectorConst_Z, pls->r.n );
	normalize( n );
	add( o, n, pls->r.o );
	if( FlatlandPointWithinSingle( iWorld, wall->iSector, o ) != INVALID_INDEX )
		scale( n, n, -1 );

	addscaled( o, pls->r.o, pls->r.n, pls->dTo );
	CreateWall( iWorld, iSector, new_wall, TRUE, INVALID_INDEX, FALSE, o, n );

	addscaled( o, pls->r.o, pls->r.n, pls->dFrom );
	CreateWall( iWorld, iSector, new_wall, FALSE, INVALID_INDEX, FALSE, o, n );

	iLine2 = DuplicateLine( iWorld, wall->iLine );
	pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine2 );
	// scale give it some substanstial offset...
	addscaled( pls->r.o, pls->r.o, n, offset );
	iLastWall = CreateWallFromLine( iWorld
		, iSector
		, pNewWall->iWallStart, TRUE
		, pNewWall->iWallEnd, TRUE
		, iLine2 );
	lastwall = GetSetMember( WALL, &world->walls, iLastWall );
	lastwall->iWallInto   = wall->iWallInto;
	other_wall->iLine	  = lastwall->iLine;
	pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, other_wall->iLine );
	pls->refcount++; // this used to be the other line we -- from
	other_wall->iWallInto = iLastWall;
	UpdateMatingLines( iWorld, iLastWall, FALSE, FALSE );

	//ComputeSectorPointList( iWorld, lastwall->iSector, NULL );
	//ComputeSectorOrigin( iWorld, lastwall->iSector );
	MarkSectorUpdated( iWorld, lastwall->iSector );
	{
		PSECTOR pTmpSect = GetSetMember( SECTOR, &world->sectors, lastwall->iSector );
		SetPoint( pTmpSect->r.n, VectorConst_Y );
	}
	{
		_POINT min, max;
		GetSectorMinMax( ps, min, max );
		//ps->spacenode = AddSpaceNode( &world->spacetree, ps, min, max );
	}

	return iLastWall;
}

//----------------------------------------------------------------------------

INDEX AddConnectedSector( INDEX iWorld, INDEX iWall, RCOORD offset )
{
	INDEX iSector, iLine;
	PSECTOR ps;
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PSECTOR pOldSect = GetSetMember( SECTOR, &world->sectors, wall->iSector );
	// build a new sector off of the given wall...
	// returns current wall... 
	if( wall->iWallInto == INVALID_INDEX )
	{
		PFLATLAND_MYLINESEG pls;
		PWALL pNewWall;
		INDEX new_wall, lastwall;
		_POINT o, n;
		pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
		ps = GetFromSet( SECTOR, &world->sectors );
		iSector = GetMemberIndex( SECTOR, &world->sectors, ps );
		/*
		{
		char name[20];
		sprintf( name, WIDE("%d"), SectorIDs++ );
		ps->name = GetName( &world->names, name );
		}
		*/
		ps->iName = INVALID_INDEX;
		SetTexture( ps->iWorld, iSector, pOldSect->iTexture );
		ps->iWorld = iWorld;
		// Uses exactly the same line....
		new_wall = CreateWallFromLine( iWorld, iSector
			, INVALID_INDEX, FALSE
			, INVALID_INDEX, FALSE
			, wall->iLine );
		pNewWall = GetSetMember( WALL, &world->walls, new_wall );
		ps->iWall = new_wall;

		// fix up into links... for now new sectors are bi-directional
		pNewWall->iWallInto = iWall;
		wall->iWallInto	 = new_wall;

		crossproduct( n, pls->r.n, VectorConst_Z );
		normalize( n );
		add( o, n, pls->r.o );
		if( FlatlandPointWithin( iWorld, 1, &wall->iSector, o ) != INVALID_INDEX )
			scale( n, n, -1 );

		addscaled( o, pls->r.o, pls->r.n, pls->dTo );
		CreateWall( iWorld, iSector, new_wall, TRUE, INVALID_INDEX, FALSE, o, n );

		addscaled( o, pls->r.o, pls->r.n, pls->dFrom );
		CreateWall( iWorld, iSector, new_wall, FALSE, INVALID_INDEX, FALSE, o, n );


		pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine = DuplicateLine( iWorld, wall->iLine ) );
		// give it some substanstial offset...
		addscaled( pls->r.o, pls->r.o, n, offset );
		lastwall = CreateWallFromLine( iWorld, iSector
			, pNewWall->iWallStart, TRUE
			, pNewWall->iWallEnd, TRUE, iLine );
		//DumpWall( ps->iWorld, ps->wall );
		//DumpWall( ps->iWorld, ps->iWall->iWallStart );
		//DumpWall( ps->iWorld, ps->iWall->iWallEnd );
		//DumpWall( ps->iWorld, ps->iWall->iWallStart->iWall_at_end ); // dump both cause we don't know...
		//DumpWall( ps->iWorld, ps->wall->iWallStart->iWallStart );

		// server doesn't need this list?
		//ComputeSectorPointList( iWorld, iSector, NULL );
		//ComputeSectorOrigin( iWorld, iSector );
		MarkSectorUpdated( iWorld, iSector );
		ps = GetSetMember( SECTOR, &world->sectors, iSector );
		SetPoint( ps->r.n, VectorConst_Y );
		{
			//_POINT min, max;
			//GetSectorMinMax( ps, min, max );
			//ps->spacenode = AddSpaceNode( &world->spacetree, ps, min, max );
		}
		return lastwall ; // return last line created - opposing line fit for updating.
	}
	return InsertConnectedSector( iWorld, iWall, offset );
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

INDEX FlatlandPointWithinLoopSingle( INDEX iSector, uintptr_t psv )
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
	pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
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
	}while( pCur != pStart );

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

	pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
	do
	{
		plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
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

	ptlist = (_POINT*)Allocate( sizeof( _POINT ) * npoints );

	npoints = 0;
	// start again...
	pCur = pStart;
	do
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

	if( pnpoints )
		*pnpoints = npoints;		
	return ptlist;
}

//----------------------------------------------------------------------------

void GetSectorPoints( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector  = GetSetMember( SECTOR, &world->sectors, iSector );
	*list = sector->pointlist;
	*npoints = sector->npoints;
}

//----------------------------------------------------------------------------

int MoveSectors( INDEX iWorld, int nSectors,INDEX *pSectors, P_POINT del )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	// now in theory this routine can be extended to
	// disallow certain updates that would cause errors
	// in mating areas... will have to evaluate some existing
	// error conditions that erroneously block updates that
	// currently exist - so for now this can override the error
	// checking in UpdateMatingLines....
	int n;
	//Log4( WIDE("Moving %d sectors by (%g,%g,%g)"), nSectors, del[0], del[1], del[2] );
	for( n = 0; n < nSectors; n++ )
	{
		PFLATLAND_MYLINESEG plsCur;
		PSECTOR sector;
		PWALL pCur, pStart;
		int priorend = TRUE;
		sector = GetSetMember( SECTOR, &world->sectors, pSectors[n] );
		pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
		do
		{
			if( !pCur->flags.bUpdating )
			{
				PWALL pWallInto = GetSetMember( WALL, &world->walls, pCur->iWallInto );
				plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
				pCur->flags.bUpdating = TRUE;
				if( pWallInto )
				{
					if( !pWallInto->flags.bUpdating )
					{
						add( plsCur->r.o, plsCur->r.o, del );
						// if other is not in this set - do not flag updating
						if( SectorInSet( nSectors, pSectors, pWallInto->iSector ) )
							// otherwise mark this so we do not double update line
							pWallInto->flags.bUpdating = TRUE;
					}
					else
					{
						// should have already counted this line...
						Log( WIDE("Unmatched matings lines - possible mislinkage!") );
					}
				}
				else
				{
					add( plsCur->r.o, plsCur->r.o, del );
					MarkLineUpdated( iWorld, pCur->iLine );
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
		}while( pCur != pStart );

		add( sector->r.o, sector->r.o, del );
		MarkSectorUpdated( iWorld, pSectors[n] );
		// - or -
		// after updating all the lines...
		//ComputeSectorOrigin( sector );
	}

	// have marked all walls which are within
	// this as updating - so updatematinglines will not
	// process any line already updated......
	// so now we casually update all mating walls outside
	// the selected area...
	for( n = 0; n < nSectors; n++ )
	{
		PSECTOR sector;
		PWALL pCur, pStart;
		int priorend = TRUE;
		sector =  GetSetMember( SECTOR, &world->sectors, pSectors[n] );
		pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
		do
		{
			PWALL pWallInto = GetSetMember( WALL, &world->walls, pCur->iWallInto );
			if( pWallInto )
			{
				if( !SectorInSet( nSectors, pSectors, pWallInto->iSector ) )
				{
					if( !UpdateMatingLines( iWorld, pCur->iWallInto, FALSE, TRUE ) )
						Log( WIDE("Some error condition not overridden...") );
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
		}while( pCur != pStart );
	}

	// so now unmark all lines in all sectors selected...
	for( n = 0; n < nSectors; n++ )
	{
		PWALL pCur, pStart;
		int priorend = TRUE;
		PSECTOR sector = GetSetMember( SECTOR, &world->sectors, pSectors[n] );
		pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
		do
		{
			/* all this loop! for this! */
			pCur->flags.bUpdating = FALSE;
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
		} while( pCur != pStart );
	}
	//UpdateSpace( nSectors, pSectors );
	return TRUE;
}

//----------------------------------------------------------------------------

int MoveWalls( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	int n;
	//if( !nWalls || !WallList )
	//  return TRUE;

	if( nWalls == 1 )
	{
		_POINT o;
		PWALL wall = GetSetMember( WALL, &world->walls, WallList[0] );
		PFLATLAND_MYLINESEG line = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
		SetPoint( o, line->r.o );
		add( line->r.o, line->r.o, del );
		if( !UpdateMatingLines( iWorld, WallList[0], bLockSlope, FALSE ) )
		{
			SetPoint( line->r.o, o );
			return FALSE;
		}
      MarkLineUpdated( iWorld, wall->iLine );
		return TRUE;
	}
	for( n = 0; n < nWalls; n++ )
	{
		PWALL wall = GetSetMember( WALL, &world->walls, WallList[n] );
		PFLATLAND_MYLINESEG line = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
		if( !wall->flags.bSkipMate )
		{
			PWALL pWallInto = GetSetMember( WALL, &world->walls, wall->iWallInto );
			if( pWallInto )
				pWallInto->flags.bSkipMate = TRUE;
			add( line->r.o, line->r.o, del );
		}
		MarkLineUpdated( iWorld, wall->iLine );
	}
	// can't mark these walls since that would
	// immediatly abort walls - and both wall sides 
	for( n = 0; n < nWalls; n++ )
	{
		PWALL wall = GetSetMember( WALL, &world->walls, WallList[n] );
		// lock out immediate opposite since we'll hit that again anyhow.
		if( !wall->flags.bSkipMate )
			UpdateMatingLines( iWorld, WallList[n], FALSE, TRUE );
	}

	for( n = 0; n < nWalls; n++ )
	{
		PWALL wall = GetSetMember( WALL, &world->walls, WallList[n] );
		wall->flags.bSkipMate = FALSE;
	}
	return TRUE;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

int RemoveWall( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PFLATLAND_MYLINESEG plsWall = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
	if( !wall )
		return TRUE;
	if( wall->iWallInto != INVALID_INDEX )
		SplitWall( iWorld, iWall );
	ServerBalanceALine( iWorld, wall->iLine ); // may have been result of a lockslope update
	//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );
	SplitNearWalls( iWorld, iWall );
	if( CountWalls( iWorld, iWall ) > 3 )
	{
		PWALL pWallStart = GetSetMember( WALL, &world->walls, wall->iWallStart );
		PFLATLAND_MYLINESEG pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWallStart->iLine );
		_POINT pt  // other point of mating segment
			, ptLine; // point on deleting line to move to.
		/*
		Log7( WIDE("%08x/%08x -> %08x -> %08x <- %08x -> %08x/%08x")
		,wall->iWallStart->iWallStart
		,wall->iWallStart->iWallEnd
		,wall->iWallStart
		,wall
		,wall->iWallEnd
		,wall->iWallEnd->iWallEnd
		,wall->iWallEnd->iWallStart
		);
		*/
		if( wall->flags.wall_start_end )
		{
			addscaled( pt, pls->r.o, pls->r.n, pls->dFrom );
			SetPoint( pls->r.o, pt );
			sub( ptLine, plsWall->r.o, pt );
			SetPoint( pls->r.n, ptLine );
			pls->dFrom = 0;
			pls->dTo = 1;
			pWallStart->iWallEnd = wall->iWallEnd;
			pWallStart->flags.wall_end_end = wall->flags.wall_end_end;
		}
		else
		{
			addscaled( pt, pls->r.o, pls->r.n, pls->dTo );
			SetPoint( pls->r.o, pt );
			sub( ptLine, pt, plsWall->r.o );
			SetPoint( pls->r.n, ptLine );
			pls->dFrom = -1;
			pls->dTo = 0;
			pWallStart->iWallStart = wall->iWallEnd;
			pWallStart->flags.wall_start_end = wall->flags.wall_end_end;
		}
		{
			PWALL pWallEnd = GetSetMember( WALL, &world->walls, wall->iWallEnd );
			pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pWallEnd->iLine );
			if( wall->flags.wall_end_end )
			{
				addscaled( pt, pls->r.o, pls->r.n, pls->dFrom );
				SetPoint( pls->r.o, pt );
				sub( ptLine, plsWall->r.o, pt );
				SetPoint( pls->r.n, ptLine );
				pls->dFrom = 0;
				pls->dTo = 1;
				pWallEnd->iWallEnd = wall->iWallStart;
				pWallEnd->flags.wall_end_end = wall->flags.wall_start_end;
			}
			else
			{
				addscaled( pt, pls->r.o, pls->r.n, pls->dTo );
				SetPoint( pls->r.o, pt );
				sub( ptLine, pt, plsWall->r.o );
				SetPoint( pls->r.n, ptLine );
				pls->dFrom = -1;
				pls->dTo = 0;
				pWallEnd->iWallStart = wall->iWallStart;
				pWallEnd->flags.wall_start_end = wall->flags.wall_start_end;
			}
		}
		/*
		Log7( WIDE("%08x/%08x -> %08x -> %08x <- %08x -> %08x/%08x")
		,wall->iWallStart->iWallStart
		,wall->iWallStart->iWallEnd
		,wall->iWallStart
		,wall
		,wall->iWallEnd
		,wall->iWallEnd->iWallEnd 
		,wall->iWallEnd->iWallStart 
		);
		*/
		// only require one wall - since other updated is directly connected as
		// a mating line....
		//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );
		UpdateMatingLines( iWorld, pWallStart->iWallInto, TRUE, TRUE );
		//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );
		{
			PSECTOR sector = GetSetMember( SECTOR, &world->sectors, wall->iSector );
			if( sector->iWall == iWall )
			{
				//Log( WIDE("Updating sector starting wall since it's natural is gone.") );
				sector->iWall = wall->iWallStart;
			}
		}
		//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );
		//ComputeSectorPointList( iWorld, wall->iSector, NULL );
		//ComputeSectorOrigin( iWorld, wall->iSector );
		MarkSectorUpdated( iWorld, wall->iSector );
		//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );

		wall->iWallStart = INVALID_INDEX;
		wall->iWallEnd = INVALID_INDEX;
		//DeleteSetMember(
		ServerDestroyWall( iWall, iWorld );
		//Log( WIDE("RemoveWall(")STRSYM(__LINE__)WIDE(")")  );

		//if( !ValidateWorldLinks(world) ) Log2( WIDE("Remove Failing %s(%d)"), __FILE__, __LINE__ );
	}
	return TRUE;
}

//----------------------------------------------------------------------------

void BreakWall( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PFLATLAND_MYLINESEG pls = GetFromSet( FLATLAND_MYLINESEG, &world->lines );
	INDEX iNewLine = GetMemberIndex( FLATLAND_MYLINESEG, &world->lines, pls );
	PWALL pNewWall = GetFromSet( WALL, &world->walls );
	INDEX iNewWall = GetMemberIndex( WALL, &world->walls, pNewWall );
	PFLATLAND_MYLINESEG pWallLine = GetSetMember( FLATLAND_MYLINESEG, &world->lines, wall->iLine );
	DumpWall( iWorld, iWall );
	pNewWall->iLine = INVALID_INDEX;
	ServerBalanceALine( iWorld, wall->iLine );
	addscaled( pls->r.o, pWallLine->r.o, pWallLine->r.n, pWallLine->dFrom );
	pWallLine->dTo = 1;
	pWallLine->dFrom = 0;
	pls->dFrom = 0;
	pls->dTo = 1;
	SetPoint( pls->r.n, pWallLine->r.n );

	SetLine( pNewWall->iLine, iNewLine );

	pNewWall->iSector = wall->iSector;

	pNewWall->iWallEnd = iWall;
	pNewWall->flags.wall_end_end = FALSE;

	pNewWall->iWallStart = wall->iWallStart;
	pNewWall->flags.wall_start_end = wall->flags.wall_start_end;
	{
		PWALL pWallStart = GetSetMember( WALL, &world->walls, wall->iWallStart );

		if( wall->flags.wall_start_end )
		{
			pWallStart->iWallEnd = iNewWall;
			pWallStart->flags.wall_end_end = FALSE;
		}
		else
		{
			pWallStart->iWallStart = iNewWall;
			pWallStart->flags.wall_start_end = FALSE;
		}
	}

	wall->iWallStart = iNewWall;
	wall->flags.wall_start_end = TRUE;

	if( wall->iWallInto != INVALID_INDEX )
	{
		PWALL pWallInto = GetSetMember( WALL, &world->walls, wall->iWallInto );
		PWALL pOtherNew = GetFromSet( WALL, &world->walls );
		INDEX iOtherNew = GetMemberIndex( WALL, &world->walls, pOtherNew );

		pNewWall->iWallInto = iOtherNew;
		pOtherNew->iWallInto = iNewWall;
		pOtherNew->iLine = INVALID_INDEX;
		SetLine( pOtherNew->iLine, iNewLine );
		pOtherNew->iSector = pWallInto->iSector;
		pOtherNew->iWallEnd = wall->iWallInto;
		pOtherNew->flags.wall_end_end = FALSE;
		pOtherNew->iWallStart = pWallInto->iWallStart;
		pOtherNew->flags.wall_start_end = pWallInto->flags.wall_start_end;

		{
			PWALL pWallStart = GetSetMember( WALL, &world->walls, pWallInto->iWallStart );
			if( pWallInto->flags.wall_start_end )
			{
				pWallStart->iWallEnd = iOtherNew;
				pWallStart->flags.wall_end_end = FALSE;
			}
			else
			{
				pWallStart->iWallStart = iOtherNew;
				pWallStart->flags.wall_start_end = FALSE;
			}
		}
		pWallInto->iWallStart = iOtherNew;
		pWallInto->flags.wall_start_end = TRUE;
		DumpWall( iWorld, iOtherNew );
	}
	else
		pNewWall->iWallInto = INVALID_INDEX;
	ServerBalanceALine( iWorld, wall->iLine );
	DumpWall( iWorld, iWall );
	DumpWall( iWorld, iNewWall );
}

int WallInSector( INDEX iWorld, INDEX iSector, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );

	PWALL pStart, pCur;
	int priorend = TRUE;

	pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
	do
	{
		if( wall == pCur )
			return TRUE;
		// code goes here....
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
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	//PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	//PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );
	for( n = 0; n < nWalls; n++ )
	{
		if( GetSetMember( WALL, &world->walls, WallList[n] )->iLine == iLine )
			return 2;
	}
	for( n = 0; n < nSectors; n++ )
	{
		PSECTOR sector = GetSetMember( SECTOR, &world->sectors, SectorList[n] );
		PWALL pStart, pCur;
		int priorend = TRUE;
		pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
		do
		{
			if( pCur->iLine == iLine )
				return 1;
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
	}
	return 0;
}

//--------------------------------------------------------------

INDEX FindIntersectingWall( INDEX iWorld, INDEX iSector, P_POINT n, P_POINT o )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	//PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );
	PWALL pStart, pCur;
	int priorend = TRUE;
	if( !sector )
		return INVALID_INDEX;
	pCur = pStart = GetSetMember( WALL, &world->walls, sector->iWall );
	//Log( WIDE("------- FindIntersectingWall ------------ ") );
	do
	{
		RCOORD T1, T2;
		PFLATLAND_MYLINESEG plsCur = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pCur->iLine );
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
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallStart );
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = GetSetMember( WALL, &world->walls, pCur->iWallEnd );
		}
	}while( pCur != pStart );
	return INVALID_INDEX;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	if( wall )
	{
		return wall->iLine;
	}
	return INVALID_INDEX;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX iWall, int *priorend )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	INDEX iLast = iWall;
	if( *priorend )
	{
		*priorend = wall->flags.wall_start_end;
		wall = GetSetMember( WALL, &world->walls, iLast = wall->iWallStart );
	}
	else
	{
		*priorend = wall->flags.wall_end_end;
		wall = GetSetMember( WALL, &world->walls, iLast = wall->iWallEnd );
	}
	return iLast;
}

//--------------------------------------------------------------

WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector = GetSetMember( SECTOR, &world->sectors, iSector );
	PWALL wall = sector?GetSetMember( WALL, &world->walls, sector->iWall ):NULL;
	//if( wall && priorend )
	//	(*priorend) = wall->flags.wall_start_end;
	return sector->iWall;
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

//--------------------------------------------------------------

static void CPROC SectorForCallback( uintptr_t psv, INDEX iSector )
{

}

//--------------------------------------------------------------


INDEX GetSectorTexture( INDEX iWorld, INDEX iSector )
{
	GETWORLD( iWorld );
	PSECTOR sector = GetSector( iSector );
	return sector->iTexture;
}

