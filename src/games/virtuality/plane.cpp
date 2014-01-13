#define TEMP_DISABLE
#ifndef VIRTUALITY_LIBRARY_SOURCE
// enables export of symbols
#define VIRTUALITY_LIBRARY_SOURCE
#endif
#include <stdhdrs.h>
#include <math.h>

#include <logging.h>
#include <sharemem.h>

#define NEED_VECTLIB_COMPARE
#include <vectlib.h>

#include "global.h"

#include <virtuality.h>

#ifdef GCC
#define DebugBreak() asm( "int $3\n" );
#endif

//#define NO_LOGGING

//#define DEBUG_EVERYTHING
#ifdef DEBUG_EVERYTHING
#define PRINT_FACETS
#define PRINT_LINES
#define PRINT_LINESEGS
//#define FULL_DEBUG
#define DEBUG_LINK_LINES
#define DEBUG_PLANE_INTERSECTION
#endif

//#define DEBUG_BIT_ERROR_LINK_LINES

INDEX tick;
_64 ticks[20];


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
int FindInt3ersection( PVECTOR presult,
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
            lprintf(WIDE("Bad!-------------------------------------------\n"));
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
		Log( WIDE("Planes are not parallel") );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   lprintf( WIDE(" a: %g b: %g c: %g cos: %g"), a, b, c, cosTheta );
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
		//Log1( DBG_FILELINEFMT WIDE("Bad choice - slope vs normal is 0") DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( WIDE("Slope and or n are near 0") );
		return FALSE; // bad vector choice - if near zero length...
	}

   cosPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

//   lprintf( WIDE(" a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n"), a, b, c, t, cosTheta,
//                  pl->dFrom, pl->dTo );

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
		Log1( WIDE("Parallel... %g\n"), cosPhi );
		PrintVector( Slope );
		PrintVector( n );
		// plane and line are parallel if slope and normal are perpendicular
//      lprintf(WIDE("Parallel...\n"));
		return 0;
   }
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
     Log( WIDE("ABORTION! \n"));
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
		Log( WIDE("Intersect failed between...") );

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
	//lprintf( WIDE("... %d"), CountUsedInSet( LINESEGP, pf->pLineSet ) );
	plp->pLine = pl;
	plp->nLineFrom = -1;
	plp->nLineTo = -1;
}

void AddLineToObjectPlane( POBJECT po, PFACET pf, PMYLINESEG pl )
{
	AddLineToPlane( po->objinfo, pf, pl );
}

PMYLINESEG CreateLine( OBJECTINFO *oi,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo )
{
	PMYLINESEG pl;
   if( oi )
		pl = GetFromSet( MYLINESEG, oi->ppLinePool );
	else
      pl = GetFromSet( MYLINESEG, g.ppLinePool );
   AddLink( &oi->lines, pl );
   pl->bDraw = TRUE;
   pl->l.dFrom = rFrom;
   pl->l.dTo = rTo;
   SetPoint( pl->l.r.o, po );
   SetPoint( pl->l.r.n, pn );


   return pl;
}

PMYLINESEG CopyLine( OBJECTINFO *oi,
                  	 PMYLINESEG orig )
{
   PMYLINESEG pl;
   pl = GetFromSet( MYLINESEG, oi->ppLinePool );
   AddLink( &oi->lines, pl );
   pl->bDraw = TRUE;
   pl->l = orig->l;
   return pl;
}


