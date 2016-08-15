#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <sharemem.h>
#include <string.h>
#include <math.h>

//#include <.h> // output debug string...
#include "logging.h"

#include "vectlib.h"

#include "object.h"

#ifdef GCC
#define DebugBreak() asm( "int $3\n" );
#endif

//#define PRINT_FACETS
//#define PRINT_LINES
//#define PRINT_LINESEGS
//#define FULL_DEBUG
//#define NO_LOGGING
//#define DEBUG_LINK_LINES
//#define DEBUG_BIT_ERROR_LINK_LINES
//#define DEBUG_PLANE_INTERSECTION

INDEX tick;
uint64_t ticks[20];


int Parallel( PVECTOR pv1, PVECTOR pv2 );
void DumpPlane( PFACET pp );
void DumpLine( PMYLINESEG pl );


#ifdef __WATCOMC__
RCOORD f(void)
{
	// use some function to load floating support
	return sin(30.0);
}
#endif

// ---------------------------------
// unused - original derivation 
// new product is shorthand(?)  

// set origin of pResult to the intersecting point
// of pL1, and pL2 assuming they are not skew
int FindIntersection( PVECTOR presult,
                       PVECTOR s1, PVECTOR o1, 
                       PVECTOR s2, PVECTOR o2 )
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

   crossproduct( denoms, s1, s2 );
   denom = denoms[2];
