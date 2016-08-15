//#define DEBUG_BANNER_DRAW_UPDATE
//#ifndef __cplusplus_cli
#ifndef FORCE_NO_INTERFACE
#define USE_RENDER_INTERFACE banner_local.pdi
#define USE_IMAGE_INTERFACE banner_local.pii
#endif
#include <stdhdrs.h>
#include <render.h>
#include <sharemem.h>
#include <controls.h>
#include <timers.h>
#include <idle.h>
#include <psi.h>
#include <sqlgetoption.h>
#include <psi.h>
#include <psi/console.h>

#define BANNER_DEFINED
#include "../include/banner.h"

BANNER_NAMESPACE

struct upd_rect{
	int32_t x, y;
	uint32_t w, h;
};


typedef struct banner_tag
{
	uint32_t flags;
	PSI_CONTROL frame;
	PRENDERER renderer;
	uint32_t owners;
	// if text controls were a little more betterer this would be good...
	// they have textcolor, background color, borders, and uhmm they're missing
	// font, and centering rules... left/right/center,top/bottom/center,
	//PCONTROL message;
	PCONTROL okay, cancel;
	PCONTROL yes, no;
	struct banner_tag **me;
	uint32_t result;
	uint32_t timeout;
	uint32_t timer;
	uint32_t _b;
	PTHREAD pWaitThread;
	CDATA basecolor;
	CDATA textcolor;

	struct {
		BIT_FIELD bounds_set : 1;
		BIT_FIELD drawing : 1;
	} bit_flags;

	struct upd_rect text_bounds;
	struct upd_rect old_bounds;
   uint32_t old_width, old_height;
} BANNER;


struct banner_local_tag {
	struct {
		BIT_FIELD bInited : 1;
		BIT_FIELD bFullDraw : 1;
	} flags;
	uint32_t w, h;
	uint32_t _w, _h;
	int32_t x, y; // x/y offset for extended banner.

	SFTFont font;
   SFTFont explorer_font;
	SFTFont custom_font;
   PBANNER banner;
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
   uint32_t count;
};
static struct banner_local_tag banner_local;

//#define BANNER_X  ( ( banner_local.w * 1 ) / 10 )
//#define BANNER_Y  ( ( banner_local.h * 1 ) / 10 )
//#define BANNER_WIDTH  ( ( banner_local.w * 8 ) / 10 )
//#define BANNER_HEIGHT ( ( banner_local.h * 8 ) / 10 )
#define BANNER_X  ( banner_local.x + ( banner_local.w * 0 ) / 10 )
#define BANNER_Y  ( banner_local.y + ( banner_local.h * 0 ) / 10 )
#define BANNER_WIDTH  ( ( banner_local.w * 10 ) / 10 )
#define BANNER_HEIGHT ( ( banner_local.h * 10 ) / 10 )

#define EXPLORER_BANNER_X  ( ( banner_local.w * 0 ) / 16 )
#define EXPLORER_BANNER_Y  ( banner_local.h - ( banner_local.h * 1 ) / 16 )
#define EXPLORER_BANNER_WIDTH  ( ( banner_local.w * 16 ) / 16 )
#define EXPLORER_BANNER_HEIGHT ( ( banner_local.h * 1 ) / 16 )

//--------------------------------------------------------------------------

//CONTROL_REGISTRATION BannerControl;

//--------------------------------------------------------------------------

static void InitBannerFrame( void )
{
	if( !banner_local.flags.bInited )
	{
		TEXTCHAR font[256];
		InvokeDeadstart(); // register my control please... (fucking optimizations)
		banner_local.pii = GetImageInterface();
		banner_local.pdi = GetDisplayInterface();

		banner_local.flags.bFullDraw = RequiresDrawAll();

		GetDisplaySizeEx( 0, NULL, NULL, &banner_local.w, &banner_local.h );
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( WIDE( "SACK/Widgets/Banner2" ), WIDE( "Default Font" ), WIDE( "arialbd.ttf" ), font, sizeof( font ), TRUE );
#else
		StrCpy( font, WIDE( "arialbd.ttf" ) );
#endif
		banner_local.font = RenderFontFile( font
													 , banner_local.w / 30, ( ( banner_local.w * 1080 ) / 1920 ) / 20
									  , 3 );
		if( !banner_local.font )
		{
#ifndef __NO_OPTIONS__
			SACK_GetProfileStringEx( WIDE( "SACK/Widgets/Banner2" ), WIDE( "Alternate Font" ), WIDE( "fonts/arialbd.ttf" ), font, sizeof( font ), TRUE );
#else
			StrCpy( font, WIDE( "fonts/arialbd.ttf" ) );
#endif
			banner_local.font = RenderFontFile( font
										  , banner_local.w / 30, ( ( banner_local.w * 1080 ) / 1920 ) / 20
														 , 3 );
		}

		banner_local.explorer_font = RenderFontFile( font
									  , banner_local.w / 60, ( ( banner_local.w * 1080 ) / 1920 ) / 40
									  , 3 );
		banner_local.flags.bInited = TRUE;
	}
}

//--------------------------------------------------------------------------

static void CPROC SomeChoiceClicked( uintptr_t psv, PCONTROL pc )
{
	PBANNER banner = (PBANNER)psv;
   int choice = GetControlID( pc );
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE( "SOME!" ) );
#endif
	banner->flags |= (BANNER_CLOSED);
	switch( choice )
	{
	case IDOK:
      banner->flags |= BANNER_OKAY;
	case IDCANCEL:
		break;

	case 3+IDOK:
		banner->flags |= BANNER_OKAY;
	case 3+IDCANCEL:
      banner->flags |= BANNER_EXTENDED_RESULT;
      break;
	}
	{
		PTHREAD thread = (banner)->pWaitThread;
		if( thread )
		{
			WakeThread( thread );
		}
	}
}

