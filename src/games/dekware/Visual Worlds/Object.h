#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE

#include "matrix.h"

//#include "vector.h" // basic vector operations 

#include "Plane.h"

typedef struct object_tag OBJECT, *POBJECT;

typedef struct object_tag
{
   TRANSFORM T;
   TRANSFORM Ti; // applied my transform with parent's...

   PLINESEGSET pLinePool;  // all lines used in object....
   //                     // these provide shared lines between planes...
   PFACETSET pPlaneSet;
   struct {
      unsigned int Closed:1;
      unsigned int Checked:1; // clear when update.
      unsigned int Updated:1;
      unsigned int Inverted:1;
      unsigned int CollideAsParent:1; // hmm dunno....
   }flags;
//   POBJECT pSelf; // 
   int nCurrentFacet; // >0 if and only if is in a parent's facet.
   POBJECT pNext,  // ppNext
           pPrior;
   POBJECT pHolds; // pointer to objects 'in' this one
   POBJECT pIn;  // points to object which 'holds' this one
} OBJECT, *POBJECT;

#define FLAGTYPE(v,n) unsigned char v[((n)+7)/8]
#define SETFLAG(v,n) (v[(n)>>3] |= 1 << ( (n) & 0x7 ))
#define RESETFLAG(v,n) ( v[(n)>>3] &= ~( 1 << ( (n) & 0x7 ) ) )
#define TESTFLAG(v,n)  ( v[(n)>>3] & ( 1 << ( (n) & 0x7 ) ) )

enum {
   SPACE_ISOPEN, // not on a body of space
   NUM_FLAGS
};

typedef struct spacetree_tag
{
   FLAGTYPE(flags, NUM_FLAGS);
   unsigned char *pData; // space configuration data - (gravity, direction)?
   POBJECT *pWithin;
   struct spacetree_tag *pLinkedTo;
} SPACETREE, *PSPACETREE;

extern POBJECT pFirstObject;

//---------------------------------
// in object.c
POBJECT CreateObject( void );
//                             'o'rigin and 'r'otation
POBJECT CreateObjectAt( PVECTOR vo, PVECTOR vr );
void FreeObject( POBJECT *ppo );

POBJECT TakeOut( POBJECT out ); // whatever it is in - it is out of now...
POBJECT PutIn( POBJECT _this, POBJECT in ); // put 'this' in 'in'
POBJECT PullOut( POBJECT _this, POBJECT in ); // 'in' out of 'this'

#define CHILD_OF 0
#define BROTHER_OF 1
#define PARENT_OF 2
void Relate( POBJECT po, int is, POBJECT pto );
//#define PutOn PutIn //radar.hpp
void SubstObject( POBJECT po, POBJECT pfor );

int PointInObject( PCVECTOR vo, POBJECT po
                  , PCVECTOR vforward
                  , PFACET *pPlane );


POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, 
                                 RCOORD fSize, PCVECTOR pv, 
                                 PCVECTOR pforward, 
                                 PCVECTOR pright,
                                 PCVECTOR pup );
//POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, RCOORD fSize, PVECTOR pv );
void SetObjectColor( POBJECT po, CDATA c ); // dumb routine!
void InvertObject( POBJECT po );
PFACET ObjectAddPlane( POBJECT pobj, PCVECTOR po, PCVECTOR pn, int d, CDATA color );
POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR pr, 
                     RCOORD size, CDATA c ); // not a real plane....

DWORD SaveObjectEx( HANDLE hFile, POBJECT po );
void SaveObject( POBJECT po );
POBJECT LoadObjectEx( HANDLE hFile );
POBJECT LoadObject( void );

#define FORALL( pStart, ppo )  \
         for( pobj = NULL; \
              (pobj)?(((pobj)==pStart)?0:(*(ppo) = pobj)):(pobj=(*(ppo)=pStart)); \
              pobj=pobj->pNext )
#define FORALL2( pStart, ppo )  \
         for( pobj2 = NULL; \
              (pobj2)?(((pobj2)==pStart)?0:(*(ppo) = pobj2)):(pobj2=(*(ppo)=pStart)); \
              pobj2=pobj2->pNext )

#endif

// $Log: Object.h,v $
// Revision 1.2  2003/03/25 08:59:01  panther
// Added CVS logging
//
