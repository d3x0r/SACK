#define NO_UNICODE_C
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>

#include "Android_local.h"

RENDER_NAMESPACE
//#define DEBUG_TOUCH_INPUTS

	static struct touch_event_state
	{
		struct touch_event_flags
		{
			BIT_FIELD owned_by_surface : 1;
		}flags;
		PVPRENDER owning_surface;
		S_32 mouse_x, mouse_y;
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
			//RCOORD begin_length;
		} two;
		struct touch_event_three{
			RCOORD x;
			RCOORD y;
			RCOORD begin_lengths[3]; //3 lengths for segments 1->2, 2->3, 1->3
		} three;
	} touch_info;

void TouchWindowClose( PVPRENDER r )
{
	if( touch_info.flags.owned_by_surface )
		if( r == touch_info.owning_surface )
		{
			touch_info.flags.owned_by_surface = 0;
			touch_info.owning_surface = NULL;
		}
}

int HandleTouches( PVPRENDER r, PINPUT_POINT touches, int nTouches )
{
	int used = 0;
	if( touch_info.flags.owned_by_surface )
	{
		lprintf( "touch event to %p; owned is %p", r, touch_info.owning_surface ); 
		if( touch_info.owning_surface != r )
			return 0;
	}
#ifndef __ANDROID__
	if( l.flags.bRotateLock )
#endif
	{

//#ifdef DEBUG_TOUCH_INPUTS
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( WIDE( "%d %5g %5g %s%s" ), t, touches[t].x, touches[t].y, touches[t].flags.new_event?"new":"", touches[t].flags.end_event?"end":"" );
		}
		lprintf( WIDE( "touch event %p" ), r );
//#endif

#ifdef __ANDROID__
		if( nTouches == 4 )
		{
			if( touches[3].flags.new_event )
			{
				SACK_Vidlib_ToggleInputDevice();
				used = 1;
			}
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
				touch_info.one.x = touches[1].x;
				touch_info.one.y = touches[1].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
			}
			else if( touches[1].flags.end_event )
			{
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				touch_info.two.x = touches[2].x;
				touch_info.two.y = touches[2].y;
			}
			else if( touches[2].flags.end_event )
			{
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
				touch_info.two.x = touches[1].x;
				touch_info.two.y = touches[1].y;
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
				lprintf( WIDE("begin  (is it a touch on a window?) %d,%d   %d,%d"), r->x, r->y, r->w + r->x, r->h+r->y );
				// begin touch
				if(  ( r->flags.fullscreen && !r->flags.not_fullscreen )
					|| ( touches[0].x >= r->x && touches[0].x <= ( r->x + r->w )
						 && touches[0].y >= r->y && touches[0].y <= ( r->y + r->h ) ) )
				{
					//lprintf( "Yes; set keyboard focus" );
					l.hVidVirtualFocused = r;
					touch_info.mouse_x
						= touch_info.one.x = touches[0].x - r->x;
					touch_info.mouse_y
						= touch_info.one.y = touches[0].y - r->y;

					if( r->mouse_callback )
					{
						if( r->flags.fullscreen && !r->flags.not_fullscreen )
						{
							touch_info.mouse_x = ( l.default_display_x * touch_info.mouse_x ) / r->w;
							touch_info.mouse_y = ( l.default_display_y * touch_info.mouse_y ) / r->h;
						}
						if( used = r->mouse_callback( r->psv_mouse_callback, touch_info.mouse_x, touch_info.mouse_y, MK_LBUTTON ) )
						{
							//lprintf( "mouse is used on that... (own touch)" );
						}
					}
					touch_info.owning_surface = r;
					touch_info.flags.owned_by_surface = 1;
					used = 1;
				}
			}
			else if( touches[0].flags.end_event )
			{
				if( touch_info.flags.owned_by_surface )
				{
					touch_info.mouse_x
						= touch_info.one.x = touches[0].x - r->x;
					touch_info.mouse_y
						= touch_info.one.y = touches[0].y - r->y;
					if( r->flags.fullscreen && !r->flags.not_fullscreen  )
					{
						touch_info.mouse_x = ( l.default_display_x * touch_info.mouse_x ) / r->w;
						touch_info.mouse_y = ( l.default_display_y * touch_info.mouse_y ) / r->h;
					}
					if( r->mouse_callback )
						r->mouse_callback( r->psv_mouse_callback, touch_info.mouse_x, touch_info.mouse_y, 0 );
					touch_info.owning_surface = NULL;
					touch_info.flags.owned_by_surface = 0;
					//lprintf(" end touch on owned surface" );
					used = 1;
				}
				else
				{
					//lprintf( "no owning surface..." );
				}
				// release
				//lprintf( WIDE("done") );
			}
			else
			{
				if( touch_info.flags.owned_by_surface )
				{
					touch_info.mouse_x = touches[0].x - r->x;
					touch_info.mouse_y = touches[0].y - r->y;
					//lprintf( "Dragging motion...%p %d %d", r->mouse_callback, touch_info.mouse_x, touch_info.mouse_y );
					if( r->flags.fullscreen && !r->flags.not_fullscreen  )
					{
						touch_info.mouse_x = ( l.default_display_x * touch_info.mouse_x ) / r->w;
						touch_info.mouse_y = ( l.default_display_y * touch_info.mouse_y ) / r->h;
					}
					if( r->mouse_callback )
						if( !r->mouse_callback( r->psv_mouse_callback, touch_info.mouse_x, touch_info.mouse_y, MK_LBUTTON ) )
						{
							//lprintf( "unused event; losing owning surface..." );
							//touch_info.flags.owned_by_surface = 0;
						}
					used = 1;
				}
				else
				{
					// drag
					//lprintf( "lost lock on surface (drag touch, no owner)" );
				}
				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
				/*
				touch_info.mouse_x = touches[0].x;
				touch_info.mouse_y = touches[0].y;
				if( r->mouse_callback )
				if( !r->mouse_callback( r->psv_mouse_callback, touch_info.mouse_x, touch_info.mouse_y, MK_LBUTTON ) )
				{
				touch_info.flags.owned_by_surface = 0;
				}
				*/
			}
		}
		return used;
	}
	return 0;
}

RENDER_NAMESPACE_END
