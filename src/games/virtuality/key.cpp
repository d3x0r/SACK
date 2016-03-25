#define ROTATION_DELTA 0.150F
#define ROTATION_ACCEL 0.01F
#define KEY_VELOCITY 10.10F
#define KEY_ACCEL 0.11F
#include <stdhdrs.h>

#include <string.h>
#include "global.h"
#include <render.h>
#include "keybrd.h"

#include "vectlib.h"

// hDisplay is passed because that's the input of all keystrokes.
// we can count on being focused... not much point in 
// responding to ALL keystrokes ALL the time...
void ScanKeyboard( PRENDERER hDisplay, PVECTOR KeySpeed, PVECTOR KeyRotation )
{
   PVECTOR v;
   RCOORD max, step;

   //if( !hDisplay )
   //	return;
   // how does this HOOK into the camera update functions?!
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
	Inertia(KEY_DOWN, KEY_UP, KEY_F, KEY_R, vRight );
	Inertia(KEY_LEFT, KEY_RIGHT, KEY_A,  KEY_D, vUp );
	Inertia(KEY_PGUP, KEY_HOME,  0, 0, vForward );

	(SPEED);
	Inertia( KEY_END, KEY_PGDN, KEY_Q, KEY_E, vRight );
	Inertia( KEY_CENTER, KEY_ENTER,  KEY_G, KEY_T, vUp );
	Inertia( KEY_DELETE, KEY_INSERT,  KEY_S, KEY_W, vForward );
}

