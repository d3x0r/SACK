#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

RENDER_NAMESPACE 

// intersection of lines - assuming lines are
// relative on the same plane....

//int FindIntersectionTime( RCOORD *pT1, LINESEG pL1, RCOORD *pT2, PLINE pL2 )

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 )
{
   VECTOR R1, R2, denoms;
   RCOORD t1, t2, denom;

#define a (o1[0])
#define b (o1[1])
#define c (o1[2])

#define d (o2[0])
#define e (o2[1])
#define f (o2[2])

#define na (s1[0])
#define nb (s1[1])
#define nc (s1[2])

#define nd (s2[0])
#define ne (s2[1])
#define nf (s2[2])

   crossproduct(denoms, s1, s2 ); // - result...
 PrintVector( denoms );
   denom = denoms[2];
//   denom = ( nd * nb ) - ( ne * na );
   if( NearZero( denom ) )
   {
      denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
      if( NearZero( denom ) )
      {
         denom = denoms[0];
//         denom = ( ne * nc ) - ( nb * nf );
         if( NearZero( denom ) )
         {
#ifdef FULL_DEBUG
            lprintf("Bad!-------------------------------------------\n");
#endif
            return FALSE;
         }
         else
         {
            DebugBreak();
            t1 = ( ne * ( c - f ) + nf * ( b - e ) ) / denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         DebugBreak();
         t1 = ( nd * ( c - f ) + nf * ( d - a ) ) / denom;
         t2 = ( na * ( c - f ) + nc * ( d - a ) ) / denom;
      }
   }
   else
   {
      // this one has been tested.......
      t1 = ( nd * ( b - e ) + ne * ( d - a ) ) / denom;
      t2 = ( na * ( b - e ) + nb * ( d - a ) ) / denom;
   }

   R1[0] = a + na * t1;
   R1[1] = b + nb * t1;
   R1[2] = c + nc * t1;

   R2[0] = d + nd * t2;
   R2[1] = e + ne * t2;
   R2[2] = f + nf * t2;

   if( ( !COMPARE(R1[0],R2[0]) ) ||
       ( !COMPARE(R1[1],R2[1]) ) ||
       ( !COMPARE(R1[2],R2[2]) ) )
   {
		PrintVector( R1 );
		PrintVector( R2 );
		lprintf( WIDE("too far from the same... %g %g "), t1, t2 );
      return FALSE;
   }
   *pT2 = t2;
   *pT1 = t1;
   return TRUE;
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef na
#undef nb
#undef nc
#undef nd
#undef ne
#undef nf
}


int Parallel( PVECTOR pv1, PVECTOR pv2 )
{
   RCOORD a,b,c,cosTheta; // time of intersection

   // intersect a line with a plane.

//   v <DOT> w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v <DOT> w)/(|v| |w|) = cos <theta>     

   a = dotproduct( pv1, pv2 );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
	{
#ifdef DEBUG_PLANE_INTERSECTION
		Log( WIDE("Planes are not parallel") );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   lprintf( WIDE(" a: %g b: %g c: %g cos: %g \n"), a, b, c, cosTheta );
#endif
   if( cosTheta > 0.99999 ||
       cosTheta < -0.999999 ) // not near 0degrees or 180degrees (aligned or opposed)
   {
      return TRUE;  // near 1 is 0 or 180... so IS parallel...
   }
   return FALSE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...
RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,  // line m, b
                            PCVECTOR n, PCVECTOR o,  // plane n, o
										RCOORD *time DBG_PASS )
#define IntersectLineWithPlane( s,o,n,o2,t ) IntersectLineWithPlane(s,o,n,o2,t DBG_SRC )
{
   RCOORD a,b,c,cosPhi, t; // time of intersection

   // intersect a line with a plane.

//   v <DOT> w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v <DOT> w)/(|v| |w|) = cos <theta>     

	//cosPhi = CosAngle( Slope, n );

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
	{
		//Log1( DBG_FILELINEFMT WIDE("Bad choice - slope vs normal is 0") DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( WIDE("Slope and or n are near 0") );
		return FALSE; // bad vector choice - if near zero length...
	}

   cosPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

//   lprintf( " a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n", a, b, c, t, cosTheta,
//                  pl->dFrom, pl->dTo );

//   if( cosTheta > e1 ) //global epsilon... probably something custom

//#define 

   if( cosPhi > 0 ||
       cosPhi < 0 ) // at least some degree of insident angle
	{
		*time = t;
		return cosPhi;
	}
	else
	{
		Log1( WIDE("Parallel... %g\n"), cosPhi );
		PrintVector( Slope );
		PrintVector( n );
      // plane and line are parallel if slope and normal are perpendicular
//      lprintf("Parallel...\n");
		return 0;
	}
}

void ComputeMouseRay( struct display_camera *camera, LOGICAL bUniverseSpace, PRAY mouse_ray, S_32 x, S_32 y )
{
#define BEGIN_SCALE 1
#define COMMON_SCALE ( 2*camera->aspect)
#define END_SCALE 1000
#define tmp_param1 (END_SCALE*COMMON_SCALE)
	if( camera->origin_camera )
	{
		VECTOR tmp1;
		VECTOR mouse_ray_origin;
		VECTOR mouse_ray_target;
		VECTOR mouse_ray_slope;
		//PTRANSFORM t = camera->origin_camera;

		addscaled( mouse_ray_origin, _0, _Z, BEGIN_SCALE );
		addscaled( mouse_ray_origin, mouse_ray_origin, _X, (x-(camera->w/2.0f) )*COMMON_SCALE/camera->w );
		addscaled( mouse_ray_origin, mouse_ray_origin, _Y, -(y-(camera->h/2.0f) )*(COMMON_SCALE/camera->aspect)/camera->h );

		addscaled( mouse_ray_target, _0, _Z, END_SCALE );
		addscaled( mouse_ray_target, mouse_ray_target, _X, tmp_param1*(x-(camera->w/2.0f) )/camera->w );
		addscaled( mouse_ray_target, mouse_ray_target, _Y, -(tmp_param1/camera->aspect)*(y-(camera->h/2.0f))/camera->h );

		// this is definaly the correct rotation
		if( bUniverseSpace )
		{
			Apply( camera->origin_camera, tmp1, mouse_ray_origin );
			SetPoint( mouse_ray_origin, tmp1 );
		}
		if( bUniverseSpace )
		{
			Apply( camera->origin_camera, tmp1, mouse_ray_target );
			SetPoint( mouse_ray_target, tmp1 );
		}

		sub( mouse_ray_slope, mouse_ray_target, mouse_ray_origin );
		normalize( mouse_ray_slope );

		SetPoint( mouse_ray->n, mouse_ray_slope );
		SetPoint( mouse_ray->o, mouse_ray_origin );
	}
}

void UpdateMouseRays( S_32 x, S_32 y )
{
	struct display_camera *camera;
	INDEX idx;
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		ComputeMouseRay( camera, TRUE, &camera->mouse_ray, x, y );
	}
}


// Maps a point on a RENDEER surface to a screen point
int InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y )
{
	if( camera->origin_camera )
	{
		VECTOR v1, v2;
		int v = 0;

		v2[0] = x;
		v2[1] = y;
		v2[2] = 1.0;
		//ApplyInverse( l.origin, v
		if( hVideo )
			Apply( hVideo->transform, v1, v2 );
		else
			SetPoint( v1, v2 );
		ApplyInverse( camera->origin_camera, v2, v1 );

		//lprintf( WIDE("%g,%g,%g  from %g,%g,%g "), v1[0], v1[1], v1[2], v2[0], v2[1] , v2[2] );

		// so this puts the point back in world space
		{
			RCOORD t;
			RCOORD cosphi;
			VECTOR v4;
			SetPoint( v4, _0 );
			v4[2] = 1.0;

			cosphi = IntersectLineWithPlane( v2, _0, _Z, v4, &t );
			//lprintf( WIDE("t is %g  cosph = %g"), t, cosphi );
			if( cosphi != 0 )
				addscaled( v1, _0, v2, t );
		}

		//lprintf( WIDE("%g,%g became like %g,%g,%g or %g,%g"), x, y
   		//		 , v1[0], v1[1], v1[2]
		//		 , (v1[0]/2.5 * l.viewport[2]) + (l.viewport[2]/2)
		//		 , (l.viewport[3]/2) - (v1[1]/(2.5/l.aspect) * l.viewport[3])
		//   	 );
		if( result_x )
			(*result_x) = (int)((v1[0]/COMMON_SCALE * camera->w) + (camera->w/2));
		if( result_y )
			(*result_y) = (int)((camera->h/2) - (v1[1]/(COMMON_SCALE/camera->aspect) * camera->h));
	}
	return 1;
}