//--------------------------------------------------------------------------

static void CPROC OkayChoiceClicked( uintptr_t psv, PCONTROL pc )
{
	PBANNER banner = (PBANNER)psv;
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE( "OKAY!" ) );
#endif
	banner->flags |= (BANNER_CLOSED|BANNER_OKAY);
	{
		PTHREAD thread = (banner)->pWaitThread;
		if( thread && ( thread != MakeThread() ) )
		{
			WakeThread( thread );
		}
	}
}

//--------------------------------------------------------------------------

static void CPROC CancelChoiceClicked( uintptr_t psv, PCONTROL pc )
{
	PBANNER banner = (PBANNER)psv;
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE( "CANCEL!" ) );
#endif
	banner->flags |= (BANNER_CLOSED);
	{
		PTHREAD thread = (banner)->pWaitThread;
		if( thread )
		{
			WakeThread( thread );
		}
	}
}

//--------------------------------------------------------------------------

#define BANNER_NAME WIDE("Large font simple banner 2")
static int OnKeyCommon( BANNER_NAME )( PSI_CONTROL pc, uint32_t key )
{
	PBANNER *ppBanner = (PBANNER*)GetCommonUserData( pc );
	PBANNER banner;
	// actually let's just pretend we don't handle any key....
   int handled = 0;
	if( !ppBanner || !(*ppBanner ) )
		return 0; // no cllick, already closed.
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE( "..." ) );
#endif
	if( IsKeyPressed( key ) )
	{
		banner = (*ppBanner);
		if( banner )
		{
			if( KEY_CODE( key ) == KEY_ENTER )
			{
#ifdef DEBUG_BANNER_DRAW_UPDATE
				lprintf( WIDE( "enter.." ) );
#endif
				if( ( banner->flags & BANNER_OPTION_OKAYCANCEL )
					&& ( banner->flags & BANNER_OPTION_YESNO ) )
				{
				}
				else if( ( banner->flags & BANNER_OPTION_OKAYCANCEL )
					|| ( banner->flags & BANNER_OPTION_YESNO ) )
					banner->flags |= BANNER_OKAY;
 			}
			if( !(banner->flags & BANNER_DEAD )
				&& ( banner->flags & BANNER_CLICK ) )
			{
				banner->flags |= (BANNER_CLOSED);
				{
					PTHREAD thread = banner->pWaitThread;
					if( thread )
					{
						WakeThread( thread );
					}
				}
				// test for obutton too so any keypress also clearsit.
				//RemoveBannerEx( ppBanner DBG_SRC );
			}
		}
	}
   return handled;
}

//--------------------------------------------------------------------------

static int OnMouseCommon( BANNER_NAME )
//static int CPROC ClickHandler
( PCONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	//ValidatedControlData();
	PBANNER *ppBanner = (PBANNER*)GetCommonUserData( pc );
	PBANNER banner;
	if( !ppBanner || !(*ppBanner ) )
		return 0; // no cllick, already closed.
	banner = (*ppBanner);
	if( banner )
	{
		if( !(banner->flags & BANNER_DEAD )
			&& ( banner->flags & BANNER_CLICK ) )
		{
         // test for obutton too so any keypress also clearsit.
			if( ( !(b&(MK_OBUTTON)) && ( banner->_b & (MK_OBUTTON) ) )
				|| ( !(b&(MK_LBUTTON)) && ( banner->_b & (MK_LBUTTON) ) ) )
			{
#ifdef DEBUG_BANNER_DRAW_UPDATE
				lprintf( WIDE("Remove banner!") );
#endif
				RemoveBanner2Ex( ppBanner DBG_SRC );
			}
			banner->_b = b;
		}
	}
	return TRUE;
}

//--------------------------------------------------------------------------


