#define WORLD_SERVICE
#define WORLD_SOURCE

#include <stdhdrs.h>
#include <logging.h>
#include <sharemem.h>
#include <math.h>
#define NEED_VECTLIB_COMPARE
#include "world.h"
#include "global.h"
#include "lines.h"
//----------------------------------------------------------------------------

#define LOG_STUFF

extern GLOBAL g;

INDEX CreateOpenLine( INDEX iWorld
						  , _POINT o, _POINT n )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEG pls = GetFromSet( FLATLAND_MYLINESEG, &world->lines );
	INDEX ils = GetMemberIndex( FLATLAND_MYLINESEG, &world->lines, pls );
#ifdef LOG_STUFF
	lprintf( WIDE("Got line %p from %p(%d) world"), pls, world, iWorld );
#endif
	SetPoint( pls->r.o, o );
	SetPoint( pls->r.n, n );
#ifdef LOG_STUFF
	lprintf( WIDE("And now we have a line %p with ...") );
   PrintVector( pls->r.o );
	PrintVector( pls->r.n );
#endif
	pls->dFrom = NEG_INFINITY;
	pls->dTo   = POS_INFINITY;
#ifdef WORLDSCAPE_SERVICE
	MarkLineUpdated( iWorld, ils );
#endif
   //lprintf( WIDE("line index is %d"), ils );
	return ils;
}

//----------------------------------------------------------------------------

INDEX DuplicateLine( INDEX iWorld, INDEX ilsDup )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEGSET *ppLines = &world->lines;
	PFLATLAND_MYLINESEG plsDup = GetSetMember( FLATLAND_MYLINESEG, ppLines, ilsDup );
	PFLATLAND_MYLINESEG pls = GetFromSet( FLATLAND_MYLINESEG, ppLines );
	INDEX ils = GetMemberIndex( FLATLAND_MYLINESEG, ppLines, pls );
	SetPoint( pls->r.o, plsDup->r.o );
	SetPoint( pls->r.n, plsDup->r.n );
	pls->dFrom = plsDup->dFrom;
	pls->dTo = plsDup->dTo;
	return ils;
}

//----------------------------------------------------------------------------

//void LogLine(

int IntersectLines( PFLATLAND_MYLINESEG pLine1, int bEnd1, PFLATLAND_MYLINESEG pLine2, int bEnd2 )
{
	// intersects line1 with line2
	// if bEnd1 then update pLine1->dTo else pLine1->dFrom
	// if bEnd2 then update pLine2->dTo else pLine2->dFrom
	int r;
	RCOORD t1, t2;
	r = FindIntersectionTime( &t1, pLine1->r.n, pLine1->r.o
									, &t2, pLine2->r.n, pLine2->r.o );
 	if( !r )
	{
		TEXTCHAR msg[256];
      /*
		sprintf( msg, WIDE("Intersect N<%g,%g,%g> O<%g,%g,%g> with N<%g,%g,%g> O<%g,%g,%g>"), 
							pLine1->r.n[0], 
							pLine1->r.n[1], 
							pLine1->r.n[2], 
							pLine1->r.o[0], 
							pLine1->r.o[1], 
							pLine1->r.o[2], 
							pLine2->r.n[0], 
							pLine2->r.n[1], 
							pLine2->r.n[2], 
							pLine2->r.o[0], 
							pLine2->r.o[1], 
							pLine2->r.o[2] );
							Log( msg );
      */
		snprintf( msg, 256, WIDE("Result %d Times: %g and %g"), r, t1, t2 );
		Log( msg );
		Log2( WIDE("End Flags: %d %d"), bEnd1, bEnd2 );
	}
	if( r )
	{
		if( bEnd1 )
			pLine1->dTo = t1;
		else
			pLine1->dFrom = t1;

		if( bEnd2 )
			pLine2->dTo = t2;
		else
			pLine2->dFrom = t2;
		return TRUE;
	}
	else
	{
		// what to do if specified lines do not intersect?
		return FALSE;
	}
}

//----------------------------------------------------------------------------
void DumpLine( PFLATLAND_MYLINESEG pls )
{
	Log8( WIDE("Line: <%g,%g,%g> <%g,%g,%g> from:%g to:%g")
						, pls->r.n[0]
						, pls->r.n[1]
						, pls->r.n[2]
						, pls->r.o[0]
						, pls->r.o[1]
						, pls->r.o[2]
						, pls->dFrom
						, pls->dTo );
}

