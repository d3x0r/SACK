#ifndef FACET_INCLUDED
#define FACET_INCLUDED

#include "matrix.h"   // includes general operations and MYTYPES.H

#include "image.h"


typedef struct poly_point_tag {
   VECTOR p;
   CDATA color;
   RCOORD u, v; // texture mounting point...
} POLYPOINT, *PPOLYPOINT;

typedef struct point_set_tag
{
   int nPoints;
   int nUsedPoints;
   PPOLYPOINT pPoints;
} POINTSET, *PPOINTSET;

typedef struct line_tag
{
   struct {
      int bUsed:1;
      int bDraw:1;
		int bDrawn:1; 
		int bVisible:1;
//		int bConstructing:1;
   };
	RAY d; // only update x and y of this - z is maintained for conversion?
	RCOORD dFrom, dTo;  // Time 1 -> Time 2

	VECTOR points[2]; // projection points after clipping....
   union 
   {
      int nInPlane;
      void *pStuff; // for Stone Data... which we are building...
   };
   union {
      int nInOtherPlane;
      void *pOtherStuff;
   };
   int nPass; // pass counter for outer looping...
} LINESEG, *PLINESEG;


typedef struct line_set_tag
{
   int nLines;
   int nUsedLines;
   PLINESEG pLines;
} LINESEGSET, *PLINESEGSET;

typedef struct line_link_tag
{
   // must register methods...
   int nLineSegMethod; // passed planes, lines...
//   void (*LineSegMethod)( int nPlane, int nLine );
   int nLineFrom,  // line at the 'from' coordinate.. (facet index)
       nLineTo;    // line at the 'to' coordinate...(facet index)
   int nLine;  // index of this line's coordinate info
} LINELINK, *PLINELINK;


typedef struct linep_set_tag
{
   int nLines;
   int nUsedLines;
   PLINESEGSET *ppls; // pointer to object's base line pool...
   volatile PLINELINK pLines; // pointer to array of links...
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
   RAY d; // should be able to create a rotation matrix and apply to all lines.
   PPOINTSET Points;
   CDATA color;
//   TEXTURE texture; 
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
   volatile PFACET pPlanes;  // dangling - so much the better!
} FACETSET, *PFACETSET;  // open ended array.......


void AddPoint( PPOINTSET ppts, PVECTOR p, CDATA c, RCOORD u, RCOORD v );

PFACETSET CreatePlaneSet( void );
void DestroyPlaneSet( PFACETSET pps );
PLINESEGSET CreateLineSet( void );
void DestroyLineSet( PLINESEGSET pps );
PLINESEGPSET CreateLinePSet( PLINESEGSET *ppBase );
void DestroyLinePSet( PLINESEGPSET pps );

PLINESEG GetLineEx( PLINESEGSET pls, BOOL bUseDeleted );
#define GetLine(pls) GetLineEx(pls, TRUE )
int CreateLine( PLINESEGSET pls, 
                  PCVECTOR po, PCVECTOR pn,
                  RCOORD rFrom, RCOORD rTo );
int AddLineToPlane( int nl, PFACET pf ); // index line link PLINESEGPSET


int IntersectPlanes( PFACETSET pPlaneSet, int bAll );
void BuildPointSets( PLINESEGSET pls, PFACETSET pfs );

// pNew is assumed to not yet be part of PFACETSET...
int IntersectNewPlane( PFACETSET pps, int nNew );

int AddPlane( PLINESEGSET *ppls, PFACETSET pPlaneSet, PCVECTOR origin, PCVECTOR norm, char d, CDATA color );
void DeletePlane( PFACETSET pp, PFACET pDel );


int IntersectLineWithPlaneEx( PCVECTOR Slope, PCVECTOR Origin, 
                            PCVECTOR n, PCVECTOR p, RCOORD *time, BOOL *bAligned );
#define IntersectLineWithPlane( m, b, n ,o,  t ) IntersectLineWithPlaneEx( m,b,n,o,t,NULL)

int Parallel( PVECTOR pv1, PVECTOR pv2 );

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 );

int PointWithin( PCVECTOR p, PLINESEGPSET pls ); 

int PointOnLine( PVECTOR pp, PRAY Line );

//--------------------------------------------
// okay maybe I'm getting carried away... maybe
// these special cases reduce more???
   
RCOORD PointToPlaneT( PVECTOR n, PVECTOR o, PCVECTOR p );

#define AbovePlane( n, o, p ) ( PointToPlaneT( n, o, p ) > 0.001 )
       


#endif
// $Log: Plane.h,v $
// Revision 1.2  2003/03/25 08:59:01  panther
// Added CVS logging
//