// normal may be non normalized... but segment is always 0-1 for origin/normal
PMYLINESEG CreateNormalOnPlane( OBJECTINFO *oi, PFACET facet, RAY line )
{
   PMYLINESEG pl;
#ifdef PRINT_LINES
//   lprintf(WIDE("Line: p1.Normal, p1.Origin, p2.Normal p2.Origin\n"));
#endif
  // slope of the intersection
   if( facet->flags.bNormalSurface )
   {
      //lprintf( WIDE("...") );
      pl = GetFromSet( MYLINESEG, oi->ppLinePool );
		AddLink( &oi->lines, pl );

      // current alogrithm does max setting.
      pl->l.dFrom = 0;
      pl->l.dTo = 1;
      SetPoint( pl->l.r.n, line.n );
      SetPoint( pl->l.r.o, line.o );
      // use other origin?
		// SetPoint( pl1->d.o, tv);

		//PrintVector( pl->r.o );  // Origin is resulting transformation
		//PrintVector( pl->r.n );   // Slope is resulting transformation
      //lprintf( WIDE("Addline to plane %p,%p"), pp1, pp2 );
      AddLineToPlane( oi, facet, pl );
	  //SortNormals( pp );
      return pl; // could return pl2 (?)
   }
   return NULL;
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
   lprintf(WIDE("Line: p1.Normal, p1.Origin, p2.Normal p2.Origin\n"));
   DumpPlane( pp1 );
   DumpPlane( pp2 );
#endif
  // slope of the intersection

  if( Parallel( pp1->d.n, pp2->d.n ) )
  {
#ifdef FULL_DEBUG
     Log( WIDE("ABORTION! \n"));
#endif
     return NULL;
  }
  if( FillLine( pp1->d.o, pp1->d.n,
               pp2->d.o, pp2->d.n,
               &t.l.r,
               tv ) == 2 )
   {
      //lprintf( WIDE("...") );
      pl = GetFromSet( MYLINESEG, oi->ppLinePool );
	   AddLink( &oi->lines, pl );

      // current alogrithm does max setting.
      pl->l.dFrom = NEG_INFINITY;
      pl->l.dTo = POS_INFINITY;
      pl->l.r = t.l.r;
      // use other origin?
		// SetPoint( pl1->d.o, tv);

		//PrintVector( pl->r.o );  // Origin is resulting transformation
		//PrintVector( pl->r.n );   // Slope is resulting transformation
      //lprintf( WIDE("Addline to plane %p,%p"), pp1, pp2 );
      AddLineToPlane( oi, pp1, pl );
      AddLineToPlane( oi, pp2, pl );
      //lprintf( WIDE("...") );

      return pl; // could return pl2 (?)
   }
   else
   {
      Log( WIDE("NON-SYMMETRIC!\n") );
   }
   return NULL;
}

PMYLINESEG CreateLineBetweenPlanes( OBJECTINFO *oi, PRAY pr1, PRAY pr2 )
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
   lprintf(WIDE("Line: p1.Normal, p1.Origin, p2.Normal p2.Origin\n"));
#endif
  // slope of the intersection

  if( Parallel( pr1->n, pr2->n ) )
  {
#ifdef FULL_DEBUG
     Log( WIDE("ABORTION! \n"));
#endif
     return NULL;
  }
  if( FillLine( pr1->o, pr1->n,
               pr2->o, pr2->n,
               &t.l.r,
               tv ) == 2 )
   {
		//lprintf( WIDE("...") );
		if( oi )
		{
			pl = GetFromSet( MYLINESEG, oi->ppLinePool );
			AddLink( &oi->lines, pl );
		}
		else
         pl = GetFromSet( MYLINESEG, g.ppLinePool );

      // current alogrithm does max setting.
      pl->l.dFrom = NEG_INFINITY;
      pl->l.dTo = POS_INFINITY;
      pl->l.r = t.l.r;

      return pl; // could return pl2 (?)
   }
   else
   {
      Log( WIDE("NON-SYMMETRIC!\n") );
   }
   return NULL;
}


PFACET AddPlane( POBJECT object, PCVECTOR o, PCVECTOR n, int d )
{
   return AddPlaneToSet( object->objinfo, o, n, d );
}

PFACET AddNormalPlane( POBJECT object, PCVECTOR o, PCVECTOR n, int d )
{
	PFACET facet = AddPlaneToSet( object->objinfo, o, n, d );
	facet->flags.bNormalSurface = TRUE;
	facet->flags.bPointNormal = FALSE;
	return facet;
}

