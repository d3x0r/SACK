static unsigned char byBuffer[256]; // for output debug string (complex formats)

#include <stdio.h>
#include <stdlib.h>
#include <sharemem.h>
#include <string.h>
#include <math.h>

#include <windows.h> // output debug string...
#include "logging.h"
#define NEED_VECTLIB_COMPARE
#include "vectlib.h"

#include "object.h"

#ifdef GCC
#define DebugBreak() asm( "int $3\n" );
#endif

//#define PRINT_FACETS
//#define PRINT_LINES
//#define FULL_DEBUG
//#define NO_LOGGING
#define DEBUG_LINK_LINES


int Parallel( PVECTOR pv1, PVECTOR pv2 );
void DumpPlane( PFACET pp );
void DumpLine( PG_LINESEG pl );


#ifdef __WATCOMC__
void f(void)
{
	// use some function to load floating support
	float x = sin(30.0); 
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
      Log( "Planes are parallel" );
      return FALSE;
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
//                  pl->dFrom, pl->dTo );
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
	int i;
	for( i = 0; i < oi->LinePool.nUsedLines; i++ )
	{
		if( !oi->LinePool.pLines[i].bUsed )
		{
			oi->LinePool.pLines[i].bUsed = TRUE;
			return i;
		}
	}
	AllocateSet( &oi->LinePool, Lines );
	oi->LinePool.pLines[oi->LinePool.nUsedLines].bUsed = TRUE;
	return oi->LinePool.nUsedLines++;
}


int GetLineSegP( PLINESEGPSET plps )
{
	int i;
	for( i = 0; i < plps->nUsedLines; i++ )
	{
		if( plps->pLines[i].nLine < 0 )
			return i;
	}
	AllocateSet( plps, Lines );
	return plps->nUsedLines++;
}

int GetFacet( OBJECTINFO *oi )
{
	int i;
	for( i = 0; i < oi->FacetPool.nUsedFacets; i++ )
	{
		if( !oi->FacetPool.pFacets[i].bUsed )
		{
			oi->FacetPool.pFacets[i].bUsed = TRUE;
			return i;
		}
	}
	AllocateSet( &oi->FacetPool, Facets );
	oi->FacetPool.pFacets[oi->FacetPool.nUsedFacets].bUsed = TRUE;
	return oi->FacetPool.nUsedFacets++;
}

int GetFacetP( OBJECTINFO *oi, int nfs )
{
	int i;
	PFACETPSET pfps = oi->FacetSetPool.pFacetSets + nfs;
	for( i = 0; i < pfps->nUsedFacets; i++ )
	{
		if( pfps->pFacets[i].nFacet < 0 )
		{
			return i;
		}
	}
	AllocateSet( pfps, Facets );
	return pfps->nUsedFacets++;
}

int GetFacetRef( PFACETREFSET pfr)
{
	int i;
	for( i = 0; i < pfr->nUsedFacets; i++ )
	{
		if( pfr->pFacets[i].nFacet < 0 )
			return i;
	}
	AllocateSet( pfr, Facets );
	return pfr->nUsedFacets++;
}