//   denom = ( nd * nb ) - ( ne * na );
   if( NearZero( denom ) )
   {
      denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
      if( NearZero( denom ) )
      {
         denom = denoms[0];
//         denom = ( ne * nc ) - ( nb * nf );
         if( NearZero( denom ) )
         {
            SetPoint( presult, VectorConst_0 );
            return FALSE;
         }
         else
         {
            t1 = ( ne * ( f - c ) + nf * ( e - b ) ) / -denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         t1 = ( nd * ( f - c ) + nf * ( d - a ) ) / -denom;
         t2 = ( na * ( c - f ) + nc * ( a - d ) ) / denom;
      }
   }
   else
   {
      t1 = ( nd * ( e - b ) + ne * ( d - a ) ) / -denom;
      t2 = ( na * ( b - e ) + nb * ( a - d ) ) / denom;
   }

   scale( R1, s1, t1 );
   add  ( R1, R1         , o1 );

   scale( R2, s2, t2 );
   add  ( R2, R2         , o2 );


   if( ( !COMPARE(R1[0] , R2[0]) ) ||
       ( !COMPARE(R1[1] , R2[1]) ) ||
       ( !COMPARE(R1[2] , R2[2]) ) )
   {
      SetPoint( presult, VectorConst_0 );
      return FALSE;
   }
   SetPoint( presult, R1);
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

// intersection of lines - assuming lines are 
// relative on the same plane....

//int FindIntersectionTime( RCOORD *pT1, LINESEG pL1, RCOORD *pT2, PLINE pL2 )

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

   crossproduct(denoms, s1, s2 ); // - result...
   denom = denoms[2];
//   denom = ( nd * nb ) - ( ne * na );
   if( NearZero( denom ) )
   {
      denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
      if( NearZero( denom ) )
      {
         denom = denoms[0];
//         denom = ( ne * nc ) - ( nb * nf );
         if( NearZero( denom ) )
         {
#ifdef FULL_DEBUG
            sprintf( (char*)byBuffer,"Bad!-------------------------------------------\n");
            Log( (char*)byBuffer );
#endif
            return FALSE;
         }
         else
         {
            DebugBreak();
            t1 = ( ne * ( c - f ) + nf * ( b - e ) ) / denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         DebugBreak();
         t1 = ( nd * ( c - f ) + nf * ( d - a ) ) / denom;
         t2 = ( na * ( c - f ) + nc * ( d - a ) ) / denom;
      }
   }
   else
   {
      // this one has been tested.......
      t1 = ( nd * ( b - e ) + ne * ( d - a ) ) / denom;
      t2 = ( na * ( b - e ) + nb * ( d - a ) ) / denom;
   }

   R1[0] = a + na * t1;
   R1[1] = b + nb * t1;
   R1[2] = c + nc * t1;

   R2[0] = d + nd * t2;
   R2[1] = e + ne * t2;
   R2[2] = f + nf * t2;

   if( ( !COMPARE(R1[0],R2[0]) ) ||
       ( !COMPARE(R1[1],R2[1]) ) ||
       ( !COMPARE(R1[2],R2[2]) ) )
   {
      return FALSE;
   }
   *pT2 = t2;
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


int Parallel( PVECTOR pv1, PVECTOR pv2 )
{
   RCOORD a,b,c,cosTheta; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

   a = dotproduct( pv1, pv2 );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
	{
#ifdef DEBUG_PLANE_INTERSECTION
		Log( "Planes are not parallel" );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   sprintf( (char*)byBuffer, " a: %g b: %g c: %g cos: %g \n", a, b, c, cosTheta );
   Log( (char*)byBuffer );
#endif
   if( cosTheta > 0.99999 ||
       cosTheta < -0.999999 ) // not near 0degrees or 180degrees (aligned or opposed)
   {
      return TRUE;  // near 1 is 0 or 180... so IS parallel...
   }
   return FALSE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...
RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,  // line m, b
                            PCVECTOR n, PCVECTOR o,  // plane n, o
										RCOORD *time DBG_PASS )
#define IntersectLineWithPlane( s,o,n,o2,t ) IntersectLineWithPlane(s,o,n,o2,t DBG_SRC )
{
   RCOORD a,b,c,cosPhi, t; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

	//cosPhi = CosAngle( Slope, n );

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
	{
		//Log1( DBG_FILELINEFMT "Bad choice - slope vs normal is 0" DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( "Slope and or n are near 0" );
		return FALSE; // bad vector choice - if near zero length...
	}

   cosPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

//   sprintf( (char*)byBuffer, " a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n", a, b, c, t, cosTheta,
//                  pl->dFrom, pl->l.dTo );
//   Log( (char*)byBuffer );

//   if( cosTheta > e1 ) //global epsilon... probably something custom

//#define 

   if( cosPhi > 0 ||
       cosPhi < 0 ) // at least some degree of insident angle
   {
      *time = t;
      return cosPhi;
   }
   else
	{
		Log1( "Parallel... %g\n", cosPhi );
      PrintVector( Slope );
      PrintVector( n );
      // plane and line are parallel if slope and normal are perpendicular
//      sprintf( (char*)byBuffer,"Parallel...\n");
//      Log( (char*)byBuffer );
      return 0;
   }
   return TRUE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...


int GetLineSeg( OBJECTINFO *oi )
{
   return GetIndexFromSet( MYLINESEG, oi->ppLinePool );
}


int GetLineSegP( PLINESEGPSETSET *pplpss, PLINESEGPSET *pplps )
{
	PLINESEGP plsp;
	plsp = GetFromSetPool( LINESEGP, pplpss, pplps );
   return GetMemberIndex( LINESEGP, pplps, plsp );
}

int FillLine( PVECTOR o1, PVECTOR n1,
              PVECTOR o2, PVECTOR n2, 
              PRAY prl, // alternate of origin1 may be used...
              PVECTOR o_origin1 )
{
   int ret;
   RCOORD time;
   VECTOR vnp1, vnp2; // vector normal perpendicular

  if( Parallel( n1, n2 ) )
  {
#ifdef FULL_DEBUG
     Log( "ABORTION! \n");
#endif
     return 0;
  }
   crossproduct( prl->n, n1, n2 );

   // this is the slope of the normal of the line...
   // or a perpendicular ray to the line... no origins - just slopes...

   // this is the line normal on the first plane...
   crossproduct( vnp1, prl->n, n1 );

   // compute normal in second plane of the line
   crossproduct( vnp2, prl->n, n2 );

   // the origin of the perpendicular vector to the normal vector
   // is the end of the normal vector.
   ret = 0;

   if( IntersectLineWithPlane( vnp2, o2,
                               n1, o1, &time ) )  // unless parallel....
   {
		VECTOR v;
      scale( v, vnp2, time );
      add( prl->o, v, o2 ); 
      ret++;
	}
	else
	{
		Log( "Intersect failed between..." );

	}
   // this origin should be valid...

   if( IntersectLineWithPlane( vnp1, o1,
                               n2, o2, &time ) )  // unless parallel....
   {
      VECTOR v;
      scale( v, vnp1, time );
      add( o_origin1, v, o1 );
      ret++;
   }
   return ret;
}


void AddLineToPlane( OBJECTINFO *oi, PFACET pf, PMYLINESEG pl )
{
	PLINESEGP plp;
	plp = GetFromSetPool( LINESEGP, oi->ppLineSegPPool, &pf->pLineSet );
	//lprintf( "... %d", CountUsedInSet( LINESEGP, pf->pLineSet ) );
	plp->pLine = pl;
	plp->nLineFrom = -1;
	plp->nLineTo = -1;
}

PMYLINESEG CreateLine( OBJECTINFO *oi,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo )
{
   PMYLINESEG pl;
   pl = GetFromSet( MYLINESEG, oi->ppLinePool );
   pl->bDraw = TRUE;
   pl->l.dFrom = rFrom;
   pl->l.dTo = rTo;
   SetPoint( pl->l.r.o, po );
   SetPoint( pl->l.r.n, pn );

   return pl;
}

// create line is passes a base pointer to an array of planes
// and 2 indexes into that array to intersect.  This is great
// for multi-segmented intersections with different base
// pointers for simplified objects....(I guess...)
  // this merely provides the line of intersection
  // does not result in any terminal caps on the line....
PMYLINESEG CreateLineBetweenFacets( OBJECTINFO *oi, PFACET pp1, PFACET pp2 )
{
   MYLINESEG t; // m (slope) of (Int)ersection
//   LINESEG l1, l2;
   PMYLINESEG pl;
   VECTOR tv;
   //PFACET pp1 = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[np1].nFacet]
   //	  , pp2 = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[np2].nFacet];
   // t is the slope of the plane which each normal and a 0,0,0
   // origin create.
#ifdef PRINT_LINES
   sprintf( (char*)byBuffer,"Line: p1.Normal, p1.Origin, p2.Normal p2.Origin\n");
   Log( (char*)byBuffer );
   DumpPlane( pp1 );
   DumpPlane( pp2 );
#endif
  // slope of the intersection

  if( Parallel( pp1->d.n, pp2->d.n ) )
  {
#ifdef FULL_DEBUG
     Log( "ABORTION! \n");
#endif
     return NULL;
  }
  if( FillLine( pp1->d.o, pp1->d.n,
               pp2->d.o, pp2->d.n,
               &t.l.r,
               tv ) == 2 )
   {
      //lprintf( "..." );
      pl = GetFromSet( MYLINESEG, oi->ppLinePool );

      pl->l.dFrom = NEG_INFINITY;
      pl->l.dTo = POS_INFINITY;
      pl->l.r = t.l.r;
      // use other origin?
		// SetPoint( pl1->d.o, tv);

		//PrintVector( pl->l.r.o );  // Origin is resulting transformation
		//PrintVector( pl->l.r.n );   // Slope is resulting transformation
      //lprintf( "Addline to plane %p,%p", pp1, pp2 );
      AddLineToPlane( oi, pp1, pl );
      AddLineToPlane( oi, pp2, pl );
      //lprintf( "..." );

      return pl; // could return pl2 (?)
   }
   else
   {
      Log( "NON-SYMMETRIC!\n" );
   }
   return NULL;
}



PFACET AddPlaneToSet( OBJECTINFO *oi,  PCVECTOR origin, PCVECTOR norm, int d )
{
   PFACET pf = GetFromSet( FACET, oi->ppFacetPool );
   pf->bDraw = TRUE;

   SetPoint( pf->d.n, norm );
   normalize( pf->d.n );
   SetPoint( pf->d.o, origin );

   if( d > 0 )
   {
      pf->bInvert = FALSE;
      pf->bDual = FALSE;
   }
   else if( d < 0 )
   {
      pf->bInvert = TRUE;
      pf->bDual = FALSE;
   }
   else
   {
      pf->bDual = TRUE;
   }
   AddLink( &oi->facets, pf );
   return pf; // positive index of plane created reference.
}

void DumpLine( PMYLINESEG pl )
{
#ifdef PRINT_LINESEGS
   sprintf( (char*)byBuffer," ---- LINESEG %p ---- ", pl );
   Log( (char*)byBuffer );
   PrintVector( pl->l.r.o );  // Origin is resulting transformation
   PrintVector( pl->l.r.n );   // Slope is resulting transformation
   sprintf( (char*)byBuffer," From: %g To: %g\n", pl->l.dFrom, pl->l.dTo );
   Log( (char*)byBuffer );
#endif
}

void DumpPlane( PFACET pp )
{
#ifdef PRINT_FACETS
   sprintf( (char*)byBuffer,"  -----  FACET %p -----", pp );
   Log( (char*)byBuffer );
   PrintVector( pp->d.o );
   PrintVector( pp->d.n );
#endif
}

uintptr_t CPROC IfLineDelete( POINTER p, uintptr_t pData )
{
	struct procdata_tag {
      PLINESEGPSET *pplsps;
		PMYLINESEG pl;
      PFACET facet;
	} *thing = (struct procdata_tag *)pData;
	PLINESEGP plsp = (PLINESEGP)p;
   int i = GetMemberIndex( LINESEGP, thing->pplsps, plsp );
	if( plsp->pLine == thing->pl )
	{
#ifdef DEBUG_LINK_LINES
		//_xlprintf( 1 DBG_RELAY )("object %p facet %d Deleting line index %d which is linked to %d,%d"
		//								, oi, nf, i, plsp->nLineFrom, plsp->nLineTo );
#endif
		if( plsp->nLineTo != -1 )
		{
			PLINESEGP pTo = GetSetMember( LINESEGP, &thing->facet->pLineSet, plsp->nLineTo );
			if( pTo->nLineTo == i )
				pTo->nLineTo = -1;
			else if( pTo->nLineFrom == i )
				pTo->nLineFrom = -1;
			else
			{
				lprintf( "Line link is one way?" );
				DebugBreak();
			}
		}
		if( plsp->nLineFrom != -1 )
		{
			PLINESEGP pTo = GetSetMember( LINESEGP, &thing->facet->pLineSet, plsp->nLineFrom );
			if( pTo->nLineTo == i )
				pTo->nLineTo = -1;
			else if( pTo->nLineFrom == i )
				pTo->nLineFrom = -1;
			else
			{
				lprintf( "Line link is one way?" );
				DebugBreak();
			}
		}
		plsp->pLine = NULL;
      DeleteFromSet( LINESEGP, *thing->pplsps, plsp );
      return 0;
	}
   return 0;
}

#if 0
uintptr_t CPROC IfFacetDelete( POINTER p, uintptr_t psvData )
{
	struct procdata_tag {
		int nf;
      int nfs;
	} *data = (struct procdata_tag*)psvData;
	PFACETREF pfr = (PFACETREF)p;
	if( pfr->nFacet == data->nf &&
		pfr->nFacetSet == data->nfs )
	{
		pfr->nFacet = -1;
		pfr->nFacetSet = -1;
      // found the facet in the set... deleted, done.
      return 1;
	}
   return 0;
}
#endif

#if 0
uintptr_t CPROC IfSomethingUsed( POINTER p, uintptr_t psvUnused )
{
	PFACETREF pfr = (PFACETREF)p;
	if( pfr->nFacet < 0 )
      return 0;
   return 1;
}
#endif

void DeleteLineEx( OBJECTINFO *oi, PFACET facet, PMYLINESEG pl DBG_PASS )
#define DeleteLine(oi,pf,nl) DeleteLineEx(oi,pf,nl DBG_SRC )
{
	//PFACETREFSET *ppfrs;
	struct procdata_tag {
      PLINESEGPSET *pplsps;
		PMYLINESEG pl;
		PFACET facet;
	} data;
	//PFACET facet = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[nf].nFacet];
   data.pplsps = &facet->pLineSet;
	data.facet = facet;
   data.pl = pl;
	ForAllInSet( LINESEGP, &facet->pLineSet, IfLineDelete, (uintptr_t)&data );
	//pLine = GetSetMember( MYLINESEG, oi->ppLinePool, pl );
	//data2.pf = pf;


	// check to see if the line is still contained in any planes...
#if 0
   if( !ForAllInSet( FACETREF, &pLine->frs, IfSomethingUsed, 0 ) )
	{
		PMYLINESEG pls = GetSetMember( MYLINESEG, oi->ppLinePool, nl );
		DeleteFromSet( FACETREFSET, oi->FacetRefSetPool, pls->frs );
      DeleteSetMember( MYLINESEG, *oi->ppLinePool, nl );
	}
#endif
}


void OrderFacetLines( OBJECTINFO *oi )
{
	int nl, nfirst;
	INDEX idx;
	PFACET pf;

   LIST_FORALL( oi->facets, idx, PFACET, pf )
	{
		PLINESEGPSET *pplps = &pf->pLineSet;
		int nfrom = -1;
		nfirst = -1;

		nl = 0;
      while( (nfirst < 0) || (nl != nfirst) )
		{
			PLINESEGP plsp = GetUsedSetMember( LINESEGP, pplps, nl );
			if( !plsp )
			{
				lprintf( "Failed linked order... " );
				break ;
			}
			if( !plsp->pLine )
			{
				lprintf( "object %p facet %d Invalid line segment %d (%p)... skipping.", oi, idx, nl, plsp->pLine );
            nl++;
				continue;  // unused line...
			}
			if( nfirst < 0 )
			{
				nfirst = nl;
				nfrom = nl;
				nl = plsp->nLineTo;
				if( nl > 500 )
               DebugBreak();
#ifdef DEBUG_LINK_LINES
				lprintf( "object %p facet %d first = %d from = %d nl = %d", oi, idx, nfirst, nfrom, nl );
#endif
            continue;
			}
			if( ( plsp->nLineFrom == nl )
				|| ( plsp->nLineTo == nl) )
			{
            lprintf( "One end of this line links to itself?" );
			}
			if( plsp->nLineFrom == nfrom )
				plsp->bOrderFromTo = TRUE;
			else if( plsp->nLineTo == nfrom )
				plsp->bOrderFromTo = FALSE;
			else
			{
				lprintf( "object %p facet %d The line seg at From doesn't link to this one?! %d %d != %d != %d"
                    , oi
                    , idx
					 , nl
					 , plsp->nLineFrom
					 , nfrom
					 , plsp->nLineTo
					 );
			}
			nfrom = nl;
         if( plsp->bOrderFromTo )
				nl = plsp->nLineTo;
         else
				nl = plsp->nLineFrom;
			if( nl > 500 )
            DebugBreak();
#ifdef DEBUG_LINK_LINES
			Log3( "first = %d from = %d nl = %d", nfirst, nfrom, nl );
#endif
			if( nl == nfrom )
			{
            lprintf( "Self referenced line..." );
			}
		}
		{
			PLINESEGP plsp = GetUsedSetMember( LINESEGP, pplps, nfirst );
			if( !plsp )
			{
				lprintf( "line at %d failed...", nfirst );
            break;
			}
			if( plsp->nLineFrom == nfrom )
				plsp->bOrderFromTo = TRUE;
			else if( plsp->nLineTo == nfrom )
				plsp->bOrderFromTo = FALSE;
			else
			{
				lprintf( "The line seg at From doesn't link to this one?! %d %d != %d != %d"
					 , nfirst
					 , plsp->nLineFrom
					 , nfrom
					 , plsp->nLineTo
					 );
			}
		}
#ifdef DEBUG_LINK_LINES
      //Log1( "Facet %d", nf );
		//for( nl = 0; nl < plps->nUsedLines; nl++ )
		//{
      //   PLINESEGP plsp = GetSetMember( LINESEGP, pplps, nl );
      //   if( plsp->bOrderFromTo )
		//		Log2( "Resulting links: %d %d"
		//			 , plsp->nLineFrom
		//			 , plsp->nLineTo );
      //   else
		//		Log2( "Resulting links: %d %d"
		//			 , plsp->nLineTo
		//			 , plsp->nLineFrom );
		//}
#endif
	}
}


static PTRANSFORM tFailureRotation;

uintptr_t CPROC TestLinkLines2( POINTER p, uintptr_t psv )
{
	struct pd {
		OBJECTINFO *oi;
		PLINESEGPSET *pplps;
		PLINESEGP pLine1;
		int retry;
      int *pnLineLink;
		_POINT to1;
	} *data = (struct pd *)psv;
   int nl1, nl2;
	PLINESEGP pLine2 = (PLINESEGP)p;
   //lprintf( "Compare line %p with %p", data->pLine1, p );
	if( pLine2 == data->pLine1 )
	{
      // don't compare a line with itself.
      return 0;
	}
	if( !pLine2->pLine )
	{
      lprintf( "this line should have been deleted from the set..." );
		DebugBreak();
      return 0;
	}
	nl1 = GetMemberIndex( LINESEGP, data->pplps, data->pLine1 );;
   nl2 = GetMemberIndex( LINESEGP, data->pplps, pLine2 );

	if( pLine2->nLineTo < 0 )
	{
		_POINT to2;
		PMYLINESEG line = pLine2->pLine;
		addscaled( to2
					, line->l.r.o
					, line->l.r.n
					, line->l.dTo );
		if( data->retry )
		{
			_POINT tmp;
			Apply( tFailureRotation, tmp, to2 );
			SetPoint( to2, tmp );
		}
		if( Near( data->to1, to2 ) )
		{
#ifdef DEBUG_LINK_LINES
			lprintf( "object %p:Facet -- to end Linking to,to %d %d", data->oi, nl2, nl1 );
			lprintf( "<%g, %g, %g> and <%g, %g, %g> were near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#endif
#endif

			(*data->pnLineLink) = nl2;
			pLine2->nLineTo = nl1;
			if( nl2 > 500 || nl1 > 500 )
            DebugBreak();
         return 1;
		}
#ifdef DEBUG_LINK_LINES
		else
		{
			lprintf( "<%g, %g, %g> and <%g, %g, %g> were not near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#endif
		}
#endif
	}
	if( pLine2->nLineFrom < 0 )
	{
		_POINT to2;
		PMYLINESEG line = pLine2->pLine;
		addscaled( to2
					, line->l.r.o
					, line->l.r.n
					, line->l.dFrom );
		if( data->retry )
		{
			_POINT tmp;
			Apply( tFailureRotation, tmp, to2 );
			SetPoint( to2, tmp );
		}
		if( Near( data->to1, to2 ) )
		{
#ifdef DEBUG_LINK_LINES
			lprintf( "object %p:Facet -- from end Linking from,to %d %d", data->oi, nl2, nl1 );
			lprintf( "<%g, %g, %g> and <%g, %g, %g> were near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2]
					 );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#endif
#endif
			(*data->pnLineLink) = nl2;
			pLine2->nLineFrom = nl1;
			if( nl2 > 500 || nl1 > 500 )
            DebugBreak();
         return 1;
			//break;
		}
#ifdef DEBUG_LINK_LINES
		else
		{
			lprintf( "<%g, %g, %g> and <%g, %g, %g> were not near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2]
					 );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#endif
		}
#endif
	}
   return 0;
}


uintptr_t CPROC TestLinkLines1( POINTER p, uintptr_t psv )
{
	struct pd {
		OBJECTINFO *oi;
      PLINESEGPSET *pplps;
      PLINESEGP pLine1;
		int retry;
      int *pnLineLink;
		_POINT to1;
	} *data = (struct pd *)psv;
   data->pLine1 = p;
	data->retry = 0;
   //lprintf( "Compare line %p with ...", p );
	if( !data->pLine1->pLine )
	{
		// this should be removed from the set already...
      // migrating to NEW sets.
      DebugBreak();
		return 0;
	}
   //nl1 = GetMemberIndex( LINESEGP, data->pplps, p );
	while( data->retry < 2 )
	{
#ifdef DEBUG_LINK_LINES
		lprintf( "retry # %d", data->retry );
#endif
		if( data->pLine1->nLineTo < 0 )
		{
			PMYLINESEG line = data->pLine1->pLine;
			addscaled( data->to1
						, line->l.r.o
						, line->l.r.n
						, line->l.dTo );
			if( data->retry )
			{
				_POINT tmp;
				Apply( tFailureRotation, tmp, data->to1 );
				SetPoint( data->to1, tmp );
			}
#ifdef DEBUG_LINK_LINES
			lprintf( "Link TO end of(%p) at %g", p,line->l.dTo );
#endif
         data->pnLineLink = &data->pLine1->nLineTo;
			ForAllInSet( LINESEGP, *data->pplps, TestLinkLines2, (uintptr_t)data );
		}
		if( data->pLine1->nLineFrom < 0 )
		{
			PMYLINESEG line = data->pLine1->pLine;
#ifdef DEBUG_LINK_LINES
			lprintf( "Link FROM end of (%p) at %g", p, line->l.dFrom );
#endif
			addscaled( data->to1
						, line->l.r.o
						, line->l.r.n
						, line->l.dFrom );
			if( data->retry )
			{
				_POINT tmp;
				Apply( tFailureRotation, tmp, data->to1 );
				SetPoint( data->to1, tmp );
			}
         data->pnLineLink = &data->pLine1->nLineFrom;
			ForAllInSet( LINESEGP, *data->pplps, TestLinkLines2, (uintptr_t)data );
		}
		if( ( ( data->pLine1->nLineFrom < 0 )
			  && ( data->pLine1->nLineTo >= 0 ) )
			|| ( ( data->pLine1->nLineFrom >= 0 )
				 && ( data->pLine1->nLineTo < 0 ) ) )
		{
#ifdef DEBUG_LINK_LINES
			//lprintf( "line %ld not linked.", GetMemberIndex( LINESEGP, data->pplps, p ) );
#endif
			lprintf( "line %p not linked.", GetMemberIndex( LINESEGP, data->pplps, p ) );
			if( data->retry )
			{
			  // DeleteFromSet( LINESEGP, data->pplps, pLine1 );
				//DebugBreak();
			}
		}
		else if( data->pLine1->nLineFrom >= 0 &&
				  data->pLine1->nLineTo >= 0 ) // end the retry loop...
         return 0;
			//break;
		data->retry++;
	} // end while(retry<2)
   return 0;
}

uintptr_t CPROC TestLinked( POINTER p, uintptr_t psv )
{
	struct pd {
		OBJECTINFO *oi;
      PLINESEGPSET *pplps;
      PLINESEGP pLine1;
		int retry;
      int *pnLineLink;
		_POINT to1;
	} *data = (struct pd *)psv;
	PLINESEGP pLine = (PLINESEGP)p;
#ifdef DEBUG_LINK_LINES
	lprintf( "Resulting links: this %p from %d to %d"
			 , pLine->pLine
			 , pLine->nLineFrom
			 , pLine->nLineTo );
#endif
	if( pLine->nLineFrom < 0 || pLine->nLineTo < 0 )
	{
		//if( pLine->nLineFrom < 0 && pLine->nLineTo < 0 )
		{
#ifdef DEBUG_LINK_LINES
			//lprintf( "...%d", nl1 );
#endif
			lprintf( "... delete line %p", pLine );
         DeleteFromSet( LINESEGP, *data->pplps, pLine );
		}
	}
#if 0
		else if( pLine->nLineFrom < 0 )
		{
			// uhmm okay fake it...
			lprintf( "Failed to link FROM segment %d", nl1 );
			for( nl2 = nl1 + 1; nl2 < plps->nUsedLines; nl2++ )
			{
				if( plps->pLines[nl2].nLineFrom < 0 )
				{
					lprintf( "Found another unused end to use... faking link." );
					pLine->nLineFrom = nl2;
					plps->pLines[nl2].nLineFrom = nl1;
					break;
				}
				if( plps->pLines[nl2].nLineTo < 0 )
				{
					lprintf( "Found another unused end to use... faking link." );
								pLine->nLineFrom = nl2;
								plps->pLines[nl2].nLineTo = nl1;
                        break;
				}
				else if( pLine->nLineTo < 0 )
				{
               lprintf( "Failed to link TO segment %d", nl1 );
					for( nl2 = nl1 + 1; nl2 < plps->nUsedLines; nl2++ )
					{
						if( plps->pLines[nl2].nLineFrom < 0 )
						{
                     lprintf( "Found another unused end to use... faking link." );
								pLine->nLineTo = nl2;
								plps->pLines[nl2].nLineFrom = nl1;
                        break;
						}
						if( plps->pLines[nl2].nLineTo < 0 )
						{
                     lprintf( "Found another unused end to use... faking link." );
								pLine->nLineTo = nl2;
								plps->pLines[nl2].nLineTo = nl1;
                        break;
						}
					}
				}
			}
		}
#endif
	return 0;
}

void LinkFacetLines( OBJECTINFO *oi )
{
	struct pd {
		OBJECTINFO *oi;
      PLINESEGPSET *pplps;
		PLINESEGP pLine1;
      int retry;
      int *pnLineLink;
		_POINT to1;
	} data ;
	int nf;
	PFACET pf;
	//PFACETPSET pfps = oi->FacetSetPool.pFacetSets + nfs;
	if( !tFailureRotation )
	{
		tFailureRotation = CreateTransform();
      PrintMatrix( tFailureRotation );
		// some arbitrary rotation factor... this allows us to translate
      // near points bounding origins to spaces where 0 is not involved...
		RotateAbs( tFailureRotation, 0.5, 0.5, 0.5 );
      //Translate( tFailureRotation, 1, 1, 1 );
      PrintMatrix( tFailureRotation );
	}
   LIST_FORALL( oi->facets, nf, PFACET, pf )
	{
      //int retry;
      // eventually we'll move this to the better set type also...
		data.oi = oi;
      // just need to delete the set set...
		data.pplps = &pf->pLineSet;
		data.pLine1 = NULL; // a scratch var, doesn't need init...
      //lprintf( "link facet %p", pf );
		ForAllInSet( LINESEGP, *data.pplps, TestLinkLines1, (uintptr_t)&data );
		ForAllInSet( LINESEGP, *data.pplps, TestLinked, (uintptr_t)&data );
	}
}

uintptr_t CPROC DeleteLinePSeg( POINTER p, uintptr_t psv )
{
	struct d{
		PFACET pf;
		POBJECTINFO oi;
	} *data = (struct d*)psv;
   PLINESEGP plsp = (PLINESEGP)p;
	DeleteLine( data->oi, data->pf, plsp->pLine );
   return 0;
}

int IntersectPlanes( OBJECTINFO *oi, int bAll )
{
   int i, j, k;
   //PFACETPSET pfps;
   PFACET pf;
   PMYLINESEG  pl;

	INDEX idx;
	//lprintf( "..." );
	//pfps = oi->FacetSetPool.pFacetSets + nfs;
   // clear all lines used by this facetset 
	LIST_FORALL( oi->facets, idx, PFACET, pf )
   {
		struct {
			PFACET pf;
			POBJECTINFO oi;
		} data;
      data.pf = pf;
      data.oi = oi;
      ForAllInSet( LINESEGP, pf->pLineSet, DeleteLinePSeg, (uintptr_t)&data );
   }
   //lprintf( "... %d", pfps->nUsedFacets );
   // for all combinations of planes intersect them.
	MarkTick( ticks[tick++] );
	LIST_FORALL( oi->facets, i, PFACET, pf )
	{
		PFACET pf2;
		j = i;
      LIST_NEXTALL( oi->facets, j, PFACET, pf2 )
      {
			PFACET pf3;
#ifdef FULL_DEBUG
         sprintf( (char*)byBuffer, "------------------------------------------------------------\n");
         Log( (char*)byBuffer );
         sprintf( (char*)byBuffer, "Between %d and %d", i, j );
         Log( (char*)byBuffer );
#endif
         // if NO line exists between said planes, line will not be created...
         pl = CreateLineBetweenFacets( oi, pf, pf2 );
                           // link 2 lines together...
                           // create all possible intersections
			if( !pl )  // no line intersecting...
			{
            //lprintf( "plane intersection failed... %d %d\n", i, j );
				continue;
			}
			{
				k = j;
            LIST_FORALL( oi->facets, k, PFACET, pf3 )
				{
               //pl->frs
					RCOORD time, s;
					VECTOR n;
               // don't compare either plane itself.
					if( k == i || k == j )
                  continue;
					SetPoint( n, pf3->d.n );
					if( pf3->bInvert )
						Invert( n );

					if( s = IntersectLineWithPlane( pl->l.r.n
															, pl->l.r.o
															, n
															, pf3->d.o
															, &time ) )
					{
						RCOORD dp;

#ifdef DEBUG_LINK_LINES
							{
								VECTOR x;
								lprintf( "object %p facet %d %d and %d Intersect time %g", oi, i, j, k, time );
								addscaled( x, pl->l.r.o, pl->l.r.n, time );
								PrintVector( x );
							}
#endif
                  	if( ( dp = dotproduct( pl->l.r.n, n ) ) > 0 )
                  	{
                  		if( time < pl->l.dTo )
                  		{
	                  		pl->l.dTo = time;
	                  	}
	                  }
                  	else
                  	{
                  		if( time > pl->l.dFrom )
                  		{
                  			pl->l.dFrom = time;
                  		}
                  	}
                  }
                  else
                  {
                  	// didn't form a point - but the line resulting above 
                  	// cannot be above any other plane SO... test for 
                  	// line above plane...
      					if( AbovePlane( pf3->d.o
      					              , pf3->d.n
      					              , pl->l.r.o ) )
      					{
      						// setup conditions to have the line deleted.
      						//k = pfps->nUsedFacets;
      						pl->l.dFrom = 1;
      						pl->l.dTo = 0;
      						break;
      					}
#ifdef FULL_DEBUG
                     //sprintf( (char*)byBuffer, "okay Facets %d, %d and %d do not form a point.\n", i,  j, k ) ;
                     //Log( (char*)byBuffer );
#endif
						}
				}
				// pf3 will be NULL at end of loop, or at empty list...
				// otherwise non-null pf3 indicates a bail.
				if( pf3 &&
					pl->l.dFrom >  pl->l.dTo )
				{
					lprintf( "DELETEING LINE - IS NOT LINKED" );
					DeleteLine( oi, pf, pl );
					DeleteLine( oi, pf2, pl );
				}
			}
#ifdef DEBUG_LINK_LINES
         lprintf( "Resulting line...." );
			DumpLine( pl );
#endif
		}
	}
#ifdef DEBUG_LINK_LINES
	lprintf( "---- Link facet lines..." );
#endif
	MarkTick( ticks[tick++] );
	MarkTick( ticks[tick++] );
	LinkFacetLines( oi );
#ifdef DEBUG_LINK_LINES
	lprintf( "---- Order Facet lines just now linked..." );
#endif
	MarkTick( ticks[tick++] );
   OrderFacetLines( oi );
	MarkTick( ticks[tick++] );

   return 0;
}

int PointWithin( PCVECTOR p, PMYLINESEGSET *ppls, PLINESEGPSET *pplps )
{
   int l1, l2;
	PMYLINESEG pl1, pl2 ;
	int lines;
   lines = CountUsedInSet( LINESEGP, *pplps );
   for( l1 = 0; l1 < lines; l1++ )
   {
      VECTOR v;
		RCOORD tl, tl2;
      PLINESEGP plsp1 = GetSetMember( LINESEGP, pplps, l1 );
      pl1 = plsp1->pLine;
      sub( v, p, pl1->l.r.o );
      for( l2 = 0; l2 < lines; l2++ )
      {
         if( l1 == l2 )
				continue;
			else
			{
            PLINESEGP plsp2 = GetSetMember( LINESEGP, pplps, l2 );
				pl2 = plsp2->pLine;
				if( FindIntersectionTime( &tl, v, pl1->l.r.o
												, &tl2, pl2->l.r.n, pl2->l.r.o ) )
				{
					if( tl > 0 &&
						( tl2 > pl2->l.dFrom &&
						 tl2 < pl2->l.dTo ) )
						break;
				}
			}
      }
      if( l2 == lines )
         return FALSE;
   }
   return TRUE;
}

RCOORD PointToPlaneT( PVECTOR n, PVECTOR o, PVECTOR p ) {
   VECTOR i;
   RCOORD t;
   SetPoint( i, n );
   Invert( i );
   IntersectLineWithPlane( i, p, n, o, &t );
   return t;
}

LOGICAL AbovePlane( PVECTOR n, PVECTOR o, PVECTOR p )
{
   if( PointToPlaneT( n, o, p ) > 0 )
      return TRUE;
   else
      return FALSE;
}
       
int GetPoints( PFACET pf, int *nPoints, VECTOR ppv[] )
{
	int nlStart, nl, np;
   int lines = CountUsedInSet( LINESEGP, pf->pLineSet );
	for( nlStart = 0; nlStart < lines; nlStart++ )
	{
      PLINESEGP pLine = GetUsedSetMember( LINESEGP, &pf->pLineSet, nlStart );
		if( !pLine || !pLine->pLine )
			continue;
		break;
	}	
	nl = nlStart;
	np = 0;
   do
	{
		PLINESEGP pLine = GetUsedSetMember( LINESEGP, &pf->pLineSet, nl );
		PMYLINESEG line;
		if( !pLine )
		{
         lprintf( "Failed linkage of facet lines." );
			break;
		}
		line = pLine->pLine;
   	if( pLine->bOrderFromTo )
		{
			add( &ppv[np]
				, scale( &ppv[np], line->l.r.n
						           , line->l.dTo )
				, line->l.r.o );
			np++;
	   	nl = pLine->nLineTo;
	   }
	   else
		{
			add( &ppv[np]
				, scale( &ppv[np], line->l.r.n
						           , line->l.dFrom )
				, line->l.r.o );
			np++;
		   nl = pLine->nLineFrom;	
		}
		if( np >= *nPoints )
		{
         Log1( "No more points to fill.. %d", np );
			return  FALSE;
		}
		if( nl < 0 )
		{
         lprintf( "facet %p Aborting chain!", pf );
			break;
		}
	} while( nl != nlStart );
	//Log1( "Resulting points: %d", np );
   *nPoints = np;
   return TRUE;
}

// mod
