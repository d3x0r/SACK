#define DEBUG_CREATE_SURFACE
//#define DEBUG_OUTPUT
//#define DEBUG_SCREEN_CHANGES

#include "Android_local.h"



static IMAGE_INTERFACE AndroidANWImageInterface;

static void InvokeDisplaySizeChange( PRENDERER r, int nDisplay, S_32 x, S_32 y, _32 width, _32 height );
static void CPROC AndroidANW_Redraw( PRENDERER r );

static void CPROC AndroidANW_UpdateDisplayPortionEx( PRENDERER r, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
static void CPROC AndroidANW_MoveSizeDisplay( PRENDERER r
													 , S_32 x, S_32 y
													 , S_32 w, S_32 h );


static void CPROC DefaultMouse( PVPRENDER r, S_32 x, S_32 y, _32 b )
{
	static int l_mouse_b;
	static _32 mouse_first_click_tick_changed;
	static LOGICAL begin_move;
	_32 tick = timeGetTime();
	//lprintf( "Default mouse on %p  %d,%d %08x", r, x, y, b );
	if( r->flags.fullscreen )
	{
		static _32 mouse_first_click_tick;
		if( MAKE_FIRSTBUTTON( b, l.mouse_b ) )
		{
			if( !mouse_first_click_tick )
				mouse_first_click_tick = timeGetTime();
			else
			{
				static int moving;
				if( moving )
					return;
				moving = 1;
				if( !l.mouse_first_click_tick )
					l.mouse_first_click_tick = timeGetTime();
				else
				{
					if( ( tick - l.mouse_first_click_tick ) > 500 )
						l.mouse_first_click_tick = tick;
					else
					{
						EnterCriticalSec( &l.cs_update );
						if( !l.flags.display_closed )
						{
							if( !r->flags.not_fullscreen )
							{
								r->flags.not_fullscreen = 1;
								AndroidANW_UpdateDisplayPortionEx( NULL, 0, 0, l.default_display_x, l.default_display_y DBG_SRC );
							}
							else
							{
								r->flags.not_fullscreen = 0;
							}
						}
						LeaveCriticalSec( &l.cs_update );
						AndroidANW_Redraw( (PRENDERER)r );
					}
               begin_move = 0; // make sure it's not a continuous state
				}
				moving = 0;
			}
		}
	}
	if( ( tick - mouse_first_click_tick_changed ) > 500 ) 
	{
		//lprintf( "Allowed move..." );
		if( !r->flags.fullscreen || r->flags.not_fullscreen )
		{
			static int l_lock_x;
			static int l_lock_y;
			if( MAKE_FIRSTBUTTON( b, l.mouse_b ) )
			{
				begin_move = 1;
				l_lock_x = x;
				l_lock_y = y;
			}
			else if( MAKE_SOMEBUTTONS( b )  )
			{
				// this function sets the image.x and image.y so it can retain
				// the last position of non-fullscreen...
				if( begin_move )
					AndroidANW_MoveSizeDisplay( (PRENDERER)r, r->x + ( x - l_lock_x ), r->y + ( y - l_lock_y ), r->w, r->h );
			}
			else
				begin_move = 0;
		}
	}
	l.mouse_b = b;
}


static int CPROC AndroidANW_InitDisplay( void )
{
	return TRUE;
}

static void CPROC AndroidANW_SetApplicationIcon( Image icon )
{
	// no support
}

static LOGICAL CPROC AndroidANW_RequiresDrawAll( void )
{
	// force application to mostly draw itself...
	return FALSE;
}

static LOGICAL CPROC AndroidANW_AllowsAnyThreadToUpdate( void )
{
	// doesn't matter what thread updates to the render surface?
	// does matter who outputs?
	return TRUE;
}

static void CPROC AndroidANW_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = StrDup( title );
}

static void CPROC AndroidANW_GetDisplaySizeEx( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height)
{
	// wait for a valid display...
	while( !l.default_display_x || !l.default_display_y )
		Relinquish();
	if( x )
		(*x) = 0;
	if( y )
		(*y) = 0;
	if( width )
		(*width) = l.default_display_x;
	if( height )
		(*height) = l.default_display_y;

}

static void CPROC AndroidANW_GetDisplaySize( _32 *width, _32 *height )
{
	AndroidANW_GetDisplaySizeEx( 0, NULL, NULL, width, height );
}

static void CPROC AndroidANW_SetDisplaySize		( _32 width, _32 height )
{
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Width", width );
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Height", height );
}



static PRENDERER CPROC AndroidANW_OpenDisplayAboveUnderSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under )
{

	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );

	Renderer->flags.hidden = 1;
	Renderer->mouse_callback = (MouseCallback)DefaultMouse;
	Renderer->psv_mouse_callback = (PTRSZVAL)Renderer;

	AddLink( &l.renderers, Renderer );
	Renderer->id = FindLink( &l.renderers, Renderer );
	Renderer->x = (x == -1)?(above?((PVPRENDER)above)->x + 10:0):x;
	Renderer->y = (y == -1)?(above?((PVPRENDER)above)->y + 10:0):y;
	Renderer->w = width;
	Renderer->h = height;
#ifdef DEBUG_CREATE_SURFACE
	lprintf( "openDisplay %p %d,%d %d,%d", Renderer, Renderer->x, Renderer->y, Renderer->w, Renderer->h );
#endif
	Renderer->attributes = attributes;
	if( !l.bottom )
		l.bottom = Renderer;

	if( !l.top )
		l.top = Renderer;
	else
		if( !under && !above )
			above = (PRENDERER)l.top;

	if( Renderer->above = (PVPRENDER)above )
	{
		if( l.top == (PVPRENDER)above )
			l.top = Renderer;
		Renderer->above->under = Renderer;
	}

	if( Renderer->under = (PVPRENDER)under )
	{
		if( l.bottom == (PVPRENDER)under )
			l.bottom = Renderer;
		Renderer->under->above = Renderer;
	}


	Renderer->image = MakeImageFileEx( width, height DBG_SRC );
	ClearImageTo( Renderer->image, 0 );
	return (PRENDERER)Renderer;
}

static PRENDERER CPROC AndroidANW_OpenDisplayAboveSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, above, NULL );
}

