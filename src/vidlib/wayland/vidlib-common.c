#ifdef _WIN32

/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *	 the first delay is between CreateWindow and WM_NCCREATE
 *	 the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/



//#define _OPENGL_ENABLED
/* this must have been done for some other collision in some other bit of code...
 * probably the update queue? the mosue queue ?
 */

#ifdef UNDER_CE
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#define USE_KEYHOOK
#define _INCLUDE_CLIPBOARD
#endif

#define DEBUG_INVALIDATE 0

#ifdef _MSC_VER
#  ifndef WINVER
#    define WINVER 0x0501
#  endif
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#  endif
#endif

#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#include <sqlgetoption.h>
#include <deadstart.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <sharemem.h>
//#define NO_LOGGING
#include <logging.h>
#include <msgclient.h>
#include <idle.h>
//#include <imglib/imagestruct.h>

#define USE_IMAGE_INTERFACE l.pii
#include <image.h>
#undef StrDup
#include <shlwapi.h> // must include this if shellapi.h is used.
#include <shellapi.h> // very last though - this is DragAndDrop definitions...

//#define LOG_ORDERING_REFOCUS
//#define LOG_MOUSE_EVENTS
//#define LOG_RECT_UPDATE
//#define LOG_DESTRUCTION
//#define LOG_STARTUP
//#define LOG_FOCUSEVENTS
#define OTHER_EVENTS_HERE
//#define LOG_SHOW_HIDE
//#define LOG_DISPLAY_RESIZE
//#define NOISY_LOGGING
// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS
//#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT
#include <vidlib/vidstruc.h>
#include "vidlib.h"

#include "keybrd.h"

static int stop;
//HWND	  hWndLastFocus;

// commands to the video thread for non-windows native ....
#define CREATE_VIEW 1


IMAGE_NAMESPACE

struct saved_location
{
	int32_t x, y;
	uint32_t w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
	Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h );
	uintptr_t psv;
};
IMAGE_NAMESPACE_END


// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

RENDER_NAMESPACE

extern KEYDEFINE KeyDefs[];

// forward declaration - staticness will probably cause compiler errors.
static int CPROC ProcessDisplayMessages(void );

//----------------------------------------------------------------------------

/* register and track drag file acceptors - per video surface... attched to hvdieo structure */
struct dropped_file_acceptor_tag {
	dropped_file_acceptor f;
	uintptr_t psvUser;
};

void WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, uintptr_t psvUser )
{
	if( renderer )
	{
		struct dropped_file_acceptor_tag *newAcceptor = New( struct dropped_file_acceptor_tag );
		newAcceptor->f = f;
		newAcceptor->psvUser = psvUser;
		AddLink( &renderer->dropped_file_acceptors, newAcceptor );
	}
}


LOGICAL InMyChain( PRENDERER hVideo, HWND hWnd )
{
	PRENDERER base;
	int count = 0 ;
		TEXTCHAR classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		if( strcmp( classname, "VideoOutputClass" ) == 0 )
			return 2;
	base = hVideo;
	while( base->pBelow ) 
	{
		count++;
		if( count > 100 )
			DebugBreak();
		base = base->pBelow;
	}
	while( base )
	{
		if( base->hWndOutput== hWnd )
			return 1;
		base = base->pAbove;
	}
	return 0;
}
void DumpMyChain( PRENDERER hVideo DBG_PASS )
#define DumpMyChain(h) DumpMyChain( h DBG_SRC )
{
#ifndef UNDER_CE
	PRENDERER base;
	base = hVideo;
	// follow to the lowest, and chain upwards...
	while( base->pAbove ) base = base->pAbove;
	_lprintf(DBG_RELAY)( "bottommost" );
	while( base )
	{
		TEXTCHAR title[256];
		TEXTCHAR classname[256];
		GetClassName( base->hWndOutput, classname, sizeof( classname ) );
		GetWindowText( base->hWndOutput, title, sizeof( title ) );
		if( base == hVideo )
			_lprintf(DBG_RELAY)( "--> %p[%p] %s", base, base->hWndOutput, title );
		else
			_lprintf(DBG_RELAY)( "	 %p[%p] %s", base, base->hWndOutput, title );
		base = base->pBelow;
	}
	lprintf( "topmost" );
#endif
}

void DumpChainAbove( PRENDERER chain, HWND hWnd )
{
#ifndef UNDER_CE
	int not_mine = 0;
	TEXTCHAR title[256];
	GetWindowText( hWnd, title, sizeof( title ) );
	lprintf( "Dumping chain of windows above %p %s", hWnd, title );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDPREV ) )
	{
		int ischain;
		TEXTCHAR title[256];
		TEXTCHAR classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		GetWindowText( hWnd, title, sizeof( title ) );
		if( ischain = InMyChain( chain, hWnd ) )
		{
			lprintf( "%s %p %s %s", ischain==2?">>>":"^^^", hWnd, title, classname );
			not_mine = 0;
		}
		else
		{
			not_mine++;
			if( not_mine < 10 )
				lprintf( "... %p %s %s", hWnd, title, classname );
		}
	}
#endif
}

void DumpChainBelow( PRENDERER chain, HWND hWnd DBG_PASS )
#define DumpChainBelow(c,h) DumpChainBelow(c,h DBG_SRC)
{
#ifndef UNDER_CE
	int not_mine = 0;
	TEXTCHAR title[256];
	GetWindowText( hWnd, title, sizeof( title ) );
	_lprintf(DBG_RELAY)( "Dumping chain of windows below %p", hWnd );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDNEXT ) )
	{
		int ischain;
		TEXTCHAR classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		GetWindowText( hWnd, title, sizeof( title ) );
		if( ischain = InMyChain( chain, hWnd ) )
		{
			lprintf( "%s %p %s %s", ischain==2?">>>":"^^^", hWnd, title, classname );
			not_mine = 0;
		}
		else
		{
			not_mine++;
			if( not_mine < 10 )
				lprintf( "... %p %s %s", hWnd, title, classname );
		}
	}
#endif
}

//----------------------------------------------------------------------------

void SafeSetFocus( HWND hWndSetTo )
{
	DWORD dwProc1;
	DWORD dwThread2;
	DWORD dwThreadMe = 0;
	if( hWndSetTo != GetForegroundWindow() )
	{
		dwThread2 = GetWindowThreadProcessId(
														 hWndSetTo
														,NULL);
		dwProc1 = GetWindowThreadProcessId(
													  GetForegroundWindow()
													 ,NULL);
		dwThreadMe = GetCurrentThreadId();
#ifndef UNDER_CE
		if( dwThreadMe != dwProc1 )
			AttachThreadInput(dwProc1, dwThreadMe
								  ,TRUE);
		if( dwThreadMe != dwThread2 && dwThread2 != dwProc1 )
			AttachThreadInput( dwThread2, dwThreadMe
								  ,TRUE);
#endif
		//Detach the attached thread
	}
	//lprintf( "Safe Set Focus %p", hWndSetTo );
	//ShowWindow( hWndSetTo, SW_SHOW );
	SetForegroundWindow( hWndSetTo );
	SetActiveWindow( hWndSetTo );
	SetFocus( hWndSetTo );

	if( dwThreadMe )
	{
#ifndef UNDER_CE
		if( dwThreadMe != dwProc1 )
			AttachThreadInput(dwProc1, dwThreadMe
								  ,FALSE);
		if( dwThreadMe != dwThread2 && dwThread2 != dwProc1 )
			AttachThreadInput( dwThread2, dwThreadMe
								  ,FALSE);
#endif
	}
}

#ifndef NO_TRANSPARENCY
void IssueUpdateLayeredEx( PRENDERER hVideo, LOGICAL bContent, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS )
{
								if( l.UpdateLayeredWindow )
								{
									//SIZE size;// = new Win32.Size(bitmap.Width, bitmap.Height);

									static POINT pointSource;// = new Win32.Point(0, 0);
									//POINT topPos;/// = new Win32.Point(Left, Top);
									// DEFINED in WinGDI.h
									BLENDFUNCTION blend;// = new Win32.BLENDFUNCTION();
									blend.BlendOp				 = AC_SRC_OVER;
									if( hVideo->fade_alpha )
									{
										if( hVideo->fade_alpha == -1 )
											blend.SourceConstantAlpha = 0;//opacity;
										else
										{
											blend.SourceConstantAlpha = hVideo->fade_alpha;//opacity;
										}
									}
									else
										blend.SourceConstantAlpha = 255;//opacity;
									blend.BlendFlags			 = 0;
									blend.AlphaFormat			= AC_SRC_ALPHA;

									hVideo->size.cx = hVideo->pWindowPos.cx;
									hVideo->size.cy = hVideo->pWindowPos.cy;
									hVideo->topPos.x = hVideo->pWindowPos.x;
									hVideo->topPos.y = hVideo->pWindowPos.y;
									// no way to specify just the x/y w/h of the portion we want
									// to update...
									if( l.flags.bLogWrites )
										lprintf( "layered... begin update. %d %d %d %d", hVideo->size.cx, hVideo->size.cy, hVideo->topPos.x, hVideo->topPos.y );
									if( bContent
										&& l.UpdateLayeredWindowIndirect
										&& ( x || y || w != hVideo->pWindowPos.cx || h != hVideo->pWindowPos.cy ) )
									{
										// this is Vista+ function.
										RECT rc_dirty;
										//UPDATELAYEREDWINDOWINFO ULWInfo;
										rc_dirty.top = y;
										rc_dirty.left = x;
										rc_dirty.right = x + w;
										rc_dirty.bottom = y + h;
										hVideo->ULWInfo.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
										hVideo->ULWInfo.hdcDst = bContent?hVideo->hDCOutput:NULL;
										hVideo->ULWInfo.pptDst = bContent?&hVideo->topPos:NULL;
										hVideo->ULWInfo.psize = bContent?&hVideo->size:NULL;
										hVideo->ULWInfo.hdcSrc = bContent?hVideo->hDCBitmap:NULL;
										hVideo->ULWInfo.pptSrc = bContent?&pointSource:NULL;
										hVideo->ULWInfo.crKey = 0;
										hVideo->ULWInfo.pblend = &blend;
										hVideo->ULWInfo.dwFlags = ULW_ALPHA;
										hVideo->ULWInfo.prcDirty = bContent?&rc_dirty:NULL;
//#ifdef LOG_RECT_UPDATE
										if( l.flags.bLogWrites )
											_lprintf(DBG_RELAY)( "using indirect (with dirty rect %d %d %d %d)", x, y, w, h );
//#endif
										if( !l.UpdateLayeredWindowIndirect( hVideo->hWndOutput, &hVideo->ULWInfo ) )
											lprintf( "Error using UpdateLayeredWindowIndirect: %d", GetLastError() );
									}
									else
									{
										// we CAN do indirect...
										l.UpdateLayeredWindow(
																	 hVideo->hWndOutput
																	, bContent?(HDC)hVideo->hDCOutput:NULL
																	, bContent?&hVideo->topPos:NULL
																	, bContent?&hVideo->size:NULL
																	, bContent?(hVideo->flags.bLayeredWindow&&hVideo->flags.bFullScreen
																					&& !hVideo->flags.bNotFullScreen)?(HDC)hVideo->hDCBitmapFullScreen:hVideo->hDCBitmap:NULL
																	, bContent?&pointSource:NULL
																	, 0 // color key
																	, &blend
																	, ULW_ALPHA);
									}
									//lprintf( "layered... end update." );
								}
}
#endif

//----------------------------------------------------------------------------
static void InvokeDisplaySizeChange( PRENDERER r, int nDisplay, uint32_t x, uint32_t y, uint32_t width, uint32_t height )
{
	void (CPROC *size_change)( PRENDERER, int nDisplay, uint32_t x, uint32_t y, uint32_t width, uint32_t height );
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( "sack/render/display", &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		size_change = GetRegisteredProcedureExx( data,(CTEXTSTR)name,void,"on_display_size_change",( PRENDERER, int nDisplay, uint32_t x, uint32_t y, uint32_t width, uint32_t height ));

		if( size_change )
			size_change( r, nDisplay, x, y, width, height );
	}

}
//----------------------------------------------------------------------------

