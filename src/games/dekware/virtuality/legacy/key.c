#define ROTATION_DELTA 0.200F
#define ROTATION_ACCEL 0.05F
#define KEY_VELOCITY 50.0F
#define KEY_ACCEL 1.0F

#include <string.h>
#include <render.h>
#include <vidlib.h> // getdisplaykeyboard
#include "keybrd.h"

#include "object.h"

#include "vectlib.h"

extern POBJECT pFirstObject;

POBJECT pfo; // pointer focus object.

PTRANSFORM T;

void ScanKeyboard( PRENDERER hDisplay, PVECTOR KeySpeed, PVECTOR KeyRotation )
{
   PVECTOR v;
   RCOORD max, step;
   if( !hDisplay )
   	return;
   if( !T )
      T = CreateTransform();
   // how does this HOOK into the camera update functions?!
   if( IsKeyDown((hDisplay), KEY_X) || IsKeyDown( (hDisplay), KEY_Q ) )
   {
      extern int gbExitProgram;
      gbExitProgram = TRUE;
   }

      if( pFirstObject )
      {
         if( KeyDown( (hDisplay), KEY_A ) ) 
         {
            PFACETSET pps;
            VECTOR v;
            RotateAbs( T, 0.1f, 0, 0 );
            pps = pFirstObject->pPlaneSet;
            ApplyRotation( T, v, pps->pPlanes->d.n );
            SetPoint( pps->pPlanes->d.n, v );
            IntersectPlanes( pFirstObject->pLinePool,
                             pps, TRUE );
         }
         if( KeyDown( (hDisplay), KEY_Z ) )
         {
            PFACETSET pps;
            VECTOR v;
            RotateAbs( T, -0.1f, 0, 0 );
            pps = pFirstObject->pPlaneSet;
            ApplyRotation( T, v, pps->pPlanes->d.n );
            SetPoint( pps->pPlanes->d.n, v );
            IntersectPlanes( pFirstObject->pLinePool,
                             pps, TRUE );
         }
      }

#define Relax(V, n)  if( (V[n]>0.001) || ( V[n]<-0.001) )  \
                      (V[n] /= 2.0f); else (V[n] = 0);  

#define Accel(V, n)          \
          if ( V[n] < max )  \
             V[n] += step;   \
            else V[n] = max; 

#define Decel(V,n)            \
          if( V[n] > -max )   \
             V[n] -= step;    \
           else V[n] = -max; 

#define ROTATION (v=KeyRotation, \
                  max = ROTATION_DELTA, \
                  step=ROTATION_ACCEL)
#define SPEED    (v=KeySpeed, \
                  max = KEY_VELOCITY, \
                  step = KEY_ACCEL )

#define Inertia( KEY_LESS, KEY_MORE,KEY_LESS2, KEY_MORE2, va ) \
         if( IsKeyDown(hDisplay, KEY_LESS ) || IsKeyDown(hDisplay, KEY_LESS2) ) Decel(v, va)  \
         else if( IsKeyDown(hDisplay, KEY_MORE ) || IsKeyDown(hDisplay, KEY_MORE2) ) Accel(v, va)   \
           else Relax( v, va );

        (ROTATION);                
       Inertia(KEY_DOWN, KEY_UP, 0, 0, vRight );
       Inertia(KEY_RIGHT, KEY_LEFT,  0, 0, vUp );
       Inertia(KEY_PGUP, KEY_HOME,  0, 0, vForward );
       
        (SPEED);
       Inertia( KEY_END, KEY_PGDN,  0, 0, vRight );
       Inertia( KEY_ENTER, KEY_GRAY_PLUS,  0, KEY_CENTER, vUp );
       Inertia( KEY_DELETE, KEY_INSERT,  0, 0, vForward );


}

// $Log: key.c,v $
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