static PRENDERER CPROC AndroidANW_OpenDisplaySizedAt	  ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, (PRENDERER)l.top, NULL );
}


static void CPROC  AndroidANW_CloseDisplay ( PRENDERER Renderer )
{
	PVPRENDER r = (PVPRENDER)Renderer;
	int real_x, real_y;
	int real_w, real_h;
	// make sure we're not updating while closing
	EnterCriticalSec( &l.cs_update );
	TouchWindowClose( (PVPRENDER)Renderer );

	UnmakeImageFileEx( (Image)(((PVPRENDER)Renderer)->image) DBG_SRC );

	real_x = r->x;
	real_y = r->y;
	real_w = r->w;
	real_h = r->h;

   //lprintf( "unlink %p from %p %p %p", r, l.top, r->above, r->under );
	if( l.top == r )
		l.top = r->above;
	if( l.bottom == r )
		l.bottom = r->under;
	if( r->above )
		r->above->under = r->under;
	if( r->under )
		r->under->above = r->above;

	if( r->flags.fullscreen )
	{
		if( l.full_screen_display == r )
		{
			PVPRENDER check_full;
			for( check_full = l.top; check_full; check_full = check_full->above )
			{
				if( check_full->flags.fullscreen )
					break;
			}
			if( check_full )
			{
				l.full_screen_display = check_full;
				l.flags.full_screen_renderer = 1;
			}
			else
			{
				l.full_screen_display = NULL;
				l.flags.full_screen_renderer = 0;
			}

		}
	}
	DeleteLink( &l.renderers, r );
	Release( r );
	if( l.top )
		AndroidANW_UpdateDisplayPortionEx( (PRENDERER)l.top, real_x - l.top->x, real_y - l.top->y
													, real_w, real_h DBG_SRC );
	else
		AndroidANW_UpdateDisplayPortionEx( NULL, real_x, real_y, real_w, real_h DBG_SRC );
	LeaveCriticalSec( &l.cs_update );

}