static void DrawBannerCaption( PSI_CONTROL pc, PBANNER banner, Image surface, TEXTCHAR *caption, CDATA color, int yofs, int height, int left, int explorer )
{
	uint32_t y = 0;
	int32_t minx = BANNER_WIDTH;
	uint32_t w, h, maxw = 0;
	uint32_t maxw2 = 0;
	uint32_t char_h;
	CTEXTSTR start = caption;
	CTEXTSTR end;
	int skip = 0;
	if( !banner_local.flags.bFullDraw && banner->bit_flags.bounds_set )
	{
		// be kinda nice to be able to result this to the control to get a partial update to screen...
		//lprintf( "Clearing box %d %d %d %d", banner->text_bounds.x, banner->text_bounds.y
		//			, banner->text_bounds.w, banner->text_bounds.h );
		banner->old_bounds = banner->text_bounds;
		BlatColor( surface, banner->text_bounds.x, banner->text_bounds.y
					, banner->text_bounds.w, banner->text_bounds.h, banner->basecolor );
	}
	else
	{
		//lprintf( "output full surface." );
		banner->old_bounds.x = 0;
		banner->old_bounds.y = 0;
		banner->old_bounds.w = surface->width;
		banner->old_bounds.h = surface->height;
		BlatColor( surface, 0, 0, surface->width, surface->height, banner->basecolor );
	}
	//ClearImageTo( surface, GetBaseColor( NORMAL ) );
	GetStringRenderSizeFontEx( caption, strlen( caption ), &maxw, &h, &char_h
									 , banner_local.custom_font?banner_local.custom_font:explorer?banner_local.explorer_font:banner_local.font );
   //lprintf( "h from render size is %d or %d (%s) %d", h, char_h, caption, maxw );
	y = yofs - (h/2);
	banner->text_bounds.y = y - 2;
	banner->text_bounds.h = h + 5; // -2 to +2 is 5 ?

	while( start )
	{
		int32_t x;
		end = StrChr( start, '\n' );
		if( !end )
		{
			end = StrChr( start, '\\' );
			if( !end || end[1] != 'n' )
			{
				end = start + StrLen(start);
			}
			else
			{
				skip = 1;
			}
		}
		w = GetStringSizeFontEx( start, end-start, NULL, &h
									  , banner_local.custom_font?banner_local.custom_font:explorer?banner_local.explorer_font:banner_local.font );
      //lprintf( "String measured %d,%d", h );
		x = ( BANNER_WIDTH - (left?maxw:w) ) /2;

		if( (x-2) < minx )
			minx = x - 2;
		if( (w+4) > maxw )
			maxw = w + 4;
      //lprintf( "output at %d,%d", x, y );
		PutStringFontEx( surface
							, x+2
							, y+2
							, SetAlpha( ~color, 0x80 ), 0
							, start, end-start
							, banner_local.custom_font?banner_local.custom_font:explorer?banner_local.explorer_font:banner_local.font );
		PutStringFontEx( surface
							, x-2
							, y-2
							, SetAlpha( ~color, 0x80 ), 0
							, start, end-start
							, banner_local.custom_font?banner_local.custom_font:explorer?banner_local.explorer_font:banner_local.font );
		PutStringFontEx( surface
							, x
							, y
							, color, 0
							, start, end-start
							, banner_local.custom_font?banner_local.custom_font:explorer?banner_local.explorer_font:banner_local.font );
		y += h;
		if( end[0] )
		{
			start = end+1+skip;
			skip = 0;
		}
		else
			start = NULL;
	}
	banner->text_bounds.x = minx;
	banner->text_bounds.w = maxw;
	banner->bit_flags.bounds_set = 1;

	{
		int32_t rx, ry;
		int32_t rx_right, ry_bottom;
		int32_t rx_tmp;
		uint32_t rw, rh;

		rx_right = banner->text_bounds.x + banner->text_bounds.w + 1;
		rx_tmp = banner->old_bounds.x + banner->old_bounds.w + 1;
		if( rx_tmp > rx_right )
         rx_right = rx_tmp;

		ry_bottom = banner->text_bounds.y + banner->text_bounds.h + 1;
		rx_tmp = banner->old_bounds.y + banner->old_bounds.h + 1;
		if( rx_tmp > ry_bottom )
			ry_bottom = rx_tmp;


		if( banner->text_bounds.x < banner->old_bounds.x )
		{
			rx = banner->text_bounds.x;
			rw = ( rx_right - rx );
		}
		else
		{
			rx = banner->old_bounds.x;
			rw = ( rx_right - rx );
		}

		if( banner->text_bounds.y < banner->old_bounds.y )
		{
			ry = banner->text_bounds.y;
			rh = ( ry_bottom - ry );
		}
		else
		{
			ry = banner->old_bounds.y;
			rh = ( ry_bottom - ry );
		}

		//lprintf( "update %d,%d to %d,%d", rx, ry, rw, rh );
		SetUpdateRegion( pc, rx, ry, rw, rh );
	}
}



//--------------------------------------------------------------------------

static int OnDrawCommon( BANNER_NAME )( PCONTROL pc )
{
	static TEXTCHAR caption[4096];
	PBANNER *ppBanner = (PBANNER*)GetCommonUserData( pc );
	if( !ppBanner )
		return 0;
	else
	{
		PBANNER banner = (*ppBanner);
		if( banner )
		{
			Image image = GetControlSurface( pc );
			if( banner->old_width != image->width || banner->old_height != image->height )
			{
				banner_local.w
					= banner->old_width
					= image->width;
            banner_local.h
					= banner->old_height
					= image->height;
				RerenderFont( banner_local.font
								,banner_local.w / 30, ( ( banner_local.w * 1080 ) / 1920 ) / 20 //image->width / 18, banner_local.h/10
								, NULL, NULL );
				// clear bounds-set so a full background is drawn.
				banner->bit_flags.bounds_set = 0;
			}
			banner->bit_flags.drawing = 1;
			GetControlText( pc, caption, sizeof( caption )/sizeof(TEXTCHAR) );
#ifdef DEBUG_BANNER_DRAW_UPDATE
			lprintf( WIDE( "--------- BANNER DRAW %s -------------" ), caption );
#endif
         //lprintf( "image is %d,%d", image->width, image->height );
			DrawBannerCaption( pc, banner, image
								  , caption
								  , banner->textcolor
								  , (banner->flags & ( BANNER_OPTION_YESNO|BANNER_OPTION_OKAYCANCEL ))
									? ( (image->height * 1 ) / 3 )
									: ((image->height)/2)
								  , 0
								  , banner->flags & BANNER_OPTION_LEFT_JUSTIFY
								  , banner->flags & BANNER_EXPLORER
								  );
			banner->bit_flags.drawing = 0;
		}
	}
	return TRUE;
}

CONTROL_REGISTRATION banner_control = { BANNER_NAME
												  , { { 0, 0 }, 0
													 , BORDER_BUMP
													 | BORDER_NOMOVE
													 | BORDER_NOCAPTION
													 | BORDER_FIXED
												  }
};
PRIORITY_PRELOAD( RegisterBanner2, 65 )
{
	DoRegisterControl( &banner_control );
}
//--------------------------------------------------------------------------


void CPROC killbanner( uintptr_t psv )
{
	PBANNER *ppBanner = (PBANNER*)psv;
	{
		//lprintf( WIDE( "killing banner..." ) );
		RemoveBanner2Ex( ppBanner DBG_SRC );
	}
}

