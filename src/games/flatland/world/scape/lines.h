#ifndef __LINE_DEFINED__
#define __LINE_DEFINED__

#include <vectlib.h>

#include <sack_types.h>
#include <worldstrucs.h>

#define LINE_STRUC_DEFINED

typedef struct linesegfile_tag { // information in the file about a lineseg
	RAY r;
	RCOORD start, end;
} LINESEGFILE, *PLINESEGFILE;

INDEX SrvrCreateOpenLine( _32 client_id, INDEX world, _POINT o, _POINT n );
INDEX SrvrDuplicateLine( _32 client_id, INDEX world, INDEX ilsDup );

// the client side will never invoke this method
// these are safe to keep non-indexed...
int IntersectLines( PFLATLAND_MYLINESEG pLine1, int bEnd1, PFLATLAND_MYLINESEG pLine2, int bEnd2 );
void DumpLine( PFLATLAND_MYLINESEG pls );

// returns TRUE is an intersection exists, and FALSE of parallel or 
// skew and no intersection exists... then if TRUE, T1, T2 will 
// contain the scalar applied to the slope to get the resulting point.
int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 );
//void BalanceLine( PFLATLAND_MYLINESEG pls );
void SrvrBalanceALine( _32 client_id, INDEX iWorld, INDEX iLine );


#endif