RENDER_PROC (void, EnableLoggingOutput)( LOGICAL bEnable )
{
	l.flags.bLogWrites = bEnable;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, UpdateDisplayPortionEx)( PRENDERER hVideo
														, int32_t x, int32_t y
														, uint32_t w, uint32_t h DBG_PASS)
{
	ImageFile *pImage;
	if (hVideo
		 && (pImage = hVideo->pImage) && hVideo->hDCBitmap && hVideo->hDCOutput)
	{
#ifdef LOG_RECT_UPDATE
		lprintf( "Entering from %s(%d)", pFile, nLine );
#endif
		if( x + (signed)w < 0 )
			return;
		if( y + (signed)h < 0 )
			return;
		if( x > hVideo->pWindowPos.cx ) 
			return;
		if( y > hVideo->pWindowPos.cy ) 
			return;
		if (!h)
			h = pImage->height;
		if (!w)
			w = pImage->width;

		if( l.flags.bLogWrites )
			_xlprintf( 1 DBG_RELAY )( "Write to Window: %p %d %d %d %d", hVideo, x, y, w, h );

		if (!hVideo->flags.bShown)
		{
			if( l.flags.bLogWrites )
				lprintf( "Setting shown..." );
			hVideo->flags.bShown = TRUE;
			hVideo->flags.bHidden = FALSE;
			if( hVideo->pRestoreCallback )
				hVideo->pRestoreCallback( hVideo->dwRestoreData );

			if( l.flags.bLogWrites )
				_xlprintf( 1 DBG_RELAY )( "Show Window: %d %d %d %d", x, y, w, h );
			//lprintf( "During an update showing window... %p", hVideo );
			if( hVideo->flags.bTopmost )
			{
#ifdef LOG_ORDERING_REFOCUS
				//DumpMyChain( hVideo );
				//DumpChainBelow( hVideo, hVideo->hWndOutput );
				//DumpChainAbove( hVideo, hVideo->hWndOutput );
				lprintf( "Putting window above ... %p %p", hVideo->pBelow?hVideo->pBelow->hWndOutput:NULL
								, GetNextWindow( hVideo->hWndOutput, GW_HWNDNEXT ) );
				lprintf( "Putting should be below ... %p %p", hVideo->pAbove?hVideo->pAbove->hWndOutput:NULL, GetNextWindow( hVideo->hWndOutput, GW_HWNDPREV ) );
#endif
				//lprintf( "Initial setup of window -> topmost.... %p", HWND_TOPMOST );
				//hVideo->pWindowPos.hwndInsertAfter = HWND_TOPMOST; // otherwise we get '0' as our desired 'after'
				if( l.flags.bLogWrites )
					lprintf( "Show window %s  %p %p  %d"
						, hVideo->flags.bTopmost?"TOpMost":"not topmost"
						, hVideo->pAbove, hVideo->pBelow, hVideo->flags.bOpenedBehind );
				/*
				SetWindowPos (hVideo->hWndOutput
								, hVideo->flags.bTopmost?HWND_TOPMOST:HWND_TOP
								, 0, 0, 0, 0,
								SWP_NOMOVE
								| SWP_NOSIZE
								| SWP_NOACTIVATE
								);
				*/
			}
					if( hVideo->pAbove || hVideo->pBelow || hVideo->flags.bOpenedBehind )
					{
						if( ( hVideo->pAbove 
#ifndef UNDER_CE
							  &&( GetNextWindow( hVideo->hWndOutput, GW_HWNDNEXT ) != hVideo->pAbove->hWndOutput ) 
#endif
							  )
							//||( hVideo->pAbove &&
							//	( GetNextWindow( hVideo->hWndOutput, GW_HWNDNEXT ) != hVideo->pBelow->hWndOutput ) )
						  )
						{
							lprintf( "is out of position, attempt to correct during show. Hope we can test this@@" );
							/*
							SetWindowPos( hVideo->hWndOutput
											, hVideo->pAbove->pWindowPos.hwndInsertAfter
											, 0, 0, 0, 0,
											 SWP_NOMOVE
											 | SWP_NOSIZE
											 | ((hVideo->flags.bLayeredWindow )?SWP_NOACTIVATE:0)
											 //| SWP_SHOWWINDOW
											);
											*/
							/*
							 else
							 SetWindowPos (hVideo->hWndOutput
							 , hVideo->pBelow?hVideo->pBelow->hWndOutput:hVideo->flags.bTopmost?HWND_TOPMOST:HWND_TOP
							 , 0, 0, 0, 0,
							 SWP_NOMOVE
							 | SWP_NOSIZE
							 | ((hVideo->flags.bLayeredWindow )?SWP_NOACTIVATE:0)
							 //| SWP_SHOWWINDOW
							 );
							 */
						}
						else
						{
#ifdef LOG_ORDERING_REFOCUS
							lprintf( "Okay it's already in the right zorder.." );
#endif
							hVideo->flags.bIgnoreChanging = 1;
							// the show window generates WM_PAINT, and MOVES THE WINDOW.
							// the window is already in the right place, and knows it.
						}
						if(l.flags.bLogWrites)lprintf( "Show WIndow NO Activate... and..." );

						ShowWindow (hVideo->hWndOutput, SW_SHOWNA );
						if(l.flags.bLogWrites)lprintf( "show window done... so..." );
					}
					else
					{
#ifdef LOG_OPEN_TIMING
						lprintf( "Showing window..." );
#endif

						// SW_shownormal does extra stuff, that I think causes top level windows to fall behind other
						// topmost windows.
						if( hVideo->flags.bTopmost )
						{
							if( l.flags.bLogWrites )
								lprintf( "Show again, topmost." );
							SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
										SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
						}
						else {
							if( l.flags.bLogWrites )
								lprintf( "ShowWindow, SHOWNORMAL." );
#ifdef UNDER_CE
							ShowWindow (hVideo->hWndOutput, SW_SHOWNORMAL );
#else
							ShowWindow (hVideo->hWndOutput, SW_SHOW );
#endif
						}
#ifdef LOG_OPEN_TIMING
						lprintf( "window shown..." );
#endif
#ifdef LOG_ORDERING_REFOCUS
						lprintf( "Having shown window... should have had a no-event paint ?" );
#endif
					}
#if 0
					if( hVideo->flags.bReady && hVideo->flags.bShown && hVideo->pRedrawCallback )
					{
						if( l.flags.bLogWrites )
							lprintf( "Bitblit out..." );
						BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
						  (HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
					}
					// show will generate a paint...
					// and on the paint we will draw, otherwise
					// we'll do it twice.
					if( !hVideo->pAbove && !hVideo->flags.bNoAutoFocus )
					{
						SafeSetFocus( hVideo->hWndOutput );
						if( hVideo->pRedrawCallback )
							hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);

					}
#endif
		} // end of if(!shown) else shown....
		else
		{
#ifdef LOG_RECT_UPDATE
			_xlprintf( 1 DBG_RELAY )( "Draw to Window: %d %d %d %d", x, y, w, h );
#endif
			if( hVideo->flags.bHidden )
				return;

			{
				PTHREAD thread = NULL;

				{
					INDEX idx;
					if( FindLink( &l.invalidated_windows, hVideo ) == INVALID_INDEX )
					LIST_FORALL( l.threads, idx, PTHREAD, thread )
					{
						// okay if it's layered, just let the draws through always.
						if( (hVideo->flags.bLayeredWindow) || IsThisThread( thread ) || ( x || y ) )
						{
							if( hVideo->flags.bOpenGL )
								if( l.actual_thread != thread )
									 continue;
							//lprintf( "Is a thread." );
							EnterCriticalSec( &hVideo->cs );
							if( hVideo->flags.bDestroy )
							{
								//lprintf( "Saving ourselves from operating a draw while destroyed." );
								// by now we could be in a place where we've been destroyed....
								LeaveCriticalSec( &hVideo->cs );
								return;
							}
#ifdef LOG_RECT_UPDATE
							lprintf( "Good thread..." ); /* can't be? */
#endif
							// StretchBlt can take the raw data by the way...
#ifdef _OPENGL_ENABLED
							{
								int n;
								for( n = 0; n < hVideo->nFractures; n++ )
								{
									BitBlt ( (HDC)hVideo->hDCBitmap
										, hVideo->pFractures[n].x, hVideo->pFractures[n].y
										, hVideo->pFractures[n].w, hVideo->pFractures[n].h
										, (HDC)hVideo->pFractures[n].hDCBitmap, 0, 0, SRCCOPY);
								}
							}
#endif
							if( hVideo->sprites )
							{
								INDEX idx;
								uint32_t _w, _h;
								PSPRITE_METHOD psm;
								// set these to set clipping for sprite routine
								hVideo->pImage->x = x;
								hVideo->pImage->y = y;
								_w = hVideo->pImage->width;
								hVideo->pImage->width = w;
								_h = hVideo->pImage->height;
								hVideo->pImage->height = h;
#ifdef DEBUG_TIMING
								lprintf( "Save screen..." );
#endif
								LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
								{
									if( ( psm->original_surface->width != psm->renderer->pImage->width ) ||
										( psm->original_surface->height != psm->renderer->pImage->height ) )
									{
										UnmakeImageFile( psm->original_surface );
										psm->original_surface = MakeImageFile( psm->renderer->pImage->width, psm->renderer->pImage->height );
									}
									//lprintf( "save Sprites" );
									BlotImageSized( psm->original_surface, psm->renderer->pImage
										, x, y, w, h );
#ifdef DEBUG_TIMING
									lprintf( "Render sprites..." );
#endif
									if( psm->RenderSprites )
									{
										// if I exported the PSPRITE_METHOD structure to the image library
										// then it could itself short circuit the drawing...
										//lprintf( "render Sprites" );
										psm->RenderSprites( psm->psv, hVideo, x, y, w, h );
									}
#ifdef DEBUG_TIMING
									lprintf( "Done render sprites...");
#endif
								}
#ifdef DEBUG_TIMING
								lprintf( "Done save screen and update spritess..." );
#endif
								hVideo->pImage->x = 0;
								hVideo->pImage->y = 0;
								hVideo->pImage->width = _w;
								hVideo->pImage->height = _h;
							}

							if( l.flags.bLogWrites )
								_lprintf(DBG_RELAY)( "Output %d,%d %d,%d", x, y, w, h);

#ifndef NO_TRANSPARENCY
							if( hVideo->flags.bLayeredWindow )
							{
								IssueUpdateLayeredEx( hVideo, TRUE, x, y, w, h DBG_SRC );
							}
							else
#endif
							{
								//lprintf( "non layered... begin update." );
								if( hVideo->flags.bFullScreen && !hVideo->flags.bNotFullScreen )
								{

									uint32_t w;
									uint32_t h;
									int32_t x, y;
									w =  hVideo->pImage->width * hVideo->full_screen.width / hVideo->pImage->width;
									h =  hVideo->pImage->height * hVideo->full_screen.width / hVideo->pImage->width;
									if( h > hVideo->full_screen.height )
									{
										w =  hVideo->pImage->width * hVideo->full_screen.height / hVideo->pImage->height;
										h =  hVideo->pImage->height * hVideo->full_screen.height / hVideo->pImage->height;
									}
									y = ( hVideo->full_screen.height - h ) / 2;
									x = ( hVideo->full_screen.width - w ) / 2;
									if( l.flags.bDoNotPreserveAspectOnFullScreen )
										StretchBlt ((HDC)hVideo->hDCOutput, hVideo->full_screen.x, hVideo->full_screen.y, hVideo->full_screen.width, hVideo->full_screen.height,
														(HDC)hVideo->hDCBitmap, 0, 0, hVideo->pImage->width, hVideo->pImage->height, SRCCOPY);
									else
										StretchBlt ((HDC)hVideo->hDCOutput, x, y, w, h,//hVideo->full_screen.width, hVideo->full_screen.height,
														(HDC)hVideo->hDCBitmap, 0, 0, hVideo->pImage->width, hVideo->pImage->height, SRCCOPY);
								}
								else
									BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
											  (HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
								//lprintf( "non layered... end update." );
							}
							if( hVideo->sprites )
							{
								INDEX idx;
								PSPRITE_METHOD psm;
								struct saved_location location;
#ifdef DEBUG_TIMING 
								lprintf( "Restore Original" );
#endif
								LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
								{
									//BlotImage( psm->renderer->pImage, psm->original_surface
									//			, 0, 0 );
									while( DequeData( &psm->saved_spots, &location ) )
									{
										// restore saved data from image to here...
										//lprintf( "Restore %d,%d %d,%d", location.x, location.y
										//					 , location.w, location.h );

										BlotImageSizedEx( hVideo->pImage, psm->original_surface
											, location.x, location.y
											, location.x, location.y
											, location.w, location.h
											, 0
											, BLOT_COPY );

									}
								}
#ifdef DEBUG_TIMING
								lprintf( "Restored Original" );
#endif
							}
							LeaveCriticalSec( &hVideo->cs );
							break;
						}
					}
				}

				if( !thread )
				{
					RECT r;
					if( hVideo->portion_update.pending ) {
						if( SUS_LT( x, int32_t, hVideo->portion_update.x, uint32_t ) )
						{
							hVideo->portion_update.w += hVideo->portion_update.x - x;
							hVideo->portion_update.x = x;
						}
						if( SUS_LT( y, int32_t, hVideo->portion_update.y, uint32_t ) )
						{
							hVideo->portion_update.h += hVideo->portion_update.y - y;
							hVideo->portion_update.y = y;
						}
						if( (x + w) > ( hVideo->portion_update.x + hVideo->portion_update.w ) )
						{
							hVideo->portion_update.w = (x + w) - hVideo->portion_update.x;
						}
						if( (y + h) > (hVideo->portion_update.y + hVideo->portion_update.h) )
						{
							hVideo->portion_update.h = (y + h) - hVideo->portion_update.y;
						}
					}
					else {
						hVideo->portion_update.pending = TRUE;
						if( x < 0 ) {
							if( (x + (int)w) < 1 )
								return;
							else {
								hVideo->portion_update.x = 0;
								hVideo->portion_update.w = (uint32_t)(w + x);
							}
						}
						else {
							hVideo->portion_update.x = (uint32_t)x;
							hVideo->portion_update.w = w;
						}
						if( y < 0 ) {
							if( (y + (int)h) < 1 )
								return;
							else {
								hVideo->portion_update.y = 0;
								hVideo->portion_update.h = (uint32_t)(h + y);
							}
						}
						else {
							hVideo->portion_update.y = (uint32_t)y;
							hVideo->portion_update.h = h;
						}
					}
					if( FindLink( &l.invalidated_windows, hVideo ) != INVALID_INDEX ) {
						if( l.flags.bLogWrites )
							lprintf( "saving from double posting... still processing prior update." );
						return;
					}
					r.left = x;
					r.top = y;
					r.right = r.left + w;
					r.bottom = r.top + h;
					//_lprintf(DBG_RELAY)( "Posting invalidation rectangle command thing..." );
					if( hVideo->flags.event_dispatched )
					{
#ifdef LOG_RECT_UPDATE
						lprintf( "No... saving the update... already IN an update..." );
#endif
					}
					else
					{
						if( l.flags.bLogWrites )
							lprintf( "setting post invalidate..." );
#if DEBUG_INVALIDATE
						lprintf( "set Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
						AddLink( &l.invalidated_windows, hVideo );
						InvalidateRect( hVideo->hWndOutput, &r, FALSE );
					}
				}
			}
		}
	}
	else
	{
		//lprintf( "Rendering surface is not able to be updated (no surface, hdc, bitmap, etc)" );
	}
#ifdef DEBUG_TIMING
	lprintf( "Done with UpdateDisplayPortionEx()" );
#endif
}

//----------------------------------------------------------------------------

void
UnlinkVideo (PRENDERER hVideo)
{
	// yes this logging is correct, to say what I am below, is to know what IS above me
	// and to say what I am above means I nkow what IS below me
#ifdef LOG_ORDERING_REFOCUS
	lprintf( " -- UNLINK Video %p from which is below %p and above %p", hVideo, hVideo->pAbove, hVideo->pBelow );
#endif
	//if( hVideo->pBelow || hVideo->pAbove )
	//	DebugBreak();
	if (hVideo->pBelow)
	{
		hVideo->pBelow->pAbove = hVideo->pAbove;
	}
	if (hVideo->pAbove)
	{
		hVideo->pAbove->pBelow = hVideo->pBelow;
	}
	//if (hVideo->pNext)
	 //  hVideo->pNext->pPrior = hVideo->pPrior;
	//if (hVideo->pPrior)
	//	hVideo->pPrior->pNext = hVideo->pNext;

	hVideo->pPrior = hVideo->pNext = hVideo->pAbove = hVideo->pBelow = NULL;
}

//----------------------------------------------------------------------------

void
FocusInLevel (PRENDERER hVideo)
{
	lprintf( "Focus IN level" );
	if (hVideo->pPrior)
	{
		hVideo->pPrior->pNext = hVideo->pNext;
		if (hVideo->pNext)
			hVideo->pNext->pPrior = hVideo->pPrior;

		hVideo->pPrior = NULL;

		if (hVideo->pAbove)
		{
			hVideo->pNext = hVideo->pAbove->pBelow;
			hVideo->pAbove->pBelow->pPrior = hVideo;
			hVideo->pAbove->pBelow = hVideo;
		}
		else		  // nothing points to this - therefore we must find the start
		{
			PRENDERER pCur = hVideo->pPrior;
			while (pCur->pPrior)
				pCur = pCur->pPrior;
			pCur->pPrior = hVideo;
			hVideo->pNext = pCur;
		}
		hVideo->pPrior = NULL;
	}
	// else we were the first in this level's chain...
}

//----------------------------------------------------------------------------

RENDER_PROC (void, PutDisplayAbove) (PRENDERER hVideo, PRENDERER hAbove)
{
	//  this above that...
	//  this->below is now that // points at things below
	// that->above = this // points at things above
#ifdef LOG_ORDERING_REFOCUS
	PRENDERER original_below = hVideo->pBelow;
	PRENDERER original_above = hVideo->pAbove;
#endif
	PRENDERER topmost = hAbove;
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "Begin Put Display Above..." );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();
	while( topmost->pBelow )
		topmost = topmost->pBelow;

#ifdef LOG_ORDERING_REFOCUS
	lprintf( "(actual)Put display above ... %p %p  [%p %p]"
		, hVideo, topmost
			 , hVideo?hVideo->hWndOutput:NULL
			 , topmost?topmost->hWndOutput: NULL );

	lprintf( "hvideo is above %p and below %p"
			 , original_above?original_above->hWndOutput:NULL
			 , original_below?original_below->hWndOutput:NULL );
	if( hAbove )
		lprintf( "hAbove is below %p and above %p"
				 , hAbove->pBelow?hAbove->pBelow->hWndOutput:NULL
				 , hAbove->pAbove?hAbove->pAbove->hWndOutput:NULL );
#endif
	// if this is already in a list (like it has pBelow things)
	// I want to insert hAbove between hVideo and pBelow...
	// if if above already has things above it, I want to put those above hvideo
	if( hVideo && hAbove && ( hVideo->pBelow || hVideo->pAbove ) 
		&& !hAbove->pBelow 
		&& !hAbove->pAbove )
	{

#ifdef LOG_ORDERING_REFOCUS
		lprintf( "Putting the above within the list of hVideo... reverse-insert." );
		//DumpMyChain( hVideo );
#endif
		if( hAbove->pAbove = hVideo->pAbove )
			hVideo->pAbove->pBelow = hAbove;

		hAbove->pBelow = hVideo;
		hVideo->pAbove = hAbove;

#ifdef LOG_ORDERING_REFOCUS
		lprintf( "(--- new chain, after linking---)" );
		//DumpMyChain( hVideo );
		// hVideo->below stays the same.
		lprintf( "Two starting windows %p %p",hVideo->hWndOutput
				 , hAbove->hWndOutput );
#endif
		if( hAbove->pBelow )
		{
			// do allow this to be re-inserted...
			// otherwise .. well... insertafteruhmm...

			// somehow, this is the correct window list order....
			// when the window 'habove' is insert 'before' 'hVideo', but above 'hvdieo's->below?
			hVideo->flags.bOrdering = 1;
			hAbove->flags.bOrdering = 1;
			SetWindowPos ( hAbove->hWndOutput
				, hAbove->under?hAbove->under->hWndOutput
				: topmost->pBelow->hWndOutput
							 , 0, 0, 0, 0,
							  SWP_NOMOVE
							  | SWP_NOSIZE
							  | SWP_NOACTIVATE
							 );
			hAbove->flags.bOrdering = 0;
			hVideo->flags.bOrdering = 0;
		}
		else
		{
			hAbove->flags.bOrdering = 1;
				SetWindowPos (hAbove->hWndOutput
							 , HWND_BOTTOM
							 , 0, 0, 0, 0,
							  SWP_NOMOVE
							  | SWP_NOSIZE
							  | SWP_NOACTIVATE
							 );
			hAbove->flags.bOrdering = 0;
		}

#ifdef LOG_ORDERING_REFOCUS
		lprintf( "------- DONE WITH PutDisplayAbove reorder.." );
#endif
		LeaveCriticalSec( &l.csList );
		return;
	}
	{

		EnterCriticalSec( &l.csList );
		UnlinkVideo (hVideo);		// make sure it's isolated...

		if( ( hVideo->pAbove = topmost ) )
		{
			//HWND hWndOver = GetNextWindow( topmost->hWndOutput, GW_HWNDPREV );
			if( hVideo->pBelow = topmost->pBelow )
			{
				hVideo->pBelow->pAbove = hVideo;
			}
			topmost->pBelow = hVideo;
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "Only thing here is... %p after %p or %p so use %p means...below?"
				, hVideo->hWndOutput
				, topmost->hWndOutput
				, topmost->pWindowPos.hwndInsertAfter
				, GetNextWindow( topmost->hWndOutput, GW_HWNDPREV ) );
#endif
			SetWindowPos ( hVideo->hWndOutput
							 , topmost->pWindowPos.hwndInsertAfter
							 , 0, 0, 0, 0
							 , SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE
							 );
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "Finished ordering..." );
			//DumpChainBelow( hVideo, hVideo->hWndOutput );
			//DumpChainAbove( hVideo, hVideo->hWndOutput );
#endif
		}
#ifdef LOG_ORDERING_REFOCUS
		//lprintf("Put Display Above (this)%p above (below)%p and before %p", hVideo->hWndOutput, hAbove->hWndOutput, hAbove->pWindowPos.hwndInsertAfter );
#endif
		LeaveCriticalSec( &l.csList );
	}
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "End Put Display Above..." );
#endif
}

