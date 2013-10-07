#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>

#include "local.h"

RENDER_NAMESPACE
//#define DEBUG_TOUCH_INPUTS

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
			RCOORD x;
         RCOORD y;
		} one;
		struct touch_event_two{
			RCOORD x;
			RCOORD y;
         RCOORD begin_length;
		} two;
		struct touch_event_three{
			RCOORD x;
			RCOORD y;
         RCOORD begin_lengths[3]; //3 lengths for segments 1->2, 2->3, 1->3
		} three;
	} touch_info;



int Handle3DTouches( struct display_camera *camera, PINPUT_POINT touches, int nTouches )
{
#ifndef __ANDROID__
	if( l.flags.bRotateLock )
#endif
	{

#ifdef DEBUG_TOUCH_INPUTS
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( WIDE( "%d %5g %5g %s%s" ), t, touches[t].x, touches[t].y, touches[t].flags.new_event?"new":"", touches[t].flags.end_event?"end":"" );
		}
		lprintf( WIDE( "touch event" ) );
#endif

#ifdef __ANDROID__
		if( nTouches == 4 )
		{
			if( touches[3].flags.new_event )
				SACK_Vidlib_ToggleInputDevice();
			else if( touches[0].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[1].x;
				touch_info.one.y = touches[1].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
				touch_info.three.x = touches[3].x;
				touch_info.three.y = touches[3].y;
			}
			else if( touches[1].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
				touch_info.three.x = touches[3].x;
				touch_info.three.y = touches[3].y;
			}
			else if( touches[2].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
				touch_info.three.x = touches[3].x;
				touch_info.three.y = touches[3].y;
			}
			else if( touches[3].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
				touch_info.three.x = touches[2].x;
				touch_info.three.y = touches[2].y;
			}
			else
			{
				/* does not track point 4; was just using this for the on down toggle for keyboard trigger....
				 however, the other points still need to be updated so next continue event has a reasonable source
				 for its delta */
				/*
				 // and right now, since the end event will happen, all of these will get set correctly then.
				 // save a few micro-cycles :)
				 touch_info.one.x = touches[0].x;
				 touch_info.one.y = touches[0].y;
				 touch_info.two.x = touches[1].x;
				 touch_info.two.y = touches[1].y;
				 touch_info.three.x = touches[2].x;
				 touch_info.three.y = touches[2].y;
				 */
				// and four if we ever use this
			}
		}
      else 
#endif
		     if( nTouches == 3 )
		{
			if( touches[2].flags.new_event )
			{
				touch_info.three.x = touches[2].x;
				touch_info.three.y = touches[2].y;
			}
			else if( touches[0].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[1].x;
				touch_info.one.y = touches[1].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
			}
			else if( touches[1].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
			}
			else if( touches[2].flags.end_event )
			{
				int n;
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
			}
			else
			{
				// all 3 points still down, figure out who moved and who didn't.
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
				touch_info.three.x = touches[2].x;
				touch_info.three.y = touches[2].y;
			}
		}
		else if( nTouches == 2 )
		{
         // begin rotate lock
			if( touches[1].flags.new_event )
			{
				VECTOR v1, v2, v3;
            v1[vRight] = touches[0].x;
				v1[vUp] = touches[0].y;
            v1[vForward] = 0;
            v2[vRight] = touches[1].x;
            v2[vUp] = touches[1].y;
				v2[vForward] = 0;
            sub( v3, v1, v2 );
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
            touch_info.two.begin_length = Length( v3 );
			}
			else if( touches[0].flags.end_event )
			{
            // otherwise, next move will cause screen to 'pop'...
            touch_info.one.x = touches[1].x;
            touch_info.one.y = touches[1].y;
			}
			else if( touches[1].flags.end_event )
			{
            // otherwise, next move will cause screen to 'pop'...
            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
			}
			else
			{
				// drag
            VECTOR v_n_old, v_n_new;
				VECTOR v_o_old, v_o_new;
				VECTOR v_mid_old, v_mid_new, v_delta_pos;
            RCOORD new_length, old_length;

				RAY rotate_axis;

				//lprintf( WIDE("drag") );
            v_o_new[vRight] = touches[0].x - camera->w/2;
            v_o_new[vUp] = camera->h/2 - touches[0].y;
				v_o_new[vForward] = 0;

				v_n_new[vRight] = touches[1].x - touches[0].x;
				v_n_new[vUp] = touches[1].y - touches[0].y;
				v_n_new[vForward] = 0;

            new_length = Length( v_n_new );
            addscaled( v_mid_new, v_o_new, v_n_new, 0.5f );

            v_o_old[vRight] = touch_info.one.x - camera->w/2;
            v_o_old[vUp] = camera->h/2 - touch_info.one.y;
				v_o_old[vForward] = 0;

				v_n_old[vRight] = touch_info.two.x - touch_info.one.x;
				v_n_old[vUp] = touch_info.two.y - touch_info.one.y;
				v_n_old[vForward] = 0;

            old_length = Length( v_n_old );
				addscaled( v_mid_old, v_o_old, v_n_old, 0.5f );

				ComputeMouseRay( camera, FALSE, &rotate_axis, v_mid_new[vRight] + camera->w/2, camera->h/2 - v_mid_new[vUp] );

				sub( v_delta_pos, v_mid_new, v_mid_old );
				add( v_o_old, v_o_old, v_delta_pos );

				scale( v_delta_pos, v_delta_pos, l.scale );
#ifdef DEBUG_TOUCH_INPUTS
				PrintVector( v_delta_pos );
#endif
            // update old origin to new; for computing rotation point versus translateion

            MoveRight( l.origin, -v_delta_pos[vRight] );
				MoveUp( l.origin, -v_delta_pos[vUp] );
            MoveForward( l.origin, ( ( new_length - old_length ) / old_length ) * camera->identity_depth * l.scale );
				//TranslateRelV( l.origin, v_delta_pos );

				{
					static int toggle;
					RCOORD angle_one;
					angle_one = atan2( v_n_new[vUp], v_n_new[vRight] ) - atan2( v_n_old[vUp], v_n_old[vRight] );
#ifdef DEBUG_TOUCH_INPUTS
					lprintf( "Rotation angle is %g", angle_one );
#endif
					if( 0 )
					 {
						PrintVector( rotate_axis.n );
						RotateAround( l.origin, rotate_axis.n, angle_one );
#if 0
						// attempting to be smart about the direction of rotation... failed.
						int result;
						RCOORD dt1, dt2;
						result = FindIntersectionTime( &dt1, v_n_old, v_o_old
															  , &dt2, v_n_new, v_o_new );

                  lprintf( "Result is %d %g %g", result, dt1, dt2 );
						if( result )
						{
                     VECTOR v1;
							addscaled( v1, v_o_old, v_n_old, dt1 );
							v1[vForward] = camera->identity_depth;
							// intersect is valid.   Otherwise ... I use the halfway point?
							PrintVector( v1 );
							RotateAround( l.origin, v1, angle_one );
						}
						//else
						{
                  //   lprintf( "not enough angle? more like a move action?" );
						}
#endif
					}
               else
						RotateRel( l.origin, 0, 0, angle_one );
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
				if( used = (PRENDERER)OpenGLMouse( (PTRSZVAL)camera, l.mouse_x, l.mouse_y, MK_LBUTTON ) )
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

RENDER_NAMESPACE_END