// x, y, width, height are passed in screen coordinates
// and have to be adjusted to be actual portion of R
// where UpdateDIsplayPortion takes the region within the renderer to update
// this takes the real screen.
static void CPROC UpdateDisplayPortionRecurse( 	ANativeWindow_Buffer *buffer
															, PVPRENDER r, S_32 x, S_32 y, _32 width, _32 height )
{
	// no-op; it will ahve already displayed(?)
	//lprintf( "recurse %p", r );
#ifdef DEBUG_OUTPUT
	lprintf( "recurse %p %d %d,%d  %d,%d    %d,%d   %d,%d", r, buffer->stride, x, y, width, height, r?r->x:x, r?r->y:y, r?r->w:width, r?r->h:height );
#endif
	if( r )
	{

		int out_x;
		int out_y;
		// during recursion, may hit layers that are hidden.
		if( r->flags.hidden
			|| ( r->x > ( x + (int)width ) )  // render to the right of the area
			|| ( r->y > ( y + (int)height ) )  // renderer below the area
			|| ( x > ( r->x + (int)r->w ) ) // area to the right of renderer
			|| ( y > ( r->y + (int)r->h ) ) // area below the renderer
		  )
		{
			//lprintf( "%s; or region outside...%p   %d,%d  %d,%d  %d,%d  %d,%d"
			//		 , r->flags.hidden?"hidden":"OR!"
			//		 , r
			//		 , x, y, width, height
			//		 , r->x, r->y, r->w, r->h );
 			UpdateDisplayPortionRecurse( buffer, r->above, x, y, width, height );
			return;
		}
		else
		{
			if( r->attributes & DISPLAY_ATTRIBUTE_LAYERED )
			{
				// send draw to things behind this one.
				UpdateDisplayPortionRecurse( buffer, r->above, x, y, width, height );
			}
			else if( !r->flags.fullscreen || r->flags.not_fullscreen ) // don't draw layers under this one, it's full screen, and my position is inaccurate.
			{
				int new_out_x, new_out_y;
				int new_out_width, new_out_height;
				// this window is opaque output; but dispatch the areas around the outside of this window....
				if( x < r->x )
				{
					// left side band for height of band...
					new_out_x = x;
					new_out_width = r->x - x;
					new_out_y = y;
					new_out_height = height;
					UpdateDisplayPortionRecurse( buffer, r->above, new_out_x, new_out_y, new_out_width, new_out_height );
					width = width - ( r->x - x );
					x = r->x;
				}

				if( y < r->y )
				{
					// left side of this band is already output, so start at r->x
					new_out_x = x;
					new_out_width = width;
					new_out_y = y;
					new_out_height = r->y - y;
					UpdateDisplayPortionRecurse( buffer, r->above, new_out_x, new_out_y, new_out_width, new_out_height );
					height = height - ( r->y - y );
					y = r->y;
				}
				if( ( x + (int)width ) > ( r->x + (int)r->w ) )
				{
					// left side of this band is already output, so start at r->x
					new_out_x = r->x + r->w;  // screen position fo right side outside this window
					new_out_width = ( x + width ) - ( r->x + r->w);
					new_out_y = y;
					new_out_height = height;
					UpdateDisplayPortionRecurse( buffer, r->above, new_out_x, new_out_y, new_out_width, new_out_height );
					width = ( r->x + r->w ) - x;
				}

				if( ( y + (int)height ) > ( r->y + (int)r->h ) )
				{
					// left side of this band is already output, so start at r->x
					new_out_x = x;
					new_out_width = width;
					new_out_y = r->y + r->h; // screen position of top of outside of this window...
					new_out_height = ( y + height ) - ( r->y + r->h );
					UpdateDisplayPortionRecurse( buffer, r->above, new_out_x, new_out_y, new_out_width, new_out_height );
					height = ( r->y + r->h ) - y;
				}
			}

		}
		// convert back to window-local coordinates and do old logic
		out_x = x - r->x;
		out_y = y - r->y;
		//lprintf( "Update %d,%d to %d,%d on %d,%d %d,%d",
		//		  x, y, width, height
		//        , out_x, out_y, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h );

		// first, fit the rectangle within the display.
		if( out_x < 0 )
		{
			// window was off the left of the display surface
			x +=  out_x;
			out_x = 0;
			if( x < 0 )
			{
				if( width < -x )
					return;
				width += x;
				// this is resulting as 0
				x = 0;
			}
		}

		if( out_y < 0 )
		{
			// window was off the top of the display surface
			y +=  out_y;
			out_y = 0;
			if( y < 0 )
			{
				if( height < -y )
					return;
				height += y;
				y = 0;
			}
		}


		if( ( width != ((PVPRENDER)r)->w && height != ((PVPRENDER)r)->h
			|| width != l.default_display_x || height != l.default_display_y
			|| x || y )
			|| ( r->attributes & DISPLAY_ATTRIBUTE_LAYERED ) )
		{
			Image tmpout;
iterate:
			tmpout = BuildImageFileEx( (PCOLOR)(((PCDATA)buffer->bits) + ( l.display_skip_top * buffer->stride ))
											 , buffer->stride, l.default_display_y DBG_SRC );
#ifdef DEBUG_OUTPUT
			lprintf( "Update %p %d,%d  to %d,%d    %d,%d (skip %d)", r, out_x, out_y, x, y, width, height, l.display_skip_top * buffer->stride );
#endif
			//LogBinary( r->image->image, 256 );
			if( r->attributes & DISPLAY_ATTRIBUTE_LAYERED )
			//lprintf( "Update %p %d,%d  to %d,%d    %d,%d", r, out_x, out_y, x, y, width, height );
			{
				//lprintf( "Update %p %d,%d  to %d,%d    %d,%d", r, out_x, out_y, x, y, width, height );
				//lprintf( "alpha output" );
				if( r->flags.fullscreen && !r->flags.not_fullscreen && r == l.full_screen_display )
				{
					BlotScaledImageSizedEx( tmpout, r->image, 0, 0 - l.display_skip_bottom, l.default_display_x, l.default_display_y, 0, 0, width, height, ALPHA_TRANSPARENT, BLOT_COPY );
				}
				else
					BlotImageSizedEx( tmpout, r->image, x, y - l.display_skip_bottom, out_x, out_y, width, height, ALPHA_TRANSPARENT, BLOT_COPY );
			}
			else
			{
				//lprintf( "Update %p %d,%d  to %d,%d    %d,%d  %d,%d  %d and %d", r
				//	, out_x, out_y
				//	, x, y
				//	, width, height
				//	, l.default_display_x, l.default_display_y
				//	, l.display_skip_bottom, l.display_skip_top );
				//lprintf( "outputs are %p %p", GetImageSurface( tmpout ), GetImageSurface( r->image ) );
				//lprintf( "a is %d,%d  and %d,%d", tmpout->width, tmpout->height, r->image->width, r->image->height );
				if( r->flags.fullscreen && !r->flags.not_fullscreen && r == l.full_screen_display )
				{
					{
						_32 w;
						_32 h;
						S_32 x, y;
						w =  r->image->width * l.default_display_x / r->image->width;
						h =  r->image->height * l.default_display_x / r->image->width;
						if( h > l.default_display_y )
						{
							w =  r->image->width * l.default_display_y / r->image->height;
							h =  r->image->height * l.default_display_y / r->image->height;
						}
						y = ( l.default_display_y - h ) / 2;
						x = ( l.default_display_x - w ) / 2;
#ifdef DEBUG_OUTPUT
						lprintf( "output scaled thing... %d,%d  %d,%d     %d,%d  %d,%d"
								 , x, y - l.display_skip_bottom, w, h, 0, 0, width, height);
#endif
						BlotScaledImageSizedEx( tmpout, r->image, x, y - l.display_skip_bottom, w, h, 0, 0, r->image->width, r->image->height, 0, BLOT_COPY );
						l.flags.full_screen_renderer = 0;
                  UpdateDisplayPortionRecurse( buffer, r->above, 0, 0, l.default_display_x, y );
                  UpdateDisplayPortionRecurse( buffer, r->above, x+w, y, l.default_display_x - (x+w), h );
                  UpdateDisplayPortionRecurse( buffer, r->above, 0, y, x, h );
						UpdateDisplayPortionRecurse( buffer, r->above, 0, (y+h), l.default_display_x, l.default_display_y - (y+h) );
						l.flags.full_screen_renderer = 1;
					}
				}
				else
					BlotImageSizedEx( tmpout, r->image, x, y - l.display_skip_bottom, out_x, out_y, width, height, 0, BLOT_COPY );
			}
			UnmakeImageFile( tmpout );
		}
		else
		{
			//lprintf( "update full image..." );
			//lprintf( "Update all would be ... %d  %d (%d lines)", buffer->stride, r->image->pwidth, ( height - (l.display_skip_top + l.display_skip_bottom) ) );
			if( buffer->stride == r->image->pwidth )
			{
				memcpy( ((PCDATA)buffer->bits) + ( l.display_skip_top * buffer->stride )
						, r->image->image
						, ( height - (l.display_skip_top + l.display_skip_bottom) ) * width * 4 );
			}
			else
			{
				goto iterate;
			}
		}
	}
	else
	{
		int row;
		if( x < 0 )
		{
			if( width < -x )
				return;
			width += x;
			x = 0;
		}
		if( y < 0 )
		{
			if( height < -y )
				return;
			height += y;
			x = 0;
		}

		if( l.default_background )
		{
			Image tmpout;
			//lprintf( "output background, %d", buffer->stride );
			tmpout = BuildImageFileEx( (PCOLOR)(((PCDATA)buffer->bits) + ( l.display_skip_top * buffer->stride ))
											 , buffer->stride, l.default_display_y DBG_SRC );

			BlotScaledImageSizedEx( tmpout, l.default_background
										 , x, y - l.display_skip_bottom
										 , width, height
										 , x * l.default_background->width / l.default_display_x
										 , y * l.default_background->height / l.default_display_y
										 , width * l.default_background->width / l.default_display_x
										 , height * l.default_background->height / l.default_display_y
										 , 0, BLOT_COPY );
		}
		else
		{
			_32 *base_bits = ((_32*)buffer->bits) + buffer->stride * ( y + l.display_skip_top ) + x;
			//lprintf( "buffer is %d %d buffer stride is %d  pwidth is %d width is %d", bounds.top, bounds.left, buffer.stride, ((PVPRENDER)r)->image->pwidth, width );
			for( row = 0; row < height; row++ )
			{
				_32 *bits = base_bits;
				int col;
				for( col = 0; col < width; col++ )
					(bits++)[0] = 0xFF000000;
				base_bits += buffer->stride;
			}
		}
	}
}