PFACET AddPlaneToSet( OBJECTINFO *oi,  PCVECTOR origin, PCVECTOR norm, int d )
{
#ifdef USE_DATA_STORE
	PFACET pf = DataStore_GetFromDataSet( iCluster, oi->cluster, iClusterFacets );
#else
	PFACET pf = GetFromSet( FACET, oi->ppFacetPool );
#endif
	pf->flags.bDraw = TRUE;

	SetPoint( pf->d.n, norm );
	normalize( pf->d.n );
	SetPoint( pf->d.o, origin );

	pf->flags.bPointNormal = TRUE;
	pf->flags.bShared = FALSE;
	pf->flags.bDual = FALSE;
	if( d > 0 )
	{
		if( d > 1 )
			pf->flags.bShared = TRUE;
		pf->flags.bInvert = FALSE;
	}
	else if( d < 0 )
	{
		if( d < -1 )
			pf->flags.bShared = TRUE;
		pf->flags.bInvert = TRUE;
	}
	else
	{
		pf->flags.bDual = TRUE;
	}
#ifdef USE_DATA_STORE
	DataStore_AddLink( iObjectInfoFacet, oi, pf );
#else
	AddLink( &oi->facets, pf );
#endif
	return pf; // positive index of plane created reference.
}

void DumpLine( PMYLINESEG pl )
{
#ifdef PRINT_LINESEGS
   lprintf(WIDE(" ---- LINESEG %p ---- "), pl );
   PrintVector( pl->l.r.o );  // Origin is resulting transformation
   PrintVector( pl->l.r.n );   // Slope is resulting transformation
   lprintf(WIDE(" From: %g To: %g\n"), pl->l.dFrom, pl->l.dTo );
#endif
}

void DumpPlane( PFACET pp )
{
#ifdef PRINT_FACETS
   lprintf(WIDE("  -----  FACET %p -----"), pp );
   PrintVector( pp->d.o );
   PrintVector( pp->d.n );
#endif
}

PTRSZVAL CPROC IfLineDelete( POINTER p, PTRSZVAL pData )
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
		//_xlprintf( 1 DBG_RELAY )(WIDE("object %p facet %d Deleting line index %d which is linked to %d,%d")
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
				lprintf( WIDE("Line link is one way?") );
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
				lprintf( WIDE("Line link is one way?") );
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
PTRSZVAL CPROC IfFacetDelete( POINTER p, PTRSZVAL psvData )
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
PTRSZVAL CPROC IfSomethingUsed( POINTER p, PTRSZVAL psvUnused )
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
	ForAllInSet( LINESEGP, &facet->pLineSet, IfLineDelete, (PTRSZVAL)&data );
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
#ifdef DEBUG_LINK_LINES
				lprintf( WIDE("Failed linked order... ") );
#endif
				break ;
			}
			if( !plsp->pLine )
			{
				lprintf( WIDE("object %p facet %d Invalid line segment %d (%p)... skipping."), oi, idx, nl, plsp->pLine );
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
				lprintf( WIDE("object %p facet %d first = %d from = %d nl = %d"), oi, idx, nfirst, nfrom, nl );
#endif
            continue;
			}
			if( ( plsp->nLineFrom == nl )
				|| ( plsp->nLineTo == nl) )
			{
            lprintf( WIDE("One end of this line links to itself?") );
			}
			if( plsp->nLineFrom == nfrom )
				plsp->bOrderFromTo = TRUE;
			else if( plsp->nLineTo == nfrom )
				plsp->bOrderFromTo = FALSE;
			else
			{
				lprintf( WIDE("object %p facet %d The line seg at From doesn't link to this one?! %d %d != %d != %d")
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
			Log3( WIDE("first = %d from = %d nl = %d"), nfirst, nfrom, nl );
#endif
			if( nl == nfrom )
			{
            lprintf( WIDE("Self referenced line...") );
			}
		}
		{
			PLINESEGP plsp = GetUsedSetMember( LINESEGP, pplps, nfirst );
			if( !plsp )
			{
#ifdef DEBUG_LINK_LINES
				lprintf( WIDE("line at %d failed..."), nfirst );
#endif
            break;
			}
			if( plsp->nLineFrom == nfrom )
				plsp->bOrderFromTo = TRUE;
			else if( plsp->nLineTo == nfrom )
				plsp->bOrderFromTo = FALSE;
			else
			{
				lprintf( WIDE("The line seg at From doesn't link to this one?! %d %d != %d != %d")
					 , nfirst
					 , plsp->nLineFrom
					 , nfrom
					 , plsp->nLineTo
					 );
			}
		}
#ifdef DEBUG_LINK_LINES
      //Log1( WIDE("Facet %d"), nf );
		//for( nl = 0; nl < plps->nUsedLines; nl++ )
		//{
      //   PLINESEGP plsp = GetSetMember( LINESEGP, pplps, nl );
      //   if( plsp->bOrderFromTo )
		//		Log2( WIDE("Resulting links: %d %d")
		//			 , plsp->nLineFrom
		//			 , plsp->nLineTo );
      //   else
		//		Log2( WIDE("Resulting links: %d %d")
		//			 , plsp->nLineTo
		//			 , plsp->nLineFrom );
		//}
#endif
	}
}