// BANNER_OPTION_YES_NO
// BANNER_OPTION_OKAY_CANCEL ... etc ? sound flare, colors, blinkings...
// all kinda stuff availabel...

int CreateBanner2Extended( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor, int lines, int cols, int display )
{
	//PBANNER newBanner;
	int32_t x, y;
	uint32_t w, h;
	PBANNER banner;
	LOGICAL bUpdateLocked = FALSE;
	InitBannerFrame();
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE("Create banner '%s' %p %p %p"), text, parent, ppBanner, ppBanner?*ppBanner:NULL );
#endif
	if( !ppBanner )
		ppBanner = &banner_local.banner;
	banner = *ppBanner;
	if( !(banner) )
	{
		banner = New( BANNER );
		MemSet( banner, 0, sizeof( BANNER ) );
		banner->flags |= BANNER_WAITING; // clear this off when done
		// but please attempt to prevent destruction during creation....
		// setting this as waiting will inhibit that, but it's not really
		// waiting, so we'll have to check and validate at the end...
		*ppBanner = banner;
	}



	if( lines || cols )
	{
		TEXTCHAR font[256];
		if( !lines )
			lines = 20;
		if( !cols )
			cols = 30;
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( WIDE( "SACK/Widgets/Banner2" ), WIDE( "Default Font" ), WIDE( "arialbd.ttf" ), font, sizeof( font ), TRUE );
#else
		StrCpy( font, WIDE( "arialbd.ttf" ) );
#endif
		//lprintf( WIDE("default font: %d,%d %d,%d"), cols, lines, w/cols, h/lines );
		banner_local.custom_font = RenderFontFile( font
															  , w /cols
															  , h/lines
															  , 3 );
		if( !banner_local.custom_font )
		{
#ifndef __NO_OPTIONS__
			SACK_GetProfileStringEx( WIDE( "SACK/Widgets/Banner2" ), WIDE( "Alternate Font" ), WIDE( "fonts/arialbd.ttf" ), font, sizeof( font ), TRUE );
#else
			StrCpy( font, WIDE( "fonts/arialbd.ttf" ) );
#endif
			//lprintf( WIDE( "default font: %d,%d %d,%d" ), cols, lines, w/cols, h/lines );
			banner_local.custom_font = RenderFontFile( font
																  , w /cols
																  , h/lines
																  , 3 );
		}
	}
	else
	{
		DestroyFont( &banner_local.custom_font );
		banner_local.custom_font = 0;
	}

	if( !banner->renderer )
	{
		banner_local._w = banner_local.w;
		banner_local._h = banner_local.h;
		x = 0;
		y = 0;
		GetDisplaySizeEx( display, &x, &y, &w, &h );
		banner_local.w = w;
		banner_local.h = h;
		//lprintf( WIDE( "resulting size/location is %d,%d %dx%d" ), x, y, w, h );
		banner->renderer = OpenDisplayAboveSizedAt( ((options & BANNER_ALPHA)?DISPLAY_ATTRIBUTE_LAYERED:0)
																, (options & BANNER_EXPLORER)?EXPLORER_BANNER_WIDTH:w
																, (options & BANNER_EXPLORER)?EXPLORER_BANNER_HEIGHT:h
																, (options & BANNER_EXPLORER)?EXPLORER_BANNER_X:x
																, (options & BANNER_EXPLORER)?EXPLORER_BANNER_Y:y
																, parent );
		//DebugBreak();
		if( (parent && IsTopmost( parent ) )|| ( options & BANNER_TOP ) )
		{
			if( options & BANNER_ABSOLUTE_TOP )
				MakeAbsoluteTopmost( banner->renderer );
         else
				MakeTopmost( banner->renderer );
		}
	}
	if( !banner->frame )
	{
		banner->textcolor = textcolor?textcolor:AColor( 0x0d, 0x0d, 0x0d, 0xff );
		banner->basecolor = basecolor?basecolor:AColor( 0x13, 0x53, 0x93, 0x40 );
		//		GetDisplaySize( &banner_local.w, &banner_local.h );
		{
			TEXTSTR result;
			TEXTSTR skip_newline;
			FormatTextToBlockEx( text, &result, w * 7 / 10, h * 7 / 10, banner_local.custom_font?banner_local.custom_font:(options & BANNER_EXPLORER)?banner_local.explorer_font:banner_local.font );
			// formatter returns a newline at the start of the block (first line doesn't have NO_RETURN flag probably...)
			for( skip_newline = result; skip_newline && skip_newline[0] && skip_newline[0] == '\n'; skip_newline++ );
			banner->frame = MakeCaptionedControl( NULL, banner_control.TypeID
															, (options & BANNER_EXPLORER)?EXPLORER_BANNER_X:x
															, (options & BANNER_EXPLORER)?EXPLORER_BANNER_Y:y
															, (options & BANNER_EXPLORER)?EXPLORER_BANNER_WIDTH:w
															, (options & BANNER_EXPLORER)?EXPLORER_BANNER_HEIGHT:h
															, 0
															, skip_newline );

			Release( result );
		}
		//banner->frame = CreateFrameFromRenderer( text
		//													, BORDER_WANTMOUSE | BORDER_BUMP | BORDER_NOMOVE | BORDER_NOCAPTION | BORDER_FIXED
		//													, banner->renderer
		//													);
		AttachFrameToRenderer( banner->frame, banner->renderer );
		SetCommonUserData( banner->frame, (uintptr_t)ppBanner );
		banner->owners = 1;
	}
	else
	{
		banner->owners++;
#ifdef DEBUG_BANNER_DRAW_UPDATE
		lprintf( WIDE("Using exisiting banner text (created twice? ohohoh count!)") );
#endif
		//if( banner_local._w != banner_local.w || banner_local._h != banner_local.h )
#if 0
			MoveSizeCommon( banner->frame
							  , (options & BANNER_EXPLORER)?EXPLORER_BANNER_X:x
							  , (options & BANNER_EXPLORER)?EXPLORER_BANNER_Y:y
							  , (options & BANNER_EXPLORER)?EXPLORER_BANNER_WIDTH:w
							  , ((options & BANNER_EXPLORER))?EXPLORER_BANNER_HEIGHT:h
							  );
#endif
		if( !banner_local.flags.bFullDraw )
		{
			EnableCommonUpdates( banner->frame, FALSE );
		}
		bUpdateLocked = TRUE;
		{
			TEXTSTR result;
			TEXTSTR skip_newline;
			FormatTextToBlock( text, &result, 30, 20 );
         // formatter returns a newline at the start of the block (first line doesn't have NO_RETURN flag probably...)
			for( skip_newline = result; skip_newline && skip_newline[0] && skip_newline[0] == '\n'; skip_newline++ );
			SetControlText( banner->frame, skip_newline );
			Release( result );
		}
	}
	if( !options ) // basic options...
		options = BANNER_CLICK|BANNER_TIMEOUT;
	SetBanner2OptionsEx( ppBanner, options, timeout );
	if( banner->owners == 1 )
	{
		//lprintf( "calling displayframe..." );
		DisplayFrame( banner->frame );
		RestoreDisplay( banner->renderer );
	}
	if( bUpdateLocked )
	{
		if( !banner_local.flags.bFullDraw )
		{
			EnableCommonUpdates( banner->frame, TRUE );
		}
		SmudgeCommon( banner->frame ); // do this so it actually draws out...
	}
	//else
	//   SmudgeCommon( banner->frame );
	banner->flags &= ~BANNER_WAITING;
	if( banner->flags & BANNER_CLOSED )
	{
#ifdef DEBUG_BANNER_DRAW_UPDATE
		lprintf( WIDE("-----------------------! BANNER DESTROYED EVEN AS IT WAS CREATED!") );
#endif
		RemoveBanner2Ex( ppBanner DBG_SRC );
		return FALSE;
	}
	banner->me = ppBanner;
