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

typedef RCOORD XYPOINT[2];

typedef struct texture_tag
{
	// d in texture space is realtive to the plane
	// the length of the normal determins the overall scaling
	// of the texture...
   RAY d; // position of the texture on the polygon.
             // origin and orientation.
	_POINT Right;
   Image pImage;
   XYPOINT *pCorners;
   // perhaps include a function method
   // for how to draw the info (transparency/shading)
} TEXTURE;

#define InitSet( pset, name ) { (pset)->n##name = 0; (pset)->nUsed##name = 0; (pset)->p##name = NULL; }

#define AllocateSet(struc,name)                    \
      if( ((struc)->n##name) == (struc)->nUsed##name )   \
      {                                            \
         void *pt;                                 \
         (struc)->n##name += 16;                   \
         pt = Allocate( sizeof( *((struc)->p##name) ) *  \
                      (struc)->n##name );          \
         memcpy( pt,                               \
                 (struc)->p##name,                 \
                 sizeof( *((struc)->p##name) ) *   \
                 (struc)->nUsed##name );           \
         MemSet( (char*)pt +                       \
                 sizeof( *((struc)->p##name) ) *   \
                 (struc)->nUsed##name,             \
                 0,                                \
                 sizeof( *((struc)->p##name) ) *   \
                 16 );                             \
         Release( (struc)->p##name );              \
         (*((void**)(&(struc)->p##name))) = pt;    \
      }
#define EmptySet(struc, name) { if( (struc)->p##name ) { Release( (struc)->p##name ); (struc)->nUsed##name = 0; (struc)->p##name = NULL;} }

typedef struct facet_referenece_tag
{
	int nFacetSet;
	int nFacet;
} FACETREF, *PFACETREF;

typedef struct planep_set_tag
{
   int nFacets;  // sizeof the array included.....
   int nUsedFacets;
   PFACETREF pFacets;  // this has to be a relative index because of reallocation
} FACETREFSET, *PFACETREFSET;  // open ended array.......

typedef struct line_tag
{
   struct {
      int bUsed:1;
      int bDraw:1;
   };
   RAY d;
   RCOORD dFrom, dTo;
   // this data is unusable - since with multiple forms(sets of facets)
   // this line MAY be contained in more than 2 planes...
   // need a way to reference planes that contain this segment
   // such that when the line is updated all mating lines 
   // in all mating facets may be updated.
   FACETREFSET frs;
} G_LINESEG, *PG_LINESEG;

typedef struct line_set_tag
{
   int nLines;
   int nUsedLines;
   PG_LINESEG pLines;
} LINESEGSET, *PLINESEGSET;

typedef struct linep_tag
{
		// index of -1 is INVALID or DELETED...
		// so if the line is within nUsedLines, and 
		// is not less than 0 it is a valid index...
	int nLine;  // index of line in line pool
	int nLineFrom; // index of line at dFrom of this line
	int nLineTo; // index of line at dTo of this line
	int bOrderFromTo; // while traversing the ordered list of lines
							// point from is first then point to...
							// otherwise point to is first and point from is
							// next...
} LINESEGP, *PLINESEGP;


typedef struct linep_set_tag
{
   int nLines;
   int nUsedLines;
   PLINESEGP pLines; // this has to be a relative index because of reallocation
} LINESEGPSET, *PLINESEGPSET;

typedef struct facet_tag
{
   struct {
      int bUsed:1; // facet allocated, and contains usable data
      int bDraw:1; // flag that it's not clipped? or that it's visible side is...
      int bInvert:1;  // plane's visible side is the back
      int bDual:1;  // both sides of the plane are visible...
      int bShared:1; // both sides are transparent?
   };
   // this can/will be used at one point - but is not now...
   /*
   int nBrain; // peirce sensor to brain....
               // shrug - collistion value for now...
   */
   RAY d;
   // TEXTURE texture;

   // lines on a plane are the resulting boundry created
   // from intersections with other planes, or loaded from
   // a file.  if a plane is a loaded entity - possibly omit
   // normal and origin information... disallow modifications.
   LINESEGPSET pLineSet; // line references in object line pool...

} FACET, *PFACET;

typedef struct facet_set_tag
{
   int nFacets;  // sizeof the array included.....
   int nUsedFacets;
   PFACET pFacets;  
} FACETSET, *PFACETSET;  // open ended array.......

typedef struct facetp_tag 
{
	int nFacet;
	struct {
		int bInvert:1; // when intersecting this consider clip below instead above
	};
} FACETP, *PFACETP;

typedef struct facetp_set_tag
{
	int nFacets;
	int nUsedFacets;
	PFACETP pFacets;
} FACETPSET, *PFACETPSET;

typedef struct facet_set_set_tag
{
	int nFacetSets;
	int nUsedFacetSets;
	PFACETPSET pFacetSets;
} FACETSETSET, *PFACETSETSET;

typedef struct object_info_tag
{
	LINESEGSET LinePool;
	FACETSET FacetPool;
	FACETSETSET FacetSetPool;
} OBJECTINFO, *POBJECTINFO;

int GetFacetSet( OBJECTINFO *oi );
int GetFacetP( OBJECTINFO *oi, int nfs );
int GetFacet( OBJECTINFO *oi );

int GetLineSeg( OBJECTINFO *oi );

int GetLineSegP( PLINESEGPSET plps );

int CreateLine( OBJECTINFO *oi, 
                  PCVECTOR po, PCVECTOR pn,
                  RCOORD rFrom, RCOORD rTo );

void AddLineToPlane( OBJECTINFO *oi, int nfs, int nf, int nl );

int CreateLineBetweenFacets( OBJECTINFO *oi, int nfs, int np1, int np2 );


int IntersectPlanes( OBJECTINFO *oi, int nfs, int bAll );

void DeletePlane( PFACETSET pp, PFACET pDel );


//RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,
//                            PCVECTOR n, PCVECTOR p, RCOORD *time );

int Parallel( PVECTOR pv1, PVECTOR pv2 );

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 );

LOGICAL AbovePlane( PVECTOR n, PVECTOR o, PVECTOR p );
int PointWithin( PCVECTOR p, PLINESEGSET pls, PLINESEGPSET plps );
RCOORD PointToPlaneT( PVECTOR n, PVECTOR o, PVECTOR p );
void DiagnosticDump( PLINESEGSET pls, PFACETSET pps );

// returns FALSE  if the number of points from pf exceeds the value
// passed as the original value of the integer pointer nPoints
// PVECTOR is a poiner to an array of POINTS the number of elements
// is orininally passed as the value in *nPoints
int GetPoints( PLINESEGSET pls, PFACET pf, int *nPoints, VECTOR ppv[] );

#endif