static PTRANSFORM tFailureRotation;

PTRSZVAL CPROC TestLinkLines2( POINTER p, PTRSZVAL psv )
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
   //lprintf( WIDE("Compare line %p with %p"), data->pLine1, p );
	if( pLine2 == data->pLine1 )
	{
      // don't compare a line with itself.
      return 0;
	}
	if( !pLine2->pLine )
	{
      lprintf( WIDE("this line should have been deleted from the set...") );
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
			Apply( (PCTRANSFORM)tFailureRotation, tmp, to2 );
			SetPoint( to2, tmp );
		}
		if( Near( data->to1, to2 ) )
		{
#ifdef DEBUG_LINK_LINES
			lprintf( WIDE("object %p:Facet -- to end Linking to,to %d %d"), data->oi, nl2, nl1 );
			lprintf( WIDE("<%g, %g, %g> and <%g, %g, %g> were near")
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( WIDE("(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were near")
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
			lprintf( WIDE("<%g, %g, %g> and <%g, %g, %g> were not near")
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( WIDE("(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near")
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
#ifdef DEBUG_LINK_LINES
			lprintf( "in retry; applying rotation to get second match" );
#endif
			Apply( tFailureRotation, tmp, to2 );
			SetPoint( to2, tmp );
		}
		if( Near( data->to1, to2 ) )
		{
#ifdef DEBUG_LINK_LINES
			lprintf( WIDE("object %p:Facet -- from end Linking from,to %d %d"), data->oi, nl2, nl1 );
			lprintf( WIDE("<%g, %g, %g> and <%g, %g, %g> were near")
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2]
					 );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( WIDE("(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were near")
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
			lprintf( WIDE("<%g, %g, %g> and <%g, %g, %g> were not near")
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2]
					 );
#ifdef DEBUG_BIT_ERROR_LINK_LINES
			lprintf( WIDE("(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near")
					 , data->to1[0], data->to1[1], data->to1[2]
					 , to2[0], to2[1], to2[2] );
#endif
		}
#endif
	}
   return 0;
}


PTRSZVAL CPROC TestLinkLines1( POINTER p, PTRSZVAL psv )
{
	struct pd {
		OBJECTINFO *oi;
		PLINESEGPSET *pplps;
		PLINESEGP pLine1;
		int retry;
		int *pnLineLink;
		_POINT to1;
	} *data = (struct pd *)psv;
	data->pLine1 = (PLINESEGP)p;
	data->retry = 0;
   //lprintf( WIDE("Compare line %p with ..."), p );
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
		lprintf( WIDE("retry # %d"), data->retry );
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
			lprintf( WIDE("Link TO end of(%p) at %g"), p,line->l.dTo );
#endif
			data->pnLineLink = &data->pLine1->nLineTo;
			ForAllInSet( LINESEGP, *data->pplps, TestLinkLines2, (PTRSZVAL)data );
		}
		if( data->pLine1->nLineFrom < 0 )
		{
			PMYLINESEG line = data->pLine1->pLine;
#ifdef DEBUG_LINK_LINES
			lprintf( WIDE("Link FROM end of (%p) at %g"), p, line->l.dFrom );
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
			ForAllInSet( LINESEGP, *data->pplps, TestLinkLines2, (PTRSZVAL)data );
		}
		if( ( ( data->pLine1->nLineFrom < 0 )
			  && ( data->pLine1->nLineTo >= 0 ) )
			|| ( ( data->pLine1->nLineFrom >= 0 )
				 && ( data->pLine1->nLineTo < 0 ) ) )
		{
#ifdef DEBUG_LINK_LINES
			//lprintf( WIDE("line %ld not linked."), GetMemberIndex( LINESEGP, data->pplps, p ) );
			lprintf( WIDE("line %p not linked."), GetMemberIndex( LINESEGP, data->pplps, p ) );
#endif
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

PTRSZVAL CPROC TestLinked( POINTER p, PTRSZVAL psv )
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
	lprintf( WIDE("Resulting links: this %p from %d to %d")
			 , pLine->pLine
			 , pLine->nLineFrom
			 , pLine->nLineTo );
#endif
	if( pLine->nLineFrom < 0 || pLine->nLineTo < 0 )
	{
		//if( pLine->nLineFrom < 0 && pLine->nLineTo < 0 )
		{
#ifdef DEBUG_LINK_LINES
			//lprintf( WIDE("...%d"), nl1 );
			lprintf( WIDE("... delete line %p"), pLine );
#endif
         DeleteFromSet( LINESEGP, *data->pplps, pLine );
		}
	}
#if 0
		else if( pLine->nLineFrom < 0 )
		{
			// uhmm okay fake it...
			lprintf( WIDE("Failed to link FROM segment %d"), nl1 );
			for( nl2 = nl1 + 1; nl2 < plps->nUsedLines; nl2++ )
			{
				if( plps->pLines[nl2].nLineFrom < 0 )
				{
					lprintf( WIDE("Found another unused end to use... faking link.") );
					pLine->nLineFrom = nl2;
					plps->pLines[nl2].nLineFrom = nl1;
					break;
				}
				if( plps->pLines[nl2].nLineTo < 0 )
				{
					lprintf( WIDE("Found another unused end to use... faking link.") );
								pLine->nLineFrom = nl2;
								plps->pLines[nl2].nLineTo = nl1;
                        break;
				}
				else if( pLine->nLineTo < 0 )
				{
               lprintf( WIDE("Failed to link TO segment %d"), nl1 );
					for( nl2 = nl1 + 1; nl2 < plps->nUsedLines; nl2++ )
					{
						if( plps->pLines[nl2].nLineFrom < 0 )
						{
                     lprintf( WIDE("Found another unused end to use... faking link.") );
								pLine->nLineTo = nl2;
								plps->pLines[nl2].nLineFrom = nl1;
                        break;
						}
						if( plps->pLines[nl2].nLineTo < 0 )
						{
                     lprintf( WIDE("Found another unused end to use... faking link.") );
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
      //ShowTransform( tFailureRotation );
		// some arbitrary rotation factor... this allows us to translate
      // near points bounding origins to spaces where 0 is not involved...
		RotateAbs( tFailureRotation, 0.5, 0.5, 0.5 );
      //Translate( tFailureRotation, 1, 1, 1 );
      //ShowTransform( tFailureRotation );
	}
   LIST_FORALL( oi->facets, nf, PFACET, pf )
	{
      //int retry;
      // eventually we'll move this to the better set type also...
		data.oi = oi;
      // just need to delete the set set...
		data.pplps = &pf->pLineSet;
		data.pLine1 = NULL; // a scratch var, doesn't need init...
      //lprintf( WIDE("link facet %p"), pf );
		ForAllInSet( LINESEGP, *data.pplps, TestLinkLines1, (PTRSZVAL)&data );
		ForAllInSet( LINESEGP, *data.pplps, TestLinked, (PTRSZVAL)&data );
	}
}

PTRSZVAL CPROC DeleteLinePSeg( POINTER p, PTRSZVAL psv )
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
	//lprintf( WIDE("...") );
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
		ForAllInSet( LINESEGP, pf->pLineSet, DeleteLinePSeg, (PTRSZVAL)&data );
	}
	//lprintf( WIDE("... %d"), pfps->nUsedFacets );
	// for all combinations of planes intersect them.
	tick = 0;
	MarkTick( ticks[tick++] );
	LIST_FORALL( oi->facets, i, PFACET, pf )
	{
		PFACET pf2;
		j = i;
		if( pf->flags.bClipOnly )
         continue;
		LIST_NEXTALL( oi->facets, j, PFACET, pf2 )
		{
			PFACET pf3;
#ifdef FULL_DEBUG
         lprintf( WIDE("------------------------------------------------------------\n"));
         lprintf( WIDE("Between facet %d and facet %d"), i, j );
#endif
         // if NO line exists between said planes, line will not be created...
         pl = CreateLineBetweenFacets( oi, pf, pf2 );
                           // link 2 lines together...
                           // create all possible intersections
			if( !pl )  // no line intersecting...
			{
            //lprintf( WIDE("plane intersection failed... %d %d\n"), i, j );
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
					if( pf3->flags.bInvert )
						Invert( n );
#ifdef DEBUG_LINK_LINES
					lprintf( "End point 1 with facet %d", k );
#endif
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
								lprintf( WIDE("object %p facet %d %d and %d Intersect time %g"), oi, i, j, k, time );
								addscaled( x, pl->l.r.o, pl->l.r.n, time );
								PrintVector( x );
							}
#endif
#define MIN_RANGE
#ifdef MIN_RANGE
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
#else
                  		if( time < pl->l.dFrom )
                  		{
	                  		pl->l.dFrom= time;
	                  	}
                  		if( time > pl->l.dTo )
                  		{
                  			pl->l.dTo = time;
								}
#endif
                  }
                  else
                  {
                  		// didn't form a point - but the line resulting above 
                  		// cannot be above any other plane SO... test for 
                  		// line above plane...
#if 0
					  /* This is an invalid test; just because the origin is above/below the plane, doesn't mean the line is... the line could still cross the plane*/
      					if( AbovePlane( pf3->d.n
      					              , pf3->d.o
      					              , pl->l.r.o ) )
							{
								// setup conditions to have the line deleted.
								//k = pfps->nUsedFacets;
#ifdef DEBUG_LINK_LINES
								lprintf( WIDE("Above plane %d %g %g"), k, pl->l.dFrom, pl->l.dTo );
								PrintVector( pl->l.r.o );
								PrintVector( pf3->d.o );
								PrintVector( pf3->d.n );
#endif
								pl->l.dFrom = 1;
								pl->l.dTo = 0;
      						break;
      					}
#endif
#ifdef FULL_DEBUG
                     //lprintf( WIDE("okay Facets %d, %d and %d do not form a point.\n"), i,  j, k ) ;
#endif
						}
				}
				// pf3 will be NULL at end of loop, or at empty list...
				// otherwise non-null pf3 indicates a bail.
				if( pf3 &&
					(pl->l.dFrom >  pl->l.dTo) )
				{
					//DebugBreak();
#ifdef DEBUG_LINK_LINES
					lprintf( WIDE("DELETEING LINE - IS NOT LINKED - above plane %d"), k );
					DeleteLine( oi, pf, pl );
					DeleteLine( oi, pf2, pl );
#endif
					continue;
				}
			}
#ifdef DEBUG_LINK_LINES
			lprintf( WIDE("Resulting line....") );
			DumpLine( pl );
#endif
		}
	}
#ifdef DEBUG_LINK_LINES
	lprintf( WIDE("---- Link facet lines...") );
#endif
	MarkTick( ticks[tick++] );
	MarkTick( ticks[tick++] );
	LinkFacetLines( oi );
#ifdef DEBUG_LINK_LINES
	lprintf( WIDE("---- Order Facet lines just now linked...") );
#endif
	MarkTick( ticks[tick++] );
	OrderFacetLines( oi );
	MarkTick( ticks[tick++] );

	return 0;
}

