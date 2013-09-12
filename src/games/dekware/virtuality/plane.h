#ifndef FACET_INCLUDED
#define FACET_INCLUDED

#include "vectlib.h"   // includes general operations and MYTYPES.H
#include "image.h"

typedef struct {
   _POINT o;
   _POINT n;
} BASIC_PLANE;

typedef struct position_tag
{
   RAY p;
} POSITION, *PPOSITION;

typedef struct texture_tag
{
   RAY p; // position of the texture on the polygon.
             // origin and orientation.
   Image pImage;
   /*
   LENGTH height, width;
   // FUNCTION * Projector function...
   CDATA *lpData;
   */
} TEXTURE;

typedef struct line_tag
{
   struct {
      int bUsed:1;
      int bDraw:1;
   };
   RAY d;
   union 
   {
      struct plane_tag *pInPlane;  // please insert the plane indexes which are used.
      void *pStuff;
   };
   union {
      struct plane_tag *pOtherPlane;
      void *pOtherStuff;
   };
   struct line_tag *pFrom;  // working data.... pointer to line at dFrom
   struct line_tag *pTo;    // working data.... pointer to line at dTo
   RCOORD dFrom, dTo;  // Time 1 -> Time 2
} LINESEG, *PLINESEG;

typedef struct line_set_tag
{
   int nLines;
   int nUsedLines;
   PLINESEG pLines;
} LINESEGSET, *PLINESEGSET;

typedef struct linep_set_tag
{
   int nLines;
   int nUsedLines;
   PLINESEG *pLines;
} LINESEGPSET, *PLINESEGPSET;

typedef struct plane_tag
{
   struct {
      int bUsed:1;
      int bDraw:1;
      int bInvert:1;  // plane's visible side is the back
      int bDual:1;  // both sides of the plane are visible...
   };
   int nBrain; // peirce sensor to brain....
               // shrug - collistion value for now...

//   struct plane_tag *pNext, *pPrior; // pOther stored in line!!!

//   RCOORD phi;  // stuff - keeps accumulator of -(n[0]*o[0] + n[1]*o[1] + n[2]*o[2] )
   RAY d;
   CDATA color;

   // lines on a plane are the resulting boundry created
   // from intersections with other planes, or loaded from
   // a file.  if a plane is a loaded entity - possibly omit
   // normal and origin information... allow not modifications.
   PLINESEGPSET pLineSet;
} FACET, *PFACET;

typedef struct plane_set_tag
{
   int nPlanes;  // sizeof the array included.....
   int nUsedPlanes;
   PFACET pPlanes;  // dangling - so much the better!
} FACETSET, *PFACETSET;  // open ended array.......

PFACETSET CreatePlaneSet( void );
void DestroyPlaneSet( PFACETSET pps );
PLINESEGSET CreateLineSet( void );
void DestroyLineSet( PLINESEGSET pps );
PLINESEGPSET CreateLinePSet( void );
void DestroyLinePSet( PLINESEGPSET pps );

PLINESEG CreateLine( PLINESEGSET pls, 
                  PCVECTOR po, PCVECTOR pn,
                  RCOORD rFrom, RCOORD rTo );
void AddLineToPlane( PLINESEG pl, PFACET pf );


int IntersectPlanes( PLINESEGSET pls, PFACETSET pPlaneSet, int bAll );
PFACET AddPlaneToSet( PFACETSET pPlaneSet, PCVECTOR origin, PCVECTOR norm, char d );
void DeletePlane( PFACETSET pp, PFACET pDel );


int IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin, 
                            PCVECTOR n, PCVECTOR p, RCOORD *time );

int Parallel( PVECTOR pv1, PVECTOR pv2 );

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 );

int AbovePlane( PVECTOR n, PVECTOR o, PVECTOR p );
int PointWithin( PCVECTOR p, PLINESEGPSET pls ); 
RCOORD PointToPlaneT( PVECTOR n, PVECTOR o, PVECTOR p );

//--------------------------------------------
// okay maybe I'm getting carried away... maybe
// these special cases reduce more???
   


#endif
// $Log: plane.h,v $
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
