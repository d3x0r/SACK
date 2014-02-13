#define ROTATION_DELTA 0.150F
#define ROTATION_ACCEL 0.001F
#define RELAX_FACTOR 1.1F
// at a timer rate of 10ms this is
// 640 units/sec.  the hex scale is for now 640, which
// means one hex .
// each hex is of course 1 hex of 64x10, which makes something...
// this is somewhere near the speed of sound.
#define KEY_VELOCITY 340.29F
//* 150
// * 150 150 times the pseed of sound
// how much faster is the speed of light over sound?
//  880991.08995268741367656998442505 hrm... that's a lot
// even 150 * is a lot...
// so we can get to 1 au at over planet speeds of course

#define KEY_ACCEL 0.1F

#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii

#include <string.h>
#include <render.h>
#include <render3d.h>
#include <keybrd.h>

#include <vectlib.h>

#include "local.h"

//1500 * 5280 = 792000
//miles in feet
// 1087.270341207349 = feet/sec of 0 degree celcius tempurature sound
//  742.336 miles/hr
//  340.29 m / s
//
// 12"/ns = speed of light
// 299 792 458 m / s

void ScanKeyboard( PRENDERER hDisplay, PVECTOR KeySpeed, PVECTOR KeyRotation )
{
   PVECTOR v;
   RCOORD max, step;
   //if( !hDisplay )
   //	return;
   // how does this HOOK into the camera update functions?!
#define Relax(V, n)  if( (V[n]>0.001) || ( V[n]<-0.001) )  \
                      (V[n] /= RELAX_FACTOR); else (V[n] = 0);

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
	Inertia(KEY_RIGHT, KEY_LEFT, 0, 0, vUp );
	Inertia(KEY_PGUP, KEY_HOME,  KEY_E, KEY_Q, vForward );

	(SPEED);
	Inertia( KEY_PGDN, KEY_END, KEY_A, KEY_D, vRight );
	Inertia( KEY_CENTER, KEY_ENTER,  KEY_G, KEY_T, vUp );
	Inertia( KEY_DELETE, KEY_INSERT,  KEY_S, KEY_W, vForward );
}



