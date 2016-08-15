#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE


#include "vectlib.h"

#include "plane.h"

typedef VECTOR VELOCITY;
typedef PVECTOR PVELOCITY;
typedef VECTOR ACCELERATION;
typedef PVECTOR PACCELERATION;


typedef struct object_tag
{
   PTRANSFORM T;  // Real transform ( applied all parents to initial )
   PTRANSFORM Ti; // Initial Transform base object transform
   OBJECTINFO objinfo;
   CDATA color; // internal color... used if nothing else is...
   struct {
   	int bInvert:1;
   };
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


extern POBJECT pFirstObject;

//---------------------------------
// in object.c
POBJECT CreateObject( void );
void SetRootObject( POBJECT po );
void FreeObject( POBJECT *ppo );

POBJECT TakeOut( POBJECT out );
POBJECT TakeOff( POBJECT on ); // take this 'on' object 'off' 
POBJECT PutIn( POBJECT _this, POBJECT in ); // put 'this' in 'in'
POBJECT PutOn( POBJECT _this, POBJECT on ); // put 'this' on 'on'

LOGICAL  ObjectOn( POBJECT po, PCVECTOR vforward,
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
int AddPlane( POBJECT pobj, PCVECTOR po, PCVECTOR pn, int d );
#define HANDLE uint32_t

uint32_t SaveObject( HANDLE hFile, POBJECT po );
POBJECT LoadObject( HANDLE hFile );

#define FORALLOBJ( pStart, ppo )  \
         for( ppo = NULL; \
              (ppo)?( ( (ppo)==pStart )?0:ppo):(ppo=pStart); \
              ppo=ppo->pNext )

#endif