#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf( WIDE("Created banner %p"), banner );
#endif
	if( !(options & BANNER_NOWAIT ) )
		return WaitForBanner2Ex( ppBanner );
	return TRUE;
}

int CreateBanner2Exx( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor )
{
	return CreateBanner2Extended( parent, ppBanner, text, options, timeout, textcolor, basecolor, 0, 0, 0 );
}

int CreateBanner2Ex( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout )
{
	return CreateBanner2Exx( parent, ppBanner, text, options, timeout, 0, 0 );
}
//--------------------------------------------------------------------------

int CreateBanner2( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text )
{
	int retval = 0;

	retval = CreateBanner2Ex( parent, ppBanner, text, BANNER_CLICK|BANNER_TIMEOUT, 5000 );

#ifdef DEBUG_BANNER_DRAW_UPDATE
	lprintf(WIDE("CreateBanner is done, retval is %d"), retval);
#endif
	return retval;
}

//--------------------------------------------------------------------------

#undef RemoveBanner2
// provided for migratory compatibility
void RemoveBanner2( PBANNER banner )
{
	RemoveBanner2Ex( &banner DBG_SRC );
}

void RemoveBanner2Ex( PBANNER *ppBanner DBG_PASS )
{
	PBANNER banner;
	//_lprintf( DBG_RELAY )( "Removing something.." );
	if( !ppBanner )
	{
		//lprintf( "remove banner local.." );
		ppBanner = &banner_local.banner;
	}
	banner = ppBanner?(*ppBanner ):(banner_local.banner);
	//#ifdef DEBUG_BANNER_DRAW_UPDATE
	{
		TEXTCHAR buf[256];
		if( banner )
			GetControlText( banner->frame, buf, sizeof( buf )/sizeof(TEXTCHAR) );
#ifdef DEBUG_BANNER_DRAW_UPDATE
		_xlprintf(LOG_ALWAYS DBG_RELAY)( WIDE("Remove banner %p %p %d %s")
												 , banner, banner_local.banner
												 , banner_local.banner?banner_local.banner->owners:123, buf  );
#endif
	}
	//#endif
	if( !banner )
	{
		banner = banner_local.banner;
	}
	if( banner )
	{
		if( banner->flags & BANNER_WAITING )
		{
#ifdef DEBUG_BANNER_DRAW_UPDATE
			lprintf( WIDE("Marking banner for later removal") );
#endif
			banner->flags |= BANNER_CLOSED;
		}
		else
		{
			if( --banner->owners )
			{
				// no - remove is intended to kill any banner exisiting at
				// this as a banner...
#ifdef DEBUG_BANNER_DRAW_UPDATE
				lprintf( WIDE("Waiting for last owner to remove banner") );
#endif
				//return;
			}
#ifdef DEBUG_BANNER_DRAW_UPDATE
			lprintf( WIDE("Banner is not waiting") );
#endif
			while( banner->bit_flags.drawing )
			{
				//lprintf( "Wait for draw to finish before destroying" );
				WakeableSleep( 10 );
				//Relinquish();
			}
			DestroyCommon( &banner->frame );
			if( banner->timer )
			{
				//lprintf( "Removingi timer..." );
				RemoveTimer( banner->timer );
				banner->timer = 0;
			}
			// stand alone banners?
			if( banner->me )
				(*banner->me) = NULL;
#ifdef DEBUG_BANNER_DRAW_UPDATE
			lprintf( WIDE("Releasing banner...") );
#endif
			Release( banner );
			(*ppBanner) = NULL;
			if( banner_local.banner == banner )
				banner_local.banner = NULL;
		}
	}
}


