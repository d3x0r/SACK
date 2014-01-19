#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE

#include <sack_types.h>

#include "vectlib.h"

#include "Plane.h"

typedef VECTOR VELOCITY;
typedef PVECTOR PVELOCITY;
typedef VECTOR ACCELERATION;
typedef PVECTOR PACCELERATION;


typedef struct object_tag
{
   PTRANSFORM T;
   PLINESEGSET pLinePool;  // all lines used in object....
                        // these provide shared lines between planes...
   PFACETSET pPlaneSet;
   struct object_tag *pNext, *pPrior, 
                     *pHolds, // pointer to objects 'in' this one
                     *pIn,  // points to object which 'holds' this one
                     *pHas, // points to objects 'On' this one
                     *pOn;  // pointer to object which 'has' this one
} OBJECT, *POBJECT;


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

int ObjectOn( POBJECT po, PCVECTOR vforward,
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

DWORD SaveObject( HANDLE hFile, POBJECT po );
POBJECT LoadObject( HANDLE hFile );

#define FORALLOBJ( pStart, ppo )  \
         for( ppo = NULL; \
              (ppo)?( ( (ppo)==pStart )?0:ppo):(ppo=pStart); \
              ppo=ppo->pNext )

#endif

// $Log: object.h,v $
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
