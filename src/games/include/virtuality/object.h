#include <stdhdrs.h>
#include <vectlib.h>
#include <virtuality.h>

#ifndef OBJECT_INCLUDE
#define OBJECT_INCLUDE



typedef VECTOR VELOCITY;
typedef PVECTOR PVELOCITY;
typedef VECTOR ACCELERATION;
typedef PVECTOR PACCELERATION;


struct object_tag
{
	// must be first for linking purposes...
   // sorry charly - this one's fancy.
	DeclareLink( struct object_tag );
   Image hud_icon; // this is rendered true size true depth. transformed to the object origin.
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
};

//#define FLAGTYPE(v,n) unsigned char v[((n)+7)/8]
//#define SETFLAG(v,n) (v[(n)>>3] |= 1 << ( (n) & 0x7 ))
//#define RESETFLAG(v,n) ( v[(n)>>3] &= ~( 1 << ( (n) & 0x7 ) ) )
//#define TESTFLAG(v,n)  ( v[(n)>>3] & ( 1 << ( (n) & 0x7 ) ) )

#ifndef OBJECT_SOURCE
extern
#endif
POBJECT //struct first_object{ DeclareLink( struct first_object ); }
FirstObject
#ifdef OBJECT_SOURCE
//={ NULL, &(FirstObject.next) }
#endif
;
#define pFirstObject FirstObject //FirstObject.next

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

VIRTUALITY_EXPORT POBJECT MakeScaledInstance( POBJECT pDefs
								, RCOORD fSize, PCVECTOR pv, 
                                 PCVECTOR pforward, 
                                 PCVECTOR pright,
                                 PCVECTOR pup );

VIRTUALITY_EXPORT POBJECT MakeGlider( void );

//POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, RCOORD fSize, PVECTOR pv );
VIRTUALITY_EXPORT void SetObjectColor( POBJECT po, CDATA c ); // dumb routine!
VIRTUALITY_EXPORT void InvertObject( POBJECT po );
VIRTUALITY_EXPORT POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR pr, 
                     RCOORD size, CDATA c ); // not a real plane....
VIRTUALITY_EXPORT PFACET AddPlane( POBJECT pobj, PCVECTOR po, PCVECTOR pn, int d );

/* same as above, but the behavior of the points defining lines changes. */
/* origin, normal of line is not directional, but is a light vector with the origin
 being the point at the start of the line, and the normal being the light normal */
/* dfrom and dto are meaningless */
/* paramters passed to the plane affect it's physical orientation for culling */
VIRTUALITY_EXPORT PFACET AddNormalPlane( POBJECT object, PCVECTOR o, PCVECTOR n, int d );

VIRTUALITY_EXPORT int IntersectObjectPlanes( POBJECT object );
#ifndef __WINDOWS__

#define HANDLE int
#endif

VIRTUALITY_EXPORT _32 SaveObject( HANDLE hFile, POBJECT po );
VIRTUALITY_EXPORT POBJECT LoadObject( HANDLE hFile );

#define FORALLOBJ( pStart, ppo )  \
        for( ppo = pStart; \
	ppo ; \
              ppo = NextThing( ppo ) )

#endif

VIRTUALITY_EXPORT void VirtualityUpdate( void ); // move all objects one tick.

VIRTUALITY_EXPORT PMYLINESEG CreateFacetLine( POBJECT pobj,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo );
VIRTUALITY_EXPORT PMYLINESEG CreateNormaledFacetLine( POBJECT pobj,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo
					, PCVECTOR normal_at_from, PCVECTOR normal_at_to );

VIRTUALITY_EXPORT void OrderObjectLines( POBJECT po );
VIRTUALITY_EXPORT void AddLineToObjectPlane( POBJECT po, PFACET pf, PMYLINESEG pl );



VIRTUALITY_EXPORT Image GetHudIcon( POBJECT po );

// identity_depth this is relative to how the display was opened.  identity is where there is a 1:1 relation
// between rendered image pixels and physical display pixels.
// This may be arbitrarily overridden also in case the physical display dpi is too dense.
VIRTUALITY_EXPORT Image DrawHudIcon( POBJECT po, PTRANSFORM camera, RCOORD identity_depth );
