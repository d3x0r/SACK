#include <vectlib.h>
#include <virtuality.h>

#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE



typedef VECTOR VELOCITY;
typedef PVECTOR PVELOCITY;
typedef VECTOR ACCELERATION;
typedef PVECTOR PACCELERATION;


typedef struct object_tag
{
	// must be first for linking purposes...
   // sorry charly - this one's fancy.
	DeclareLink( struct object_tag );
   PTRANSFORM T;  // My Base Transform... when this gets updated update Ti...
   PTRANSFORM Ti; // My Transform applied with all containers
   POBJECTINFO objinfo;
   CDATA color; // internal color... used if nothing else is...
   struct {
   	int bInvert:1;
	};
   struct object_tag *pHolds, // pointer to objects 'in' this one
                     *pIn,  // points to object which 'holds' this one
                     *pHas, // points to objects 'On' this one
                     *pOn;  // pointer to object which 'has' this one
} OBJECT, *POBJECT;

//#define FLAGTYPE(v,n) unsigned char v[((n)+7)/8]
//#define SETFLAG(v,n) (v[(n)>>3] |= 1 << ( (n) & 0x7 ))
//#define RESETFLAG(v,n) ( v[(n)>>3] &= ~( 1 << ( (n) & 0x7 ) ) )
//#define TESTFLAG(v,n)  ( v[(n)>>3] & ( 1 << ( (n) & 0x7 ) ) )

#ifndef OBJECT_SOURCE
extern
#endif
struct first_object{ DeclareLink( struct first_object ); }
FirstObject
#ifdef OBJECT_SOURCE
={ NULL, &(FirstObject.next) }
#endif
;
#define pFirstObject FirstObject.next

//---------------------------------
// in object.c
VIRTUALITY_EXPORT POBJECT CreateObject( void );
VIRTUALITY_EXPORT void SetRootObject( POBJECT po );
VIRTUALITY_EXPORT void FreeObject( POBJECT *ppo );

VIRTUALITY_EXPORT POBJECT TakeOut( POBJECT out );
VIRTUALITY_EXPORT POBJECT TakeOff( POBJECT on ); // take this 'on' object 'off' 
VIRTUALITY_EXPORT POBJECT PutIn( POBJECT _this, POBJECT in ); // put 'this' in 'in'
VIRTUALITY_EXPORT POBJECT PutOn( POBJECT _this, POBJECT on ); // put 'this' on 'on'

VIRTUALITY_EXPORT LOGICAL  ObjectOn( POBJECT po, PCVECTOR vforward,
                           PCVECTOR vright,
                           PCVECTOR vo );


VIRTUALITY_EXPORT POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, 
                                 RCOORD fSize, PCVECTOR pv, 
                                 PCVECTOR pforward, 
                                 PCVECTOR pright,
                                 PCVECTOR pup );
//POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, RCOORD fSize, PVECTOR pv );
VIRTUALITY_EXPORT void SetObjectColor( POBJECT po, CDATA c ); // dumb routine!
VIRTUALITY_EXPORT void InvertObject( POBJECT po );
VIRTUALITY_EXPORT POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR pr, 
                     RCOORD size, CDATA c ); // not a real plane....
VIRTUALITY_EXPORT int AddPlane( POBJECT pobj, PCVECTOR po, PCVECTOR pn, int d );
#define HANDLE _32

VIRTUALITY_EXPORT _32 SaveObject( HANDLE hFile, POBJECT po );
VIRTUALITY_EXPORT POBJECT LoadObject( HANDLE hFile );

#define FORALLOBJ( pStart, ppo )  \
         for( ppo = NULL; \
              (ppo)?( ( (ppo)==pStart )?0:ppo):(ppo=pStart); \
              ppo = NextLink( ppo ) )

#endif