//--------------------------------------------------------------------------

void SetBanner2Text( PBANNER banner, TEXTCHAR *text )
{
	if( !banner )
	{
		if( !banner_local.banner )
			CreateBanner2( NULL, &banner_local.banner, text );
		banner = banner_local.banner;
	}
	if( banner )
	{
		TEXTSTR result;
		TEXTSTR skip_newline;
		FormatTextToBlock( text, &result, 30, 20 );
		// formatter returns a newline at the start of the block (first line doesn't have NO_RETURN flag probably...)
		for( skip_newline = result; skip_newline && skip_newline[0] && skip_newline[0] == '\n'; skip_newline++ );
		SetControlText( banner->frame, skip_newline );
		Release( result );
	}
}

//--------------------------------------------------------------------------

static void CPROC BannerTimeout( uintptr_t ppsv )
{
	PBANNER *ppBanner = (PBANNER*)ppsv;
	int delta = (int)(*ppBanner)->timeout - (int)timeGetTime();
	//PBANNER banner = ppBanner?(*ppBanner):NULL;
	if( delta < 0 )
	{
		PTHREAD thread = ( ppBanner && (*ppBanner) )?(*ppBanner)->pWaitThread:NULL;
		RemoveBanner2Ex( ppBanner DBG_SRC );
		// timer goes away after this.
		if( thread )
		{
			WakeThread( thread );
		}
	}
	else
	{
		//lprintf( "Timer hasn't actually expired yet... %d %d", (*ppBanner)->timer, delta );
		RescheduleTimerEx( (*ppBanner)->timer, delta );
	}
}

//--------------------------------------------------------------------------
/*
void CPROC DrawButton( uintptr_t psv, PCONTROL pc )
{
	Image Surface = GetControlSurface( pc );
	ClearImageTo( pb->common.common.Surface, GetBaseColor( NORMAL ) );
	DrawStrong( banner->font,
}
*/
//--------------------------------------------------------------------------

void SetBanner2OptionsEx( PBANNER *ppBanner, uint32_t flags, uint32_t extra  )
{
	PBANNER banner;
	if( !ppBanner )
		ppBanner  = &banner_local.banner;
	banner = (*ppBanner);
	if( !banner )
	{
		banner = banner_local.banner;
	}
	if( banner )
	{
		banner->flags = flags;
		if( flags & BANNER_TIMEOUT )
		{
			banner->timeout = timeGetTime() + extra;
			if( !banner->timer )
			{
				banner->timer = AddTimerEx( extra, 0, BannerTimeout, (uintptr_t)ppBanner );
			}
			else
			{
				RescheduleTimerEx( banner->timer, extra );
			}
		}
		else
		{
			if( banner->timer )
			{
				RemoveTimer( banner->timer );
				banner->timer = 0;
			}
		}
		if( (banner->flags & (BANNER_OPTION_YESNO|BANNER_OPTION_OKAYCANCEL)) == (BANNER_OPTION_YESNO|BANNER_OPTION_OKAYCANCEL) )
		{
			banner->cancel = MakeButton( banner->frame
												, 5
												, ( ( BANNER_HEIGHT * 5 ) / 6 ) - 5 - 10
												, ( BANNER_WIDTH / 2) - 10
												, ((BANNER_HEIGHT * 1 ) / 6 ) - 20
												, 3+IDCANCEL
												, WIDE( "Cancel" ), 0
												, SomeChoiceClicked
												, (uintptr_t)banner );
			SetButtonColor( banner->cancel, AColor( 0x9a, 0x05, 0x1d, 0xdf ) );
			banner->okay = MakeButton( banner->frame
											 , 5 + ( BANNER_WIDTH / 2) - 5
											 , ( ( BANNER_HEIGHT * 5 ) / 6 ) - 5 - 10
											 , ( BANNER_WIDTH / 2) - 10
											 , ((BANNER_HEIGHT * 1 ) / 6 ) - 20
											 , 3+IDOK
											 , WIDE( "Okay!" ), 0
											 , SomeChoiceClicked
											 , (uintptr_t)banner );
			SetButtonColor( banner->okay, AColor( 0x18, 0x98, 0x6c, 0xdf ) );
			banner->no = MakeButton( banner->frame
												, 5, ( ( BANNER_HEIGHT * 4 ) / 6 ) - 5 - 10
											, ( BANNER_WIDTH / 2) - 10
											, ((BANNER_HEIGHT * 1 ) / 6 ) - 20
												, IDCANCEL
												, WIDE( "No" ), 0
												, SomeChoiceClicked
												, (uintptr_t)banner );
			SetButtonColor( banner->no, AColor( 0x9a, 0x05, 0x1d, 0xdf ) );
			banner->yes = MakeButton( banner->frame
										  , 5 + ( BANNER_WIDTH / 2) - 5
										  , ( ( BANNER_HEIGHT * 4 ) / 6 ) - 5 - 10
											 , ( BANNER_WIDTH / 2) - 10, ((BANNER_HEIGHT * 1 ) / 6 ) - 20
											 , IDOK
											 , WIDE( "Yes" ), 0
											 , SomeChoiceClicked
											 , (uintptr_t)banner );
			SetButtonColor( banner->yes, AColor( 0x18, 0x98, 0x6c, 0xdf ) );

			//SetBaseColor( TEXTCOLOR, 0xffFFFFFF );
			SetControlColor( banner->okay, TEXTCOLOR, 0xffFFFFFF );
			SetControlColor( banner->cancel, TEXTCOLOR, 0xffFFFFFF );
			SetControlColor( banner->yes, TEXTCOLOR, 0xffFFFFFF );
			SetControlColor( banner->no, TEXTCOLOR, 0xffFFFFFF );

		}
		//if( flags & BANNER_TOP )
		//	MakeTopmost( banner->renderer );
		else if( banner->flags & (BANNER_OPTION_YESNO|BANNER_OPTION_OKAYCANCEL) )
		{
			banner->cancel = MakeButton( banner->frame
												, 5, ( ( BANNER_HEIGHT * 2 ) / 3 ) - 5 - 10
												, ( BANNER_WIDTH / 2) - 10, ((BANNER_HEIGHT * 1 ) / 3 ) - 20
												, IDCANCEL
												, ( flags & BANNER_OPTION_YESNO )?WIDE( "No" ):WIDE( "Cancel" ), 0
												, CancelChoiceClicked
												, (uintptr_t)banner );
			SetButtonColor( banner->cancel, AColor( 0x9a, 0x05, 0x1d, 0xdf )  );
			banner->okay = MakeButton( banner->frame
											 , 5 + ( BANNER_WIDTH / 2) - 5, ( ( BANNER_HEIGHT * 2 ) / 3 ) - 5 - 10
											 , ( BANNER_WIDTH / 2) - 10, ((BANNER_HEIGHT * 1 ) / 3 ) - 20
											 , IDOK
											 , ( flags & BANNER_OPTION_YESNO )?WIDE( "Yes" ):WIDE( "Okay!" ), 0
											 , OkayChoiceClicked
											 , (uintptr_t)banner );
			SetButtonColor( banner->okay, AColor( 0x18, 0x98, 0x6c, 0xdf ) );
			SetControlColor( banner->okay, TEXTCOLOR, 0xffFFFFFF );
			SetControlColor( banner->cancel, TEXTCOLOR, 0xffFFFFFF );
			//SetCommonMouse( banner->frame, NULL, 0 );
		}
		else
		{
			if( banner->okay )
			{
				DestroyCommon( &banner->okay );
				// this is taken care of by DestroyCommon- which is why we pass the address of banner->okay
				//banner->okay = NULL;
			}
			if( banner->cancel )
			{
				DestroyCommon( &banner->cancel );
				// this is taken care of by DestroyCommon- which is why we pass the address of banner->cancel
				//banner->cancel = NULL;
			}
			//SetFrameMouse( banner->frame, ClickHandler, (uintptr_t)banner );
		}
#if REQUIRE_PSI(1,1)
		SetFrameFont( banner->frame, banner_local.font );
#endif
		if( !banner_local.flags.bFullDraw )
		{
			SmudgeCommon( banner->frame );
		}
 	}
}

