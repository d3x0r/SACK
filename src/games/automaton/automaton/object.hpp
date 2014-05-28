#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE


#include <vectlib.h>

//#include "vector.h" // basic vector operations 
typedef RAY BASIC_PLANE;
#include "Plane.hpp"

typedef struct object_tag
{
   TRANSFORM T;
   PLINESEGSET pLinePool;  // all lines used in object....
                        // these provide shared lines between planes...
   PFACETSET pPlaneSet;
   struct object_tag *pNext, *pPrior, 
                     *pHolds, // pointer to objects 'in' this one
                     *pIn,  // points to object which 'holds' this one
                     *pHas, // points to objects 'On' this one
                     *pOn;  // pointer to object which 'has' this one
} OBJECT, *POBJECT;

//#define FLAGTYPE(v,n) unsigned char v[((n)+7)/8]
//#define SETFLAG(v,n) (v[(n)>>3] |= 1 << ( (n) & 0x7 ))
//#define RESETFLAG(v,n) ( v[(n)>>3] &= ~( 1 << ( (n) & 0x7 ) ) )
//#define TESTFLAG(v,n)  ( v[(n)>>3] & ( 1 << ( (n) & 0x7 ) ) )

enum {
   SPACE_ISOPEN, // not on a body of space
   NUM_FLAGS
};

typedef struct spacetree_tag
{
   FLAGSET(flags, NUM_FLAGS);
   unsigned char *pData; // space configuration data - (gravity, direction)?
   POBJECT *pWithin;
   struct spacetree_tag *pLinkedTo;
} SPACETREE, *PSPACETREE;

extern POBJECT pFirstObject;

//---------------------------------
// in object.c
POBJECT CreateObject( void );
void FreeObject( POBJECT *ppo );

POBJECT TakeOut( POBJECT out );
POBJECT TakeOff( POBJECT on ); // take this 'on' object 'off' 
POBJECT PutIn( POBJECT _this, POBJECT in ); // put 'this' in 'in'
POBJECT PutOn( POBJECT _this, POBJECT on ); // put 'this' on 'on'

bool ObjectOn( POBJECT po, PCVECTOR vforward,
                           PCVECTOR vright,
                           PCVECTOR vo );


POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, 
                                 RCOORD fSize, PCVECTOR pv, 
                                 PCVECTOR pforward, 
                                 PCVECTOR pright,
                                 PCVECTOR pup );
//POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, RCOORD fSize, PVECTOR pv );
void SetObjectColor( POBJECT po, CDATA c ); // dumb routine!
void InvertObject( POBJECT po );
POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR pr, 
                     RCOORD size, CDATA c ); // not a real plane....

_32 SaveObject( int hFile, POBJECT po );
POBJECT LoadObject( int hFile );

#define FORALL( pStart, ppo )  \
         for( POBJECT pobj = NULL; \
              (pobj)?(((pobj)==pStart)?0:*(ppo) = pobj):(pobj=*(ppo)=pStart); \
              pobj=pobj->pNext )

#endif