RENDER_PROC (void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hIn)
{
	lprintf( "Relate hVideo as a child of hIn..." );
}

//----------------------------------------------------------------------------


void DoDestroy (PRENDERER hVideo)
{
	if (hVideo)
	{
		hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
		SetWindowLongPtr (hVideo->hWndOutput, WD_HVIDEO, 0);
		if (hVideo->pWindowClose)
		{
			hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
		if (SelectObject ((HDC)hVideo->hDCBitmap, hVideo->hOldBitmap) != hVideo->hBm)
		{
			Log ("Don't think we deselected the bm right");
		}
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCBitmap);
		if (!DeleteObject (hVideo->hBm))
		{
			Log ("Yup this bitmap is expanding...");
		}
		ReleaseEx( hVideo->pTitle DBG_SRC );
		DestroyKeyBinder( hVideo->KeyDefs );
		//hVideo->pImage->image = NULL; // cheat this out for now...
		// Image library tracks now that someone else gave it memory
		// and it does not deallocate something it didn't allocate...
		UnmakeImageFile (hVideo->pImage);

#ifdef LOG_DESTRUCTION
		lprintf( "In DoDestroy, destroyed a good bit already..." );
#endif

		// this will be cleared at the next statement....
		// which indicates we will be ready to be released anyhow...
		//hVideo->flags.bReady = FALSE;
		// unlink from the stack of windows...
		UnlinkVideo (hVideo);
		if( l.hCaptured == hVideo )
		{
			l.hCapturedPrior = NULL;
			l.hCaptured = NULL;
		}
		//Log ("Cleared hVideo - is NOW !bReady");
		if( !hVideo->flags.event_dispatched )
		{
			int bInDestroy = hVideo->flags.bInDestroy;
			MemSet (hVideo, 0, sizeof (VIDEO));
			// restore this flag... need to keep this so
			// we don't release the structure cuasing a
			// infinite hang while the bit is checked through
			// a released memory pointer.
			hVideo->flags.bInDestroy = bInDestroy;
		}
		else
			hVideo->flags.bReady = 0; // leave as much as we can if in a key...
	}
}