int GetFacetSet( OBJECTINFO *oi )
{
	int i;
	for( i = 0; i < oi->FacetSetPool.nUsedFacetSets; i++ )
	{
		if( !oi->FacetSetPool.pFacetSets[i].pFacets )
		{
			return i;
		}
	}
	AllocateSet( &oi->FacetSetPool, FacetSets );
	return oi->FacetSetPool.nUsedFacetSets++;
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


void AddLineToPlane( OBJECTINFO *oi, int nfs, int nf, int nl )
{
	
	if( oi && nfs >= 0 && nf >= 0 && nl >= 0 )
	{
		PG_LINESEG pl = oi->LinePool.pLines + nl;
		PFACETREFSET pfrs = &pl->frs;

		PFACET pf = oi->FacetPool.pFacets + oi->FacetSetPool.pFacetSets[nfs].pFacets[nf].nFacet;
		int nfr;
		PLINESEGPSET plps = &pf->pLineSet;
		PLINESEGP plp;

		nfr = GetFacetRef( pfrs );
		pfrs->pFacets[nfr].nFacet = nf;
		pfrs->pFacets[nfr].nFacetSet = nfs;
		plp = plps->pLines + GetLineSegP(plps);
		plp->nLine = nl;
		plp->nLineFrom = -1;
		plp->nLineTo = -1;
	}
}

int CreateLine( OBJECTINFO *oi, 
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo )
{
   PG_LINESEG pl;
   int nl;
   nl = GetLineSeg(oi);
   pl = oi->LinePool.pLines + nl;
   //pl->bUsed = TRUE;
   pl->bDraw = TRUE;
   pl->dFrom = rFrom;
   pl->dTo = rTo;
   SetPoint( pl->d.o, po );
   SetPoint( pl->d.n, pn );

   return nl;
}

// create line is passes a base pointer to an array of planes
// and 2 indexes into that array to intersect.  This is great
// for multi-segmented intersections with different base
// pointers for simplified objects....(I guess...)
  // this merely provides the line of intersection
  // does not result in any terminal caps on the line....
int CreateLineBetweenFacets( OBJECTINFO *oi, int nfs, int np1, int np2 )
{
   G_LINESEG t; // m (slope) of (Int)ersection
//   G_LINESEG l1, l2;
   PG_LINESEG pl;
   VECTOR tv;
   PFACET pp1 = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[np1].nFacet]
   	  , pp2 = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[np2].nFacet];
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
     return -1;
  }
  if( FillLine( pp1->d.o, pp1->d.n, 
               pp2->d.o, pp2->d.n,
               &t.d,
               tv ) == 2 )
   {
   	int nl;
   	nl = GetLineSeg( oi );
      pl = oi->LinePool.pLines + nl;

      pl->dFrom = NEG_INFINITY;
      pl->dTo = POS_INFINITY;
      pl->d = t.d;
      // use other origin?
      // SetPoint( pl1->d.o, tv);

      AddLineToPlane( oi, nfs, np1, nl );
      AddLineToPlane( oi, nfs, np2, nl );

      return nl; // could return pl2 (?)
   }
   else
   {
      Log( "NON-SYMMETRIC!\n" );
   }
   return -1;
}



int AddPlaneToSet( OBJECTINFO *oi, int nfs, PCVECTOR origin, PCVECTOR norm, char d )
{
   PFACET pf;
   int nf, nfp;
   nf = GetFacet( oi );
   nfp = GetFacetP( oi, nfs );
   oi->FacetSetPool.pFacetSets[nfs].pFacets[nfp].nFacet = nf;
   pf = oi->FacetPool.pFacets + nf;
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
     
   return nfp; // positive index of plane created reference.
}

void DumpLine( PG_LINESEG pl )
{
#ifdef PRINT_LINESEGS
   sprintf( (char*)byBuffer," ---- G_LINESEG ---- \n ");
   Log( (char*)byBuffer );
   PrintVector( pl->d.o );  // Origin is resulting transformation
   PrintVector( pl->d.n );   // Slope is resulting transformation
   sprintf( (char*)byBuffer," From: %g To: %g\n", pl->dFrom, pl->dTo );
   Log( (char*)byBuffer );
#endif
}

void DumpPlane( PFACET pp )
{
#ifdef PRINT_FACETS
   sprintf( (char*)byBuffer,"  -----  FACET ----- \n" );
   Log( (char*)byBuffer );
   PrintVector( pp->d.o );
   PrintVector( pp->d.n );
#endif
}