int CPROC OpenGLMouse( PTRSZVAL psvMouse, S_32 x, S_32 y, _32 b )
{
	int used = 0;
	PRENDERER check = NULL;
	struct display_camera *camera = (struct display_camera *)psvMouse;
	if( camera->origin_camera )
	{
		ComputeMouseRay( camera, TRUE, &camera->mouse_ray, x, y );
		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				if( ref->Mouse3d )
				{
					used = ref->Mouse3d( ref->psv, &camera->mouse_ray, b );
					if( used )
						break;
				}
			}
		}

		if( !used )
		for( check = l.top; check ;check = check->pAbove)
		{
			VECTOR target_point;
			if( l.hCaptured )
				if( check != l.hCaptured )
					continue;
			if( check->flags.bHidden || (!check->flags.bShown) )
				continue;
			{
				RCOORD t;
				RCOORD cosphi;

				//PrintVector( GetOrigin( check->transform ) );
				cosphi = IntersectLineWithPlane( camera->mouse_ray.n, camera->mouse_ray.o
												, GetAxis( check->transform, vForward )
												, GetOrigin( check->transform ), &t );
				if( cosphi != 0 )
					addscaled( target_point, camera->mouse_ray.o, camera->mouse_ray.n, t );
				//PrintVector( target_point );
				scale( target_point, target_point, 1/l.scale );
			}
			// okay so here's the theory now
			//   there's an origin where the camera is, I know where that is
			//   I don't nkow how the model projection is used...
			//   can reverse enginner it I guess... but then we need to break it into
			//    resolution spots,
			{
				VECTOR target_surface_point;
				int newx;
				int newy;

				l.real_mouse_x = (int)target_point[0];
				l.real_mouse_y = (int)target_point[1];
				//PrintVector( target_point );

				ApplyInverse( check->transform, target_surface_point, target_point );
				//PrintVector( target_surface_point );
				newx = (int)target_surface_point[0];
				newy = (int)target_surface_point[1];
				//lprintf( WIDE("Is %d,%d in %d,%d(%dx%d) to %d,%d")
				//   	 ,newx, newy
				//   	 ,check->pWindowPos.x, check->pWindowPos.y
				//   	 ,check->pWindowPos.cx, check->pWindowPos.cy
				//   	 , check->pWindowPos.x+ check->pWindowPos.cx
				//   	 , check->pWindowPos.y+ check->pWindowPos.cy );

				if( check == l.hCaptured ||
					( ( newx >= 0 && newx < (check->pWindowPos.cx ) )
					 && ( newy >= 0 && newy < (check->pWindowPos.cy ) ) ) )
				{
					if( check && check->pMouseCallback)
					{
						if( l.flags.bLogMouseEvent )
							lprintf( WIDE("Sent Mouse Proper. %d,%d %08x"), newx, newy, b );
						//InverseOpenGLMouse( camera, check, newx, newy, NULL, NULL );
						l.current_mouse_event_camera = camera;
						if( !l.flags.bManuallyCapturedMouse )
						{
							if( MAKE_SOMEBUTTONS( b ) )
								l.hCaptured = check;
							else
								l.hCaptured = NULL;
						}
						used = check->pMouseCallback( check->dwMouseData
													, newx
													, newy
													, b );
						l.current_mouse_event_camera = NULL;
						if( used )
							break;
					}
				}
			}
		}
	}
	return (int)check;
}

RENDER_NAMESPACE_END