//----------------------------------------------------------------------------

HWND MoveWindowStack( PRENDERER hInChain, HWND hwndInsertAfter, int use_under )
{
	HWND result_after = hwndInsertAfter;
	HDWP hWinPosInfo = BeginDeferWindowPos( 1 );
	PRENDERER current;
	PRENDERER save_current;
	PRENDERER check;
	for( current = hInChain; current && current->pBelow; current = current->pBelow );
	for( check = current; check; check = check->pAbove )
		if( check->under )
		{
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "Found we want to be under something... so use that window instead." );
#endif
			hwndInsertAfter = check->under->hWndOutput;
		}

	save_current = current;
	for( ; current; current = current->pAbove )
	{
		if( current->flags.bHidden )
			continue;

#ifdef LOG_ORDERING_REFOCUS
		lprintf( "Add defered pos put %p after %p"
			, current->hWndOutput
			, hwndInsertAfter
			);
#endif
		current->flags.bDeferedPos = 1;
		DeferWindowPos( hWinPosInfo, current->hWndOutput, hwndInsertAfter
						  , 0, 0, 0, 0
						  , SWP_NOMOVE|SWP_NOSIZE);
		current->hDeferedAfter = hwndInsertAfter;
		if( current == hInChain )
			result_after = hwndInsertAfter;

		hwndInsertAfter = current->hWndOutput;
	}								
	EndDeferWindowPos( hWinPosInfo );

	current = save_current;
	while( current )
	{
		current->flags.bDeferedPos = 0;
		current = current->pAbove;
	}

	return result_after;
}

#ifdef _MSC_VER

static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
		if( l.flags.bLogKeyEvent )
			lprintf( "Access violation - OpenGL layer at this moment.." );
	return EXCEPTION_EXECUTE_HANDLER;
	default:
		if( l.flags.bLogKeyEvent )
			lprintf( "Filter unknown : %08X", n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
	// unreachable code.
	//return EXCEPTION_CONTINUE_EXECUTION;
}
#endif
//----------------------------------------------------------------------------

static void SendApplicationDraw( PRENDERER hVideo )
{
	// if asked to paint we have definatly been shown.
#ifdef LOG_OPEN_TIMING
	lprintf( "Application should redraw... %p", hVideo );
#endif
	if( hVideo && hVideo->pRedrawCallback )
	{
		if( !hVideo->flags.bShown || hVideo->flags.bHidden )
		{
#ifdef LOG_SHOW_HIDE
			lprintf(" hidden." );
#endif
			// oh - opps, it's not allowed to draw.
			return;
		}
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( "Auto-enable window GL." );
#endif
			//if( hVideo->flags.event_dispatched )
			{
				//lprintf( "Fatality..." );
				//Return 0;
			}
			//lprintf( "Allowed to draw..." );
#ifdef _OPENGL_ENABLED
			//if( !SetActiveGLDisplay( hVideo ) )
			//{
				// if the opengl failed, dont' let the application draw.
			//	return;
			//}
#endif
		}
		hVideo->flags.event_dispatched = 1;
		//					lprintf( "Disaptched..." );
#ifdef _MSC_VER
		__try
		{
			//try
#elif 0 && defined( __WATCOMC__ )
#ifndef __cplusplus
			_try
			{
#endif
#endif
				//if( !hVideo->flags.bShown || !hVideo->flags.bLayeredWindow )
				if( !hVideo->flags.bDestroy )
				{
					//HDWP hDeferWindowPos = BeginDeferWindowPos( 1 );
#ifdef NOISY_LOGGING
					lprintf( "redraw... WM_PAINT (sendapplicationdraw)" );
					lprintf( "%p %p %p", hVideo->pRedrawCallback, hVideo->dwRedrawData, (PRENDERER) hVideo );
#endif
					hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
				}
				//catch(...)
				{
					//lprintf( "Unknown exception during Redraw Callback" );
				}
#ifdef _MSC_VER
			}
			__except( EvalExcept( GetExceptionCode() ) )
			{
				lprintf( "Caught exception in video output window" );
				;
			}
#elif 0 && defined( __WATCOMC__ )
#ifndef __cplusplus
		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
			lprintf( "Caught exception in video output window" );
			;
		}
#endif
#endif
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( "Auto disable (swap) window GL" );
#endif
#ifdef _OPENGL_ENABLED
			//SetActiveGLDisplay( NULL );
			//if( hVideo->flags.bLayeredWindow )
			//{
			//	UpdateDisplay( hVideo );
			//}
#endif
		}
		// might have 'controls' over the open...
		// these would need to be updated seperately?
		hVideo->flags.event_dispatched = 0;
		if( hVideo->flags.bShown )
		{
#ifdef NOISY_LOGGING
			lprintf( "painting... shown... %p", hVideo );
#endif
			// application should issue update display as appropriate.
			//UpdateDisplayPortion(hVideo, 0, 0, 0, 0);
		}
		//else
		//	lprintf( "Not painting... not shown yet..." );
	}
	else if( hVideo )
	{
		// default update
		UpdateDisplay( hVideo );
	}
#ifdef LOG_OPEN_TIMING
	//lprintf( "Application should have redrawn..." );
#endif
}

static void InvokeSurfaceInput( int nPoints, PINPUT_POINT points )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( "sack/render/surface input/SurfaceInput", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(int,PINPUT_POINT);
		{
			PCLASSROOT root = GetClassRootEx( data, name );
			CTEXTSTR file = GetRegisteredValue( (CTEXTSTR)root, "Source File" );
			int line = (int)(uintptr_t)GetRegisteredValueEx( (CTEXTSTR)root, "Source Line", TRUE );
			lprintf( "Surface input event handler %s at %s(%d)", name, file, line );
		}

		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (int,PINPUT_POINT) );
		if( f )
			f( nPoints, points );
	}
}

static void InvokeSimpleSurfaceInput( int32_t x, int32_t y, int new_touch, int last_touch )
{
	struct input_point point;
	point.x = x;
	point.y = y;
	point.flags.new_event = new_touch;
	point.flags.end_event = last_touch;
	InvokeSurfaceInput( 1, &point );
}