static void CPROC AndroidANW_UpdateDisplayPortionEx( PRENDERER r, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	// no-op; it will ahve already displayed(?)
	ANativeWindow_Buffer buffer;
	S_32 out_x;
	S_32 out_y;
#ifdef DEBUG_OUTPUT
	_lprintf(DBG_RELAY)( "update begin %p %d,%d  %d,%d", l.displayWindow, x, y, width, height );
#endif
	if( l.flags.display_closed )
	{
		//lprintf( "We closed...; no draw" );
		return;
	}
	if( r )
	{
		// current full screen is in full screen
		// don't render anything else
		if( l.flags.full_screen_renderer && !l.full_screen_display->flags.not_fullscreen )
			if( ((PVPRENDER)r) != l.full_screen_display )
			{
#ifdef DEBUG_OUTPUT
				lprintf( "not the fullscreen display.." );
#endif
				return;
			}
		if( ((PVPRENDER)r)->flags.hidden )
		{
#ifdef DEBUG_OUTPUT
			lprintf( "hidden; not showing..." );
#endif
			// if it's not hidden it shouldn't be doing an update...
			return;
		}
		//lprintf( "Update %p %d,%d  %d,%d", r, x, y, ((PVPRENDER)r)->x, ((PVPRENDER)r)->y );
		out_x = ((PVPRENDER)r)->x + x;
		out_y = ((PVPRENDER)r)->y + y;
		// compute the screen position of the desired rectangle
		// do not clip the rectangle to the renderer, so layers can be fixed up

#ifdef DEBUG_OUTPUT
		lprintf( "Update %d,%d to %d,%d on %d,%d %d,%d",
				  x, y, width, height
				 , out_x, out_y, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h );
#endif
	}
	else
	{
		// and up updating black background-only; and this is the positon (on screen, cause we don't have a window)
		out_x = x;
		out_y = y;
	}
	if( out_x >= l.default_display_x )
	{
#ifdef DEBUG_OUTPUT
		lprintf( "display x is ... %d", l.default_display_x );
#endif
		return;
	}
	if( out_y >= l.default_display_y )
	{
#ifdef DEBUG_OUTPUT
		lprintf( "display y is ... %d", l.default_display_y );
#endif
		return;
	}
	if( out_x < 0 )
	{
		if( (int)width < (-out_x ) )
		{
#ifdef DEBUG_OUTPUT
			lprintf( "fail  %d < %d", width, -out_x );
#endif
			return;
		}
		width += out_x;
		out_x = 0;
	}

	if( out_y < 0 )
	{
		if( (int)height < -out_y )
		{
#ifdef DEBUG_OUTPUT
			lprintf( "fail  %d < %d", height, -out_y );
#endif
			return;
		}
		height += out_y;
		out_y = 0;
	}
	//lprintf( "enter crit..." );
	EnterCriticalSec( &l.cs_update );
	// also check closed here; wasn't in the protected section before; and now it may be changed.
	if( l.flags.display_closed )
	{
		//lprintf( "We closed...; no draw" );
		LeaveCriticalSec( &l.cs_update );
		return;
	}

	if( r && ( ((PVPRENDER)r)->flags.fullscreen && !((PVPRENDER)r)->flags.not_fullscreen ) )
	{
		// if the renderer is 1280x720 on a screen that's 720x1280
      // then clipping to fit would be 720x720 output *fail*
#ifdef DEBUG_OUTPUT
		lprintf( "lie about full screen..." );
#endif
		out_x = 0;
		out_y = 0;
		width = l.default_display_x;
		height = l.default_display_y;
	}
	else
	{
		// chop the part that's off the right side ....
		if( ( out_x + (int)width ) > l.default_display_x )
		{
#ifdef DEBUG_OUTPUT
			lprintf( "Fix width ..." );
#endif
			width = l.default_display_x - out_x;
		}
		// chop the part that's off the bottom side ....
		if( ( out_y + (int)height ) > l.default_display_y )
		{
			height = l.default_display_y - out_y;
		}
	}

	if( !l.flags.paused )
	{
		ARect bounds;
		int a;
      int attempts = 0;
		// can still lock just the region we needed...
		bounds.left = out_x;
		bounds.top = out_y + l.display_skip_top;
		bounds.right = out_x + width;
		bounds.bottom = out_y + height + l.display_skip_top;
		//lprintf( "lock is %d,%d %d,%d", bounds.left, bounds.top, bounds.right, bounds.bottom );
		//_lprintf(DBG_RELAY)( "Native window lock... %d,%d  %d,%d", out_x, out_y, width, height );
		do
		{
			a = ANativeWindow_lock( l.displayWindow, &buffer, &bounds );
			if( a == 0 )
			{
				//lprintf( "buffer stride result is %d    %d", buffer.stride, a );
				lprintf( "---V Update screen %p %p %d,%d  %d,%d   %d,%d   %d,%d"
						 , r, l.top, out_x, out_y, out_y+width, out_y+height
						 , bounds.left, bounds.top, bounds.right, bounds.bottom
						 );
				//lprintf( "the only one..." );
				UpdateDisplayPortionRecurse( &buffer, l.top, out_x, out_y, width, height );
				lprintf( "---^ And the final unlock...." );
				ANativeWindow_unlockAndPost(l.displayWindow);
            lprintf( "is there a lock state there?" );
				break;
			}
			else
			{
				/*
				 attempts++;
				 if( attempts > 10 )
				 {
				 lprintf( "Okay give up on this draw (BLACK SCREEN)" );
				 break;
				 }
				 */
				lprintf( "lock failed." );
				//Relinquish();
				//contineu;
				break;;
			}
		}
		while( 1 );
	}
	//else
	//   lprintf( "Display is paused..." );
	LeaveCriticalSec( &l.cs_update );
	//lprintf( "update end" );
}