void DeleteLine( OBJECTINFO *oi, int nfs, int nf, int nl )
{
	int i;
	PFACETREFSET pfrs;
	{
		PLINESEGPSET plps = &oi->FacetPool.pFacets[oi->FacetSetPool.pFacetSets[nfs].pFacets[nf].nFacet].pLineSet;
		for( i = 0; i < plps->nUsedLines; i++ )
		{
			if( plps->pLines[i].nLine == nl )
			{
				plps->pLines[i].nLine = -1;
				break;
			}
		}
	}	

	pfrs = &oi->LinePool.pLines[nl].frs;
	for( i = 0; i < pfrs->nUsedFacets; i++ )
	{
		if( pfrs->pFacets[i].nFacet == nf &&
		    pfrs->pFacets[i].nFacetSet == nfs )
		{
			pfrs->pFacets[i].nFacet = -1;
			pfrs->pFacets[i].nFacetSet = -1;
		}
	}
	// check to see if the line is still contained in any planes...
	for( i = 0; i < pfrs->nUsedFacets; i++ )
	{
		if( pfrs->pFacets[i].nFacet < 0 )
			continue;

		break;
	}
	if(  i == pfrs->nUsedFacets )
	{
		pfrs->nUsedFacets = 0;
		oi->LinePool.pLines[nl].bUsed = FALSE;
	}
}


void OrderFacetLines( OBJECTINFO *oi, int nfs )
{
	int nf, nl, nfirst;
	PFACETPSET pfps;
	pfps = oi->FacetSetPool.pFacetSets + nfs;
	for( 	nf = 0; nf < pfps->nUsedFacets; nf++ )
	{
		PLINESEGPSET plps;
		int nfrom;
      nfirst = -1;
		if( pfps->pFacets[nf].nFacet < 0 )
			continue;
		plps = &oi->FacetPool.pFacets[pfps->pFacets[nf].nFacet].pLineSet;
		//for( nl = 0; nl < plps->nUsedLines; nl ++ )
      nl = 0;
      while( (nfirst < 0) || (nl != nfirst) )
		{
			if( plps->pLines[nl].nLine < 0 )
				continue;  // unused line...
			if( nfirst < 0 )
			{
				nfirst = nl;
				nfrom = nl;
				nl = plps->pLines[nl].nLineTo;
				Log3( "first = %d from = %d nl = %d", nfirst, nfrom, nl );
            continue;
			}
			if( plps->pLines[nl].nLineFrom == nfrom )
				plps->pLines[nl].bOrderFromTo = TRUE;
			else if( plps->pLines[nl].nLineTo == nfrom )
				plps->pLines[nl].bOrderFromTo = FALSE;
			else
			{
				Log4( "The line seg at From doesn't link to this one?! %d %d != %d != %d"
					 , nl
					 , plps->pLines[nl].nLineFrom
					 , nfrom
					 , plps->pLines[nl].nLineTo
					 );
			}
			nfrom = nl;
         if( plps->pLines[nl].bOrderFromTo )
				nl = plps->pLines[nl].nLineTo;
         else
				nl = plps->pLines[nl].nLineFrom;
          Log3( "first = %d from = %d nl = %d", nfirst, nfrom, nl );
		}
      if( plps->pLines[nfirst].nLineFrom == nfrom )
			plps->pLines[nfirst].bOrderFromTo = TRUE;
      else if( plps->pLines[nfirst].nLineTo == nfrom )
			plps->pLines[nfirst].bOrderFromTo = FALSE;
			else
			{
				Log4( "The line seg at From doesn't link to this one?! %d %d != %d != %d"
					 , nfirst
					 , plps->pLines[nfirst].nLineFrom
					 , nfrom
					 , plps->pLines[nfirst].nLineTo
					 );
			}
#ifdef DEBUG_LINK_LINES
      Log1( "Facet %d", nf );
		for( nl = 0; nl < plps->nUsedLines; nl++ )
		{
         if( plps->pLines[nl].bOrderFromTo )
				Log2( "Resulting links: %d %d"
					 , plps->pLines[nl].nLineFrom
					 , plps->pLines[nl].nLineTo );
         else
				Log2( "Resulting links: %d %d"
					 , plps->pLines[nl].nLineTo
					 , plps->pLines[nl].nLineFrom );
		}
#endif
	}
}