void OrderObjectLines( POBJECT po )
{
	LinkFacetLines( po->objinfo );
	OrderFacetLines( po->objinfo );
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
#ifdef DEBUG_LINK_LINES
	lprintf( WIDE("PointToPlaneT=%g"), t );
#endif
   return t;
}

LOGICAL AbovePlane( PVECTOR n, PVECTOR o, PVECTOR p )
{
   if( PointToPlaneT( n, o, p ) >= 0 )
      return TRUE;
   else
      return FALSE;
}
       
int GetPoints( PFACET pf, int *nPoints, VECTOR ppv[] )
{
	int np = 0;
	if( 1 )
	{
	int nlStart, nl;
	int lines = CountUsedInSet( LINESEGP, pf->pLineSet );
	//DebugBreak();
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
#ifndef TEMP_DISABLE
			lprintf( WIDE("Failed linkage of facet lines.(%d)"), nl );
#endif
			break;
		}
		//else
        // lprintf( WIDE("got line %d"), nl );
		line = pLine->pLine;
	   	if( pLine->bOrderFromTo )
		{
			{
			add( (PVECTOR)&ppv[np]
				, scale( (PVECTOR)&ppv[np], (PVECTOR)line->l.r.n
						           , line->l.dTo )
				, line->l.r.o );
			}
			//else if( pf->bNormalSurface )
			{
			//	SetPoint( ppv[np], line->l.r.o );
			}
			np++;
	   		nl = pLine->nLineTo;
		}
		else
		{
			{
				add( (PVECTOR)&ppv[np]
				, scale( (PVECTOR)&ppv[np], line->l.r.n
						           , line->l.dFrom )
				, line->l.r.o );
			}
			//else if( pf->bNormalSurface )
			{
			//	SetPoint( ppv[np], line->l.r.o );
			}
			np++;
		   nl = pLine->nLineFrom;	
		}
		if( np >= *nPoints )
		{
         lprintf( WIDE("No more points to fill.. %d"), np );
			return  FALSE;
		}
		if( nl < 0 )
		{
			np = 0;
			lprintf( WIDE("facet %p Aborting chain!"), pf );
			break;
		}
	} while( nl != nlStart );
	}
	else if( pf->flags.bNormalSurface )
	{

	}
	//Log1( WIDE("Resulting points: %d"), np );
	*nPoints = np;
	return TRUE;
}