static void CPROC AndroidANW_UpdateDisplayEx( PRENDERER r DBG_PASS)
{
	((PVPRENDER)r)->flags.hidden = 0;
	// no-op; it will ahve already displayed(?)
	//lprintf( "Output full display %p %p %d,%d", l.top, r, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h );
	AndroidANW_UpdateDisplayPortionEx( r, 0, 0, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h DBG_RELAY );
}

static void CPROC AndroidANW_GetDisplayPosition ( PRENDERER r, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	PVPRENDER pRender = (PVPRENDER)r;
	if( r )
	{
		//lprintf( "Get display pos of %p into %p %p %p %p", r, x, y, width, height );
		if( x )
			(*x) = pRender->x;
		if( y )
			(*y) = pRender->y;
		if( width )
			(*width) = pRender->w;
		if( height )
			(*height) = pRender->h;
	}
}

static void CPROC AndroidANW_MoveSizeDisplay( PRENDERER r
													 , S_32 x, S_32 y
													 , S_32 w, S_32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	//lprintf( "move size %d %d   %d %d", x, y, w, h );
	S_32 real_x, real_y;
	S_32 real_w;
	S_32 real_h;

   //-------------------------------
	if( w > pRender->w )
		real_w = w;
	else
		real_w = pRender->w;
	//-------------------------------
	if( h > pRender->h )
		real_h = h;
	else
		real_h = pRender->h;

	//-------------------------------
	if( x < pRender->x )
	{
		real_x = 0;
		real_w += pRender->x - x;
	}
	else
	{
		// save upper x
		real_x = pRender->x - x;
		real_w += x - pRender->x;
	}
	//-------------------------------
	if( y < pRender->y )
	{
		real_y = 0;
		real_h += pRender->y - y;
	}
	else
	{
		// save upper y
		real_y = pRender->y - y;
		real_h += y - pRender->y;
	}
	//lprintf( "TOtal by now is %d,%d  %d,%d %d,%d", real_x, real_y, real_w, real_h, w, h );
	pRender->x = x;
	pRender->y = y;
	pRender->w = w;
	pRender->h = h;


	AndroidANW_UpdateDisplayPortionEx( r, real_x, real_y, real_w, real_h DBG_SRC );
}

static void CPROC AndroidANW_MoveDisplay		  ( PRENDERER r, S_32 x, S_32 y )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								x,
								y,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_MoveDisplayRel( PRENDERER r, S_32 delx, S_32 dely )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_SizeDisplay( PRENDERER r, _32 w, _32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								w,
								h
								);
}

