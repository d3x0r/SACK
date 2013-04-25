#include <malloc.h>
#include <string.h> // memcpy etc...
#include <stdio.H>

#include <sharemem.h>

#include "motion.hpp"
#include "object.hpp"  // includes vector.h .... link motion to objects....
extern "C" {
#include "keybrd.H"
}

extern VECTOR KeySpeed;
extern VECTOR KeyRotation;

PMOTION pFirstMotion;
PMOTION pLast;

PMOTION CreateMotion( POBJECT pObject, int nType )
{
   PMOTION pMotion;
#ifdef LOG_ALLOC
   printf("malloc(MOTION)\n");
#endif
   pMotion              = (PMOTION)Allocate( sizeof( MOTION ) );
   memset( pMotion, 0, sizeof( MOTION ) );
   pMotion->pObject     = pObject;
   pMotion->pNext       = pFirstMotion;
   pMotion->pPrior      = NULL;
   if( pFirstMotion )
      pFirstMotion->pPrior = pMotion;
   pFirstMotion         = pMotion;
   return pMotion;
}

void FreeMotion( PMOTION pMotion )
{
   if( pMotion->pNext )
      pMotion->pNext->pPrior = 0;
   if( pMotion->pPrior )
      pMotion->pPrior->pNext = 0;
   else
      pFirstMotion = pMotion->pNext;
#ifdef LOG_ALLOC
   printf("Free(MOTION)\n");
#endif
   Release( pMotion );

}
/*
void MotionComputeFollow( PMOTION pMotion )
{
   POBJECT pPrior, po;
   pPrior = (po = pMotion->pObject )->pPrior;
   if( pPrior && po )
   {
      memcpy( po->pr.r, pPrior->pr.r );
      if( Distance( po->pr.o, pPrior->pr.o ) > 75 )
      {
         sub( Slope, pPrior->pr.o, po->pr.o );
         Scale( Slope, Slope, 0.1 );
         add( po->pr.o, po->pr.o, Slope );
      }
   }
}
*/

void KeyboardMove( TRANSFORM &t )
{
   t.SetSpeed( KeySpeed );
   t.SetRotation( KeyRotation );
   t.Move();
}

// this is sort of a hybrid routine between motion.c and vector.c
// because some of the information about motino is stored with the
// translations.

void ApplyDynamics( void )
{
   PMOTION pMotion;
   POBJECT pObject;
   POBJECT pol = NULL;
      int nObjects;

   // Update all objects which have motion descriptors.
   nObjects = 0;
   pMotion = pFirstMotion;
   if( pLast )
      pol = pLast->pObject;
   while( pMotion )
   {
      nObjects++;
      if( pol && pLast->pObject != pol )
         printf(" going to break at %d\n", nObjects );
      pObject = pMotion->pObject;
      switch( pMotion-> nType )
      {
      case KEYBOARD:
         KeyboardMove( pObject->T );
         break;
      case FOLLOW_PRIOR:
//         MotionComputeFollow( pMotion );
         break;
      }
      pMotion = pMotion->pNext;
   }
}



