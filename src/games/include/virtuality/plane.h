#ifndef FACET_INCLUDED
#define FACET_INCLUDED
#include <virtuality.h>

#include <vectlib.h>   // includes general operations and MYTYPES.H
#include <image.h>     // texture image...
#include <colordef.h>

//#include "../box2d/include/box2d.h"
//#include "Engine/Dynamics/b2World.h"
//#include "Engine/Collision/b2Shape.h"
//#include "Engine/Dynamics/Joints/b2RevoluteJoint.h"

	
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

typedef struct my_line_tag
{
	LINESEG l;
	struct {
		VECTOR at_from;
		VECTOR at_to;
	} normals; 
   struct {
      int bUsed:1;
      int bDraw:1;
	};
   // this data is unusable - since with multiple forms(sets of facets)
   // this line MAY be contained in more than 2 planes...
   // need a way to reference planes that contain this segment
   // such that when the line is updated all mating lines 
   // in all mating facets may be updated.
	//PFACETREFSET frs;
	struct facet_tag *facets[3]; // only in a mathematically perfect world do more than 2 facets make a line.
     	// and landscape-zone-based engines (which has invisible walls at each ground-fracture line.
} MYLINESEG, *PMYLINESEG;

#define MAXMYLINESEGSPERSET 512
DeclareSet( MYLINESEG );
typedef struct linep_tag
{
	// index of -1 is INVALID or DELETED...
	// so if the line is within nUsedLines, and 
	// is not less than 0 it is a valid index...
	PMYLINESEG pLine;  // index of line in line pool
	struct {
		// nLineTo/From accounts for 1000 lines in a closed facet (excessive)
		//_32 nLineFrom : 10;
		//_32 nLineTo : 10;
		_32 bOrderFromTo : 1;
	};
   // nordering from, to
	int nLineFrom; // index of line at dFrom of this line
	int nLineTo; // index of line at dTo of this line
	//int bOrderFromTo; // while traversing the ordered list of lines
							// point from is first then point to...
							// otherwise point to is first and point from is
							// next...
} LINESEGP, *PLINESEGP;

#define MAXLINESEGPSPERSET 16
DeclareSet( LINESEGP );

// this is in groups of facet sets...
#define MAXLINESEGPSETSPERSET 64
DeclareSet( LINESEGPSET );

typedef struct facet_tag
{
   struct {
      unsigned int bUsed:1; // facet allocated, and contains usable data
      unsigned int bDraw:1; // flag that it's not clipped? or that it's visible side is...
      unsigned int bInvert:1;  // plane's visible side is the back
      unsigned int bDual:1;  // both sides of the plane are visible...
      unsigned int bShared:1; // both sides are transparent?
		unsigned int bPointNormal:1;
		unsigned int bNormalSurface : 1; // lines on plane make up points of texture with light normals.
		unsigned int bClipOnly : 1; // only clips another plane, is not visible.
   }flags;
   // this can/will be used at one point - but is not now...
   /*
   int nBrain; // peirce sensor to brain....
               // shrug - collistion value for now...
   */
   RAY d;
	CDATA color; // if zero then uses object color.
	Image image;  // if not NULL, use this image instead of color; if color is also not zero, use that as a single-light shader
   // TEXTURE texture; // includes coordinate on texture and texture reference.

   // lines on a plane are the resulting boundry created
   // from intersections with other planes, or loaded from
   // a file.  if a plane is a loaded entity - possibly omit
   // normal and origin information... disallow modifications.
   PLINESEGPSET pLineSet; // line references in object line pool...
   
   //b2World *world;

} FACET, *PFACET;

#define MAXFACETSPERSET 256
DeclareSet( FACET );

typedef struct facetp_tag 
{
	PFACET pFacet;
	struct {
		int bInvert:1; // when intersecting this consider clip below instead above
	};
} FACETP, *PFACETP;


#define MAXFACETSETSPERSET 1024
DeclareSet( FACETSET );

typedef struct object_info_tag
{
	// this is taken from the cluster, so all objects within
   // a cluster use the same pool...
	PMYLINESEGSET *ppLinePool;
	PLIST lines;
	PLINESEGPSETSET *ppLineSegPPool;
	PLIST facets; // added list of facets...
	PFACETSET *ppFacetPool;
	PFACETSETSET *ppFacetSetPool;
	//PFACETREFSETSET FacetRefSetPool;
	struct cluster_info_tag *cluster;
} OBJECTINFO, *POBJECTINFO;

#define MAXOBJECTINFOSPERSET 256
DeclareSet( OBJECTINFO );

// a grouping of objects...
// so that lineseg sets may be used with less
// underflow
typedef struct cluster_info_tag
{
	POBJECTINFOSET objects;
	PMYLINESEGSET LinePool;
	PLINESEGPSETSET LineSegPPool;
	PFACETSET FacetPool;
	PFACETSETSET FacetSetPool;
	//PFACETREFSETSET FacetRefSetPool;
} CLUSTER, *PCLUSTER;

int GetFacetSet( OBJECTINFO *oi );
int GetFacetP( OBJECTINFO *oi, int nfs );
int GetFacet( OBJECTINFO *oi );

int GetLineSeg( OBJECTINFO *oi );

int GetLineSegP( PLINESEGPSETSET *pplpss, PLINESEGPSET *pplps );

PMYLINESEG CreateLine( OBJECTINFO *oi, 
                  PCVECTOR po, PCVECTOR pn,
                  RCOORD rFrom, RCOORD rTo );
PMYLINESEG CopyLine( OBJECTINFO *oi,
                  	 PMYLINESEG orig );

void AddLineToPlane( OBJECTINFO *oi, PFACET pf, PMYLINESEG pl );
PFACET AddPlaneToSet( OBJECTINFO *oi, PCVECTOR origin, PCVECTOR norm, int d );

//int CreateLineBetweenFacets( OBJECTINFO *oi, int nfs, int np1, int np2 );


int IntersectPlanes( OBJECTINFO *oi, int bAll );

void DeletePlane( PFACETSET pp, PFACET pDel );


//RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,
//                            PCVECTOR n, PCVECTOR p, RCOORD *time );

int Parallel( PVECTOR pv1, PVECTOR pv2 );

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 );

LOGICAL AbovePlane( PVECTOR n, PVECTOR o, PVECTOR p );
int PointWithin( PCVECTOR p, PMYLINESEGSET *ppls, PLINESEGPSET *pplps );
RCOORD PointToPlaneT( PVECTOR n, PVECTOR o, PVECTOR p );
void DiagnosticDump( PMYLINESEGSET *ppls, PFACETSET pps );

// returns FALSE  if the number of points from pf exceeds the value
// passed as the original value of the integer pointer nPoints
// PVECTOR is a poiner to an array of POINTS the number of elements
// is orininally passed as the value in *nPoints
int GetPoints( PFACET pf, int *nPoints, VECTOR ppv[] );
int GetNormals( PFACET pf, int *nPoints, VECTOR ppv[] );

	static void MarkTick( _64 *var)  {
#ifdef __WATCOMC__
	extern void GetCPUTicks();
#pragma aux GetCPUTicks = "rdtsc"   \
   "mov ecx, var"                    \
	"mov dword ptr [ecx], eax"    \
	"mov dword ptr [ecx+4], edx "
	GetCPUTicks();
#elif defined( GCC )
	_64 tick;
	asm( "rdtsc\n" : "=A"(tick) );
#endif

}
#define MarkTick(n) MarkTick( &(n) )



#endif
