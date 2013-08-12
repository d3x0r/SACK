#include <stdhdrs.h>

#include "local.h"


	static struct touch_event_state
	{
		struct touch_event_flags
		{
			BIT_FIELD owned_by_surface : 1;
		}flags;
		PRENDERER owning_surface;

		struct touch_event_one{
			struct touch_event_one_flags {
				BIT_FIELD bDrag : 1;
			} flags;
			int x;
         int y;
		} one;
		struct touch_event_two{
			int x;
         int y;
		} two;
		struct touch_event_three{
			int x;
         int y;
		} three;
	} touch_info;



int Handle3DTouches( struct display_camera *camera, PINPUT_POINT touches, int nTouches )
{
#ifndef __ANDROID__
	if( l.flags.bRotateLock )
#endif
	{
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( WIDE( "%d %5g %5g %s%s" ), t, touches[t].x, touches[t].y, touches[t].flags.new_event?"new":"", touches[t].flags.end_event?"end":"" );
		}
		lprintf( WIDE( "touch event" ) );
		if( nTouches == 3 )
		{
			if( touches[2].flags.new_event )
			{
				touch_info.three.x = touches[2].x;
				touch_info.three.y = touches[2].y;
			}
			else if( touches[2].flags.end_event )
			{
			}
			else
			{
				// third state.



			}
		}
		else if( nTouches == 2 )
		{
         // begin rotate lock
			if( touches[1].flags.new_event )
			{
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
			}
			else if( touches[1].flags.end_event )
			{
			}
			else
			{
				// drag
            VECTOR v_n_old, v_n_new;
            VECTOR v_o_old, v_o_new;
				int delx, dely;
				int delx2, dely2;
				int delxt, delyt;
				int delx2t, dely2t;
				lprintf( WIDE("drag") );
            v_o_new[vRight] = touches[0].x;
            v_o_new[vUp] = touches[0].y;
            v_o_new[vForward] = 0;
				v_n_new[vRight] = touches[1].x - touches[0].x;
				v_n_new[vUp] = touches[1].y - touches[0].y;
				v_n_new[vForward] = 0;

            v_o_old[vRight] = touch_info.one.x;
            v_o_old[vUp] = touch_info.one.y;
            v_o_old[vForward] = 0;
				v_n_old[vRight] = touch_info.two.x - touch_info.one.x;
				v_n_old[vUp] = touch_info.two.y - touch_info.one.y;
				v_n_old[vForward] = 0;

				delx = -touch_info.one.x + touches[0].x;
				dely = -touch_info.one.y + touches[0].y;
				delx2 = -touch_info.two.x + touches[1].x;
				dely2 = -touch_info.two.y + touches[1].y;
				{
					VECTOR v1,v2/*,vr*/;
					RCOORD delta_x = delx;
					RCOORD delta_y = dely;
					static int toggle;
               RCOORD angle_one;
					v1[vUp] = delyt;
					v1[vRight] = delxt;
					v1[vForward] = 0;
					v2[vUp] = dely2t;
					v2[vRight] = delx2t;
					v2[vForward] = 0;
					normalize( v1 );
					normalize( v2 );
					lprintf( WIDE("angle %g"), angle_one = atan2( v2[vUp], v2[vRight] ) - atan2( v1[vUp], v1[vRight] ) );
					{
						RCOORD dot = dotproduct( v_n_old, v_n_new );
						RCOORD angle = acos( dot );
						int result;
						RCOORD dt1, dt2;
						result = FindIntersectionTime( &dt1, v_o_old, v_n_old
															  , &dt2, v_o_new, v_n_new );
						if( result || dt1 > 100000 || dt1 < -100000 )
						{
							addscaled( v1, v_o_old, v_n_old, dt1 );
							v1[vForward] = camera->identity_depth;
							// intersect is valid.   Otherwise ... I use the halfway point?
                     PrintVector( v1 );
							RotateAround( l.origin, v1, angle_one );
						}
						else
						{
                     lprintf( "not enough angle? more like a move action?" );
						}
						lprintf( WIDE( "angle is also %g"), angle );

					}
               //lprintf(
					//RotateRel( l.origin, 0, 0, - atan2( v2[vUp], v2[vRight] ) + atan2( v1[vUp], v1[vRight] ) );
					//toggle = 1-toggle;
				}

            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
		}
		else if( nTouches == 1 )
		{
			if( touches[0].flags.new_event )
			{
            PRENDERER used;
            lprintf( WIDE("begin  (is it a touch on a window?)") );
				// begin touch
				l.mouse_x
					= touch_info.one.x = touches[0].x;
            l.mouse_y
					= touch_info.one.y = touches[0].y;
				if( used = OpenGLMouse( (PTRSZVAL)camera, l.mouse_x, l.mouse_y, MK_LBUTTON ) )
				{
               l.hCaptured = used;
					touch_info.owning_surface = used;
               touch_info.flags.owned_by_surface = 1;
				}
				else
				{
				}
			}
			else if( touches[0].flags.end_event )
			{
				if( touch_info.flags.owned_by_surface )
				{
					OpenGLMouse( (PTRSZVAL)camera, l.mouse_x, l.mouse_y, 0 );
					l.hCaptured = NULL;
					touch_info.owning_surface = NULL;
					touch_info.flags.owned_by_surface = 0;
				}
				// release
            lprintf( WIDE("done") );
			}
			else
			{
				if( touch_info.flags.owned_by_surface )
				{
					l.mouse_x = touches[0].x;
					l.mouse_y = touches[0].y;
					if( !OpenGLMouse( (PTRSZVAL)camera, l.mouse_x, l.mouse_y, MK_LBUTTON ) )
					{
                  touch_info.flags.owned_by_surface = 0;
					}
				}
				else
				{
					// drag
					int delx, dely;
					lprintf( WIDE("drag") );
					delx = -touch_info.one.x + touches[0].x;
					dely = -touch_info.one.y + touches[0].y;
					{
						RCOORD delta_x = -delx;
						RCOORD delta_y = -dely;
						static int toggle;
						delta_x /= camera->w;
						delta_y /= camera->h;
						if( toggle )
						{
							RotateRel( l.origin, delta_y, 0, 0 );
							RotateRel( l.origin, 0, delta_x, 0 );
						}
						else
						{
							RotateRel( l.origin, 0, delta_x, 0 );
							RotateRel( l.origin, delta_y, 0, 0 );
						}
						toggle = 1-toggle;
					}

				}
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
			}
		}
		return 1;
	}
	return 0;
}

