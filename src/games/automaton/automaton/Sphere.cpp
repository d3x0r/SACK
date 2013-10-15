//#include "mytypes.h"
#include <math.h>
#include <stdio.h>
#include <vectlib.h>
#include <logging.h>
#include "object.hpp"

static unsigned char byBuffer[256];

#define SPHERE_PLANES 24
BASIC_PLANE SphereDef[SPHERE_PLANES];

void InitSphere( void )
{
   // this is called to create a BASIC_PLANE structure to represent
   // a superclass of spheres.
   int i;

      static VECTOR NulVector={0,0,0};
      VECTOR V, r;
//      VECTOR a1, a2;
	  TRANSFORM t;

      Log(" About to fuck with sphere\n");
#define PI 3.14159268

      V[0] = 1.0/2.0;
      V[1] = (RCOORD)(sqrt(3)/2);
      V[2] = 0;
      // set relative rotation - step in 30 degrees...
      r[0] = 0;
      r[1] = (RCOORD)(PI / 6.0);
      r[2] = 0;
	  t.RotateRel( r );
	  t.RotateRel( r ); 
//      RotateRel( pr->m, r );
//      RotateRel( pr->m, r );  // no intermediate translate...
      for( i = 0; i < 6; i++ )
      {
         SetPoint( SphereDef[i].n, V );
		 t.ApplyRotation( V, V );
//         ApplyRotationOnly( pr, V, V );
      }
      Log(" Step 1...\n");
	  t.clear();
//      ClearRegister( pr );

      V[0] = (RCOORD)(sqrt(3)/2);
      V[1] = (RCOORD)(1.0/2.0);
      V[2] = 0;
	  t.RotateRel( r );
	  t.ApplyRotation( V, V );
//      ApplyRotationOnly( pr, V, V ); // intermediate half step...
	  t.RotateRel( r );
//      RotateRel( pr->m, r );
      for( ; i < 12; i++ )
      {
         SetPoint( SphereDef[i].n, V );
		 t.ApplyRotation( V, V );
//         ApplyRotationOnly( pr, V, V );
      }
      Log(" Step 2...\n");
	  t.clear();
//      ClearRegister( pr );

      V[0] = (RCOORD)(sqrt(3)/2);
      V[1] = -1.0/2.0;
      V[2] = 0;
	  t.RotateRel( r );
	  t.RotateRel( r );
//      RotateRel( pr->m, r );
//      RotateRel( pr->m, r );
      for( ; i < 18; i++ )
      {
         SetPoint( SphereDef[i].n, V );
		 t.ApplyRotation( V, V );
//         ApplyRotationOnly( pr, V, V );
      }
      Log(" Step 3...\n");
	  t.clear();
//      ClearRegister( pr );

      V[0] = 1.0/2.0;
      V[1] = (RCOORD)(-sqrt(3)/2);
      V[2] = 0;
	  t.RotateRel( r );
//      RotateRel( pr->m, r );
	  t.ApplyRotation( V, V );
//      ApplyRotationOnly( pr, V, V ); // intermediate half step...
	  t.RotateRel( r );
//      RotateRel( pr->m, r );
      for( ; i < 24; i++ )
      {
         SetPoint( SphereDef[i].n, V );
		 t.ApplyRotation( V, V );
         //ApplyRotationOnly( pr, V, V );
      }
      Log(" Step 4...\n");
//      ClearRegister( pr );

      for( i = 0; i < 24; i++ )
         {
//            SphereDef[i].bInvert = FALSE;
//			SphereDef[i].
//            SphereDef[i].color  = 11 + i * 10;
//            SetColor( SphereDef[i].color,  11 + i * 10 );
            sprintf((char*)byBuffer, "{{%g,%g,%g},{%g,%g,%g},1,90},\n",
                    SphereDef[i].o[0],
                     SphereDef[i].o[1],
                     SphereDef[i].o[2],
                     SphereDef[i].n[0],
                     SphereDef[i].n[1],
                     SphereDef[i].n[2] );
            Log( (char*)byBuffer );
         }

      // okay - so what are the normals of a sphere arranged
      // in some semblance of order?
      // and once they are computes, will IntersectPlanes()
      // still result in the correct intersections of said planes?
      // what then?  joined bubbles of spheres?

      // collision detection for regions of planes......
}

POBJECT CreateSphere( RCOORD dSize, PVECTOR pv )
{
   // This is called to create a sphere instance... which
   // has all lines of all planes which are visible....
   // this is ideal for wire frame projection.
   return CreateScaledInstance( SphereDef, SPHERE_PLANES, dSize, pv, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
}