static void InvokeBeginShutdown( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;

	for( name = GetFirstRegisteredName( "SACK/System/Begin Shutdown", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(void);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
}

int IsVidThread( void )
{
	// used by opengl to allow selecting context.
	if( IsThisThread( l.actual_thread ) )\
		return TRUE;
	return FALSE;
}

void Redraw( PRENDERER hVideo )
{
	if( hVideo )
	{
		if( IsThisThread( hVideo->pThreadWnd ) )
			//if( IsVidThread() )
		{
#ifdef LOG_RECT_UPDATE
			lprintf( "..." );
#endif
			SendApplicationDraw( hVideo );
		}
		else
		{
			if( l.flags.bLogWrites )
				lprintf( "Posting invalidate rect..." );
#if DEBUG_INVALIDATE
			lprintf( "set Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
			AddLink( &l.invalidated_windows, hVideo );
			InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
		}
	}
}

static uintptr_t CPROC DoExit( PTHREAD thread )
{
	BAG_Exit( (int)GetThreadParam( thread ) );
	return 0;
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
uintptr_t CPROC VideoThreadProc (PTHREAD thread)
{
#ifdef LOG_STARTUP
	Log( "Video thread..." );
#endif
	if (l.bThreadRunning)
	{
#ifdef LOG_STARTUP
		Log( "Already exists - leaving." );
#endif
		return 0;
	}
#ifdef LOG_STARTUP
	lprintf( "Video Thread Proc %x, adding hook and thread.", GetCurrentThreadId() );
#endif
	{
		// creat the thread's message queue so that when we set
		// dwthread, it's going to be a valid target for
		// setwindowshookex
		MSG msg;
#ifdef LOG_STARTUP
		Log( "reading a message to create a message queue" );
#endif
		PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	}
	l.actual_thread = thread;
	l.dwThreadID = GetCurrentThreadId ();
	//l.pid = (uint32_t)l.dwThreadID;
	l.bThreadRunning = TRUE;
	AddIdleProc( (int(CPROC*)(uintptr_t))ProcessDisplayMessages, 0 );
	//AddIdleProc ( ProcessClientMessages, 0);
#ifdef LOG_STARTUP
	Log( "Registered Idle, and starting message loop" );
#endif
	{
		MSG Msg;
		while( !l.bExitThread && GetMessage (&Msg, NULL, 0, 0) )
		{
			HandleMessage (Msg);
		}
	}
	l.bThreadRunning = FALSE;
	//lprintf( "Video Exited volentarily" );
	//ExitThread( 0 );
	return 0;
}

//----------------------------------------------------------------------------

RENDER_PROC (int, InitDisplay) (void)
{
	return TRUE;
}

//----------------------------------------------------------------------------

RENDER_PROC (const TEXTCHAR *, GetKeyText) (int key)
{
	int c;
	WCHAR wch[15];
	static char *ch;
#ifdef UNICODE
	static wchar_t *out;
	Deallocate( wchar_t *, out );
#endif

	//if( key & KEY_MOD_DOWN )
	//return 0;
	key ^= 0x80000000;
	//LogBinary( l.kbd.key, 256 );
	c =  
#ifndef UNDER_CE
		ToUnicode (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
					l.kbd.key,  wch, 15, 0);
	if( c > 0 )
	{
		if( ch )
			Deallocate( char *, ch );
		wch[c] = 0;
		ch = WcharConvertExx( wch, c DBG_SRC );
	}
	else
		return 0;
		//ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
		//			l.kbd.key, (unsigned short *) ch, 0);
#else
		key;
#endif
	if (!c)
	{
		// check prior key bindings...
		lprintf( "no translation\n" );
		return 0;
	}
	//printf( "Key Translated: %d(%c)\n", ch[0], ch[0] );
#ifdef UNICODE
	out = DupCStr( ch );
	return out;
#else
	return ch;
#endif
}

void CPROC Vidlib_SetDisplayFullScreen( PRENDERER hVideo, int target_display )
{
}

static int CPROC DefaultMouse( PRENDERER r, int32_t x, int32_t y, uint32_t b )
{
	static int l_mouse_b;
	static uint32_t mouse_first_click_tick_changed;
	uint32_t tick = timeGetTime();
	//lprintf( "Default mouse on %p  %d,%d %08x", r, x, y, b );
	if( !l.flags.bDisableAutoDoubleClickFullScreen )
	{
	if( r->flags.bFullScreen )
	{
		static uint32_t mouse_first_click_tick;
		if( MAKE_FIRSTBUTTON( b, l_mouse_b ) )
		{
			if( !mouse_first_click_tick )
				mouse_first_click_tick = timeGetTime();
			else
			{
				static int moving;
				if( moving )
					return 1;
				moving = 1;
				if( ( tick - mouse_first_click_tick_changed ) > 500 ) 
				{
					if( ( tick - mouse_first_click_tick ) > 500 )
						mouse_first_click_tick = tick;
					else
					{
						//EnterCriticalSec( &l.cs_update );
						if( !r->flags.bNotFullScreen )
						{
							r->flags.bNotFullScreen = 1;
							mouse_first_click_tick_changed = tick;
							MoveSizeDisplay( r, r->pImage->x, r->pImage->y, r->pImage->width, r->pImage->height );
							//AndroidANW_UpdateDisplayPortionEx( NULL, 0, 0, l.default_display_x, l.default_display_y DBG_SRC );
						}
						else
						{
							r->flags.bNotFullScreen = 0;
							mouse_first_click_tick_changed = tick;
							GetDisplaySizeEx( r->full_screen.target_display, &r->full_screen.x, &r->full_screen.y
									, &r->full_screen.width, &r->full_screen.height );
							{
								int w =  r->pImage->width * r->full_screen.width / r->pImage->width;
								int h =  r->pImage->height * r->full_screen.width / r->pImage->width;
								if( SUS_GT( h, int, r->full_screen.height, uint32_t ) )
								{
									w =  r->pImage->width * r->full_screen.height / r->pImage->height;
									h =  r->pImage->height * r->full_screen.height / r->pImage->height;
								}
								if( l.flags.bDoNotPreserveAspectOnFullScreen )
									MoveSizeDisplay( r
										, r->full_screen.x// + (( r->full_screen.width - w ) / 2)
										, r->full_screen.y// + (( r->full_screen.height - h ) / 2)
													, r->full_screen.width, r->full_screen.height );
								else
									MoveSizeDisplay( r
										, r->full_screen.x + (( r->full_screen.width - w ) / 2)
										, r->full_screen.y + (( r->full_screen.height - h ) / 2)
										, w, h );
							}
						}
						//LeaveCriticalSec( &l.cs_update );
						//AndroidANW_Redraw( (PRENDERER)r );
					}
				}
				moving = 0;
			}
		}
	}
	if( ( tick - mouse_first_click_tick_changed ) > 500 ) 
	{
		if( !r->flags.bFullScreen || r->flags.bNotFullScreen )
		{
			static int l_lock_x;
			static int l_lock_y;
			if( MAKE_FIRSTBUTTON( b, l_mouse_b ) )
			{
				l_lock_x = x;
				l_lock_y = y;
			}
			else if( MAKE_SOMEBUTTONS( b )  )
			{
				// this function sets the image.x and image.y so it can retain
				// the last position of non-fullscreen...
				MoveDisplayRel( r, x - l_lock_x, y - l_lock_y );
			}
		}
	}
	}
	l_mouse_b = b;
	return 1;
}
//----------------------------------------------------------------------------

static LOGICAL DoOpenDisplay( PRENDERER hNextVideo )
{
	InitDisplay ();
	if (!l.flags.bThreadCreated && !hNextVideo->hWndContainer)
	{
		int failcount = 0;
		l.flags.bThreadCreated = 1;
		//Log( "Starting video thread..." );
		AddLink( &l.threads, ThreadTo (VideoThreadProc, 0) );
#ifdef LOG_STARTUP
		Log( "Started video thread..." );
#endif
		{
#ifdef __cplusplus_clr
			HHOOK added = NULL;
		do
		{
#endif
			failcount++;
			do
			{
				Sleep (0);
			}
			while (!l.bThreadRunning);
#ifndef __NO_WIN32API__
#ifdef __cplusplus_clr
// the problem with this code is... the .NET library in all of its wisdom created its window
// on the thred here.  and I'm making that window mine... so I have to consider that this thread
// might be waiting and have to dispatch a windows message.

// normal applications Just fall asleeep... some tests and quick forms
// they stay awake waiting on event... and this causes 100% cpu usage to check messages
// that aren't going to come in anyway.
			AddLink( &l.threads, MakeThread() );
			// add a secondidle proc so a second pass to GetMessage can be called.
			AddIdleProc( (int(CPROC*)(uintptr_t))ProcessDisplayMessages, 0 );
#ifdef USE_KEYHOOK

			if( l.flags.bUseLLKeyhook )
				AddLink( &l.ll_keyhooks, added =
						  SetWindowsHookEx (WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
												 ,GetModuleHandle(TARGETNAME), 0/*GetCurrentThreadId()*/
												 ) );
			else
				AddLink( &l.keyhooks,
						  added = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyHook
														  , NULL /*GetModuleHandle(TARGETNAME)*/, GetCurrentThreadId()
														  )
						 );
#endif
#endif
#endif
#ifdef __cplusplus_clr
		} while( !added && (failcount < 100) );
#endif
		}
#ifdef USE_XP_RAW_INPUT
		{
			RAWINPUTDEVICE Rid[2];

			Rid[0].usUsagePage = 0x01;
			Rid[0].usUsage = 0x02;
			Rid[0].dwFlags = 0 /*RIDEV_NOLEGACY*/;	// adds HID mouse and also ignores legacy mouse messages

			Rid[1].usUsagePage = 0x01;
			Rid[1].usUsage = 0x06;
			Rid[1].dwFlags = 0 /*RIDEV_NOLEGACY*/;	// adds HID keyboard and also ignores legacy keyboard messages

			if (RegisterRawInputDevices(Rid, 2, sizeof (Rid [0])) == FALSE)
			{
				lprintf( "Registration failed!?" );
				//registration failed. Call GetLastError for the cause of the error
			}
		}
#endif
	}

	AddLink( &l.pActiveList, hNextVideo );
	//hNextVideo->pid = l.pid;
	hNextVideo->KeyDefs = CreateKeyBinder();
	// set a default handler
	SetMouseHandler( hNextVideo, (MouseCallback)DefaultMouse, (uintptr_t)hNextVideo );
#ifdef LOG_OPEN_TIMING
	lprintf( "Doing open of a display..." );
#endif
	if( ( GetCurrentThreadId () == l.dwThreadID ) || hNextVideo->hWndContainer )
	{
#ifdef LOG_OPEN_TIMING
		lprintf( "Allowed to create my own stuff..." );
#endif
		CreateWindowStuffSizedAt( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										 , hNextVideo->pWindowPos.cy);
	}
	else
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			d = PostThreadMessage (l.dwThreadID, WM_USER_CREATE_WINDOW, 0,
										  (LPARAM) hNextVideo);
			if (!d)
			{
				Log1( "Failed to post create new window message...%d",
						GetLastError ());
				cnt--;
			}
#ifdef LOG_STARTUP
			else
			{
				lprintf( "Posted create new window message..." );
			}
#endif
			Relinquish();
		}
		while (!d && cnt);
		if (!d)
		{
			return FALSE;
			DebugBreak ();
		}
		if( hNextVideo )
		{
			uint32_t timeout = timeGetTime() + 5000;
			hNextVideo->thread = MakeThread();
			while (!hNextVideo->flags.bReady && timeout > timeGetTime())
			{
				// need to do this on the possibility that
				// thIS thread did create this window...
				if( !Idle() )
				{
#ifdef NOISY_LOGGING
					lprintf( "Sleeping until the window is created." );
#endif
					WakeableSleep( SLEEP_FOREVER );
					//Relinquish();
				}
			}
			if( !hNextVideo->flags.bReady )
			{
				CloseDisplay( hNextVideo );
				lprintf( "Fatality.. window creation did not complete in a timely manner." );
				// hnextvideo is null anyhow, but this is explicit.
				return FALSE;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( "Resulting new window %p %d", hNextVideo, hNextVideo->hWndOutput );
#endif
	return TRUE;
}

RENDER_PROC (PRENDERER, MakeDisplayFrom) (HWND hWnd) 
{	
	PRENDERER hNextVideo;
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
	InitializeCriticalSec( &hNextVideo->cs );

#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
	hNextVideo->hWndContainer = hWnd;
	hNextVideo->flags.bFull = TRUE;
	// should check styles and set rendered according to hWnd
	hNextVideo->flags.bLayeredWindow = l.flags.bLayeredWindowDefault;
	if( DoOpenDisplay( hNextVideo ) )
		return hNextVideo;
	ReleaseEx( hNextVideo DBG_SRC );
	return NULL;
#if 0
	SetWindowLongPtr( hWnd, GWL_WNDPROC, (DWORD)VideoWindowProc );
	{
		CREATESTRUCT cs;
		cs.lpCreateParams = (void*)hNextVideo;
		SendMessage( hWnd, WM_CREATE, 0, (LPARAM)&cs );
	}
#endif
	//return hNextVideo;
}


RENDER_PROC (PRENDERER, OpenDisplaySizedAt) (uint32_t attr, uint32_t wx, uint32_t wy, int32_t x, int32_t y) // if native - we can return and let the messages dispatch...
{
	PRENDERER hNextVideo;
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
	InitializeCriticalSec( &hNextVideo->cs );
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
	if (wx == -1)
		wx = CW_USEDEFAULT;
	if (wy == -1)
		wy = CW_USEDEFAULT;
	if (x == -1)
		x = CW_USEDEFAULT;
	if (y == -1)
		y = CW_USEDEFAULT;
#ifdef UNDER_CE
	l.WindowBorder_X = 0;
	l.WindowBorder_Y = 0;
#else
	l.WindowBorder_X = GetSystemMetrics (SM_CXFRAME);
	l.WindowBorder_Y = GetSystemMetrics (SM_CYFRAME)
		+ GetSystemMetrics (SM_CYCAPTION) + GetSystemMetrics (SM_CYBORDER);
#endif
	// NOT MULTI-THREAD SAFE ANYMORE!
	//lprintf( "Hardcoded right here for FULL window surfaces, no native borders." );
	hNextVideo->flags.bFull = TRUE;
#ifndef UNDER_CE
	if( l.UpdateLayeredWindow )
		hNextVideo->flags.bLayeredWindow = (attr & DISPLAY_ATTRIBUTE_LAYERED)?1:(l.flags.bLayeredWindowDefault);
	else
#endif
		hNextVideo->flags.bLayeredWindow = 0;
	hNextVideo->flags.bNoAutoFocus = (attr & DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS)?TRUE:FALSE;
	hNextVideo->flags.bChildWindow = (attr & DISPLAY_ATTRIBUTE_CHILD)?TRUE:FALSE;
	hNextVideo->flags.bTopmost = (attr & DISPLAY_ATTRIBUTE_TOPMOST)?TRUE:FALSE;
	hNextVideo->flags.bNoMouse = (attr & DISPLAY_ATTRIBUTE_NO_MOUSE)?TRUE:FALSE;
	hNextVideo->pWindowPos.x = x;
	hNextVideo->pWindowPos.y = y;
	hNextVideo->pWindowPos.cx = wx;
	hNextVideo->pWindowPos.cy = wy;
	if( DoOpenDisplay( hNextVideo ) )
	{
		return hNextVideo;
	}
	ReleaseEx( hNextVideo DBG_SRC );
	return NULL;
}

RENDER_PROC( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse )
{
	if( hVideo ) 
	{
		if( hVideo->flags.bNoMouse != bNoMouse )
		{
			hVideo->flags.bNoMouse = bNoMouse;
#ifndef NO_MOUSE_TRANSPARENCY
			if( bNoMouse )
			{
				SetWindowLongPtr( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
				SetWindowLongPtr( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
#endif
		}

	}
}

//----------------------------------------------------------------------------

RENDER_PROC (PRENDERER, OpenDisplayAboveSizedAt) (uint32_t attr, uint32_t wx, uint32_t wy,
															  int32_t x, int32_t y, PRENDERER parent)
{
	PRENDERER newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
	if (parent)
		PutDisplayAbove (newvid, parent);
	return newvid;
}

RENDER_PROC (PRENDERER, OpenDisplayAboveUnderSizedAt) (uint32_t attr, uint32_t wx, uint32_t wy,
															  int32_t x, int32_t y, PRENDERER parent, PRENDERER barrier)
{
	PRENDERER newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
	if( barrier )
	{
		// use initial SW_RESTORE instead of SW_NORMAL
		newvid->flags.bOpenedBehind = 1;
		newvid->under = barrier;
		//if( barrier->over )
		{
			//if( barrier->over != parent )
			//	DebugBreak();
		}
		barrier->over = newvid;
		//lprintf( "Opening window behind another." );
		//lprintf( "--- before SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );
		SetWindowPos( newvid->hWndOutput, barrier->hWndOutput, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
		//lprintf( "--- after SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );
	}
	if (parent)
		PutDisplayAbove (newvid, parent);
	return newvid;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, CloseDisplay) (PRENDERER hVideo)
{
	int bEventThread;
	// just kills this video handle....
	if (!hVideo)			// must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log ("Unlinking destroyed window...");
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	hVideo->flags.bDestroy = 1;
	// this isn't the therad to worry about...
	bEventThread = ( l.dwEventThreadID == GetCurrentThreadId() );
	if (GetCurrentThreadId () == l.dwThreadID )
	{
#ifdef LOG_DESTRUCTION
		lprintf( "Scheduled for deletion" );
#endif
		HandleDestroyMessage( hVideo );
	}
	else
	{
		int d = 1;
#ifdef LOG_DESTRUCTION
		Log ("Dispatching destroy and resulting...");
#endif
		//SendServiceEvent( l.pid, WM_USER + 513, &hVideo, sizeof( hVideo ) );
		d = PostThreadMessage (l.dwThreadID, WM_USER_DESTROY_WINDOW, 0, (LPARAM) hVideo);
		if (!d)
		{
			Log ("Failed to post create new window message...");
			DebugBreak ();
		}
	}
	{
#ifdef LOG_DESTRUCTION
		int logged = 0;
#endif
		hVideo->flags.bInDestroy = 1;
#ifdef LOG_DESTRUCTION
		while (hVideo->flags.bReady && !bEventThread )
	{	
			if( !logged )
			{
				lprintf( "Wait for window to go unready." );
				logged = 1;
			}
			Idle();
			//Sleep (0);
		}
#else
		{
			uint32_t fail_time = timeGetTime() + 500;
			while (l.bThreadRunning && hVideo->flags.bReady && !bEventThread && (fail_time < timeGetTime()))
				Idle();
			if(!(fail_time < timeGetTime()))
			{
			//lprintf( "Failed to wait for close of display....(well we might have caused it somewhere in back hsack... return now k?" );
			}
		}
#endif
		hVideo->flags.bInDestroy = 0;
		// the scan of inactive windows releases the hVideo...
		AddLink( &l.pInactiveList, hVideo );
		// generate an event to dispatch pending...
		// there is a good chance that a window event caused a window
		// and it will be sleeping until the next event...
	}
	return;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SizeDisplay) (PRENDERER hVideo, uint32_t w, uint32_t h)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "Size Display..." );
#endif
	if( w == hVideo->pWindowPos.cx && h == hVideo->pWindowPos.cy )
		return;
	// if this isn't updated, this determines a forced size during changing.
   hVideo->flags.bFullScreen = FALSE;
	hVideo->pWindowPos.cx = w;
	hVideo->pWindowPos.cy = h;
	if( hVideo->flags.bLayeredWindow )
	{
		// need to remake image surface too...
		hVideo->flags.bForceSurfaceUpdate = 1;
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
					, SWP_NOMOVE|SWP_NOACTIVATE);
	}
	else
	{
		hVideo->flags.bForceSurfaceUpdate = 1;
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
						 , SWP_NOMOVE|SWP_NOACTIVATE);
	}
}


//----------------------------------------------------------------------------

RENDER_PROC (void, SizeDisplayRel) (PRENDERER hVideo, int32_t delw, int32_t delh)
{
	if (delw || delh)
	{
		int32_t cx, cy;
		cx = (hVideo->pWindowPos.cx += delw);
		cy = (hVideo->pWindowPos.cy += delh);
		if (hVideo->pWindowPos.cx < 50)
			cx = hVideo->pWindowPos.cx = 50;
		if (hVideo->pWindowPos.cy < 20)
			cy = hVideo->pWindowPos.cy = 20;
#ifdef LOG_RESIZE
		Log2 ("Resized display to %d,%d", hVideo->pWindowPos.cx,
				hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( "size display relative" );
#endif
		hVideo->flags.bForceSurfaceUpdate = 1;
		SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
						  SWP_NOZORDER | SWP_NOMOVE);
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveDisplay) (PRENDERER hVideo, int32_t x, int32_t y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "Move display %d,%d", x, y );
#endif
	stop = 1;
	if( hVideo->flags.bLayeredWindow )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
			hVideo->cursor_bias.x = x;
			hVideo->cursor_bias.y = y;
			if( hVideo->flags.bShown )
			{
				// layered window requires layered output to be called to move the display.
				UpdateDisplay( hVideo );
			}
		}
	}
	else
	{
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter, x, y, 0, 0,
						  SWP_NOZORDER | SWP_NOSIZE);
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveDisplayRel) (PRENDERER hVideo, int32_t x, int32_t y)
{
	if (x || y)
	{
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
		/* this is a specific case used by fullscreen internal-DefaultMouse*/
		hVideo->pImage->x = hVideo->pWindowPos.x;
		hVideo->pImage->y = hVideo->pWindowPos.y;
#ifdef LOG_ORDERING_REFOCUS
		lprintf( "Move display relative" );
#endif
		SetWindowPos( hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
		            , hVideo->pWindowPos.x
		            , hVideo->pWindowPos.y
		            , 0, 0
		            , SWP_NOZORDER | SWP_NOSIZE);
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplay) (PRENDERER hVideo, int32_t x, int32_t y, int32_t w,
												 int32_t h)
{
	int32_t cx, cy;
	UINT moveflags = SWP_NOZORDER|SWP_NOACTIVATE;
	if( !hVideo )
		return;
	if( hVideo->pWindowPos.x == x && hVideo->pWindowPos.y == y )
		moveflags |= SWP_NOMOVE;
	else
	{
		hVideo->pWindowPos.x = x;
		hVideo->pWindowPos.y = y;
	}
	if( hVideo->pWindowPos.cx == w && hVideo->pWindowPos.cy == h )
		moveflags |= SWP_NOSIZE;
	else
	{
		hVideo->pWindowPos.cx = w;
		hVideo->pWindowPos.cy = h;
	}
	cx = w;
	cy = h;
	if (cx < 50)
		cx = 50;
	if (cy < 20)
		cy = 20;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( "move and size display." );
#endif
	if( !( moveflags & SWP_NOSIZE ) )
		hVideo->flags.bForceSurfaceUpdate = 1;
	SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
				, hVideo->pWindowPos.x
				, hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				 , SWP_NOZORDER|SWP_NOACTIVATE|moveflags );
	if( hVideo->flags.bLayeredWindow && !(moveflags & SWP_NOSIZE ) )
	{
		Redraw( hVideo );
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplayRel) (PRENDERER hVideo, int32_t delx, int32_t dely,
													 int32_t delw, int32_t delh)
{
	int32_t cx, cy;
	UINT moveflags = 0;
	if( !( delx || dely ) )
		moveflags |= SWP_NOMOVE;
	if( !( delw || delh ) )
		moveflags |= SWP_NOSIZE;
	hVideo->pWindowPos.x += delx;
	hVideo->pWindowPos.y += dely;
	cx = (hVideo->pWindowPos.cx += delw);
	cy = (hVideo->pWindowPos.cy += delh);
	if (cx < 50)
		cx = 50;
	if (cy < 20)
		cy = 20;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( "move and size relative %d,%d %d,%d", delx, dely, delw, delh );
#endif
	hVideo->flags.bForceSurfaceUpdate = 1;
	SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
				, hVideo->pWindowPos.x
					 , hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				, SWP_NOZORDER | moveflags );
	if( hVideo->flags.bLayeredWindow && !(moveflags & SWP_NOSIZE ) )
	{
		SendApplicationDraw( hVideo );
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, UpdateDisplayEx) (PRENDERER hVideo DBG_PASS )
{
	// copy hVideo->lpBuffer to hVideo->hDCOutput
	if (hVideo && hVideo->hWndOutput && hVideo->hBm)
	{
		if( hVideo->flags.bLayeredWindow && hVideo->flags.bFullScreen  && !hVideo->flags.bNotFullScreen )
		{
			BlotScaledImage( hVideo->pImageLayeredStretch, hVideo->pImage );
			// causes color distortion.
			//StretchBlt ((HDC)hVideo->hDCBitmapFullScreen, 0, 0, hVideo->pImageLayeredStretch->width, hVideo->pImageLayeredStretch->height,
			//			(HDC)hVideo->hDCBitmap, 0, 0, hVideo->pImage->width, hVideo->pImage->height, SRCCOPY);
			UpdateDisplayPortionEx (hVideo, 0, 0, hVideo->pImageLayeredStretch->width, hVideo->pImageLayeredStretch->height DBG_RELAY);
		}
		else
			UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
	}
	return;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetMousePosition) (PRENDERER hVid, int32_t x, int32_t y)
{
	if (hVid->flags.bFull)
		SetCursorPos (x + hVid->cursor_bias.x, y + hVid->cursor_bias.y);
	else
		SetCursorPos (x + l.WindowBorder_X + hVid->cursor_bias.x,
							 y + l.WindowBorder_Y + hVid->cursor_bias.y);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetMousePosition) (int32_t * x, int32_t * y)
{
	POINT p;
	GetCursorPos (&p);
	if (x)
		(*x) = p.x;
	if (y)
		(*y) = p.y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(int32_t * x, int32_t * y, uint32_t *b)
{
	GetMousePosition( x, y );
	if( b )
		(*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetCloseHandler) (PRENDERER hVideo,
												 CloseCallback pWindowClose,
												 uintptr_t dwUser)
{
	if( hVideo )
	{
		hVideo->dwCloseData = dwUser;
		hVideo->pWindowClose = pWindowClose;
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetMouseHandler) (PRENDERER hVideo,
												 MouseCallback pMouseCallback,
												 uintptr_t dwUser)
{
	hVideo->dwMouseData = dwUser;
	hVideo->pMouseCallback = pMouseCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetHideHandler) (PRENDERER hVideo,
												 HideAndRestoreCallback pHideCallback,
												 uintptr_t dwUser)
{
	hVideo->dwHideData = dwUser;
	hVideo->pHideCallback = pHideCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRestoreHandler) (PRENDERER hVideo,
												 HideAndRestoreCallback pRestoreCallback,
												 uintptr_t dwUser)
{
	hVideo->dwRestoreData = dwUser;
	hVideo->pRestoreCallback = pRestoreCallback;
}

//----------------------------------------------------------------------------
#ifndef NO_TOUCH
RENDER_PROC (void, SetTouchHandler) (PRENDERER hVideo,
												 TouchCallback pTouchCallback,
												 uintptr_t dwUser)
{
	hVideo->dwTouchData = dwUser;
	hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRedrawHandler) (PRENDERER hVideo,
												  RedrawCallback pRedrawCallback,
												  uintptr_t dwUser)
{
	hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		if( hVideo->flags.bShown )
			Redraw( hVideo );
	}

}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetKeyboardHandler) (PRENDERER hVideo, KeyProc pKeyProc,
													 uintptr_t dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetLoseFocusHandler) (PRENDERER hVideo,
													  LoseFocusCallback pLoseFocus,
													  uintptr_t dwUser)
{
	hVideo->dwLoseFocus = dwUser;
	hVideo->pLoseFocus = pLoseFocus;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetApplicationTitle) (const TEXTCHAR *pTitle)
{
	l.gpTitle = pTitle;
	if (l.hWndInstance)
	{
		//DebugBreak();
		SetWindowText ((HWND)l.hWndInstance, l.gpTitle);
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetRendererTitle) (PRENDERER hVideo, const TEXTCHAR *pTitle)
{
	//l.gpTitle = pTitle;
	//if (l.hWndInstance)
	{
		if( hVideo->pTitle )
			ReleaseEx( hVideo->pTitle DBG_SRC );
		hVideo->pTitle = StrDupEx( pTitle DBG_SRC );
		SetWindowText( hVideo->hWndOutput, pTitle );
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetApplicationIcon) (ImageFile * hIcon)
{
#ifdef _WIN32
	//HICON hIcon = CreateIcon();
#endif
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MakeTopmost) (PRENDERER hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		if( l.flags.bLogWrites )
			lprintf( "Make Topmost... (set window style)" );
		//SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TOPMOST );
		if( hVideo->flags.bShown )
		{
			//lprintf( "Forcing topmost" );
			if( l.flags.bLogWrites )
				lprintf( "Was shown already, so set TOPMOST stack... (set window style)" );
			SetWindowPos( hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							  SWP_NOMOVE | SWP_NOSIZE);
		}
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MakeAbsoluteTopmost) (PRENDERER hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		hVideo->flags.bAbsoluteTopmost = 1;
		//SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TOPMOST );
		hVideo->top_force_timer_id = SetTimer( hVideo->hWndOutput, 100, 100, NULL );
		if( hVideo->flags.bShown )
		{
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							  SWP_NOMOVE | SWP_NOSIZE);
		}
	}
}

//----------------------------------------------------------------------------

RENDER_PROC( int, IsTopmost )( PRENDERER hVideo )
{
	return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, HideDisplay) (PRENDERER hVideo)
{
#ifdef LOG_SHOW_HIDE
	if( hVideo )
		lprintf("Hiding the window! %p %p %p", hVideo->hWndOutput, hVideo->pAbove, hVideo->pBelow );
	else
		lprintf( "Someone tried to hide NULL window!" );
#endif
	if( hVideo )
	{
		//hVideo->flags.bShown = 0;
		{
			int found = 0;
			INDEX idx;
			PTHREAD thread;
			LIST_FORALL( l.threads, idx, PTHREAD, thread )
			{
				//lprintf( "Checking thread..." );
				if( ( GetCurrentThreadId () == l.dwThreadID ) && IsThisThread( thread ) )
				{
					HWND hWndActive = GetActiveWindow();
					HWND hWndFocus = GetFocus ();
					HWND hWndFore = GetForegroundWindow();
					found = 1;
					//lprintf( "..." );
					if( hVideo->hWndOutput == GetForegroundWindow() ||
						hVideo->hWndOutput == GetFocus() )
					{
						if (hVideo->pAbove)
						{
							SafeSetFocus( hVideo->pAbove->hWndOutput );
						}
						else if( hVideo->pBelow )
						{
							SafeSetFocus( hVideo->pBelow->hWndOutput );
						}
						else
						{
							SafeSetFocus( GetDesktopWindow() );
						}
					}

					lprintf(" Think this thead is a videothread?!" );
					ShowWindow (hVideo->hWndOutput, SW_HIDE);
					hVideo->flags.bHidden = TRUE;
					if( hVideo->pHideCallback )
						hVideo->pHideCallback( hVideo->dwHideData );
					if (hVideo->pAbove)
					{
						lprintf( "Focusing and activating my parent window." );
						{
							//if( hVideo->pLoseFocus )
							//	hVideo->pLoseFocus(hVideo->dwLoseFocus, hVideo->pAbove);
							//if( hVideo->pAbove->pLoseFocus )
							//	hVideo->pAbove->pLoseFocus(hVideo->pAbove->dwLoseFocus, NULL );
						}

						if( !hVideo->pAbove->flags.bNoAutoFocus )
						{
							if( hVideo->hWndOutput == GetActiveWindow() ||
								hVideo->hWndOutput == GetFocus() )
							{
								SetFocus( hVideo->pAbove->hWndOutput );
								SetActiveWindow (hVideo->pAbove->hWndOutput);
							}
						}
					}
					if (hVideo->pBelow)
					{
						{
							//if( hVideo->pLoseFocus )
							//	hVideo->pLoseFocus(hVideo->dwLoseFocus, hVideo->pBelow);
							//if( hVideo->pBelow->pLoseFocus )
							//	hVideo->pBelow->pLoseFocus(hVideo->pBelow->dwLoseFocus, NULL );
						}
						SetFocus( hVideo->pBelow->hWndOutput );
						SetActiveWindow (hVideo->pBelow->hWndOutput);
					}
				}
			}
			if( !found )
			{
#ifndef UNDER_CE
				//lprintf( "Async HIDE." );
				ShowWindowAsync( hVideo->hWndOutput, SW_HIDE );
#else
				PostThreadMessage (l.dwThreadID, WM_USER_HIDE_WINDOW, 0, (LPARAM)hVideo );
#endif
			}
		}
		{
			uint32_t shortwait = timeGetTime() + 250;
			while( !hVideo->flags.bHidden && shortwait > timeGetTime() )
			{
				IdleFor( 10 );
			}
			if( !hVideo->flags.bHidden )
			{
				lprintf( "window did not hide." );
			}
		}
	}
}

//----------------------------------------------------------------------------
#undef RestoreDisplay
void RestoreDisplay( PRENDERER hVideo  )
{
	RestoreDisplayEx( hVideo DBG_SRC );
}


void RestoreDisplayEx(PRENDERER hVideo DBG_PASS )
{
#ifdef LOG_SHOW_HIDE
	_lprintf(DBG_RELAY)( "Restore display. %p", hVideo->hWndOutput );
#endif
	if( hVideo )
	{
		if( hVideo->flags.bHidden )
		{
			hVideo->flags.bHidden = 0;
			if( hVideo->pRestoreCallback )
				hVideo->pRestoreCallback( hVideo->dwRestoreData );
		}
		{
			INDEX idx;
			LOGICAL isthread = FALSE;
			PTHREAD thread;
			LIST_FORALL( l.threads, idx, PTHREAD, thread )
				if( IsThisThread( thread ) )
				{
					isthread = TRUE;
					break;
				}
			{
				if( ( GetCurrentThreadId () != l.dwThreadID ) && !isthread )
				{
#if defined( OTHER_EVENTS_HERE )
					if( l.flags.bLogMessages )
						lprintf( "Sending SHOW_WINDOW to window thread.. %p", hVideo  );
#endif
					PostThreadMessage (l.dwThreadID, WM_USER_SHOW_WINDOW, 0, (LPARAM)hVideo );
				}
				else
				{
#if defined( OTHER_EVENTS_HERE )
					if( l.flags.bLogMessages )
						lprintf( "Doing the show window." );
#endif
					if( hVideo->flags.bShown )
					{
#if defined( OTHER_EVENTS_HERE )
						if( l.flags.bLogMessages )
							lprintf( "window was shown, use restore." );
#endif
						hVideo->flags.bRestoring = 1;
						ShowWindow( hVideo->hWndOutput, SW_RESTORE );
						hVideo->flags.bRestoring = 0;
					}
					else
					{
						hVideo->flags.bShown = 1;
#if defined( OTHER_EVENTS_HERE )
						if( l.flags.bLogMessages )
							lprintf( "Generating initial show (restore display, never shown)" );
#endif
#if DEBUG_INVALIDATE
						lprintf( "set Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
						AddLink( &l.invalidated_windows, hVideo );
						ShowWindow( hVideo->hWndOutput, SW_SHOW );
					}
					if( hVideo->flags.bTopmost )
					{
#if defined( OTHER_EVENTS_HERE )
						if( l.flags.bLogMessages )
							lprintf( "Setting possition topmost..." );
#endif
						SetWindowPos( hVideo->hWndOutput
										 , HWND_TOPMOST
										 , 0, 0, 0, 0,
										  SWP_NOMOVE
										  | SWP_NOSIZE
										);
					}
				}
			}
		}
	}

}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetDisplaySizeEx) ( int nDisplay
												 , int32_t *x, int32_t *y
												 , uint32_t *width, uint32_t *height)
{
#ifndef NO_ENUM_DISPLAY
		if( nDisplay > 0 )
		{
			TEXTSTR teststring = NewArray( TEXTCHAR, 20 );
			//int idx;
			int v_test = 0;
			int i;
			int found = 0;
			DISPLAY_DEVICE dev;
			DEVMODE dm;
			if( x )
				(*x) = 256;
			if( y )
				(*y) = 256;
			if( width )
				(*width) = 720;
			if( height )
				(*height) = 540;
			dm.dmSize = sizeof( DEVMODE );
			dev.cb = sizeof( DISPLAY_DEVICE );
			for( v_test = 0; !found && ( v_test < 2 ); v_test++ )
			{
				// go ahead and try to find V devices too... not sure what they are, but probably won't get to use them.
				tnprintf( teststring, 20, "\\\\.\\DISPLAY%s%d", (v_test==1)?"V":"", nDisplay );
				for( i = 0;
					 !found && EnumDisplayDevices( NULL // all devices
														  , i
														  , &dev
														  , 0 // dwFlags
														  ); i++ )
				{
					if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
					{
						if( l.flags.bLogDisplayEnumTest )
							lprintf( "display %s is at %d,%d %dx%d", dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					//else
					//	lprintf( "Found display name, but enum current settings failed? %s %d", teststring, GetLastError() );
					if( StrCaseCmp( teststring, dev.DeviceName ) == 0 )
					{
						if( l.flags.bLogDisplayEnumTest )
							lprintf( "[%s] might be [%s]", teststring, dev.DeviceName );
						if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
						{
							if( x )
								(*x) = dm.dmPosition.x;
							if( y )
								(*y) = dm.dmPosition.y;
							if( width )
								(*width) = dm.dmPelsWidth;
							if( height )
								(*height) = dm.dmPelsHeight;
							found = 1;
							break;
						}
						else
							lprintf( "Found display name, but enum current settings failed? %s %d", teststring, GetLastError() );
					}
					else
					{
						//lprintf( "[%s] is not [%s]", teststring, dev.DeviceName );
					}
				}
			}
		}
		else
#endif
		{
			if( x )
				(*x)= 0;
			if( y )
				(*y)= 0;
			GetDisplaySize( width, height );
		}

}

RENDER_PROC (void, GetDisplaySize) (uint32_t * width, uint32_t * height)
{
	RECT r;
	GetWindowRect (GetDesktopWindow (), &r);
	//Log4( "Desktop rect is: %d, %d, %d, %d", r.left, r.right, r.top, r.bottom );
	if (width)
		*width = r.right - r.left;
	if (height)
		*height = r.bottom - r.top;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetDisplayPosition) (PRENDERER hVid, int32_t * x, int32_t * y,
													 uint32_t * width, uint32_t * height)
{
	if (!hVid)
		return;
	if (width)
		*width = hVid->pWindowPos.cx;
	if (height)
		*height = hVid->pWindowPos.cy;
#ifndef NO_ENUM_DISPLAY
	{
		int posx = 0;
		int posy = 0;
		{
			WINDOWINFO wi;
			wi.cbSize = sizeof( wi);
			
			GetWindowInfo( hVid->hWndOutput, &wi ); 
			posx += wi.rcClient.left;
			posy += wi.rcClient.top;
		}
		if (x)
			*x = posx;
		if (y)
			*y = posy;
	}
#endif
}

//----------------------------------------------------------------------------
RENDER_PROC (LOGICAL, DisplayIsValid) (PRENDERER hVid)
{
	if( hVid )
		return hVid->flags.bReady;
	return FALSE;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetDisplaySize) (uint32_t width, uint32_t height)
{
	SizeDisplay (l.hVideoPool, width, height);
}

//----------------------------------------------------------------------------

RENDER_PROC (ImageFile *,GetDisplayImage )(PRENDERER hVideo)
{
	return hVideo->pImage;
}

//----------------------------------------------------------------------------

RENDER_PROC (PKEYBOARD, GetDisplayKeyboard) (PRENDERER hVideo)
{
	return &hVideo->kbd;
}

//----------------------------------------------------------------------------

RENDER_PROC (LOGICAL, HasFocus) (PRENDERER hVideo)
{
	return hVideo->flags.bFocused;
}

//----------------------------------------------------------------------------

#if ACTIVE_MESSAGE_IMPLEMENTED
RENDER_PROC (int, SendActiveMessage) (PRENDERER dest, PACTIVEMESSAGE msg)
{
	return 0;
}

RENDER_PROC (PACTIVEMESSAGE, CreateActiveMessage) (int ID, int size,...)
{
	return NULL;
}

RENDER_PROC (void, SetDefaultHandler) (PRENDERER hVideo,
													GeneralCallback general, uintptr_t psv)
{
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (void, OwnMouseEx) (PRENDERER hVideo, uint32_t own DBG_PASS)
{
	if (own)
	{
		//lprintf( "Capture is set on %p",hVideo );
		if( !l.hCaptured )
		{
			l.hCaptured = hVideo;
			hVideo->flags.bCaptured = 1;
			SetCapture (hVideo->hWndOutput);
		}
		else
		{
			if( l.hCaptured != hVideo )
			{
				lprintf( "Another window now wants to capture the mouse... the prior window will ahve the capture stolen." );
				l.hCaptured = hVideo;
				hVideo->flags.bCaptured = 1;
				SetCapture (hVideo->hWndOutput);
			}
			else
			{
				if( !hVideo->flags.bCaptured )
				{
					lprintf( "This should NEVER happen!" );
					*(int*)0 = 0;
				}
				// should already have the capture...
			}
		}
	}
	else
	{
		if( l.hCaptured == hVideo )
		{
			//lprintf( "No more capture." );
			//ReleaseCapture ();
			hVideo->flags.bCaptured = 0;
			l.hCapturedPrior = NULL;
			l.hCaptured = NULL;
		}
	}
}

//----------------------------------------------------------------------------
void
NoProc (void)
{
	// empty do nothing prodecudure for unimplemented features
}

//----------------------------------------------------------------------------
#undef GetNativeHandle
	RENDER_PROC (HWND, GetNativeHandle) (PRENDERER hVideo)
	{
		return hVideo->hWndOutput;
	}

RENDER_PROC (int, BeginCalibration) (uint32_t nPoints)
{
	return 1;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, SyncRender)( PRENDERER hVideo )
{
	// sync has no consequence...
	return;
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayFocus )( PRENDERER pRender )
{
	//SetActiveWindow( GetParent( pRender->hWndOutput ) );
	//SetForegroundWindow( GetParent( pRender->hWndOutput ) );
	//SetFocus( GetParent( pRender->hWndOutput ) );
	//lprintf( "... 3 step?" );
	//SetActiveWindow( pRender->hWndOutput );
	//SetForegroundWindow( pRender->hWndOutput );
	if( pRender )
		SafeSetFocus( pRender->hWndOutput );
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayFront )( PRENDERER pRender )
{
	//lprintf( "Forcing display front.." );
	if( !SetWindowPos( pRender->hWndOutput, pRender->flags.bTopmost?HWND_TOPMOST:HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE ) )
		lprintf( "Window Pos set failed: %d", GetLastError() );
	//if( !SetActiveWindow( pRender->hWndOutput ) )
	//	lprintf( "active window failed: %d", GetLastError() );

	//if( !SetForegroundWindow( pRender->hWndOutput ) )
	//	lprintf( "okay well foreground failed...?" );
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayBack )( PRENDERER pRender )
{
	// uhmm...
	//lprintf( "Force display backward." );
	SetWindowPos( pRender->hWndOutput, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
RENDER_PROC (void, UpdateDisplay) (PRENDERER hVideo )
{
	//DebugBreak();
	UpdateDisplayEx( hVideo DBG_SRC );
}

RENDER_PROC (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable )
{
	// this whole thing needs to inherit some intelligence.
	if( hVideo->flags.bIdleMouse != bEnable )
	{
		if( bEnable )
		{
			if( !hVideo->idle_timer_id )
				hVideo->idle_timer_id = SetTimer( hVideo->hWndOutput, 100, 100, NULL );
			l.last_mouse_update = timeGetTime(); // prime the hider.
			hVideo->flags.bIdleMouse = bEnable;
		}
		else // disabling...
		{
			if( l.last_mouse_update_display == hVideo )
				l.last_mouse_update = 0;
			hVideo->flags.bIdleMouse = bEnable;
			if( !l.flags.mouse_on )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "Mouse was off... want it on..." );
#endif
				SendMessage( hVideo->hWndOutput, WM_USER_MOUSE_CHANGE, 0, 0 );
			}
			if( hVideo->idle_timer_id )
			{
				KillTimer( hVideo->hWndOutput, hVideo->idle_timer_id );
				hVideo->idle_timer_id = 0;
			}
		}
	}
}


RENDER_PROC( void, SetDisplayFade )( PRENDERER hVideo, int level )
{
	if( hVideo )
	{
		if( level < 0 )
			level = 0;
		if( level > 254 )
			level = 254;
		hVideo->fade_alpha = 255 - level;
		if( l.flags.bLogWrites )
			lprintf( "Output fade %d %p", hVideo->fade_alpha, hVideo->hWndOutput );
#ifndef UNDER_CE
		IssueUpdateLayeredEx( hVideo, FALSE, 0, 0, 0, 0 DBG_SRC );
#endif
	}
}

static void CPROC SetClipboardEventCallback( PRENDERER pRenderer, ClipboardCallback callback, uintptr_t psv )
{
	if( pRenderer ) {
		pRenderer->pWindowClipboardEvent = callback;
		pRenderer->dwClipboardEventData = psv;
		SetClipboardViewer( pRenderer->hWndOutput );
	}
}

void CPROC Vidlib_SuspendSystemSleep( int suspend )
{
#if WIN32
	if( suspend )
		SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS );
	else
		SetThreadExecutionState( ES_USER_PRESENT | ES_CONTINUOUS );
#endif
}



LOGICAL RequiresDrawAll ( void )
{
	return FALSE;
}

static LOGICAL CPROC AllowsAnyThreadToUpdate( void )
{
	return FALSE;
}

void MarkDisplayUpdated( PRENDERER renerer )
{

}

LOGICAL IsDisplayRedrawForced( PRENDERER renderer )
{
	if( renderer )
		return renderer->flags.bForceSurfaceUpdate;
	return FALSE;
}

void CPROC SetDisplayCursor( CTEXTSTR nCursor )
{
	if( l.old_cursor != nCursor )
		l.new_cursor = nCursor;
}

#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay

													, SetApplicationTitle
													, (void (CPROC*)(Image)) SetApplicationIcon
													, GetDisplaySize
													, SetDisplaySize
													, (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t)) OpenDisplaySizedAt
													, (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t, PRENDERER)) OpenDisplayAboveSizedAt
													, (void (CPROC*)(PRENDERER)) CloseDisplay
													, (void (CPROC*)(PRENDERER, int32_t, int32_t, uint32_t, uint32_t DBG_PASS)) UpdateDisplayPortionEx
													, (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
													, GetDisplayPosition
													, (void (CPROC*)(PRENDERER, int32_t, int32_t)) MoveDisplay
													, (void (CPROC*)(PRENDERER, int32_t, int32_t)) MoveDisplayRel
													, (void (CPROC*)(PRENDERER, uint32_t, uint32_t)) SizeDisplay
													, (void (CPROC*)(PRENDERER, int32_t, int32_t)) SizeDisplayRel
													, MoveSizeDisplayRel
													, (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
													, (Image (CPROC*)(PRENDERER)) GetDisplayImage
													, SetCloseHandler
													, SetMouseHandler
													, SetRedrawHandler
													, (void (CPROC*)(PRENDERER, KeyProc, uintptr_t)) SetKeyboardHandler
													,  SetLoseFocusHandler
													, NULL
													, (void (CPROC*)(int32_t *, int32_t *)) GetMousePosition
													, (void (CPROC*)(PRENDERER, int32_t, int32_t)) SetMousePosition
													, HasFocus  // has focus
													, GetKeyText
													, IsKeyDown
													, KeyDown
													, DisplayIsValid
													, OwnMouseEx
													, BeginCalibration
													, SyncRender	// sync
													, MoveSizeDisplay
													, MakeTopmost
													, HideDisplay
													, RestoreDisplay
													, ForceDisplayFocus
													, ForceDisplayFront
													, ForceDisplayBack
													, BindEventToKey
													, UnbindKey
													, IsTopmost
													, NULL // OkaySyncRender is internal.
													, IsTouchDisplay
													, GetMouseState
													, EnableSpriteMethod
													, WinShell_AcceptDroppedFiles
													, PutDisplayIn
													, MakeDisplayFrom
													, SetRendererTitle
													, DisableMouseOnIdle
													, OpenDisplayAboveUnderSizedAt
													, SetDisplayNoMouse
													, Redraw
													, MakeAbsoluteTopmost
													, SetDisplayFade
													, IsDisplayHidden
													, GetNativeHandle
													, GetDisplaySizeEx
													, LockRenderer
													, UnlockRenderer
													, IssueUpdateLayeredEx
													, RequiresDrawAll
#ifndef NO_TOUCH
													, SetTouchHandler
#endif
													, MarkDisplayUpdated
													, SetHideHandler
													, SetRestoreHandler
													, RestoreDisplayEx
													, NULL // show input device
													, NULL // hide input device
													, AllowsAnyThreadToUpdate
													, Vidlib_SetDisplayFullScreen
													, Vidlib_SuspendSystemSleep
													, NULL /* is instanced */
													, NULL /* render allows copy (not remote network render) */
													, SetDisplayCursor 
													, IsDisplayRedrawForced
};

#undef GetDisplayInterface
#undef DropDisplayInterface

RENDER_PROC (POINTER, GetDisplayInterface) (void)
{
	InitDisplay();
	return (POINTER)&VidInterface;
}

RENDER_PROC (void, DropDisplayInterface) (POINTER p)
{
}

static LOGICAL CPROC DefaultExit( uintptr_t psv, uint32_t keycode )
{
	lprintf( "Default Exit..." );
	ThreadTo( DoExit, 0 );
	return 1;
}

int IsTouchDisplay( void )
{
	return 0;
}

LOGICAL IsDisplayHidden( PRENDERER video )
{
	if( video )
		return video->flags.bHidden;
	return 0;
}

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 46
#endif
PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	if( l.flags.bLogRegister )
		lprintf( "Regstering video interface..." );
#ifndef UNDER_CE
	l.UpdateLayeredWindow = ( BOOL (WINAPI *)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD))LoadFunction( "user32.dll", "UpdateLayeredWindow" );
	// this is Vista+ function.
	l.UpdateLayeredWindowIndirect = ( BOOL (WINAPI *)(HWND,const UPDATELAYEREDWINDOWINFO *))LoadFunction( "user32.dll", "UpdateLayeredWindowIndirect" );
#ifndef NO_TOUCH
	l.GetTouchInputInfo = (BOOL (WINAPI *)( HTOUCHINPUT, UINT, PTOUCHINPUT, int) )LoadFunction( "user32.dll", "GetTouchInputInfo" );
	l.CloseTouchInputHandle =(BOOL (WINAPI *)( HTOUCHINPUT ))LoadFunction( "user32.dll", "CloseTouchInputHandle" );
	l.RegisterTouchWindow = (BOOL (WINAPI *)( HWND, ULONG  ))LoadFunction( "user32.dll", "RegisterTouchWindow" );
#endif
   l.pii = GetImageInterface();

#endif
	{
#ifdef SACK_BAG_EXPORTS  // symbol defined by visual studio sack_bag.vcproj
#  ifdef __cplusplus
#    define name		"sack.render++"
#  else
#    define name		"sack.render"
#  endif
#else
#  ifdef UNDER_CE
#    define name			"render"
#  else
#	 ifdef __cplusplus
#     define name		"sack.render++"
#	 else
#     define name			"sack.render"
#	 endif
#  endif
#endif
			;
		VidInterface._SetClipboardEventCallback = SetClipboardEventCallback;
		RegisterInterface( name, GetDisplayInterface, DropDisplayInterface );
		// if there hasn't been a default set already, default to this.
		// DLL this will not be set, but will end up overridden later
		// Static library, this gets set after interface.conf is read, which
      // means the alias should aready be set.
      if( !CheckClassRoot( "system/interfaces/render" ) )
			RegisterClassAlias( "system/interfaces/" name, "system/interfaces/render" );
	}
	if( SACK_GetProfileInt( "SACK/Video Render", "enable alt-f4 exit", 1 ) )
		BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	//EnableLoggingOutput( TRUE );
	VideoLoadOptions();
}


//typedef struct sprite_method_tag *PSPRITE_METHOD;

RENDER_PROC( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h ), uintptr_t psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = New( struct sprite_method_tag );
	psm->renderer = render;
	psm->original_surface = MakeImageFile( render->pImage->width, render->pImage->height );
	//psm->original_surface = MakeImageFile( render->pImage->width, render->pImage->height );
	psm->saved_spots = CreateDataQueue( sizeof( struct saved_location ) );
	psm->RenderSprites = RenderSprites;
	psm->psv = psv;
	AddLink( &render->sprites, psm );
	return psm; // the sprite should assign this...
}

// this is a magic routine, and should only be called by sprite itself
// and therefore this is handed to the image library via an export into image library
// this is done this way, because the image library MUST exist before this library
// therefore relying on the linker to handle this export is not possible.
static void CPROC SavePortion( PSPRITE_METHOD psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h )
{
	struct saved_location location;
	location.x = x;
	location.y = y;
	location.w = w;
	location.h = h;
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
	EnqueData( &psm->saved_spots, &location );
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
	/*
	BlotImageSizedEx( psm->original_surface, psm->renderer->pImage
						 , x, y
						 , x, y
						 , w, h
						 , 0
						 , BLOT_COPY );
						 */
}

PRELOAD( InitSetSavePortion ){
	SetSavePortion( SavePortion );
}

void LockRenderer( PRENDERER render )
{
	EnterCriticalSec( &render->cs );
}

void UnlockRenderer( PRENDERER render )
{
	LeaveCriticalSec( &render->cs );
}

#ifdef __cplusplus_cli
// provide a trigger point for onload code
PUBLIC( void, InvokePreloads )( void )
{
}
#endif

RENDER_NAMESPACE_END

#endif