void LinkFacetLines( OBJECTINFO *oi, int nfs )
{
	int nf, nl1, nl2;
	PFACETPSET pfps = oi->FacetSetPool.pFacetSets + nfs;
	for( nf = 0; nf < pfps->nUsedFacets; nf++)
	{
		PLINESEGPSET plps = &oi->FacetPool.pFacets[pfps->pFacets[nf].nFacet].pLineSet;
		RCOORD dist;
		PLINESEGSET pls = &oi->LinePool;
		for( nl1 = 0; nl1 < plps->nUsedLines; nl1++ )
		{
			if( plps->pLines[nl1].nLineTo < 0 )
			{
				_POINT to1;
				add( to1
					, scale( to1, pls->pLines[plps->pLines[nl1].nLine].d.n
							      , pls->pLines[plps->pLines[nl1].nLine].dTo )
					, pls->pLines[plps->pLines[nl1].nLine].d.o );
				for( nl2 = nl1+1; nl2 < plps->nUsedLines; nl2++ )
				{                        	
					if( plps->pLines[nl2].nLineTo < 0 )
					{
						_POINT to2;
						add( to2
							, scale( to2, pls->pLines[plps->pLines[nl2].nLine].d.n
									      , pls->pLines[plps->pLines[nl2].nLine].dTo )
							, pls->pLines[plps->pLines[nl2].nLine].d.o );
						if( Near( to1, to2 ) )
						{
#ifdef DEBUG_LINK_LINES
							Log2( "Linking to,to %d %d", nl2, nl1 );
#endif
							plps->pLines[nl1].nLineTo = nl2;
							plps->pLines[nl2].nLineTo = nl1;
							break;
						}
#ifdef DEBUG_LINK_LINES
						else
						{
							Log6( "(%g,%g,%g) and (%g,%g,%g) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
							Log6( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
						}
#endif
					}
					if( plps->pLines[nl2].nLineFrom < 0 )
					{
						_POINT to2;
						add( to2
							, scale( to2, pls->pLines[plps->pLines[nl2].nLine].d.n
									      , pls->pLines[plps->pLines[nl2].nLine].dFrom )
							, pls->pLines[plps->pLines[nl2].nLine].d.o );
						if( Near( to1, to2 ) )
						{
#ifdef DEBUG_LINK_LINES
							Log2( "Linking to,from %d %d", nl2, nl1 );
#endif
							plps->pLines[nl1].nLineTo = nl2;
							plps->pLines[nl2].nLineFrom = nl1;
							break;
						}
#ifdef DEBUG_LINK_LINES
						else
						{
							Log6( "(%g,%g,%g) and (%g,%g,%g) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
							Log6( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
						}
#endif						
					}
				}
			}
			if( plps->pLines[nl1].nLineFrom < 0 )
			{
				_POINT to1;
				add( to1
					, scale( to1, pls->pLines[plps->pLines[nl1].nLine].d.n
							      , pls->pLines[plps->pLines[nl1].nLine].dFrom )
					, pls->pLines[plps->pLines[nl1].nLine].d.o );
				for( nl2 = nl1+1; nl2 < plps->nUsedLines; nl2++ )
				{                        	
					if( plps->pLines[nl2].nLineTo < 0 )
					{
						_POINT to2;
						add( to2
							, scale( to2, pls->pLines[plps->pLines[nl2].nLine].d.n
									      , pls->pLines[plps->pLines[nl2].nLine].dTo )
							, pls->pLines[plps->pLines[nl2].nLine].d.o );
						if( Near( to1, to2 ) )
						{
#ifdef DEBUG_LINK_LINES
							Log2( "Linking from,to %d %d", nl2, nl1 );
#endif
							plps->pLines[nl1].nLineFrom = nl2;
							plps->pLines[nl2].nLineTo = nl1;
							break;
						}
#ifdef DEBUG_LINK_LINES
						else
						{
							Log6( "(%g,%g,%g) and (%g,%g,%g) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
							Log6( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
						}
#endif
					}
					if( plps->pLines[nl2].nLineFrom < 0 )
					{
						_POINT to2;
						add( to2
							, scale( to2, pls->pLines[plps->pLines[nl2].nLine].d.n
									      , pls->pLines[plps->pLines[nl2].nLine].dFrom )
							, pls->pLines[plps->pLines[nl2].nLine].d.o );
						if( Near( to1, to2 ) )
						{
#ifdef DEBUG_LINK_LINES
							Log2( "Linking from,from %d %d", nl2, nl1 );
#endif
							plps->pLines[nl1].nLineFrom = nl2;
							plps->pLines[nl2].nLineFrom = nl1;
							break;
						}
#ifdef DEBUG_LINK_LINES
						else
						{
							Log6( "(%g,%g,%g) and (%g,%g,%g) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
							Log6( "(%016Lx,%016Lx,%016Lx) and (%08Lx,%016Lx,%016Lx) were not near"
									, to1[0], to1[1], to1[2]
									, to2[0], to2[1], to2[2] );
						}
#endif
					}
				}
			}
		}
		// at this point all lines are linked to their near segments...
		for( nl1 = 0; nl1 < plps->nUsedLines; nl1++ )
		{
#ifdef DEBUG_LINK_LINES
			Log2( "Resulting links: %d %d"
					, plps->pLines[nl1].nLineFrom
					, plps->pLines[nl1].nLineTo );
#endif
			if( plps->pLines[nl1].nLineFrom < 0 ||
			    plps->pLines[nl1].nLineTo < 0 )
			{
				DeleteLine( oi, nfs, nf, nl1 );
			}
		}
	}
}

int IntersectPlanes( OBJECTINFO *oi, int nfs, int bAll )
{
   int i, j, k, nf, l, l2, nl;
   PFACETPSET pfps;
   PFACET pf;
   PG_LINESEG  pl, tpl;

	pfps = oi->FacetSetPool.pFacetSets + nfs;
   // clear all lines used by this facetset 
   for( nf = 0; nf < pfps->nUsedFacets; nf++ )
   {
   	PLINESEGPSET plps;
      if( pfps->pFacets[nf].nFacet < 0 ||
          !oi->FacetPool.pFacets[pfps->pFacets[nf].nFacet].bUsed )
      	continue;
	   plps = &oi->FacetPool.pFacets[pfps->pFacets[nf].nFacet].pLineSet;
	   for( l = 0; l < plps->nUsedLines; l++ )
	   {
	   	if( plps->pLines[l].nLine < 0 )
	   		continue;
			DeleteLine( oi, nfs, nf, plps->pLines[l].nLine );
		}
   }

   // for all combinations of planes intersect them.
   for( i = 0; i < pfps->nUsedFacets; i++ )
   {
      if( pfps->pFacets[i].nFacet < 0 )
      	continue;
      for( j = i + 1; j < pfps->nUsedFacets; j++ )
      {
         if( pfps->pFacets[j].nFacet < 0 )
         	continue;
#ifdef FULL_DEBUG
         sprintf( (char*)byBuffer, "------------------------------------------------------------\n");
         Log( (char*)byBuffer );
         sprintf( (char*)byBuffer, "Between %d and %d\n", i, j );
         Log( (char*)byBuffer );
#endif
         // if NO line exists between said planes, line will not be created...
         nl = CreateLineBetweenFacets( oi, nfs, i, j );
                           // link 2 lines together...
                           // create all possible intersections
         if( nl  < 0 )  // no line intersecting...
         	continue; 
			pl = oi->LinePool.pLines + nl;
         if( pl )
         {
            for( k = 0; k < pfps->nUsedFacets; k++ )
				{
               //pl->frs
               if( k != i && k != j )
               {
                  RCOORD time, s;
                  VECTOR n;
                  int nfk;
                  nfk = pfps->pFacets[k].nFacet;
                  SetPoint( n, oi->FacetPool.pFacets[nfk].d.n );
   					if( pfps->pFacets[k].bInvert )
   						Invert( n );
					
                  if( s = IntersectLineWithPlane( pl->d.n
                                            , pl->d.o
                                            , n
                                            , oi->FacetPool.pFacets[nfk].d.o
                                            , &time ) )
                  {
                  	RCOORD dp;
                  	if( ( dp = dotproduct( pl->d.n, n ) ) > 0 )
                  	{
                  		if( time < pl->dTo )
                  		{
	                  		pl->dTo = time;
	                  	}
	                  }
                  	else
                  	{
                  		if( time > pl->dFrom )
                  		{
                  			pl->dFrom = time;
                  		}
                  	}
                  }
                  else
                  {
                  	// didn't form a point - but the line resulting above 
                  	// cannot be above any other plane SO... test for 
                  	// line above plane...
      					if( AbovePlane( oi->FacetPool.pFacets[nfk].d.o
      					              , oi->FacetPool.pFacets[nfk].d.n
      					              , pl->d.o ) )
      					{
      						// setup conditions to have the line deleted.
      						k = pfps->nUsedFacets;
      						pl->dFrom = 1;
      						pl->dTo = 0;
      						break;
      					}
#ifdef FULL_DEBUG
                     sprintf( (char*)byBuffer, "okay Facets %d, %d and %d do not form a point.\n", i,  j, k ) ;
                     Log( (char*)byBuffer );
#endif
                  }
               }
            }
            if( k == pfps->nUsedFacets &&
               pl->dFrom >  pl->dTo )
            {
      	       DeleteLine( oi, nfs, i, nl );
         	    DeleteLine( oi, nfs, j, nl );
            }
         }
#ifdef FULL_DEBUG
         else
         {
            sprintf( (char*)byBuffer, "plane intersection failed...\n");
            Log( (char*)byBuffer );
         }
#endif
      }
   }
   LinkFacetLines( oi, nfs ); 
   OrderFacetLines( oi, nfs );

   return 0;
}

int PointWithin( PCVECTOR p, PLINESEGSET pls, PLINESEGPSET plps )
{
   int l1, l2;
   PG_LINESEG pl1, pl2 ;
   for( l1 = 0; l1 < plps->nUsedLines; l1++ )
   {
      VECTOR v;
      RCOORD tl, tl2;
      pl1 = pls->pLines + plps->pLines[l1].nLine;
      sub( v, p, pl1->d.o );
      for( l2 = 0; l2 < plps->nUsedLines; l2++ )
      {
         if( l1 == l2 )
            continue;
         pl2 = pls->pLines +  plps->pLines[l2].nLine;
         if( FindIntersectionTime( &tl, v, pl1->d.o
                                 , &tl2, pl2->d.n, pl2->d.o ) )
         {
            if( tl > 0 && 
                ( tl2 > pl2->dFrom && 
                  tl2 < pl2->dTo ) ) 
              break;
         }
         
      }
      if( l2 == plps->nUsedLines )
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
       
int GetPoints( PLINESEGSET pls, PFACET pf, int *nPoints, VECTOR ppv[] )
{
	int nlStart, nl, np;
	PLINESEGPSET plps = &pf->pLineSet;
	PLINESEGP pLines = plps->pLines;
   PG_LINESEG pSegs = pls->pLines;
	for( nlStart = 0; nlStart < plps->nUsedLines; nlStart++ )
	{
		if( pLines[nlStart].nLine < 0 )
			continue;
		break;
	}	
	nl = nlStart;
	np = 0;
   do
   {
   	if( pLines[nl].bOrderFromTo )
		{
         int nLine = pLines[nl].nLine;
			add( &ppv[np]
				, scale( &ppv[np], pSegs[nLine].d.n
						           , pSegs[nLine].dTo )
				, pSegs[nLine].d.o );
			np++;
	   	nl = pLines[nl].nLineTo;
	   }
	   else
		{
         int nLine = pLines[nl].nLine;
			add( &ppv[np]
				, scale( &ppv[np], pSegs[nLine].d.n
						           , pSegs[nLine].dFrom )
				, pSegs[nLine].d.o );
			np++;
		   nl = pLines[nl].nLineFrom;	
		}
		if( np >= *nPoints )
		{
         Log1( "No more points to fill.. %d", np );
			return  FALSE;
		}
		if( nl < 0 )
		{
         Log( "Aborting chain!" );
			break;
		}
	} while( nl != nlStart );
	//Log1( "Resulting points: %d", np );
   *nPoints = np;
   return TRUE;
}
