#define ROTATION_DELTA 0.200F
#define ROTATION_ACCEL 0.05F
#define KEY_VELOCITY 50.0F
#define KEY_ACCEL 1.0F

#include <string.h>
extern "C" {
#include "keybrd.h"
}

#include "object.hpp"

#include <vectlib.h>
#include <render.h>
VECTOR KeyRotation, KeySpeed;

extern POBJECT pFirstObject;

POBJECT pfo; // pointer focus object.

TRANSFORM T;

void ScanKeyboard( PRENDERER display )
{
   PVECTOR v;
   RCOORD max, step;
   // how does this HOOK into the camera update functions?!
   if( IsKeyDown( display, KEY_X ) || IsKeyDown( display, KEY_Q ))
   {
      extern unsigned char gbExitProgram;
      gbExitProgram = TRUE;
   }

      if( pFirstObject )
      {
         if( KeyDown( display, KEY_A ) ) 
         {
            PFACETSET pps;
            VECTOR v;
            T.RotateAbs( 0.1f, 0, 0 );
            pps = pFirstObject->pPlaneSet;
            T.ApplyRotation( v, pps->pPlanes->d.n );
            SetPoint( pps->pPlanes->d.n, v );
            IntersectPlanes( pFirstObject->pLinePool,
                             pps, TRUE );
         }
         if( KeyDown( display, KEY_Z ) )
         {
            PFACETSET pps;
            VECTOR v;
            T.RotateAbs( -0.1f, 0, 0 );
            pps = pFirstObject->pPlaneSet;
            T.ApplyRotation( v, pps->pPlanes->d.n );
            SetPoint( pps->pPlanes->d.n, v );
            IntersectPlanes( pFirstObject->pLinePool,
                             pps, TRUE );
         }
      }

#define Relax(V, n) ( ( (V[n]>0.001) || ( V[n]<-0.001) ) ? \
                      (V[n] /= 2.0f) : (V[n] = 0)  )

#define Accel(V, n)        \
          ( ( V[n] < max )  \
            ? V[n] += step       \
            : V[n] = max ) 

#define Decel(V,n)                   \
          ( ( V[n] > -max )  \
            ? V[n] -= step       \
            : V[n] = -max ) 

#define ROTATION (v=KeyRotation, \
                  max = ROTATION_DELTA, \
                  step=ROTATION_ACCEL)
#define SPEED    (v=KeySpeed, \
                  max = KEY_VELOCITY, \
                  step = KEY_ACCEL )

#define Inertia( KEY_LESS, KEY_MORE,KEY_LESS2, KEY_MORE2, va ) \
         ( ( IsKeyDown( display, KEY_LESS ) || IsKeyDown( display, KEY_LESS2 ) ) ? (Decel(v, va) , 1 ) : \
           ( IsKeyDown( display, KEY_MORE ) || IsKeyDown( display, KEY_MORE2 )) ? (Accel(v, va) , 1 ) : \
           Relax( v, va ) )
                                   
      ( (ROTATION), Inertia(KEY_DOWN, KEY_UP, 0, 0, vRight ),
                    Inertia(KEY_RIGHT, KEY_LEFT,  0, 0, vUp ),
                    Inertia(KEY_PGUP, KEY_HOME,  0, 0, vForward ));
       

      ( (SPEED), Inertia( KEY_END, KEY_PGDN,  0, 0, vRight ),
                 Inertia( KEY_ENTER, KEY_GRAY_PLUS,  0, KEY_CENTER, vUp ),
                 Inertia( KEY_DELETE, KEY_INSERT,  0, 0, vForward ) );

      

}