static void CPROC AndroidANW_SizeDisplayRel( PRENDERER r, S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC AndroidANW_MoveSizeDisplayRel( PRENDERER r
																 , S_32 delx, S_32 dely
																 , S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC AndroidANW_PutDisplayAbove		( PRENDERER r, PRENDERER above )
{
	lprintf( "window ordering is not implemented" );
}

static Image CPROC AndroidANW_GetDisplayImage( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return (Image)pRender->image;
}

static void CPROC AndroidANW_SetCloseHandler	 ( PRENDERER r, CloseCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_SetMouseHandler  ( PRENDERER r, MouseCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->mouse_callback = c;
	render->psv_mouse_callback = p;
}

static void CPROC AndroidANW_Redraw( PRENDERER r )
{
	PVPRENDER render = (PVPRENDER)r;
	if( !render->flags.hidden )
	{
		//lprintf( "wait for critical sec" );
		EnterCriticalSec( &l.cs_update );
		//lprintf( "Sending application draw.... %p %p", render?render->redraw:0, render );
		if( render->redraw )
			render->redraw( render->psv_redraw, (PRENDERER)render );
		AndroidANW_UpdateDisplayEx( r DBG_SRC );
		LeaveCriticalSec( &l.cs_update );
	}
	//else
	//   lprintf( "This display is hidden..." );
}

static void CPROC AndroidANW_SetRedrawHandler  ( PRENDERER r, RedrawCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->redraw = c;
	render->psv_redraw = p;
}

static void CPROC AndroidANW_SetKeyboardHandler	( PRENDERER r, KeyProc c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->key_callback = c;
	render->psv_key_callback = p;
}

static void CPROC AndroidANW_SetLoseFocusHandler  ( PRENDERER r, LoseFocusCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_GetMousePosition	( S_32 *x, S_32 *y )
{
   // used to position controls relative to the last touch
	if( x )
		(*x) = l.mouse_x;
	if( y )
      (*y) = l.mouse_y;
}

static void CPROC AndroidANW_SetMousePosition  ( PRENDERER r, S_32 x, S_32 y )
{
}

static LOGICAL CPROC AndroidANW_HasFocus		 ( PRENDERER  r )
{
	return TRUE;
}

static _32 CPROC AndroidANW_IsKeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static _32 CPROC AndroidANW_KeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static LOGICAL CPROC AndroidANW_DisplayIsValid ( PRENDERER r )
{
	return (r != NULL);
}

static void CPROC AndroidANW_OwnMouseEx ( PRENDERER r, _32 Own DBG_PASS)
{

}

static int CPROC AndroidANW_BeginCalibration ( _32 points )
{
	return 0;
}

static void CPROC AndroidANW_SyncRender( PRENDERER pDisplay )
{
}

static void CPROC AndroidANW_MakeTopmost  ( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	lprintf( "Should hook into topmost display chain" );
	//pRender->
}

static void CPROC AndroidANW_HideDisplay	 ( PRENDERER r )
{
	((PVPRENDER)r)->flags.hidden = 1;
	//lprintf( "hding display %d,%d  %d,%d", ((PVPRENDER)r)->x, ((PVPRENDER)r)->y, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h );
	if( ((PVPRENDER)r)->flags.fullscreen && !((PVPRENDER)r)->flags.not_fullscreen )
	{
		l.flags.full_screen_renderer = 0;
		AndroidANW_UpdateDisplayPortionEx( (PRENDERER)l.top, - (l.top?l.top->x:0), - (l.top?l.top->y:0), l.default_display_x, l.default_display_y DBG_SRC );
	}
	else
		AndroidANW_UpdateDisplayPortionEx( (PRENDERER)l.top, ((PVPRENDER)r)->x - (l.top?l.top->x:0), ((PVPRENDER)r)->y - (l.top?l.top->x:0), ((PVPRENDER)r)->w, ((PVPRENDER)r)->h DBG_SRC );
}


static void CPROC AndroidANW_ForceDisplayFocus ( PRENDERER r )
{
}

static void CPROC AndroidANW_ForceDisplayFront( PRENDERER r )
{
}

static void CPROC AndroidANW_ForceDisplayBack( PRENDERER r )
{
}

static int CPROC  AndroidANW_BindEventToKey( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	return 0;
}

static int CPROC AndroidANW_UnbindKey( PRENDERER pRenderer, _32 scancode, _32 modifier )
{
	return 0;
}

static int CPROC AndroidANW_IsTopmost( PRENDERER r )
{
	return 0;
}

static void CPROC AndroidANW_OkaySyncRender( void )
{
	// redundant thing?
}

static int CPROC AndroidANW_IsTouchDisplay( void )
{
	return 1;
}

static void CPROC AndroidANW_GetMouseState( S_32 *x, S_32 *y, _32 *b )
{
}

static PSPRITE_METHOD CPROC AndroidANW_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	return NULL;
}

static void CPROC AndroidANW_WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
}

static void CPROC AndroidANW_PutDisplayIn(PRENDERER r, PRENDERER hContainer)
{
}

static void CPROC AndroidANW_SetRendererTitle( PRENDERER render, const TEXTCHAR *title )
{
}

static void CPROC AndroidANW_DisableMouseOnIdle(PRENDERER r, LOGICAL bEnable )
{
}

static void CPROC AndroidANW_SetDisplayNoMouse( PRENDERER r, int bNoMouse )
{
	if( r )
	{
      ((PVPRENDER)r)->flags.mouse_transparent = bNoMouse;
	}
}

static void CPROC AndroidANW_MakeAbsoluteTopmost(PRENDERER r)
{
}

static void CPROC AndroidANW_SetDisplayFade( PRENDERER r, int level )
{
}

static LOGICAL CPROC AndroidANW_IsDisplayHidden( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return pRender->flags.hidden;
}

#ifdef WIN32
static HWND CPROC AndroidANW_GetNativeHandle( PRENDERER r )
{
}
#endif

static void CPROC AndroidANW_LockRenderer( PRENDERER render )
{
}

static void CPROC AndroidANW_UnlockRenderer( PRENDERER render )
{
}

static void CPROC AndroidANW_IssueUpdateLayeredEx( PRENDERER r, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
}


static void CPROC AndroidANW_SetTouchHandler  ( PRENDERER r, TouchCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->touch_callback = c;
	render->psv_touch_callback = p;
}

static void CPROC AndroidANW_MarkDisplayUpdated( PRENDERER r  )
{
}

static void CPROC AndroidANW_SetHideHandler		( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_SetRestoreHandler  ( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_RestoreDisplayEx ( PRENDERER r DBG_PASS )
{
	((PVPRENDER)r)->flags.hidden = 0;
	if( ((PVPRENDER)r)->flags.fullscreen )
	{
		l.flags.full_screen_renderer = 1;
      l.full_screen_display = (PVPRENDER)r;
	}
	// issue draw callback and update to screen
	AndroidANW_Redraw( r );
}
static void CPROC AndroidANW_RestoreDisplay  ( PRENDERER r )
{
	AndroidANW_RestoreDisplayEx ( r  DBG_SRC );
}

static void CPROC AndroidANW_SetFullScreen( PRENDERER r, int nDisplay )
{
	// cannot change this sort of paramter while rendering.
	EnterCriticalSec( &l.cs_update );
	if( (PVPRENDER)r == l.full_screen_display )
	{
		// same display, still fullscreen
		// some systems might move displays...
		return;
	}
	if( l.full_screen_display )
	{
		// reset the previous fullscreen
		l.full_screen_display->flags.fullscreen = 0;
		l.full_screen_display->flags.not_fullscreen = 0;
	}

	if( r )
	{
		((PVPRENDER)r)->flags.fullscreen = 1;
		l.flags.full_screen_renderer = 1;
		l.full_screen_display = (PVPRENDER)r;
		//ANativeWindow_setBuffersGeometry( l.displayWindow, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h, WINDOW_FORMAT_RGBA_8888);
//		SurfaceHolder.setFixedSize();
	}
	else
	{
		l.flags.full_screen_renderer = 0;
		l.full_screen_display = NULL;
		//ANativeWindow_setBuffersGeometry( l.displayWindow, l.default_display_x, l.default_display_y, WINDOW_FORMAT_RGBA_8888);
//		SurfaceHolder.setFixedSize();
	}
	LeaveCriticalSec( &l.cs_update );
}

static void CPROC AndroidANW_SuspendSleep( int bool_suspend_enable )
{
	if( l.SuspendSleep )
		l.SuspendSleep( bool_suspend_enable );
}


static RENDER_INTERFACE AndroidANWInterface = {
	AndroidANW_InitDisplay
													  , AndroidANW_SetApplicationTitle
													  , AndroidANW_SetApplicationIcon
													  , AndroidANW_GetDisplaySize
													  , AndroidANW_SetDisplaySize
													  , AndroidANW_OpenDisplaySizedAt
													  , AndroidANW_OpenDisplayAboveSizedAt
													  , AndroidANW_CloseDisplay
													  , AndroidANW_UpdateDisplayPortionEx
													  , AndroidANW_UpdateDisplayEx
													  , AndroidANW_GetDisplayPosition
													  , AndroidANW_MoveDisplay
													  , AndroidANW_MoveDisplayRel
													  , AndroidANW_SizeDisplay
													  , AndroidANW_SizeDisplayRel
													  , AndroidANW_MoveSizeDisplayRel
													  , AndroidANW_PutDisplayAbove
													  , AndroidANW_GetDisplayImage
													  , AndroidANW_SetCloseHandler
													  , AndroidANW_SetMouseHandler
													  , AndroidANW_SetRedrawHandler
													  , AndroidANW_SetKeyboardHandler
	 /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
		 
		 \ \																												 */
													  , AndroidANW_SetLoseFocusHandler
			 ,  0  //POINTER junk1;
													  , AndroidANW_GetMousePosition
													  , AndroidANW_SetMousePosition
													  , AndroidANW_HasFocus

													  , AndroidANW_GetKeyText
													  , AndroidANW_IsKeyDown
													  , AndroidANW_KeyDown
													  , AndroidANW_DisplayIsValid
													  , AndroidANW_OwnMouseEx
													  , AndroidANW_BeginCalibration
													  , AndroidANW_SyncRender

													  , AndroidANW_MoveSizeDisplay
													  , AndroidANW_MakeTopmost
													  , AndroidANW_HideDisplay
													  , AndroidANW_RestoreDisplay
													  , AndroidANW_ForceDisplayFocus
													  , AndroidANW_ForceDisplayFront
													  , AndroidANW_ForceDisplayBack
													  , AndroidANW_BindEventToKey
													  , AndroidANW_UnbindKey
													  , AndroidANW_IsTopmost
													  , AndroidANW_OkaySyncRender
													  , AndroidANW_IsTouchDisplay
													  , AndroidANW_GetMouseState
													  , AndroidANW_EnableSpriteMethod
													  , AndroidANW_WinShell_AcceptDroppedFiles
													  , AndroidANW_PutDisplayIn
													  , NULL // make renderer from native handle (junk4)
													  , AndroidANW_SetRendererTitle
													  , AndroidANW_DisableMouseOnIdle
													  , AndroidANW_OpenDisplayAboveUnderSizedAt
													  , AndroidANW_SetDisplayNoMouse
													  , AndroidANW_Redraw
													  , AndroidANW_MakeAbsoluteTopmost
													  , AndroidANW_SetDisplayFade
													  , AndroidANW_IsDisplayHidden
#ifdef WIN32
													, NULL // get native handle from renderer
#endif
													  , AndroidANW_GetDisplaySizeEx

													  , AndroidANW_LockRenderer
													  , AndroidANW_UnlockRenderer
													  , AndroidANW_IssueUpdateLayeredEx
													  , AndroidANW_RequiresDrawAll
#ifndef NO_TOUCH
													  , AndroidANW_SetTouchHandler
#endif
													  , AndroidANW_MarkDisplayUpdated
													  , AndroidANW_SetHideHandler
													  , AndroidANW_SetRestoreHandler
													  , AndroidANW_RestoreDisplayEx
												, SACK_Vidlib_ShowInputDevice
												, SACK_Vidlib_HideInputDevice
															 , AndroidANW_AllowsAnyThreadToUpdate
															 , AndroidANW_SetFullScreen
                                              , AndroidANW_SuspendSleep
};

static void InitAndroidANWInterface( void )
{
	AndroidANWInterface._RequiresDrawAll = AndroidANW_RequiresDrawAll;
}


static POINTER CPROC GetAndroidANWDisplayInterface( void )
{
	// open server socket
	return &AndroidANWInterface;
}
static void CPROC DropAndroidANWDisplayInterface( POINTER i )
{
	// close connections
}

PRIORITY_PRELOAD( RegisterAndroidNativeWindowInterface, VIDLIB_PRELOAD_PRIORITY )
{
	l.flags.display_closed = 1;
	LoadFunction( "libbag.image.so", NULL );
	l.real_interface = (PIMAGE_INTERFACE)GetInterface( "sack.image" );
	RegisterInterface( WIDE( "sack.render.android" ), GetAndroidANWDisplayInterface, DropAndroidANWDisplayInterface );
	InitAndroidANWInterface();
   l.default_background = LoadImageFile( "images/sky.jpg" );
}


//-------------- Android Interface ------------------

static void HostSystem_InitDisplayInfo(void )
{
	// this is passed in from the external world; do nothing, but provide the hook.
	// have to wait for this ....
	//default_display_x	ANativeWindow_getFormat( camera->displayWindow)
}

// status metric truncates display
// keyboard metric pans display (although should offer later options for applicatoins to handle it themselves....
void SACK_Vidlib_SetNativeWindowHandle( ANativeWindow *displayWindow )
{
	_32 new_w, new_h;
   _32 real_h;
	//lprintf( "Setting native window handle... (shouldn't this do something else?)" );
	l.displayWindow = displayWindow;

	EnterCriticalSec( &l.cs_update );
	new_w = ANativeWindow_getWidth( l.displayWindow);
	new_h = (real_h = ANativeWindow_getHeight( l.displayWindow) )
		- ( l.display_skip_top = 0 * SACK_Vidlib_GetStatusMetric() );
	// got a new display; no longer closed.
	lprintf( "Native Window size reports : %dx%d", new_w, new_h );
	if( new_w != l.default_display_x || new_h != l.default_display_y )
	{
		PVPRENDER check;

		l.default_display_x = new_w;
		l.default_display_y = new_h;

      //InvokeDisplaySizeChange( NULL, 0, 0, 0, new_w, new_h );

		lprintf( "Format is :%dx%d %d", l.default_display_x, l.default_display_y, ANativeWindow_getFormat( displayWindow ) );
		ANativeWindow_setBuffersGeometry( displayWindow
												  , l.default_display_x
												  , real_h//l.default_display_y
												  , WINDOW_FORMAT_RGBA_8888);
		//lprintf( "Format is :%dx%d %d", l.default_display_x, l.default_display_y, ANativeWindow_getFormat( displayWindow ) );
		if( !l.flags.full_screen_renderer || l.full_screen_display->flags.not_fullscreen )
		{
			// make sure the buffer is what I think it should be...
			// set actual display size l.default_x, l.default_y
		}
		else
		{
			// need to make sure the new window has the geometry expected...
			//ANativeWindow_setBuffersGeometry( displayWindow,l.full_screen_display->w,l.full_screen_display->h,WINDOW_FORMAT_RGBA_8888);
			// set to the size of this buffer.
		}

		if( (l.old_display_x != new_w) || (l.old_display_y != new_h ))
		{
			for( check = l.top; check; check = check->above )
			{
#ifdef DEBUG_SCREEN_CHANGES
				lprintf( "check for auto resize %d %d    %d %d    %d %d",check->w, check->h, new_w, new_h, l.old_display_x, l.old_display_y );
#endif
				if( check->x == 0 && check->y == 0
					&& check->w == l.old_display_x && check->h == l.old_display_y )
				{
					PCOLOR newBuffer = NewArray( COLOR, check->w * check->h );
					// use the existing image so its children remain.
					check->w = new_w;
					check->h = new_h;
					check->image = RemakeImageEx( check->image, newBuffer, check->w, check->h DBG_SRC );
					check->image->flags &= ~IF_FLAG_EXTERN_COLORS;  // allow imglib to release the colors.
					// this would have been resized; so redraw it.
#ifdef DEBUG_SCREEN_CHANGES
					lprintf( "is this a bad time to send a redraw? " );
#endif
					if( !check->flags.hidden )
					{
#ifdef DEBUG_SCREEN_CHANGES
						lprintf( "Sending application draw.... %p %p", check?check->redraw:0, check );
#endif
						if( check->redraw )
							check->redraw( check->psv_redraw, (PRENDERER)check );
					}
				}
			}
			l.old_display_x = new_w;
			l.old_display_y = new_h;
		}
		//if( !l.top )
      //lprintf( "do a refresh update of some sort here..." );
	}
   //lprintf( "new full screen; copy bitmapts to output. (enabled anmiating draw later?)" );
	//AndroidANW_UpdateDisplayPortionEx( (PRENDERER)l.top, l.top?-l.top->x:0, l.top?-l.top->y:0, l.default_display_x, l.default_display_y DBG_SRC );
	LeaveCriticalSec( &l.cs_update );
	//lprintf( "Format is :%dx%d %d", l.default_display_x, l.default_display_y, ANativeWindow_getFormat( displayWindow ) );
}


void SACK_Vidlib_DoFirstRender( void )
{
	/* no render pass; should return FALSE or somethig to stop animating... */
	l.flags.display_closed = 0;
	//lprintf( "RENDER PASS UPDATE..." );
#ifdef DEBUG_OUTPUT
	lprintf( "full screen; copy bitmapts to output." );
#endif
	AndroidANW_UpdateDisplayPortionEx( (PRENDERER)l.top, l.top?-l.top->x:0, l.top?-l.top->y:0, l.default_display_x, l.default_display_y DBG_SRC );
}

void SACK_Vidlib_SetKeyboardMetric( int keyboard_size )
{
   lprintf( "keyboard size is reported as %d", keyboard_size );
	//l.display_skip_bottom = keyboard_size;
#ifdef DEBUG_OUTPUT
	lprintf( "First render from keyboard metric..." );
#endif
	SACK_Vidlib_DoFirstRender();
#ifdef DEBUG_OUTPUT
   lprintf( "Done with keyboard metric?" );
#endif
}


void SACK_Vidlib_OpenCameras( void )
{
	/* no cameras to open; this is on screen rotation; maybe re-set the window handle (see above)*/
}


int SACK_Vidlib_SendTouchEvents( int nPoints, PINPUT_POINT points )
{
	INDEX idx;
	PVPRENDER render;
	//lprintf( "Received touch %d", nPoints );
	LIST_FORALL( l.renderers, idx, PVPRENDER, render )
	{
		if( !HandleTouches( render, points, nPoints ) )
		{
			if( render->touch_callback )
			{
				//lprintf( "And somenoe can handle them.." );
				if( render->touch_callback( render->psv_touch_callback, points, nPoints ) )
					return;
			}
		}
		else
		{
			break;
		}
	}
}

void SACK_Vidlib_CloseDisplay( void )
{
	EnterCriticalSec( &l.cs_update );
	l.flags.display_closed = 1;
	// make sure size is different on new display so
	// the buffer is set correctly
	l.default_display_x = 0;
	l.default_display_y = 0;
	LeaveCriticalSec( &l.cs_update );
	// not much to do...
	//lprintf( "display marked as closed; no more draws..." );
   //if( fullscreen_display
}

void SACK_Vidlib_SetSleepSuspend( void(*Suspend)(int) )
{
   l.SuspendSleep = Suspend;
}


//------------------------------------------------------------------------------
// pause/resume
//------------------------------------------------------------------------------
static void InvokeDisplaySizeChange( PRENDERER r, int nDisplay, S_32 x, S_32 y, _32 width, _32 height )
{
	void (CPROC *size_change)( PTRSZVAL, int nDisplay, S_32 x, S_32 y, _32 width, _32 height );
   PVPRENDER render = (PVPRENDER)r;
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( WIDE("sack/render/display"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		size_change = GetRegisteredProcedureExx( data,(CTEXTSTR)name,void,WIDE("on_display_size_change"),( PTRSZVAL psv_redraw, int nDisplay, S_32 x, S_32 y, _32 width, _32 height ));

		if( size_change )
			size_change( render->psv_redraw, nDisplay, x, y, width, height );
	}
}


static void InvokePause( void )
{
	void (CPROC *pause)(void);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( WIDE("sack/render/android/display"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		pause = GetRegisteredProcedureExx( data,(CTEXTSTR)name,void,WIDE("on_display_pause"),(void));

		if( pause )
			pause();
	}

}

//------------------------------------------------------------------------------
static void InvokeResume( void )
{
	void (CPROC *resume)(void);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( WIDE("sack/render/android/display"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		resume = GetRegisteredProcedureExx( data,(CTEXTSTR)name,void,WIDE("on_display_resume"),(void));

		if( resume )
			resume();
	}

}


//------------------------------------------------------------------------------

void SACK_Vidlib_PauseDisplay( void )
{
	//lprintf( "pause..." );
	l.flags.paused = 1;
	InvokePause();
	// just make sure that any current draw is completed, next output will short-circuit.
	EnterCriticalSec( &l.cs_update );
	LeaveCriticalSec( &l.cs_update );
}

//------------------------------------------------------------------------------

void SACK_Vidlib_ResumeDisplay( void )
{
	//lprintf( "unpause..." );
	l.flags.paused = 0;
	InvokeResume();
	//SACK_Vidlib_DoFirstRender();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



