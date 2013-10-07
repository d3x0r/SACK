static unsigned char byBuffer[256]; // for output debug string (complex formats)

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#include <vectlib.h>
#include <timers.h>

#include "object.hpp"

//#define PRINT_FACETS
//#define PRINT_LINES
//#define FULL_DEBUG
//#define NO_LOGGING

//#define POS_INFINITY 9999999.0F
//#define NEG_INFINITY -9999999.0F

#define Alloc(struc,name)                       \
      if( (struc->n##name) == struc->nUsed##name )   \
      {                                            \
         void *pt;                                 \
         struc->n##name += 16;                     \
         pt = Allocate( sizeof( *(struc->p##name) ) *  \
                      struc->n##name );            \
         memcpy( pt,                               \
                 struc->p##name,                   \
                 sizeof( *(struc->p##name) ) *     \
                 struc->nUsed##name );             \
         memset( (char*)pt +                       \
                 sizeof( *(struc->p##name) ) *     \
                 struc->nUsed##name,               \
                 0,                                \
                 sizeof( *(struc->p##name) ) *     \
                 16 );                             \
         Release( struc->p##name );                   \
         (*((void**)(&struc->p##name))) = pt;                      \
      }

//#define e1 (0010)  // allowable error between results...
//#define NearZero(n)     ( fabs(n) < e1 )
//#define COMPARE(c1,c2)  ( fabs( c1 - c2 ) < e1 )
int Parallel( PVECTOR pv1, PVECTOR pv2 );
void DumpPlane( PFACET pp );
void DumpLine( LINESEG pl );

PLINESEG GetLine( PLINESEGSET pls )
{
   PLINESEG pl;
   if( pls )
   {
      pl = pls->pLines + pls->nUsedLines;
      pls->nUsedLines++;
      return pl;
   }
   return NULL;
}



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
            //DebugBreak();
            t1 = ( ne * ( c - f ) + nf * ( b - e ) ) / denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         //DebugBreak();
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

void Distance( PVECTOR n, PVECTOR p )
{
//   normalize( n );
//    (n[0] * p[0] + n[1] *p[1] + n[2]*p[2]) - p

}

int Parallel( PVECTOR pv1, PVECTOR pv2 )
{
   RCOORD a,b,c,cosTheta; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

   a = ( pv1[0] * pv2[0] +
         pv1[1] * pv2[1] +
         pv1[2] * pv2[2] );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
   {
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
int IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,  // line m, b
                            PCVECTOR n, PCVECTOR o,  // plane n, o
                            RCOORD *time )
{
   RCOORD a,b,c,sinPhi, t; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

#ifdef PRINT_LINES
#ifdef PRINT_FACETS
//   PrintVector( pl->iSlope );
//   PrintVector( pp->in );
//   sprintf( (char*)byBuffer, "[0] = %g\n", pl->iSlope[0] * pp->in[0] );
//   Log( (char*)byBuffer );
//   sprintf( (char*)byBuffer, "[1] = %g\n", pl->iSlope[1] * pp->in[1] );
//   Log( (char*)byBuffer );
//   sprintf( (char*)byBuffer, "[2] = %g\n", pl->iSlope[2] * pp->in[2] );
//   Log( (char*)byBuffer );
//   Sleep( 5 );
#endif
#endif

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
   {
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
   if( !b || !c )
      return FALSE; // bad vector choice - if near zero length...

   sinPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

#pragma message( "Sorry - lost mathmatical reverse of plane..." )
//   cosTheta *= pp->d;   // reverse the plane...

//   sprintf( (char*)byBuffer, " a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n", a, b, c, t, cosTheta,
//                  pl->dFrom, pl->dTo );
//   Log( (char*)byBuffer );

//   if( cosTheta > e1 ) //global epsilon... probably something custom

//#define 

   if( sinPhi > 0.0009999 ||
       sinPhi < -0.00099999 ) // at least some degree of insident angle
//   if( cosTheta < 0.99999 &&
//       cosTheta > -0.99999 ) // at least some degree of insident angle
   {
      *time = t;
      return TRUE;
   }
   else
   {
//      sprintf( (char*)byBuffer,"Parallel...\n");
//      Log( (char*)byBuffer );
      return FALSE;
   }
   return TRUE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...

   
PLINESEGSET CreateLineSet( void )
{
   PLINESEGSET pr;
   pr = (PLINESEGSET)malloc( sizeof( PLINESEGSET ) );
   pr->nLines = 0;
   pr->nUsedLines = 0;
   pr->pLines = NULL;
   return pr;
}

PLINESEGPSET CreateLinePSet( void )
{
   PLINESEGPSET pr;
   pr = (PLINESEGPSET)malloc( sizeof( PLINESEGPSET ) );
   pr->nLines = 0;
   pr->nUsedLines = 0;
   pr->pLines = NULL;
   return pr;
}

void DestroyLineSet( PLINESEGSET pls )
{
   if( pls )
   {
      if( pls->pLines )
         free( pls->pLines );
      free( pls );
   }
}


void DestroyLinePSet( PLINESEGPSET plps )
{
   if( plps )
   {
      if( plps->pLines )
         free( plps->pLines );
      free( plps );
   }
}

PFACETSET CreatePlaneSet( void )
{
   PFACETSET pr;
   pr = (PFACETSET)malloc( sizeof( PFACETSET ) );
   pr->nPlanes = 0;
   pr->nUsedPlanes = 0;  // no used planes
   pr->pPlanes = NULL;
   return pr;
}

void DestroyPlaneSet( PFACETSET pps )
{
   if( pps )
   {
      if( pps->pPlanes )
      {
         int i;
         for( i = 0; i < pps->nUsedPlanes; i++ )
            DestroyLinePSet( pps->pPlanes[i].pLineSet );

         free( pps->pPlanes );
      }
      free( pps );
   }
}

void DiagnosticDump( PFACETSET pps );

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
     return NULL;
  }
   crossproduct( prl->n, n1, n2 );
//   normalize( o_slope );

   // this is the slope of the normal of the line...
   // or a perpendicular ray to the line... no origins - just slopes...

   // this is the line normal on the first plane...
   crossproduct( vnp1, prl->n, n1 );
//   normalize( l1.iSlope );

   // compute normal in second plane of the line
   crossproduct( vnp2, prl->n, n2 );
//   normalize( vnp2 );

   // the origin of the perpendicular vector to the normal vector
   // is the end of the normal vector.
   ret = 0;

// needs to know origin of slopeing line - which is what we're hoping to find.
   {
//      RCOORD time1, time2;

//      FindIntersection( o_origin,
//                        vnp1, o1, 
//                        vnp2, o2 );
//      return;
   }

   if( IntersectLineWithPlane( vnp2, o2,
                               n1, o1, &time ) )  // unless parallel....
   {
      VECTOR v;
      scale( v, vnp2, time );
      add( prl->o, v, o2 ); 
      ret++;
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


void AddLineToPlane( PLINESEG pl, PFACET pf )
{
   PLINESEGPSET plps;
   if( !pl || !pf )
      return;

   plps = pf->pLineSet;
   Alloc( plps, Lines ); // make sure there will be enough space....
   plps->pLines[plps->nUsedLines] = pl;
   plps->nUsedLines++;
}

PLINESEG CreateLine( PLINESEGSET pls, 
                  PCVECTOR po, PCVECTOR pn,
                  RCOORD rFrom, RCOORD rTo )
{
   PLINESEG pl;
   Alloc( pls, Lines ); // make sure there's room...
   pl = pls->pLines + pls->nUsedLines++;
   pl->bUsed = TRUE;
   pl->bDraw = TRUE;
   pl->dFrom = rFrom;
   pl->dTo = rTo;
   SetPoint( pl->d.o, po );
   SetPoint( pl->d.n, pn );

   return pl;
}

// create line is passes a base pointer to an array of planes
// and 2 indexes into that array to intersect.  This is great
// for multi-segmented intersections with different base
// pointers for simplified objects....(I guess...)
  // this merely provides the line of intersection
  // does not result in any terminal caps on the line....
PLINESEG CreateLine( PLINESEGSET pls, PFACET pp1, PFACET pp2 )
{
   LINESEG t; // m (slope) of (Int)ersection
//   LINESEG l1, l2;
   PLINESEG pl;
   VECTOR tv;

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
               &t.d,
               tv ) == 2 )
   {
      Alloc( pls, Lines );
      pl = pls->pLines + pls->nUsedLines;

      pl->bUsed = TRUE;
      pl->dFrom = POS_INFINITY;
      pl->dTo = NEG_INFINITY;
      pl->pInPlane = pp1;
      pl->d = t.d;
      // use other origin?
      // SetPoint( pl1->d.o, tv);

      pls->nUsedLines++;

      AddLineToPlane( pl, pp1 );
      AddLineToPlane( pl, pp2 );

      return pl; // could return pl2 (?)
      }
  else
  {
     Log( "NON-SYMMETRIC!\n" );
  }
      /*
      else
      {
         // origins and therefore lines on the planes normal to the line
         // do not intersect... except on the plane projected by the slope
         // of the line... OH!
         LINESEG projection;
         // project line 2 (normal to line on plane 2)
         //    on the same plane as the normal on plane 1....
         // bad wording - but yeah - that's the concept...

         ProjectLineOnPlane( &projection, &l2, t.iSlope, l1.iOrigin );
         if( FindIntersection( &t, &l1, &projection ) )
         {
            sprintf( (char*)byBuffer, "Intersect RECOVER!\n");
            Log( (char*)byBuffer );
         }
         else
         {
            Log("Intersect FAILED!\n");
         }
      }
      */
//   }
   return NULL;
}



PFACET AddPlane( PFACETSET pps, PCVECTOR origin, PCVECTOR norm, char d )
{
   PFACET pp;

   Alloc( pps, Planes );    // make sure there's enough room in the plane set...
   pp = pps->pPlanes + pps->nUsedPlanes;
   pp->pLineSet = CreateLinePSet();
   pp->bDraw = TRUE;
   pp->bUsed = TRUE;
   pps->nUsedPlanes++;  // we have just used a plane definition.
#ifdef PRINT_FACETS

   sprintf( (char*)byBuffer, "Planes = %d\n", pps->nUsedPlanes );
   Log( (char*)byBuffer );
#endif
/*
   sprintf( (char*)byBuffer, " We now have %d planes \n", po->nUsedPlanes );
   Log( (char*)byBuffer );
*/

   SetPoint( pp->d.n, norm );
   normalize( pp->d.n );
   SetPoint( pp->d.o, origin );

   if( d > 0 )
   {
	   pp->bInvert = FALSE;
	   pp->bDual = FALSE;
   }
   else if( d < 0 )
   {
	   pp->bInvert = TRUE;
	   pp->bDual = FALSE;
   }
   else
   {
	   pp->bDual = TRUE;
   }
     
#ifdef PRINT_FACETS
   Log( " Added ---->\n");
   DumpPlane( pp );
#endif
   return pp; // positive index of plane created reference.
}

void DeletePlane( PFACETSET po, PFACET pp )
{
   int i;
   i = pp - po->pPlanes;
   DestroyLinePSet( pp->pLineSet );
   free( pp->pLineSet ); // don't need these at all...
   if( ( i + 1 ) != po->nUsedPlanes )
      memcpy( pp, pp + 1, sizeof( FACET ) * (po->nUsedPlanes - i));
   else
      memset( pp, 0, sizeof( FACET ) );
   po->nUsedPlanes--;
}

void DeleteLine( PFACET pp, PLINESEG pl )
// should delete line from BOTH planes.
{
   PLINESEGPSET plps;
   pl->bUsed = FALSE;
   plps = pp->pLineSet;
/*
   if( pls->nUsedLines )
   {
      int l = pl - pls->pLines;
      if( pl->pOther )
      {
         pl->pOther->pOther = NULL;
         DeleteLine( pl->pOther->pInPlane, pl->pOther );
      }
      pls->nUsedLines--;  // will be deleted...
      if( l != pls->nUsedLines )
         memcpy( pl, pl + 1, sizeof( LINESEG ) * (pls->nUsedLines - l) );
      else
         memset( pl, 0, sizeof( LINESEG ) );
   }
*/
}

void DumpLine( PLINESEG pl )
{
#ifdef PRINT_LINESEGS
   sprintf( (char*)byBuffer," ---- LINESEG ---- \n ");
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


int LinesJoin( int atend, PLINESEG pl1, PLINESEG pl2 )
{
   RCOORD sl1, sl2, el1, el2;
//   VECTOR End1, End2;
//   VECTOR tEnd1, tEnd2;
   int i, temp_result;

#define START 0x01
#define END   0x02
#define S_WITH_START 0x4
#define S_WITH_END   0x8
#define E_WITH_START 0x10
#define E_WITH_END   0x20
#define BOTH_ENDS_JOINED 3

   temp_result = START|END|S_WITH_START|S_WITH_END|E_WITH_END|E_WITH_START;
   for( i = 0; i < DIMENSIONS && temp_result; i++ )
   {
      /*  or vice versa rather...
#define End1[i] sl1
#define tEnd1[i] sl2
#define End2[i] el1
#define tEnd2[i] el2
        */
/*
        sprintf( (char*)byBuffer,"%g %g %g %g\n", pl1->iOrigin[i],
                           pl1->iSlope[i],
                           pl1->dFrom,
                           pl1->dTo );
*/
      sl1 = pl1->d.o[i] + pl1->d.n[i] * pl1->dFrom;
      sl2 = pl2->d.o[i] + pl2->d.n[i] * pl2->dFrom;

      el1 = pl1->d.o[i] + pl1->d.n[i] * pl1->dTo;
      el2 = pl2->d.o[i] + pl2->d.n[i] * pl2->dTo;

      if( temp_result & START )
      {
         if( (temp_result & S_WITH_START) &&
             !COMPARE( sl1, sl2 ) )
         {
//            sprintf( (char*)byBuffer,"%g %g\n", sl1, sl2 );
//            Log( (char*)byBuffer );
            temp_result &= ~(S_WITH_START);
         }
         if( (temp_result & S_WITH_END) &&
             !COMPARE( sl1, el2 ) )
         {
//            sprintf( (char*)byBuffer,"%g %g\n", sl1, el2 );
//            Log( (char*)byBuffer );
            temp_result &= ~(S_WITH_END);
         }
         if( !(temp_result & (S_WITH_START|S_WITH_END) ) )
            temp_result &= ~START;
      }
      if( temp_result & END )
      {
         if( (temp_result & E_WITH_START) &&
             !COMPARE( el1, sl2 ) )
         {
//            sprintf( (char*)byBuffer,"%g %g\n", el1, sl2 );
//            Log( (char*)byBuffer );
            temp_result &= ~(E_WITH_START);
         }
         if( (temp_result & E_WITH_END) &&
             !COMPARE( el1, el2 ) )
         {
//            sprintf( (char*)byBuffer,"%g %g\n", el1, el2 );
//            Log( (char*)byBuffer );
            temp_result &= ~(E_WITH_END);
         }
         if( !(temp_result & (E_WITH_START|E_WITH_END) ) )
            temp_result &= ~END;
      }
   }
/*
   if( temp_result )
   {
      if( temp_result & START )sprintf( (char*)byBuffer,"START:");
      if( temp_result & END )sprintf( (char*)byBuffer,"END:");
      if( temp_result & S_WITH_START )sprintf( (char*)byBuffer,"S_WITH_START:");
      if( temp_result & E_WITH_START )sprintf( (char*)byBuffer,"E_WITH_START:");
      if( temp_result & S_WITH_END )sprintf( (char*)byBuffer,"S_WITH_END:");
      if( temp_result & E_WITH_END )sprintf( (char*)byBuffer,"E_WITH_END:");
   }
   sprintf( (char*)byBuffer,"\n");
*/
   return temp_result;
}


void DiagnosticDump( PFACETSET pps )
{
   int p, l, v, t, n;
   PFACET pp;
   PLINESEG pl;
   #define MAX_DIAGNOSTIC_STACK 200
   int    d[MAX_DIAGNOSTIC_STACK];
   int    np[MAX_DIAGNOSTIC_STACK];
   int    nl[MAX_DIAGNOSTIC_STACK];
   VECTOR va[MAX_DIAGNOSTIC_STACK];  //maximum points used.
   VECTOR vt;
   t = 0;
#ifndef FULL_DEBUG
   return;
#endif

   /*
   for( p = 0; p < 100; p++ )
   {
      sprintf( (char*)byBuffer," BIG FILL! ------------------------------------------------\n");
      Log( (char*)byBuffer );
   }
   */
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      PLINESEGPSET plps;

      pp = pps->pPlanes + p;
      plps = pp->pLineSet;
      for( l = 0; l < plps->nUsedLines; l++ )
      {
         pl = plps->pLines[l];
         pl->bDraw = TRUE;
         d[t] = -1;
         np[t] = p;
         nl[t] = l;
         scale( va[t], pl->d.n, pl->dFrom );
         add( va[t], va[t], pl->d.o );
         t++;

         sprintf( (char*)byBuffer, "level %d (%d of %d) and (%d of %d) \n",
                               t,
                               l, plps->nUsedLines,
                               p, pps->nUsedPlanes );
         Log((char*)byBuffer );
         WakeableSleep( 2 );
      if( t >= MAX_DIAGNOSTIC_STACK )

         return;
      }
   }

   for( v = 0; v < t; v++ )
   {
      PFACET pp, pop;
      PLINESEG pl;

      pp = pps->pPlanes + np[v];
      if( !pp )
      {
         sprintf( (char*)byBuffer, "no plane\n");
         Log( (char*)byBuffer );
         continue;
      }
      pl = pp->pLineSet->pLines[nl[v]];
      if( !pl)
      {
         sprintf( (char*)byBuffer, "no line.\n");
         Log( (char*)byBuffer );
       continue;
      }
      if( !pl->pOtherPlane )
      {
         sprintf( (char*)byBuffer, "No Other\n");
         Log( (char*)byBuffer );
         continue;
      }
      pop = pl->pOtherPlane;
      if( !pop->pLineSet )
      {
         sprintf( (char*)byBuffer, "NO OTHER LINE SET\n");
         Log( (char*)byBuffer );
         continue;
      }
      /* mating in this way irrelavent
      sprintf( (char*)byBuffer, "Plane %2d Line %2d mates (%2d in %2d) \n",
                     np[v],
                     nl[v],
                     pol - pop->pLineSet->pLines,
                     pop - pps->pPlanes
                  );
      Log( (char*)byBuffer );
      PrintVector( va[v] );
      */ 
   }
   /*
   for( v = 0; v < t; v++ )
   {
      sprintf( (char*)byBuffer, "Plane %2d Line %2d ",
                     np[v],
                     nl[v] );
      Log( (char*)byBuffer );
      PrintVector( va[v] );
   }
   */
   return;

   {
      int i, j;
      for(i = 0; i < t; i++ )
      {
         for( j = i + 1; j < t; j++ )
         {
            if( va[j][0] < va[i][0] )
            {
               n = d[j];
               d[j] = d[i];
               d[i] = n;
               p = np[j];
               np[j] = np[i];
               np[i] = p;
               l = nl[j];
               nl[j] = nl[i];
               nl[i] = l;
               SetPoint( vt, va[j] );
               SetPoint( va[j], va[i] );
               SetPoint( va[i], vt );
            }
            else if( va[j][0] == va[i][0] )
            {
               if( va[j][1] < va[i][1] )
               {
                  p = np[j];
                  n = d[j];
                  d[j] = d[i];
                  d[i] = n;
                  np[j] = np[i];
                  np[i] = p;
                  l = nl[j];
                  nl[j] = nl[i];
                  nl[i] = l;
                  SetPoint( vt, va[j] );
                  SetPoint( va[j], va[i] );
                  SetPoint( va[i], vt );
               }
               else // if( va[j][2] < va[i][2] ) // test z level coord...
               {
               }
            }
         }
      }
   }
   /* mating is irrelavant anymore...
   for( v = 0; v < t; v++ )
   {
      PFACET pp;
      PLINESEG pl;
      pp = pps->pPlanes + d[v];
      pl = pp->pLineSet->pLines + np[v];
      sprintf( (char*)byBuffer, "Plane %2d Line %2d mates (%2d in %2d)\n",
                     np[v],
                     nl[v],
                     pl->pOther - pl->pOtherPlane->pLineSet->pLines,
                     pl->pOtherPlane - pps->pPlanes );
      Log( (char*)byBuffer );
      PrintVector( va[v] );
   }
   */
}

void RemoveExtraLines( PFACETSET pps )
{
//   int sl, el;
   int l, l2, p;
   PFACET pp;
//    PLINESEG pl1, pl2;
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      PLINESEGPSET plps;
      pp = pps->pPlanes + p;
      plps = pp->pLineSet;
      for( l = 0; l < plps->nUsedLines; l++ )
      {
         for( l2 = 0; l2 < plps->nUsedLines; l2++ )
         {
            if( l2 == l )
               continue;  // ignore same line on this plane.
         }
         // stuff here?
      }
   }
}


int IntersectPlanes( PLINESEGSET pls, PFACETSET pps, int bAll )
{
   int i, j, k, p, l, l2;
   PFACET pp;
   PLINESEG  pl, tpl;

   for( p = 0; p < pps->nUsedPlanes; p++ )
		      pps->pPlanes[p].pLineSet->nUsedLines = 0;


   // for all combinations of planes intersect them.
   for( i = 0; i < pps->nUsedPlanes; i++ )
      for( j = i + 1; j < pps->nUsedPlanes; j++ )
      {
         // if NO line exists between said planes, line will not be created...
#ifdef FULL_DEBUG
         sprintf( (char*)byBuffer, "------------------------------------------------------------\n");
         Log( (char*)byBuffer );
         sprintf( (char*)byBuffer, "Between %d and %d\n", i, j );
         Log( (char*)byBuffer );
#endif
         pl = CreateLine( pls,
                          pps->pPlanes + i,
                          pps->pPlanes + j ); // link 2 lines together...
                                          // create all possible intersections
         if( pl )
         {
            for( k = 0; k < pps->nUsedPlanes; k++ )
            {
               if( k != i && k != j )
               {
                  RCOORD time;
                  if( IntersectLineWithPlane( pl->d.n
                                            , pl->d.o
                                            , pps->pPlanes[k].d.n
                                            , pps->pPlanes[k].d.o
                                            , &time ) )
                  {
                     if( time < pl->dFrom )
                     {
#ifdef FULL_DEBUG
                        sprintf( (char*)byBuffer, "setting time in dFrom to : %g\n", time );
                        Log( (char*)byBuffer );
#endif
                        pl->dFrom = time;
                     }
                     if( time > pl->dTo )
                     {
#ifdef FULL_DEBUG
                        sprintf( (char*)byBuffer, "settimg time in dTO to : %g \n", time );
                        Log( (char*)byBuffer );
#endif
                        pl->dTo = time;
                     }
                  }
#ifdef FULL_DEBUG
                  else
                  {
                     sprintf( (char*)byBuffer, "okay planes %d, %d and %d do not form a point.\n", i,  j, k ) ;
                     Log( (char*)byBuffer );
                  }
#endif
               }
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
 /*
   if( bAll )
   {
      for( p = 0; p < pps->nUsedPlanes; p++ )
      {
         pp = pps->pPlanes + p;
         for( ip = 0; ip < pps->nUsedPlanes; ip++ )
         {
            PLINESET pls;
            pip = pps->pPlanes + ip;
            pls = pip->pLineSet;
            for( l = 0; l < pls->nUsedLines; l++ )
            {
               pl = pls->pLines + l;
               if( pp != pl->pInPlane && // line is not shared on intersecting plane
                   pp != pl->pOther->pInPlane )
               {
                  RCOORD time;
//                  pl->bDraw = FALSE;
                  pl->bDraw = TRUE;
                  IntersectLineWithPlane( pp, pl, &time );
//                  pl->dTo = time;
//                  pl->dFrom = time;
              }
            }
         }
      }
   }
 */

   DiagnosticDump( pps );

// SICK! SICK! SICK!
// nJoinResult starts at 0
// when a line intersects with another line, LinesJoin returns
// a non-zero value.  If both ends joined then the resulting
// OR(|) will be 3+ depending on if one or the other
// line joined at it's beginning or it's end... otherwise...
// this line joins at the same end of the first line, and the
// bit will already have been set.


   RemoveExtraLines( pps ); // just go ahead and call something to handle this
   return 0;


   {
      static int nPass;
#ifdef FULL_DEBUG
      sprintf( (char*)byBuffer,"Pass %d\n", nPass++);
      Log( (char*)byBuffer );
#endif
// this is to remove extra facets...
      for( p = 0; p < pps->nUsedPlanes; p++ )
      {
         PLINESEGPSET plps;
         pp = pps->pPlanes + p;
         plps = pp->pLineSet;
         for( l = 0; l < plps->nUsedLines; l++ )
         {
            int nJoinResult = 0; // line has not joined anything...
            pl = plps->pLines[l];
            if( !pl->bDraw )
               continue;  // don't do lines we're not going to draw...

               for( l2 = 0; l2 < plps->nUsedLines; l2++ )
               {
                  if( l2 == l )
                     continue;
                  tpl = plps->pLines[l2];
                  if( !tpl->bDraw ) // dont include not drawn lines...
                     continue;

                  nJoinResult |= LinesJoin( nJoinResult, pl, tpl );
                  if( ( nJoinResult & (START|END) ) == (START|END) )
                  {
                     break;
                  }
               } // for line 2
               if( l2 == plps->nUsedLines )
               {
                  pl->bDraw = FALSE;
               }

         } // for line 1
      }
   }
   for( p = 0; p < pps->nUsedPlanes; p++)
   {
      PLINESEGPSET plps;
      pp = pps->pPlanes + p;
      plps = pp->pLineSet;
      for( l = 0; l < plps->nUsedLines; l++ )
      {
         pl = plps->pLines[l];
         if( !pl->bDraw ) // did not find 2 lines for this line.
         {
//             sprintf( (char*)byBuffer,"%d/%d:wontdraw\n", p, l);
//            Log( (char*)byBuffer );
             pl->bUsed = FALSE;  // collect garbage later.
          }
#ifdef PRINT_LINES
          else
          {
            sprintf( (char*)byBuffer,"%d/%d:willdraw\n",p,l);
            Log( (char*)byBuffer );
          }
#endif
      }
   }
   return 0;
}

bool PointWithin( PCVECTOR p, PLINESEGPSET plps )
{
   int l1, l2;
   PLINESEG pl1, pl2 ;
   for( l1 = 0; l1 < plps->nUsedLines; l1++ )
   {
      VECTOR v;
      RCOORD tl, tl2;
      pl1 = plps->pLines[l1];
      sub( v, p, pl1->d.o );
      for( l2 = 0; l2 < plps->nUsedLines; l2++ )
      {
         if( l1 == l2 )
            continue;
         pl2 = plps->pLines[l2];
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
         return false;
   }
   return true;
}