//----------------------------------------------------------------------------

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 )
{
   VECTOR R1, R2, denoms;
   RCOORD t1, t2, denom;

#define a (o1[0])
#define b (o1[1])
#define c (o1[2])

#define d (o2[0])
#define e (o2[1])
#define f (o2[2])

#define na (s1[0])
#define nb (s1[1])
#define nc (s1[2])

#define nd (s2[0])
#define ne (s2[1])
#define nf (s2[2])
	if( ( !s1[0] && !s1[1] && !s1[2] )||
       ( !s2[0] && !s2[1] && !s2[2] ) )
      return FALSE;
	crossproduct(denoms, s1, s2 ); // - (negative) result...
	denom = denoms[2];
//   denom =  ( ne * na ) - ( nd * nb );
	if( NearZero( denom ) )
	{
		denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
		if( NearZero( denom ) )
		{
			denom = denoms[0];
//         denom = ( nb * nf ) - ( ne * nc );
			if( NearZero( denom ) )
			{
            /*
//#ifdef FULL_DEBUG
				Log( WIDE("Bad!-------------------------------------------\n") );
//#endif
				Log6( WIDE("Line 1: <%g %g %g> <%g %g %g>")
							, s1[0], s1[1], s1[2] 
							, o1[0], o1[1], o1[2] );
				Log6( WIDE("Line 2:<%g %g %g> <%g %g %g>")
							, s2[0], s2[1], s2[2] 
							, o2[0], o2[1], o2[2] );
				*/
				return FALSE;
			}
			else
			{
				t1 = ( ne * ( c - f ) + nf * ( e - b ) ) / denom;
				t2 = ( nb * ( c - f ) + nc * ( e - b ) ) / denom;
			}
		}
		else
		{
			t1 = ( nd * ( f - c ) + nf * ( a - d ) ) / denom;
			t2 = ( na * ( f - c ) + nc * ( a - d ) ) / denom;
		}
	}
	else
	{
		// this one has been tested.......
		t1 = ( nd * ( b - e ) + ne * ( d - a ) ) / denom;
		t2 = ( na * ( b - e ) + nb * ( d - a ) ) / denom;
	}
	addscaled( R1, o1, s1, t1 );
	//R1[0] = a + na * t1;
	//R1[1] = b + nb * t1;
	//R1[2] = c + nc * t1;
	addscaled( R2, o2, s2, t2 );
	//R2[0] = d + nd * t2;
	//R2[1] = e + ne * t2;
	//R2[2] = f + nf * t2;

	if( pT1 )
		*pT2 = t2;
	if( pT2 )
		*pT1 = t1;
	{	
		int i;
		if( ( ((i=0),!COMPARE(R1[0],R2[0]) )) ||
			( ((i=1),!COMPARE(R1[1],R2[1]) )) ||
			( ((i=2),!COMPARE(R1[2],R2[2]) )) )
		{ 
			/*
			Log7( WIDE("Points (%12.12g,%12.12g,%12.12g) and (%12.12g,%12.12g,%12.12g) coord %d is too far apart")
			, R1[0], R1[1], R1[2]
			, R2[0], R2[1], R2[2] 
			, i );
			Log7( WIDE("Points (%08X,%08X,%08X) and (%08X,%08X,%08X) coord %d is too far apart")
			, *(int*)&R1[0], *(int*)&R1[1], *(int*)&R1[2]
			, *(int*)&R2[0], *(int*)&R2[1], *(int*)&R2[2] 
			, i );
			*/
			return FALSE;
		}
	}
	if( pT1 )
		*pT2 = t2;
	if( pT2 )
		*pT1 = t1;
	return TRUE;
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef na
#undef nb
#undef nc
#undef nd
#undef ne
#undef nf
}

//----------------------------------------------------------------------------

static void BalanceLine( PFLATLAND_MYLINESEG pls )
{
	_POINT ptStart, ptEnd, ptMid;
	addscaled( ptStart, pls->r.o, pls->r.n, pls->dFrom );
	addscaled( ptEnd	, pls->r.o, pls->r.n, pls->dTo );
	scale( ptMid, add( ptMid, ptStart, ptEnd ), 0.5 );
	SetPoint( pls->r.o, ptMid );
	scale( ptMid, sub( ptMid, ptEnd, ptStart ), 0.5 );
	SetPoint( pls->r.n, ptMid );
	pls->dFrom = -1;
	pls->dTo = 1;
}

void ServerBalanceALine( INDEX iWorld, INDEX iLine )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_MYLINESEG pls = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine );
	BalanceLine( pls );
	MarkLineUpdated( iWorld, iLine );
	//SendLineChanged( iWorld, iWall, iLine, new_seg, FALSE );
}

void GetLineData( INDEX iWorld, INDEX iLine, PFLATLAND_MYLINESEG *ppls )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	(*ppls) = GetSetMember( FLATLAND_MYLINESEG, &world->lines, iLine );
}