int GetNormals( PFACET pf, int *nPoints, VECTOR ppv[] )
{
	int np = 0;
	int nlStart, nl;
	int lines = CountUsedInSet( LINESEGP, pf->pLineSet );
	//DebugBreak();
	if( !pf->flags.bNormalSurface )
		return 0;
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
			lprintf( WIDE("Failed linkage of facet lines.(%d)"), nl );
			break;
		}
		//else
        // lprintf( WIDE("got line %d"), nl );
		line = pLine->pLine;
	   	if( pLine->bOrderFromTo )
		{
			SetPoint( ppv[np], line->normals.at_from );
			np++;
			//SetPoint( ppv[np], line->normals.at_to );
			//np++;
	   		nl = pLine->nLineTo;
		}
		else
		{
			SetPoint( ppv[np], line->normals.at_to );
			np++;
			//SetPoint( ppv[np], line->normals.at_from );
			//np++;
			nl = pLine->nLineFrom;	
		}
		if( np >= *nPoints )
		{
			Log1( WIDE("No more points to fill.. %d"), np );
			return  FALSE;
		}
		if( nl < 0 )
		{
			lprintf( WIDE("facet %p Aborting chain!"), pf );
			break;
		}
	} while( nl != nlStart );
	//Log1( WIDE("Resulting points: %d"), np );
   *nPoints = np;
   return TRUE;
}

// mod