//--------------------------------------------------------------------------

#undef WaitForBanner2
int WaitForBanner2( PBANNER banner )
{
	return WaitForBanner2Ex( &banner );
}

int WaitForBanner2Ex( PBANNER *ppBanner )
{
	if( !ppBanner )
		ppBanner = &banner_local.banner;
	if( *ppBanner )
	{
		int result = 0;
#ifdef DEBUG_BANNER_DRAW_UPDATE
		lprintf( WIDE("------- BANNER BEGIN WAITING --------------") );
#endif
		(*ppBanner)->flags |= BANNER_WAITING;
#ifdef DEBUG_BANNER_DRAW_UPDATE
		lprintf( WIDE("------- BANNER BEGIN WAITING  REALLY --------------") );
#endif
		(*ppBanner)->pWaitThread = MakeThread();
		while( (*ppBanner) && ( !((*ppBanner)->flags & BANNER_CLOSED) ) )
		{
			if( !Idle() )
				WakeableSleep( 250 );
		}
		if( *ppBanner )
		{
			(*ppBanner)->flags &= ~BANNER_WAITING;
			result = (((*ppBanner)->flags & BANNER_OKAY )?1:0) | (((*ppBanner)->flags & BANNER_EXTENDED_RESULT )?2:0);
#ifdef DEBUG_BANNER_DRAW_UPDATE
			lprintf( WIDE("Remove (*ppBanner)!") );
#endif
			RemoveBanner2Ex( ppBanner DBG_SRC );
			(*ppBanner) = NULL;
		}
		return result;
	}
	return FALSE;
}

//--------------------------------------------------------------------------

SFTFont GetBanner2Font( void )
{
	InitBannerFrame();
	return banner_local.font;
}

//--------------------------------------------------------------------------

uint32_t GetBanner2FontHeight( void )
{
	InitBannerFrame();
	return GetFontHeight( banner_local.font );
}

//--------------------------------------------------------------------------

PRENDERER GetBanner2Renderer( PBANNER banner )
{
	if( !banner )
		banner = banner_local.banner;
	if( !banner )
		return NULL;
	return banner->renderer;
}

PSI_CONTROL GetBanner2Control( PBANNER banner )
{
	if( !banner )
		banner = banner_local.banner;
	if( !banner )
		return NULL;
	return banner->frame;
}

//--------------------------------------------------------------------------

struct banner_confirm_local
{
	CTEXTSTR name;
	struct {
		BIT_FIELD bNoResult : 1;
		BIT_FIELD key_result : 1;
		BIT_FIELD key_result_yes : 1;
		BIT_FIELD key_result_no : 1;
		BIT_FIELD bLog : 1;
	} flags;
	PTHREAD pWaiting;
	PBANNER banner;
	DoConfirmProc dokey;
	DoConfirmProc nokey;
};

typedef struct banner_confirm_local *PCONFIRM_BANNER;
typedef struct banner_confirm_local CONFIRM_BANNER;
static PLIST confirmation_banners;

struct thread_params {
	CTEXTSTR type;
	int yesno;
	CTEXTSTR msg;
	DoConfirmProc dokey;
	DoConfirmProc nokey;
	int received;
};


static PCONFIRM_BANNER GetWaitBanner2( CTEXTSTR name )
{
	INDEX idx;
	PCONFIRM_BANNER cb;
	LIST_FORALL( confirmation_banners, idx, PCONFIRM_BANNER, cb )
	{
		if( StrCaseCmp( name, cb->name ) == 0 )
		{
			break;
		}
	}
	if( !cb )
	{
		cb = New( CONFIRM_BANNER );
		MemSet( cb, 0, sizeof( CONFIRM_BANNER ) );
		cb->name = StrDup( name );
#ifndef __NO_OPTIONS__
		cb->flags.bLog = SACK_GetProfileIntEx( WIDE("SACK/Widgets/Banner2"), WIDE("Log wait banner"), 0, TRUE );
#endif
		AddLink( &confirmation_banners, cb );
	}
	return cb;
}

void Banner2AnswerYes( CTEXTSTR type )
{
	PCONFIRM_BANNER cb = GetWaitBanner2( type );
	if( cb->banner )
	{
		cb->flags.key_result_yes = 1;
		cb->flags.key_result_no = 0;
		cb->flags.key_result = 1;
		RemoveBanner2( cb->banner );
	}
}
void Banner2AnswerNo( CTEXTSTR type )
{
	PCONFIRM_BANNER cb = GetWaitBanner2( type );
	if( cb->banner )
	{
		cb->flags.key_result_no = 1;
		cb->flags.key_result_yes = 0;
		cb->flags.key_result = 1;
		RemoveBanner2( cb->banner );
	}
}

static uintptr_t CPROC Confirm( PTHREAD thread )
{
	struct thread_params *parms = ( struct thread_params * )GetThreadParam( thread );
	TEXTSTR msg;
	int result;
	int yesno;
	PCONFIRM_BANNER cb = GetWaitBanner2( parms->type );
	// only one ...
	cb->dokey = parms->dokey;
	cb->nokey = parms->nokey;
	msg = StrDup( parms->msg );
	yesno = parms->yesno;
	parms->received = 1;
	cb->pWaiting = thread;
	result = CreateBanner2Ex( NULL, &cb->banner, msg, BANNER_TOP|(yesno?BANNER_OPTION_YESNO:(BANNER_CLICK|BANNER_TIMEOUT))
									, (yesno?0:5000) );
	Release( msg );
	if( cb->flags.bLog )
		lprintf( WIDE( "returned from banner..." ) );

	if( cb->flags.key_result )
	{
		// consume the result
		cb->flags.key_result = 0;
		if( cb->flags.key_result_yes )
		{
			if( cb->dokey )
				cb->dokey();
			cb->dokey = NULL;
			return 1;
		}
		else if( cb->flags.key_result_no )
		{
			if( cb->nokey )
				cb->nokey();
		}
		return 0;
	}
	else
	{
		if( cb->flags.bLog )
			lprintf( WIDE( "Banner yesno was clicked..." ) );
		if( result )
		{
			if( cb->flags.bLog )
				lprintf( WIDE( "Okay do that ... invoke pending event." ) );
			if( cb->dokey )
				cb->dokey();
			cb->dokey = NULL;
		}
		else
		{
			if( cb->flags.bLog )
				lprintf( WIDE( "Okay do that ... invoke pending event(no)." ) );
			if( cb->nokey )
				cb->nokey();
			cb->nokey = NULL;
		}

	}
	if( cb->flags.bNoResult )
	{
		if( cb->flags.bLog )
			lprintf( WIDE( "Result already consumed..." ) );
		return -1;
	}

	if( cb->flags.bLog )
		lprintf( WIDE( "Yes no is %d" ), result );
	cb->flags.bNoResult = 1;
	return result;


}

static int Banner2ThreadConfirmExx( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey, DoConfirmProc nokey, int yesno )
{
	struct thread_params parms;
	parms.type = type;
	parms.received = 0;
	parms.dokey = dokey;
	parms.nokey = nokey;
	parms.msg = msg;
	parms.yesno = yesno;
	ThreadTo( Confirm, (uintptr_t)&parms );
	while( !parms.received )
		Relinquish();
	return 0;
}

int Banner2ThreadConfirm( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey )
{
	return Banner2ThreadConfirmExx( type, msg, dokey, NULL, TRUE );
}

int Banner2ThreadConfirmEx( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey, DoConfirmProc doNoKey )
{
	return Banner2ThreadConfirmExx( type, msg, dokey, doNoKey, TRUE );
}
//--------------------------------------------------------------------------

#ifdef __cplusplus_cli
#include <vcclr.h>

namespace SACK {
	public ref class Banner
	{
	public:
		//PBANNER banner;
		static Banner()
		{
			// don't really do anything....

		}

		static void Message( System::String^ message )
		{
			pin_ptr<const wchar_t> wch = PtrToStringChars(message);
			size_t convertedChars = 0;
			size_t  sizeInBytes = ((message->Length + 1) * 2);
			errno_t err = 0;
			TEXTCHAR    *ch = DupWideToText(wch);

			CreateBanner2Ex( NULL, NULL, ch, BANNER_NOWAIT|BANNER_DEAD, 0 );
			Release( ch );
		}
		static void Remove( void )
		{
			RemoveBanner2Message();
			//BannerMessageNoWait( string );
		}

	}  ;
}


#endif

BANNER_NAMESPACE_END
