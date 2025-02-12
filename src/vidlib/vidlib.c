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
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
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

#define WM_USER_CREATE_WINDOW  WM_USER+512
#define WM_USER_DESTROY_WINDOW  WM_USER+513
#define WM_USER_SHOW_WINDOW  WM_USER+514
#define WM_USER_HIDE_WINDOW  WM_USER+515
#define WM_USER_SHUTDOWN	  WM_USER+516
#define WM_USER_MOUSE_CHANGE	  WM_USER+517

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


LOGICAL InMyChain( PVIDEO hVideo, HWND hWnd )
{
	PVIDEO base;
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
void DumpMyChain( PVIDEO hVideo DBG_PASS )
#define DumpMyChain(h) DumpMyChain( h DBG_SRC )
{
#ifndef UNDER_CE
	PVIDEO base;
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

void DumpChainAbove( PVIDEO chain, HWND hWnd )
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

void DumpChainBelow( PVIDEO chain, HWND hWnd DBG_PASS )
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
void IssueUpdateLayeredEx( PVIDEO hVideo, LOGICAL bContent, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS )
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
RENDER_PROC (void, UpdateDisplayPortionEx)( PVIDEO hVideo
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
							int entered_crit;
							if( hVideo->flags.bOpenGL )
								if( l.actual_thread != thread )
									 continue;
							//lprintf( "Is a thread." );
							if( !hVideo->flags.event_dispatched ) {
								entered_crit = 1;
								EnterCriticalSec( &hVideo->cs );
								if( hVideo->flags.bDestroy )
								{
									//lprintf( "Saving ourselves from operating a draw while destroyed." );
									// by now we could be in a place where we've been destroyed....
									LeaveCriticalSec( &hVideo->cs );
									return;
								}
							} else
								entered_crit = 0;
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
							if( entered_crit ) {
								LeaveCriticalSec( &hVideo->cs );
							}
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
UnlinkVideo (PVIDEO hVideo)
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
FocusInLevel (PVIDEO hVideo)
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
			PVIDEO pCur = hVideo->pPrior;
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

RENDER_PROC (void, PutDisplayAbove) (PVIDEO hVideo, PVIDEO hAbove)
{
	//  this above that...
	//  this->below is now that // points at things below
	// that->above = this // points at things above
#ifdef LOG_ORDERING_REFOCUS
	PVIDEO original_below = hVideo->pBelow;
	PVIDEO original_above = hVideo->pAbove;
#endif
	PVIDEO topmost = hAbove;
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

RENDER_PROC (void, PutDisplayIn) (PVIDEO hVideo, PVIDEO hIn)
{
	lprintf( "Relate hVideo as a child of hIn..." );
}

//----------------------------------------------------------------------------


BOOL CreateDrawingSurface (PVIDEO hVideo)
{
	HBITMAP hBmNew = NULL;
	int resize_surface  = 0;
	// can use handle from memory allocation level.....
	if (!hVideo)			// wait......
		return FALSE;
	
	if( !( hVideo->flags.bFullScreen && !hVideo->flags.bNotFullScreen ) || !hVideo->pImage )
		resize_surface = 1;
	if( hVideo->flags.bLayeredWindow && hVideo->flags.bFullScreen )
	{
		resize_surface = 1;
	}
	if( resize_surface )
	{
		BITMAPINFO bmInfo;
		// the color array.
		PCOLOR pBuffer;
		RECT r;
		if (hVideo->flags.bFull)
		{
			GetWindowRect (hVideo->hWndOutput, &r);
			r.right -= r.left;
			r.bottom -= r.top;
			r.right = hVideo->pWindowPos.cx;
			r.bottom = hVideo->pWindowPos.cy;
		}
		else
		{
			GetClientRect (hVideo->hWndOutput, &r);
			r.right = hVideo->pWindowPos.cx;
			r.bottom = hVideo->pWindowPos.cy;
		}

		//lprintf( "made image %d,%d", r.right, r.bottom );
		bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth = r.right; // size of window...
		bmInfo.bmiHeader.biHeight = r.bottom;
		bmInfo.bmiHeader.biPlanes = 1;
		bmInfo.bmiHeader.biBitCount = 32;	// 24, 16, ...
		bmInfo.bmiHeader.biCompression = BI_RGB;
		bmInfo.bmiHeader.biSizeImage = 0;	// zero for BI_RGB
		bmInfo.bmiHeader.biXPelsPerMeter = 0;
		bmInfo.bmiHeader.biYPelsPerMeter = 0;
		bmInfo.bmiHeader.biClrUsed = 0;
		bmInfo.bmiHeader.biClrImportant = 0;
		if( l.flags.bLogWrites )
			lprintf( "Create new DIB section.." );
		hBmNew = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer, NULL,	// hVideo (hMemView)
											0); // offset DWORD multiple

		if (!hBmNew)
		{
			// but this will leave our prior bitmap selected... so we should be able to live after this.

			//DWORD dwError = GetLastError();
			// this is normal if window minimizes...
			if (bmInfo.bmiHeader.biWidth || bmInfo.bmiHeader.biHeight)  // both are zero on minimization
					MessageBox (hVideo->hWndOutput, "Failed to create Window DIB",
								"ERROR", MB_OK);
			return FALSE;
		}
		//lprintf( "Remake Image to %p %dx%d", pBuffer, bmInfo.bmiHeader.biWidth,
		//		  bmInfo.bmiHeader.biHeight );
		if( hVideo->flags.bLayeredWindow && hVideo->flags.bFullScreen )
		{
			HBITMAP hPriorBm;
			hVideo->pImageLayeredStretch =
				RemakeImage( hVideo->pImageLayeredStretch, pBuffer, bmInfo.bmiHeader.biWidth,
								 bmInfo.bmiHeader.biHeight);
			if (!hVideo->hDCBitmapFullScreen) 
				hVideo->hDCBitmapFullScreen = CreateCompatibleDC ((HDC)hVideo->hDCOutput);
			hPriorBm = (HBITMAP)SelectObject ((HDC)hVideo->hDCBitmapFullScreen, hBmNew);
			if (hVideo->hBmFullScreen && hVideo->hWndOutput)
			{
				// if we had an old one, we'll want to delete it.
				if (hPriorBm != hVideo->hBmFullScreen)
				{
					Log ("Hmm Somewhere we lost track of which bitmap is selected?! bitmap resource not released");
				}
				else
				{
					// delete the prior one, we have a new one.
					DeleteObject (hVideo->hBmFullScreen);
				}
			}
			else // first time through hBm will be NULL... so we save the original bitmap for the display.
				hVideo->hOldBitmapFullScreen = hPriorBm;
			// okay and now this is the bitmap to use for output
			hVideo->hBmFullScreen = hBmNew;
		}
		else
		{
			HBITMAP hPriorBm;
			hVideo->pImage =
				RemakeImage( hVideo->pImage, pBuffer, bmInfo.bmiHeader.biWidth,
								 bmInfo.bmiHeader.biHeight);
			hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER | IF_FLAG_IN_MEMORY;
			if (!hVideo->hDCBitmap) // first time ONLY...
				hVideo->hDCBitmap = CreateCompatibleDC ((HDC)hVideo->hDCOutput);
			hPriorBm = (HBITMAP)SelectObject ((HDC)hVideo->hDCBitmap, hBmNew);
			if (hVideo->hBm && hVideo->hWndOutput)
			{
				// if we had an old one, we'll want to delete it.
				if ( hPriorBm != hVideo->hBm)
				{
					Log ("Hmm Somewhere we lost track of which bitmap is selected?! bitmap resource not released");
				}
				else
				{
					// delete the prior one, we have a new one.
					DeleteObject (hVideo->hBm);
				}
			}
			else
				hVideo->hOldBitmap = hPriorBm; // original BM, don't change.
			//else // first time through hBm will be NULL... so we save the original bitmap for the display.
			//	hVideo->hOldBitmap = SelectObject ((HDC)hVideo->hDCBitmap, hBmNew);
			// okay and now this is the bitmap to use for output
			hVideo->hBm = hBmNew;
		}

	}


	if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
	{
		InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
	}
	//lprintf( "And here I might want to update the video, hope someone else does for me." );
	return TRUE;
}

void DoDestroy (PVIDEO hVideo)
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
#ifdef USE_KEYHOOK
#ifndef __NO_WIN32API__
LRESULT CALLBACK
	KeyHook (int code,		// hook code
				WPARAM wParam,	 // virtual-key code
				LPARAM lParam	  // keystroke-message information
			  )
{
	if( l.flags.bLogKeyEvent )
		lprintf( "KeyHook %d %08lx %08lx", code, wParam, lParam );
	{
		int dispatch_handled = 0;
		PVIDEO hVid;
		int key, scancode, keymod = 0;
		HWND hWndActive = GetActiveWindow ();
		HWND hWndFocus = GetFocus ();
		HWND hWndFore = GetForegroundWindow();
		ATOM aThisClass;

		if( l.flags.bLogKeyEvent )
			lprintf( "window's %p %p %p", hWndFocus, hWndFore, hWndActive );
		if( code == HC_NOREMOVE )
		{
			{
				PTHREAD thread = MakeThread();
				INDEX idx;
				PTHREAD check_thread;
				LIST_FORALL( l.threads, idx, PTHREAD, check_thread )
				{
					if( check_thread == thread )
					{
						//LRESULT result;
						return CallNextHookEx ( (HHOOK)GetLink( &l.keyhooks, idx ), code, wParam, lParam);
						
					}
					else
						if( l.flags.bLogKeyEvent )
						{
							lprintf( "skipping thread %p", check_thread );
						}
				}
			}
			return 0;
		}
		if( l.flags.bLogKeyEvent )
		{
			lprintf( "%x Received key to %p %p", GetCurrentThreadId(), hWndFocus, hWndFore );
			lprintf( "Received key %d %08x %08x", code, wParam, lParam );
		}
		aThisClass = (ATOM) GetClassLong (hWndFocus, GCW_ATOM);
		if (aThisClass != l.aClass && hWndFocus != l.hWndInstance )
		{
			//aThisClass = (ATOM) GetClassLong (hWndFore, GCW_ATOM);
			//if (aThisClass != l.aClass && hWndFocus != l.hWndInstance )
			{
				PTHREAD thread = MakeThread();
				INDEX idx;
				PTHREAD check_thread;
				LIST_FORALL( l.threads, idx, PTHREAD, check_thread )
				{
					if( check_thread == thread )
					{
						if( l.flags.bLogKeyEvent )
						{
							lprintf( "Chained to next hook..." );
						}
						return CallNextHookEx ( (HHOOK)GetLink( &l.keyhooks, idx ), code, wParam, lParam);
					}
				}
			}
		}
		if( l.flags.bLogKeyEvent )
			lprintf( "Keyhook mesasage... %08x %08x", wParam, lParam );
		//lprintf( "hWndFocus is %p", hWndFocus );
		if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			if( l.flags.bLogKeyEvent )
				lprintf( "hwndfocus is something..." );
#endif
			hVid = l.hVidFocused;
		}
		else
		{
			hVid = (PVIDEO) GetWindowLongPtr (hWndFocus, WD_HVIDEO);
			{
				INDEX idx;
				PVIDEO hVidTest;
				LIST_FORALL( l.pActiveList, idx, PVIDEO, hVidTest )
				{
					if( hVid == hVidTest )
						break;
				}
				hVid = hVidTest;
			}
			if( hVid && !hVid->flags.bReady )
			{
				DebugBreak();
				hVid = NULL;
			}
		}
		if( l.flags.bLogKeyEvent )
			lprintf( "hvid is %p", hVid );
		if(hVid)
		{
			 /*
			  // I think this is upside down...
			  struct {
			  BIT_FIELD  base  : 8;
			  BIT_FIELD  extended  : 1;
			  BIT_FIELD  nothing  : 7;
			  BIT_FIELD  scancode  : 8;
			  BIT_FIELD  morenothing  : 4;
			  BIT_FIELD  shift  : 1;
			  BIT_FIELD  control  : 1;
			  BIT_FIELD  alt  : 1;
			  BIT_FIELD  down  : 1;
			  } keycode;
			  */
			key = ( scancode = (wParam & 0xFF) ) // base keystroke
				| ((lParam & 0x1000000) >> 16)	// extended key
				| (lParam & 0x80FF0000) // repeat count? and down status
				^ (0x80000000) // not's the single top bit... becomes 'press'
				;
			// lparam MSB is keyup status (strange)

			if (key & 0x80000000)	// test keydown...
			{
				l.kbd.key[wParam & 0xFF] |= 0x80;	// set this bit (pressed)
				l.kbd.key[wParam & 0xFF] ^= 1;	// toggle this bit...
			}
			else
			{
				l.kbd.key[wParam & 0xFF] &= ~0x80;  //(unpressed)
			}
			//lprintf( "Set local keyboard %d to %d", wParam& 0xFF, l.kbd.key[wParam&0xFF]);
			if( hVid )
			{
				hVid->kbd.key[wParam & 0xFF] = l.kbd.key[wParam & 0xFF];
			}


			if( (l.kbd.key[VK_LSHIFT]|l.kbd.key[VK_RSHIFT]|l.kbd.key[KEY_SHIFT]) & 0x80)
			{
				key |= 0x10000000;
				l.mouse_b |= MK_SHIFT;
				keymod |= 1;
			}
			else
				l.mouse_b &= ~MK_SHIFT;
			if ((l.kbd.key[VK_LCONTROL]|l.kbd.key[VK_RCONTROL]|l.kbd.key[KEY_CTRL]) & 0x80)
			{
				key |= 0x20000000;
				l.mouse_b |= MK_CONTROL;
				keymod |= 2;
			}
			else
				l.mouse_b &= ~MK_CONTROL;
			if((l.kbd.key[VK_LMENU]|l.kbd.key[VK_RMENU]|l.kbd.key[KEY_ALT]) & 0x80)
			{
				key |= 0x40000000;
				l.mouse_b |= MK_ALT;
				keymod |= 4;
			}
			else
				l.mouse_b &= ~MK_ALT;

			{
				PVIDEO hVideo = hVid;
				if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				{
					if( hVideo && hVideo->pKeyProc )
					{
						hVideo->flags.event_dispatched = 1;
						if( l.flags.bLogKeyEvent )
							lprintf( "Dispatched KEY!" );
						if( hVideo->flags.key_dispatched )
						{
							if( l.flags.bLogKeyEvent )
								lprintf( "already dispatched, delay it." );
							EnqueLink( &hVideo->pInput, (POINTER)(uintptr_t)key );
						}
						else
						{
							hVideo->flags.key_dispatched = 1;
							do
							{
								if( l.flags.bLogKeyEvent )
									lprintf( "Dispatching key %08lx", key );
								if( keymod & 6 )
									if( HandleKeyEvents( KeyDefs, key )  )
									{
										lprintf( "Sent global first." );
										dispatch_handled = 1;
									}

								if( !dispatch_handled )
								{
									// previously this would then dispatch the key event...
									// but we want to give priority to handled keys.
									dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, key );
									if( l.flags.bLogKeyEvent )
										lprintf( "Result of dispatch was %ld", dispatch_handled );
									if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
										break;

									if( !dispatch_handled )
									{
										if( l.flags.bLogKeyEvent )
											lprintf( "Local Keydefs Dispatch key : %p %08lx", hVideo, key );
										if( hVideo && !( dispatch_handled = HandleKeyEvents( hVideo->KeyDefs, key ) ) )
										{
											if( l.flags.bLogKeyEvent )
												lprintf( "Global Keydefs Dispatch key : %08lx", key );
											if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
											{
												if( l.flags.bLogKeyEvent )
													lprintf( "lost window..." );
												break;
											}
										}
									}
								}
								if( !dispatch_handled )
								{
									if( !(keymod & 6) )
										dispatch_handled = HandleKeyEvents( KeyDefs, key );
								}
								if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
								{
									if( l.flags.bLogKeyEvent )
										lprintf( "lost active window." );
									break;
								}
								key = (uint32_t)(uintptr_t)DequeLink( &hVideo->pInput );
								if( l.flags.bLogKeyEvent )
									lprintf( "key from deque : %p", key );
							} while( key );
							if( l.flags.bLogKeyEvent )
								lprintf( "completed..." );
							hVideo->flags.key_dispatched = 0;
						}
						hVideo->flags.event_dispatched = 0;
					}
					else
					{
						HandleKeyEvents( KeyDefs, key ); /* global events, if no keyproc */
					}
				}
				else
				{
					if( l.flags.bLogKeyEvent )
						lprintf( "Not active window?" );
				}
			}
		}
		// do we REALLY have to call the next hook?!
		// I mean windows will just fuck us in the next layer....
		//lprintf( "%d %d", code, dispatch_handled );
		if( ( code < 0 )|| !dispatch_handled )
		{
			PTHREAD thread = MakeThread();
			INDEX idx;
			PTHREAD check_thread;
			LIST_FORALL( l.threads, idx, PTHREAD, check_thread )
			{
				if( check_thread == thread )
				{
					LRESULT result;
					if( l.flags.bLogKeyEvent )
						lprintf( "Chained to next hook...(2)" );
					result = CallNextHookEx ( (HHOOK)GetLink( &l.keyhooks, idx ), code, wParam, lParam);
					if( l.flags.bLogKeyEvent )
						lprintf( "and result is %d", result );
					return result;
				}
				//else
				//	lprintf( "skipping thread %p", check_thread );
			}
		}
	}
	//lprintf( "Finished keyhook..." );
	return 1; // stop handling - zero allows continuation...
}

LRESULT CALLBACK
	KeyHook2 (int code,		// hook code
				WPARAM wParam,	 // virtual-key code
				LPARAM lParam	  // keystroke-message information
			  )
{
	int dispatch_handled = 0;
	{
		PVIDEO hVid;
		int key, scancode, keymod = 0;
		int vkcode;
		KBDLLHOOKSTRUCT *kbhook = (KBDLLHOOKSTRUCT*)lParam;
		HWND hWndFocus = GetFocus ();
		//HWND hWndFore = GetForegroundWindow();
		//ATOM aThisClass;
		//LogBinary( kbhook, sizeof( *kbhook ) );
		//lprintf( "Received key to %p %p", hWndFocus, hWndFore );
		//lprintf( "Received key %08x %08x", wParam, lParam );

		if( l.flags.bLogKeyEvent )
			if( kbhook )
			lprintf( "KeyHook2 %d %08lx %d %d %d %d %p"
					 , code, wParam
					 , kbhook->vkCode, kbhook->scanCode, kbhook->flags, kbhook->time, kbhook->dwExtraInfo );
			else
			{
				lprintf( "kbhook data is NULL!" );
				return 0;
			}

		//lprintf( "Keyhook mesasage... %08x %08x", wParam, lParam );
		//lprintf( "hWndFocus is %p", hWndFocus );
		if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( "hwndfocus is something..." );
#endif
			hVid = l.hVidFocused;
		}
		else
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( "hVid from focus" );
#endif
			hVid = (PVIDEO) GetWindowLongPtr (hWndFocus, WD_HVIDEO);
			{
				INDEX idx;
				PVIDEO hVidTest;
				LIST_FORALL( l.pActiveList, idx, PVIDEO, hVidTest )
				{
					if( hVid == hVidTest )
						break;
				}
				hVid = hVidTest;
			}
			if( hVid && !hVid->flags.bReady )
			{
				DebugBreak();
				hVid = NULL;
			}
		}
		{
			/*
			 // I think this is upside down...
			 struct {
			 BIT_FIELD  base  : 8;
			 BIT_FIELD  extended  : 1;
			 BIT_FIELD  nothing  : 7;
			 BIT_FIELD  scancode  : 8;
			 BIT_FIELD  morenothing  : 4;
			 BIT_FIELD  shift  : 1;
			 BIT_FIELD  control  : 1;
			 BIT_FIELD  alt  : 1;
			 BIT_FIELD  down  : 1;
			 } keycode;
			 */
			vkcode = kbhook->vkCode;

			scancode = kbhook->scanCode;


			key = ( vkcode ) // base keystroke
				| (( kbhook->flags&LLKHF_EXTENDED)?0x0100:0)	// extended key
				| (scancode << 16)
				| (( kbhook->flags & LLKHF_UP )?0:0x80000000)
				;
			// lparam MSB is keyup status (strange)

			if (key & 0x80000000)	// test keydown...
			{
				if( l.flags.bLogKeyEvent )
					lprintf( "keydown" );
				l.kbd.key[vkcode] |= 0x80;	// set this bit (pressed)
				l.kbd.key[vkcode] ^= 1;	// toggle this bit...
			}
			else
			{
				if( l.flags.bLogKeyEvent )
					lprintf( "keyup" );
				l.kbd.key[vkcode] &= ~0x80;  //(unpressed)
			}

			l.kbd.key[KEY_SHIFT] = l.kbd.key[VK_RSHIFT]|l.kbd.key[VK_LSHIFT];
			l.kbd.key[KEY_CONTROL] = l.kbd.key[VK_RCONTROL]|l.kbd.key[VK_LCONTROL];
			//lprintf( "Set local keyboard %d to %d", wParam& 0xFF, l.kbd.key[wParam&0xFF]);
			if( hVid )
			{
				hVid->kbd.key[vkcode] = l.kbd.key[vkcode];
			}

			if( (l.kbd.key[VK_LSHIFT]|l.kbd.key[VK_RSHIFT]|l.kbd.key[KEY_SHIFT]) & 0x80)
			{
				key |= 0x10000000;
				l.mouse_b |= MK_SHIFT;
				keymod |= 1;
			}
			else
				l.mouse_b &= ~MK_SHIFT;
			if ((l.kbd.key[VK_LCONTROL]|l.kbd.key[VK_RCONTROL]|l.kbd.key[KEY_CTRL]) & 0x80)
			{
				key |= 0x20000000;
				l.mouse_b |= MK_CONTROL;
				keymod |= 2;
			}
			else
				l.mouse_b &= ~MK_CONTROL;
			if((l.kbd.key[VK_LMENU]|l.kbd.key[VK_RMENU]|l.kbd.key[KEY_ALT]) & 0x80)
			{

				key |= 0x40000000;
				l.mouse_b |= MK_ALT;
				keymod |= 4;
			}
			else
				l.mouse_b &= ~MK_ALT;

			if( l.flags.bLogKeyEvent )
				lprintf( "keymod is %d from (%d,%d,%d)", keymod
						 , (l.kbd.key[VK_LMENU]|l.kbd.key[VK_RMENU]|l.kbd.key[KEY_ALT])
						 , (l.kbd.key[VK_LCONTROL]|l.kbd.key[VK_RCONTROL]|l.kbd.key[KEY_CTRL])
						 , (l.kbd.key[VK_LSHIFT]|l.kbd.key[VK_RSHIFT]|l.kbd.key[KEY_SHIFT])
						 );
		}

		//lprintf( "hvid is %p", hVid );
 		if(hVid)
		{
			{
				PVIDEO hVideo = hVid;
				//lprintf( "..." );
				if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
					if( hVideo && hVideo->pKeyProc )
					{
						hVideo->flags.event_dispatched = 1;
						//lprintf( "Dispatched KEY!" );
						if( hVideo->flags.key_dispatched )
							EnqueLink( &hVideo->pInput, (POINTER)(uintptr_t)key );
						else
						{
							hVideo->flags.key_dispatched = 1;
							do
							{
								if( (keymod & 6) )
								{
									dispatch_handled = HandleKeyEvents( KeyDefs, key );
								}
								if( !dispatch_handled )
								{
									if( l.flags.bLogKeyEvent )
										lprintf( "Dispatching key %08lx", key );
									dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, key );
								}
								if( l.flags.bLogKeyEvent )
									lprintf( "Result of dispatch was %ld", dispatch_handled );
								if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
									break;

								if( !dispatch_handled )
								{
									if( l.flags.bLogKeyEvent )
										lprintf( "Local Keydefs Dispatch key : %p %08lx", hVideo, key );
									if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, key ) )
									{
										if( l.flags.bLogKeyEvent )
											lprintf( "Global Keydefs Dispatch key : %08lx", key );
										if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
											break;

										// if had ctrl or alt, keymod will be 2 or 4... and dispatched above
										if( !(keymod & 6) )
											if( !HandleKeyEvents( KeyDefs, key ) )
											{
												// previously this would then dispatch the key event...
												// but we want to give priority to handled keys.
											}
									}
								}
								if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
									break;
								key = (uint32_t)(uintptr_t)DequeLink( &hVideo->pInput );
							} while( key );
							hVideo->flags.key_dispatched = 0;
						}
						hVideo->flags.event_dispatched = 0;
					}
					else
					{
						//lprintf( "calling global events." );
						dispatch_handled = HandleKeyEvents( KeyDefs, key ); /* global events, if no keyproc */
					}
				//else
				//	lprintf( "Failed to find active window..." );
			}
		}
		else
		{
			dispatch_handled = HandleKeyEvents( KeyDefs, key );
		}
		//lprintf( "code:%d handled:%d", code, dispatch_handled );
		// do we REALLY have to call the next hook?!
		// I mean windows will just fuck us in the next layer....
		if( ( code < 0 )|| !dispatch_handled )
		{
			PTHREAD thread = MakeThread();
			INDEX idx;
			PTHREAD check_thread;
			LIST_FORALL( l.threads, idx, PTHREAD, check_thread )
			{
				if( check_thread == thread )
				{
					if( l.flags.bLogKeyEvent )
						lprintf( "Chained to next hook... %08x %08x", wParam, lParam );
					return CallNextHookEx ( (HHOOK)GetLink( &l.ll_keyhooks, idx ), code, wParam, lParam);
				}
			}
		}
	}
	return dispatch_handled; // stop handling - zero allows continuation...
}

#endif
#endif
//----------------------------------------------------------------------------

HWND MoveWindowStack( PVIDEO hInChain, HWND hwndInsertAfter, int use_under )
{
	HWND result_after = hwndInsertAfter;
	HDWP hWinPosInfo = BeginDeferWindowPos( 1 );
	PVIDEO current;
	PVIDEO save_current;
	PVIDEO check;
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

static void SendApplicationDraw( PVIDEO hVideo )
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

void Redraw( PVIDEO hVideo )
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

static HCURSOR hCursor;

LRESULT CALLBACK
VideoWindowProc2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch( uMsg )
	{
	case WM_CREATE:
		return TRUE;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
//#ifndef __NO_WIN32API__
LRESULT CALLBACK
VideoWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if defined( OTHER_EVENTS_HERE )
	static int level;
#define Return		if( l.flags.bLogMessages ) lprintf( "Finished Message %p %d %d", hWnd, uMsg, level-- ); return
#else
#define Return	return 
#endif
	PVIDEO hVideo;
	uint32_t _mouse_b = l.mouse_b;
	//static UINT uLastMouseMsg;
#if defined( OTHER_EVENTS_HERE )
	if( l.flags.bLogMessages )
		//if( uMsg != 13 && uMsg != WM_TIMER ) // get window title?
		{
			lprintf( "Got message %p %d(%04x) %p %p %d", hWnd, uMsg, uMsg, wParam, lParam, ++level );
		}
#endif
	switch (uMsg)
	{
#ifndef UNDER_CE
	case WM_NCCREATE: // very first message to be processed in creation.
		break;
#endif

	case 13:
#if defined( OTHER_EVENTS_HERE )
		//level++;
#endif
		break;
	case WM_MOVE:
		// wParam 
		// lParam X|Y
	case WM_SIZE:
		// wParam is a state type indicator...
		// lParam WIDTH|HEIGHT
		Return 0;
#ifndef UNDER_CE
	case WM_NCACTIVATE:
		// don't do default window border junk in defwindowproc()
		// wParam True - we get activate, return meaningless
		// wParam FALSE - losing activation, return 1 to allow, return 0 to prevent.
		Return 1;
#endif
#ifndef UNDER_CE
	case WM_MOUSEACTIVATE:
		//MA_ACTIVATE Activates the window, and does not discard the mouse message. 
		//MA_ACTIVATEANDEAT Activates the window, and discards the mouse message. 
		//MA_NOACTIVATE Does not activate the window, and does not discard the mouse message. 
		//MA_NOACTIVATEANDEAT Does not activate the window, but discards the mouse message. 
#endif

#if defined( OTHER_EVENTS_HERE )
		if( l.flags.bLogMessages )
			lprintf( "No, thanx, you don't need to activate." );
#endif
#ifndef UNDER_CE
		Return MA_ACTIVATE;
#endif
#ifndef UNDER_CE

	case WM_NCHITTEST:
		{
#if defined( OTHER_EVENTS_HERE )
			POINTS p = MAKEPOINTS( lParam );
			if( l.flags.bLogMessages )
				lprintf( "%d,%d Hit Test is Client", p.x,p.y );
#endif
		}
		Return HTCLIENT;
		//Return HTNOWHERE;
#endif
#ifndef UNDER_CE
	case WM_DRAWCLIPBOARD:
		{
			PVIDEO hVideo = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
			if( hVideo->pWindowClipboardEvent ) {
				hVideo->pWindowClipboardEvent( hVideo->dwClipboardEventData );
			}
		}
		break;
	case WM_DROPFILES:
		{
			//hDrop = (HANDLE) wParam
			//#ifndef HDROP
			//#define HDROP HANDLE
			//#endif
			HDROP hDrop = (HDROP)wParam;
			// messages are thread safe.
			static TEXTCHAR buffer[2048];
			PVIDEO hVideo = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
			INDEX nFiles = DragQueryFile( hDrop, (UINT)-1, NULL, 0 );
			INDEX iFile;
			POINT pt;
			// AND we should...
			DragQueryPoint( hDrop, &pt );
			for( iFile = 0; iFile < nFiles; iFile++ )
			{
				INDEX idx;
				struct dropped_file_acceptor_tag *callback;
				//uint32_t namelen = DragQueryFile( hDrop, iFIle, NULL, 0 );
				DragQueryFile( hDrop, (UINT)iFile, buffer, sizeof( buffer ) );
				//lprintf( "Accepting file drop [%s]", buffer );
				LIST_FORALL( hVideo->dropped_file_acceptors, idx, struct dropped_file_acceptor_tag*, callback )
				{
					callback->f( callback->psvUser, buffer, pt.x, pt.y );
				}
			}
			/*
			 The DragQueryFile function retrieves the filenames of dropped files.

			 UINT DragQueryFile(

	 HDROP hDrop,	// handle to structure for dropped files
	 UINT iFile,	// index of file to query
	 LPTSTR lpszFile,	// buffer for returned filename
	 UINT cch 	// size of buffer for filename
	);	
 

Parameters

hDrop

Identifies the structure containing the filenames of the dropped files. 

iFile
Specifies the index of the file to query. If the value of 
the iFile parameter is 0xFFFFFFFF, DragQueryFile returns a 
count of the files dropped. If the value of the 
iFile parameter is between zero and the total number of files dropped, 
DragQueryFile copies the filename with the corresponding value to the 
buffer pointed to by the lpszFile parameter. 

lpszFile

Points to a buffer to receive the filename of a dropped file when the function returns. This filename is a null-terminated string. If this parameter is NULL, DragQueryFile returns the required size, in characters, of the buffer. 

cch

Specifies the size, in characters, of the lpszFile buffer. 

 

Return Values

When the function copies a filename to the buffer, 
the return value is a count of the characters copied, 
not including the terminating null character. 
If the index value is 0xFFFFFFFF, 
the return value is a count of the dropped files. 
If the index value is between zero and the total number 
of dropped files and the lpszFile buffer address is NULL, 
the return value is the required size, in characters, 
of the buffer, not including the terminating null character. 

See Also

DragQueryPoint 			*/
			DragFinish( hDrop );
/*
			The DragFinish function releases memory that Windows allocated for use in transferring filenames to the application. 

VOID DragFinish(

	 HDROP hDrop 	// handle to memory to free
	);	
 

Parameters

hDrop

Identifies the structure describing dropped files. This handle is retrieved from the wParam parameter of the WM_DROPFILES message. 

 

Return Values

This function does not return a value. 

See Also

WM_DROPFILES 
*/
			Return 0;
		}
#endif
	case WM_SETFOCUS:
		{
			if( hWnd == l.hWndInstance )
			{
				PVIDEO hVidPrior = (PVIDEO)GetWindowLongPtr( (HWND)wParam, WD_HVIDEO );
				if( hVidPrior )
				{
#ifdef LOG_ORDERING_REFOCUS
					if( l.flags.bLogFocus )
					{
						lprintf( "-------------------------------- GOT FOCUS --------------------------" );
						lprintf( "Instance is %p", hWnd );
						lprintf( "prior is %p", hVidPrior->hWndOutput );
					}
#endif
					if( hVidPrior->pBelow )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( "Set to window prior was below..." );
#endif
						SetFocus( hVidPrior->pBelow->hWndOutput );
					}
					else if( hVidPrior->pNext )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( "Set to window prior last interrupted.." );
#endif
						SetFocus( hVidPrior->pNext->hWndOutput );
					}
					else if( hVidPrior->pPrior )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( "Set to window prior which interrupted us..." );
#endif
						SetFocus( hVidPrior->pPrior->hWndOutput );
					}
					else if( hVidPrior->pAbove )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( "Set to a window which prior was above." );
#endif
						SetFocus( hVidPrior->pAbove->hWndOutput );
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
						if( l.flags.bLogFocus )
							lprintf( "prior window is not around anythign!?" );
					}
#endif
				}
			}
#ifdef LOG_ORDERING_REFOCUS
			if( l.flags.bLogFocus )
				Log ("Got setfocus...");
#endif
			//SetWindowPos( l.hWndInstance, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			//SetWindowPos( hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			l.hVidFocused = hVideo;
			if (hVideo)
			{
				if( hVideo->flags.bDestroy )
				{
					Return FALSE;
				}
				if( !hVideo->flags.bFocused )
				{
					hVideo->flags.bFocused = 1;
					{
						if( l.flags.bLogFocus )
							lprintf( "Got a losefocus for %p", hVideo );
						if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
						{
							if( hVideo && hVideo->pLoseFocus )
								hVideo->pLoseFocus (hVideo->dwLoseFocus, NULL );
						}
					}
				}
			}
			//SetFocus( l.hWndInstance );
		}
	  Return 1;
#ifndef __cplusplus
		break;
#endif
	case WM_KILLFOCUS:
		{
#ifdef LOG_ORDERING_REFOCUS
			lprintf("Got Killfocus new focus to %p %p", hWnd, wParam);
#endif
			l.hVidFocused = NULL;
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			if (hVideo)
			{
				if( hVideo->flags.bDestroy )
				{
					Return FALSE;
				}
				if( hVideo->flags.bFocused )
				{
					hVideo->flags.bFocused = 0;
					// clear keyboard state...
					{
						if( !l.flags.bUseLLKeyhook )
							MemSet( &l.kbd, 0, sizeof( l.kbd ) );
						MemSet( &hVideo->kbd, 0, sizeof( hVideo->kbd ) );
					}
					if ( hVideo->pLoseFocus && !hVideo->flags.bHidden )
					{
						// don't really have a purpose for secondary argument...
						PVIDEO hVidRecv =
							(PVIDEO) GetWindowLongPtr ((HWND) wParam, WD_HVIDEO);
						if (!hVidRecv)
							hVidRecv = (PVIDEO) 1;
						else
						{
							PVIDEO me = hVideo;
							//lprintf( "killfocus window thing %d", hVidRecv );
							while( me )
							{
								if( me && me->pAbove == hVidRecv )
								{
#ifdef LOG_ORDERING_REFOCUS
									lprintf( "And - we need to stop this from happening, I'm stacked on the window getting the focus... restore it back to me" );
#endif
									//SetFocus( hVideo->hWndOutput );
									Return 0;
								}
								me = me->pAbove;
							}
						}
#ifdef LOG_ORDERING_REFOCUS
						lprintf( "Dispatch lose focus callback...." );
#endif
						{
							if( l.flags.bLogFocus )
								lprintf( "Got a losefocus for %p at %P", hVideo, hVidRecv );
							if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
							{
								if( hVideo && hVideo->pLoseFocus )
									hVideo->pLoseFocus (hVideo->dwLoseFocus, hVidRecv );
							}
						}
						//hVideo->pLoseFocus (hVideo->dwLoseFocus, hVidRecv);
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
						lprintf( "Hidden window or lose focus was not active." );
					}
#endif
				}
#ifdef LOG_ORDERING_REFOCUS
				else
				{
					lprintf( "this video window was not focused." );
				}
#endif
			}
		}
	  Return 1;
#ifndef __cplusplus
		break;
#endif
#ifndef UNDER_CE
	case WM_ACTIVATEAPP:
#ifdef OTHER_EVENTS_HERE
		if( l.flags.bLogMessages )
			lprintf( "activate app on this window? %d", wParam );
#endif
		break;
#endif
	case WM_ACTIVATE:
		if( hWnd == l.hWndInstance ) {
#ifdef LOG_ORDERING_REFOCUS
			Log2 ("Activate: %08x %08x", wParam, lParam);
#endif
			if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
			{
				hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
				if (hVideo)
				{
#ifdef LOG_ORDERING_REFOCUS
					Log2 ("Window %08x is below %08x", hVideo, hVideo->pBelow);
#endif
					if (hVideo->pBelow && hVideo->pBelow->flags.bShown)
					{
#ifdef LOG_ORDERING_REFOCUS
						Log ("Setting active window the the lower(upper?) one...");
#endif
						SetActiveWindow (hVideo->pBelow->hWndOutput);
					}
					else
					{
#ifdef LOG_ORDERING_REFOCUS
						Log ("Within same level do focus...");
#endif
						//FocusInLevel (hVideo);
					}
				}
			}
		}
		Return 1;
	case WM_RUALIVE:
		{
			int *alive = (int *) lParam;
			*alive = TRUE;
		}
		break;
#ifndef UNDER_CE
	case WM_NCPAINT:
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if (hVideo && hVideo->flags.bFull)	// do not allow system draw...
	  {
		  Return 0;
	  }
		break;
#endif
#ifndef UNDER_CE
	case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pwp;
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			if( !hVideo )
			{
#ifdef NOISY_LOGGING
				lprintf( "Too early, hVideo is not setup yet to reference." );
#endif
				Return 1;
			}
			pwp = (LPWINDOWPOS) lParam;
			if( hVideo->flags.bRestoring )
			{
				if( !( pwp->flags & SWP_NOSIZE ) )
				{
					pwp->cx = hVideo->pWindowPos.cx;
					pwp->cy = hVideo->pWindowPos.cy;
				}
			}
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "In Changing, window is %s", hVideo->flags.bTopmost?"Topmost":"NORMAL" );
#endif
			{
				if( !( pwp->flags & SWP_NOSIZE ) )
				{
					//lprintf( "Got resizing poschange..." );
					if( ( pwp->cx != hVideo->pWindowPos.cx )
						|| ( pwp->cy != hVideo->pWindowPos.cy ) )
					{
						//lprintf( "Forced size of display to %dx%d back to %dx%d", pwp->cx, pwp->cy, hVideo->pWindowPos.cx, hVideo->pWindowPos.cy );
						pwp->cx = hVideo->pWindowPos.cx;
						pwp->cy = hVideo->pWindowPos.cy;
						pwp->flags |= SWP_NOSIZE;
					}
					//else
					//	lprintf( "Target is correct." );
				}
			}
			if( hVideo->flags.bDeferedPos )
			{
				if( !(pwp->flags & SWP_NOZORDER ) )	
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( "PosChanging ? %p %p", hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
					pwp->hwndInsertAfter = hVideo->hDeferedAfter;
#ifdef LOG_ORDERING_REFOCUS
					lprintf( "PosChanging ? %p %p", hVideo->hWndOutput, pwp->hwndInsertAfter );
					lprintf( "Someone outside knew something about the ordering and this is a mass-reorder, must be correct? %p %p", hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
				}
				Return 0;
			}

			if( !(pwp->flags & SWP_NOZORDER ) )	
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( "Include set Z-Order" );
#endif
				// being moved in zorder...
				if( !hVideo->pBelow && !hVideo->pAbove && !hVideo->under )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( "... not above below or under, who cares. return. %p", pwp->hwndInsertAfter );							
#endif
					if( hVideo->flags.bTopmost )
					{
						if( pwp->hwndInsertAfter != HWND_TOPMOST )
						{
#ifdef LOG_ORDERING_REFOCUS
							lprintf( "Just make sure we're the TOP TOP TOP window.... (more than1?!)" );
#endif
							pwp->hwndInsertAfter = HWND_TOPMOST;
							//pwp->hwndInsertAfter = MoveWindowStack( hVideo, HWND_TOPMOST, 1 );
						}
						//else
						//{
						//	Return 0;
						//}
					}
					//else
					Return 0;
				}
				if( !pwp->hwndInsertAfter )				
				{											
					//lprintf( "..." );
					pwp->hwndInsertAfter = MoveWindowStack( hVideo, pwp->hwndInsertAfter,1  );

				}

				if( hVideo->flags.bIgnoreChanging )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( "Ignoreing the change generated by draw.... %p ", hVideo->pWindowPos.hwndInsertAfter );
#endif
					pwp->hwndInsertAfter = hVideo->pWindowPos.hwndInsertAfter;
					hVideo->flags.bIgnoreChanging = 0; // one shot. ... set by ShowNA
					Return 1;
				}
#ifdef LOG_ORDERING_REFOCUS
				lprintf( "Window pos Changing %p(used to be %p) (want to be after %p and before %p) %d,%d %d,%d"
					 , pwp->hwndInsertAfter
					 , hVideo->pWindowPos.hwndInsertAfter
					 , hVideo->pBelow?hVideo->pBelow->hWndOutput:NULL
					 , hVideo->pAbove?hVideo->pAbove->hWndOutput:NULL
					 , pwp->x
					 , pwp->y
					 , pwp->cx
					 , pwp->cy );
#endif
				hVideo->pWindowPos.hwndInsertAfter = pwp->hwndInsertAfter;
			}
			{
				pwp->flags &= ~SWP_NOZORDER;
				if( hVideo->pBelow )
				{
				
					if( hVideo->pBelow->hWndOutput != pwp->hwndInsertAfter )
					{
#ifdef LOG_ORDERING_REFOCUS
						lprintf( "Moving myself below what I'm expected to be below." );
						lprintf( "This action stops all reordering with other windows." );
#endif
						//if( level > 1 )
						pwp->hwndInsertAfter = MoveWindowStack( hVideo, pwp->hwndInsertAfter, 1 );
					}
				}
				else
				{
					PVIDEO check;
					for( check = hVideo; check; check = check->pAbove )
					{
						if( check->under ) 
						{
							if( IsWindow( check->under->hWndOutput ) )
							{
#ifdef LOG_ORDERING_REFOCUS
								lprintf( "Fixup for being the top window, but a parent is under something." );
#endif
								pwp->hwndInsertAfter = check->under->hWndOutput;
							}
#ifdef LOG_ORDERING_REFOCUS
							else
								lprintf( "uhmm... well it's going away.." );
#endif
						}
					}
				}
#ifdef LOG_ORDERING_REFOCUS
				lprintf( "Window pos Changing %p %d,%d %d,%d %08x"
						 , pwp->hwndInsertAfter
						 , pwp->x
						 , pwp->y
						 , pwp->cx
						 , pwp->cy
						 , pwp->flags );
#endif
			}
			if( pwp->flags & SWP_HIDEWINDOW )
			{
				// hide window happens under XP when closing the display.
				// XP also requires copybits to restore the correct background.
				// l.UpdateLayeredWindowIndirect
				if( l.flags.bOptimizeHide  ) // is vista (windows7)
				{
					//lprintf( "set no redraw too? ");
					// one of these under XP causes display artifacts.
					pwp->flags |= SWP_NOREDRAW | SWP_NOCOPYBITS |SWP_NOZORDER;
				}
			}
		}
		Return 0;
		//break;
#endif
#if 0
		// this ended up being useless - think it's just inside area anyhow
	case WM_NCCALCSIZE:
		if( wParam )
		{
			LPNCCALCSIZE_PARAMS  nccalcsize_params = (LPNCCALCSIZE_PARAMS)lParam;
			nccalcsize_params->rgrc[0] = nccalcsize_params->rgrc[1];
			// 0 is a safe enough return.
		}
		else
		{
			LPRECT rect = (LPRECT)lParam;
		}
		Return 0;
#endif
	case WM_WINDOWPOSCHANGED:
	//	break;
			{
			// global hVideo is pPrimary Video...
			LPWINDOWPOS pwp;
			pwp = (LPWINDOWPOS) lParam;
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
 #ifdef LOG_ORDERING_REFOCUS
			if( !pwp->hwndInsertAfter )
			{
				//WakeableSleep( 500 );
				//lprintf( "..." );
			}
			lprintf( "Being inserted after %x %x", pwp->hwndInsertAfter, hWnd );
#endif
			if (!hVideo)		// not attached to anything...
			{
				Return 0;
			}
			if( hVideo->flags.bDestroy )
			{
#ifdef NOISY_LOGGING
				lprintf( "Oh - don't care what is done with ordering to destroy." );
#endif
				Return 0;
			}
			if( hVideo->flags.bHidden )
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( "Hmm don't really care about the motion of a hidden window..." );
#endif
				hVideo->pWindowPos = *pwp; // save always....!!!
				Return 0;
			}

			EnterCriticalSec( &hVideo->cs );

			if( (!(pwp->flags & SWP_NOMOVE ) ) &&
				( pwp->x != hVideo->pWindowPos.x ||
				  pwp->y != hVideo->pWindowPos.y ) )
			{
				hVideo->pWindowPos.x = pwp->x;
				hVideo->pWindowPos.y = pwp->y;
			}

			if( ( (!(pwp->flags & SWP_NOSIZE ) ) &&
				( pwp->cx != hVideo->pWindowPos.cx ||
				 pwp->cy != hVideo->pWindowPos.cy ) ) ||
			  hVideo->flags.bForceSurfaceUpdate )
			{
				hVideo->pWindowPos.cx = pwp->cx;
				hVideo->pWindowPos.cy = pwp->cy;
#ifdef LOG_DISPLAY_RESIZE
				lprintf( "Resize happened, recreate drawing surface..." );
#endif
				CreateDrawingSurface( hVideo );
				// make sure this is set so a query can see if draw is required
				// even if the surface didn't change size or the pointer.
				hVideo->flags.bForceSurfaceUpdate = 1;
				SendApplicationDraw( hVideo );
				// ??
				hVideo->flags.bForceSurfaceUpdate = 0;
			}
			LeaveCriticalSec( &hVideo->cs );

#ifdef LOG_ORDERING_REFOCUS
			lprintf( "window pos changed - new ordering includes %p for %p(%p)", pwp->hwndInsertAfter, pwp->hwnd, hWnd );
#endif
			// maybe maintain my link according to outside changes...
			// tried to make it work the other way( and it needs to work the other way)

			//// don't save always, only parts are valid at times.
			//hVideo->pWindowPos = *pwp; // save always....!!!

			if( (!(pwp->flags & SWP_NOZORDER))
				&& (!hVideo->pWindowPos.hwndInsertAfter)
				&& hVideo->flags.bTopmost
				)
			{
				hVideo->pWindowPos.hwndInsertAfter = HWND_TOPMOST;
			}
			{
				RECT pwp2;
				RECT pwp3;
				GetWindowRect( hWnd, &pwp2 );
				GetClientRect( hWnd, &pwp3 );
				hVideo->cursor_bias.x = pwp2.left;// - l.WindowBorder_X + 5;
				hVideo->cursor_bias.y = pwp2.top;// - l.WindowBorder_Y + 7;
			}
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "Window pos %p %d,%d %d,%d (bias %d,%d)"
					, hVideo->pWindowPos.hwndInsertAfter
					 , hVideo->pWindowPos.x
					 , hVideo->pWindowPos.y
					 , hVideo->pWindowPos.cx
					 , hVideo->pWindowPos.cy 
					 , hVideo->cursor_bias.x
					 , hVideo->cursor_bias.y
					 );
#endif
		}
		Return 0;			// indicate handled message... no WM_MOVE/WM_SIZE generated.
#ifndef NO_TOUCH
	case WM_TOUCH:
		{
			if( l.GetTouchInputInfo )
			{
				TOUCHINPUT inputs[20];
				struct input_point outputs[20];
				int count = LOWORD(wParam);
				PVIDEO hVideo = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
				if( count > 20 )
					count = 20;
				l.GetTouchInputInfo( (HTOUCHINPUT)lParam, count, inputs, sizeof( TOUCHINPUT ) );
				//lprintf( "touch event with %d", count );
				l.CloseTouchInputHandle( (HTOUCHINPUT)lParam );
				{
					int n;
					for( n = 0; n < count; n++ )
					{
						// windows coordiantes some in in hundreths of pixesl as a long
#ifdef _DEBUG
						lprintf( "input point %d,%d %08x  %s %s %s %s %s %s %s"
								 , inputs[n].x, inputs[n].y
								 , inputs[n].dwFlags
								 , ( inputs[n].dwFlags & TOUCHEVENTF_MOVE)?"MOVE":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_DOWN)?"DOWN":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_UP)?"UP":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_INRANGE)?"InRange":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_PRIMARY)?"Primary":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_NOCOALESCE)?"NoCoales":""
								 , ( inputs[n].dwFlags & TOUCHEVENTF_PALM)?"PALM":""
								 );
#endif
						outputs[n].x = inputs[n].x / 100.0f;
						outputs[n].y = inputs[n].y / 100.0f;

						if( inputs[n].dwFlags & TOUCHEVENTF_DOWN )
							outputs[n].flags.new_event = 1;
						else
							outputs[n].flags.new_event = 0;

						if( inputs[n].dwFlags & TOUCHEVENTF_UP )
							outputs[n].flags.end_event = 1;
						else
							outputs[n].flags.end_event = 0;
					}
				}
				if( hVideo )
				{
					int n;
					PINPUT_POINT new_inputs = NewArray( struct input_point, count );
					for( n = 0; n < count; n++ )
					{
						new_inputs[n].x = inputs[n].x;
						new_inputs[n].y = inputs[n].y;
						new_inputs[n].flags.new_event = inputs[n].dwFlags & TOUCHEVENTF_DOWN;
						new_inputs[n].flags.end_event = inputs[n].dwFlags & TOUCHEVENTF_UP;
					}
					InvokeSurfaceInput( count, new_inputs );
					Deallocate( PINPUT_POINT, new_inputs );
					if( hVideo->pTouchCallback )
					{
						hVideo->pTouchCallback( hVideo->dwTouchData, outputs, count );
					}
				}
			}
		}
		Return 0;
#endif
		/*
		* DM_POINTERHITTEST
		* WM_NCPOINTERDOWN
		* WM_NCPOINTERUP
		* WM_NCPOINTERUPDATE
		* WM_PARENTNOTIFY
		* WM_POINTERACTIVATE
		* WM_POINTERCAPTURECHANGED
		* WM_POINTERDEVICECHANGE
		* WM_POINTERDEVICEINRANGE
		* WM_POINTERDEVICEOUTOFRANGE
		* WM_POINTERDOWN
		* WM_POINTERENTER
		* WM_POINTERLEAVE
		* WM_POINTERROUTEDAWAY  // crosschained
		* WM_POINTERROUTEDRELEASED // crosschained
		* WM_POINTERROUTEDTO  // crosschained
		* WM_POINTERUP
		* WM_POINTERUPDATE
		* WM_POINTERWHEEL
		* WM_POINTERHWHEEL
		* WM_TOUCHHITTESTING
		*/
		case WM_POINTERDOWN:
		case WM_POINTERUP:
			hVideo           = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
			UINT32 PointerID = GET_POINTERID_WPARAM( wParam );
			LOGICAL isNew = IS_POINTER_NEW_WPARAM( wParam );
			LOGICAL isInRange = IS_POINTER_INRANGE_WPARAM( wParam );
			LOGICAL isInContact     = IS_POINTER_INCONTACT_WPARAM( wParam );
			LOGICAL isPrimary = IS_POINTER_PRIMARY_WPARAM( wParam );
			LOGICAL isPrimaryButton = IS_POINTER_FIRSTBUTTON_WPARAM( wParam );
			LOGICAL isSecondaryButton = IS_POINTER_SECONDBUTTON_WPARAM( wParam );
			LOGICAL isTertiaryButton  = IS_POINTER_THIRDBUTTON_WPARAM( wParam );
			LOGICAL isFourthButton    = IS_POINTER_FOURTHBUTTON_WPARAM( wParam );
			LOGICAL isFifthButton     = IS_POINTER_FIFTHBUTTON_WPARAM( wParam );

			int x = ((int16_t)(lParam&0xFFFF));// the x (horizontal point) coordinate.
			int y = ((int16_t)(lParam >> 16));//: the y (vertical point) coordinate.
			struct pen_event Info;
			Info.x = x;
			Info.y = y;
			break;
		case WM_POINTERUPDATE: {
			hVideo           = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );

			UINT32 PointerID = GET_POINTERID_WPARAM( wParam );
			// ResumeThread(ThreadHandle);

			POINTER_PEN_INFO Info = { 0 };
			if( IS_POINTER_INRANGE_WPARAM( wParam ) && GetPointerPenInfo( PointerID, &Info ) ) {
				lprintf( "FLAGS: %x\r", Info.pointerInfo.pointerFlags );
				/*
				*     POINTER_INFO    pointerInfo;
					PEN_FLAGS       penFlags;
					PEN_MASK        penMask;
					UINT32          pressure;
					UINT32          rotation;
					INT32           tiltX;
					INT32           tiltY;

				*/
				struct pen_event event;
				event.penFlags = Info.penFlags;
				event.penMask  = Info.penMask;
				event.pressure = Info.pressure;
				event.rotation = Info.rotation;
				event.tiltX    = Info.tiltX;
				event.tiltY    = Info.tiltY;
				event.nOverflow = 0;
				POINTER_PEN_INFO infos[ 10 ];
				UINT32 points;
				if( GetPointerFramePenInfo( PointerID, &points, infos ) ) {
				}
				SkipPointerFrameMessages( PointerID );

				if( hVideo->pPenCallback ) {
					hVideo->pPenCallback( hVideo->dwPenData, &event );
				}
				// PointerInfo struct + pressure (set touchmask + flags appropriately)
				//unsigned char *spreadStruct = serialize( &Info );

			   /*printf("(%5d,%5d) %5u\r",
			      Info.pointerInfo.ptPixelLocation.x,
			      Info.pointerInfo.ptPixelLocation.y,
			      Info.pressure);*/

			   // sendInput(&Info, ...)
			   // Or add to thread's work queue (maybe use GetPointerPenInfoHistory()?)
		   }

	   } break;

#ifndef UNDER_CE
	case WM_NCMOUSEMOVE:
#endif
		// normal fall through without processing button states
		if (0)
		{
			int16_t wheel;
	case WM_MOUSEWHEEL:
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			wheel = (int16_t)(( wParam & 0xFFFF0000 ) >> 16);
			if( wheel >= 120 )
				l.mouse_b |= MK_SCROLL_UP;
			if( wheel <= -120 )
				l.mouse_b |= MK_SCROLL_DOWN;
		}
		else if (0)
		{
#ifndef UNDER_CE
	case WM_NCLBUTTONDOWN:
			l.mouse_b |= MK_LBUTTON;
		}
		else if (0)
		{
	case WM_NCMBUTTONDOWN:
			l.mouse_b |= MK_MBUTTON;
		}
		else if (0)
		{
	case WM_NCRBUTTONDOWN:
			l.mouse_b |= MK_RBUTTON;
		}
		else if (0)
		{
	case WM_NCLBUTTONUP:
			l.mouse_b &= ~MK_LBUTTON;
		}
		else if (0)
		{
	case WM_NCMBUTTONUP:
			l.mouse_b &= ~MK_MBUTTON;
		}
		else if (0)
		{
	case WM_NCRBUTTONUP:
			l.mouse_b &= ~MK_RBUTTON;
#endif
		}
		else if (0)
		{
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | (int)wParam;
		}
		else if (0)
		{
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | (int)wParam;
		}
		else if (0)
		{
	case WM_MOUSEMOVE:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | (int)wParam;
		}

		//hWndLastFocus = hWnd;
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if (!hVideo)
		{
			Return 0;
		}
		if( l.hCaptured )
		{
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( "Captured mouse already - don't do anything?" );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
				if( l.flags.bLogMouseEvents )
					lprintf( "Auto owning mouse to surface which had the mouse clicked DOWN." );
#endif
				if( !l.hCaptured )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( "Auto release mouse from surface which had the mouse unclicked." );
				if( !l.hCaptured )
					ReleaseCapture();
			}
		}

		{
			POINT p;
			int dx, dy;
			GetCursorPos (&p);
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( "Mouse position %d,%d", p.x, p.y );
#endif
			p.x -= (dx =(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.x);
			p.y -= (dy=(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.y);
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( "Mouse position results %d,%d %d,%d", dx, dy, p.x, p.y );
#endif
			if (!(l.hCaptured?l.hCaptured:hVideo)->flags.bFull)
			{
				p.x -= l.WindowBorder_X;
				p.y -= l.WindowBorder_Y;
			}
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( "Mouse position results %d,%d %d,%d", dx, dy, p.x, p.y );
#endif
			l.mouse_x = p.x;
			l.mouse_y = p.y;
			// save now, so idle timer can hide cursor.
		}
		if( l.last_mouse_update )
		{
			l.last_mouse_update_display = hVideo;
			if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
			{
				if( !l.flags.mouse_on ) // mouse is off...
				{
					l.last_mouse_update = 0;
				}
			}
		}
		if( l.mouse_x != l._mouse_x ||
			l.mouse_y != l._mouse_y ||
			l.mouse_b != l._mouse_b ||
			l.mouse_last_vid != hVideo ) // this hvideo!= last hvideo?
		{
			if( l.flags.bLogMouseEvents )
				lprintf( "Mouse Moved" );
			if( (!hVideo->flags.mouse_on || !l.flags.mouse_on ) && !hVideo->flags.bNoMouse)
			{
				int x;
				if (!hCursor)
					hCursor = LoadCursor (NULL, IDC_ARROW);
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "cursor on." );
#endif
				x = ShowCursor( TRUE );
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "cursor count %d %d", x, hCursor );
#endif
				SetCursor (hCursor);
				l.flags.mouse_on = 1;
				hVideo->flags.mouse_on = 1;
			}
			if( hVideo->flags.bIdleMouse )
			{
				l.last_mouse_update = timeGetTime();
				l.last_mouse_update_display = hVideo;
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "Tick ... %d", l.last_mouse_update );
#endif
			}
			l.mouse_last_vid = hVideo;
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( "Generate mouse message %p(%p?) %d %d,%08x %d+%d=%d", l.hCaptured, hVideo, l.mouse_x, l.mouse_y, l.mouse_b, l.dwMsgBase, MSG_MouseMethod, l.dwMsgBase + MSG_MouseMethod );
#endif
			InvokeSimpleSurfaceInput( l.mouse_x, l.mouse_y, (l.mouse_b & ~l._mouse_b ) != 0, (l.mouse_b & l._mouse_b ) != 0 );
			if (hVideo->pMouseCallback)
			{
				hVideo->pMouseCallback (hVideo->dwMouseData,
												l.mouse_x, l.mouse_y, l.mouse_b);
			}
			if( l.new_cursor )
			{
				SetCursor (LoadCursor (NULL, l.new_cursor) );
				l.old_cursor = l.new_cursor;
				l.new_cursor = 0;
			}
			l._mouse_x = l.mouse_x;
			l._mouse_y = l.mouse_y;
			// clear scroll buttons...
			// otherwise circumstances of mouse wheel followed by any other event
			// continues to generate scroll clicks.
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			l._mouse_b = l.mouse_b;
		}
		Return 0;			// don't allow windows to think about this...
	case WM_SHOWWINDOW:
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		//lprintf( "Show Window hidden = %d (%d)", wParam, !wParam );
		if( hVideo->flags.bHidden != !wParam )
		{
			
			if( hVideo->flags.bHidden = !wParam )
			{
				if( hVideo->pHideCallback )
					hVideo->pHideCallback( hVideo->dwHideData );
			}
			else
			{
				if( hVideo->pRestoreCallback )
					hVideo->pRestoreCallback( hVideo->dwRestoreData );
			}
		}
		if( !wParam )
		{
			// hiding the window... don't fall through paint.
			Return 0;
		}
		if( !hVideo->flags.bLayeredWindow )
		{
			//lprintf( "not a layered window... don't need to do auto-paint" );
			// a layered window doesn't get an initial draw!?
			Return 0;
		}
		//lprintf( "Show Window Message! %p", hVideo );
		if( !hVideo->flags.bShown )
		{
			//lprintf( "Window is hiding? that is why we got it?" );
			Return 0;
		}
		//lprintf( "Fall through to WM_PAINT..." );
	case WM_PAINT:
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if( hVideo->flags.bDestroy )
		{
#if DEBUG_INVALIDATE
			lprintf( "clear Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
			DeleteLink( &l.invalidated_windows, hVideo );
			ValidateRect( hWnd, NULL );
			Return 0;
		}
		if( l.flags.bLogWrites )
			lprintf( "Paint Message! %p", hVideo );
		if( hVideo->flags.event_dispatched )
		{
#if DEBUG_INVALIDATE
			lprintf( "clear Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
			DeleteLink( &l.invalidated_windows, hVideo );
			ValidateRect( hWnd, NULL );
#ifdef NOISY_LOGGING
			lprintf( "Validated rect... will you stop calling paint!?" );
#endif
			Return 0;
		}
		{
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				EnterCriticalSec( &hVideo->cs );
				if( hVideo->flags.bDestroy )
				{
					lprintf( "Aborting WM_PAINT (is bDestroy'ed)" );
					LeaveCriticalSec( &hVideo->cs );
					Return 0;
				}
				//lprintf( "Found window - send draw" );
				SendApplicationDraw( hVideo );
				LeaveCriticalSec( &hVideo->cs );
			}
			else
				lprintf( "Failed to find window to show?" );
			while( FindLink( &l.invalidated_windows, hVideo ) != INVALID_INDEX ) {
#if DEBUG_INVALIDATE
					lprintf( "clear Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
					DeleteLink( &l.invalidated_windows, hVideo );
					if( hVideo->portion_update.pending ) {
						hVideo->portion_update.pending = FALSE;
						UpdateDisplayPortion( hVideo
							, hVideo->portion_update.x, hVideo->portion_update.y
							, hVideo->portion_update.w, hVideo->portion_update.h );
					}
					else
						UpdateDisplayPortion( hVideo, 0, 0, 0, 0 );
					//if( l.flags.bPostedInvalidate )
					//	lprintf( "triggered to draw too soon!" );
#if DEBUG_INVALIDATE
					lprintf( "clear Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
#if 0
			} else if( l.invalidated_window ) {
					lprintf( " failed %d %p %p", l.flags.bPostedInvalidate, l.invalidated_window, hVideo );
					break;
				}
#endif
				ValidateRect( hWnd, NULL );
			}
			//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
		}
		//lprintf( "redraw... WM_PAINT" );
		//if( l.flags.bPostedInvalidate )
		//	lprintf( "triggered to draw too soon!" );
		//lprintf( "redraw... WM_PAINT" );
		/// allow a second invalidate to post.
#ifdef NOISY_LOGGING
		lprintf( "Finished and clearing post Invalidate." );
#endif
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_RESTORE:
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			SendApplicationDraw( hVideo );
			break;
		case SC_KEYMENU:
			if( lParam != ' ' )
				return 0;
			break;
		case SC_CLOSE:

			//DestroyWindow( hWnd );
			Return TRUE;
		}
		break;
#ifndef _ARM_ // maybe win32_CE?
	case WM_QUERYENDSESSION:
		xlprintf(1500)( "QUERY ENDING SESSION! (respond OK)" );
#ifdef WIN32
		SetEvent( CreateEvent( NULL, TRUE, FALSE, "Windows Is Shutting Down" ) );
#endif
		InvokeBeginShutdown();
		Return TRUE; // uhmm okay.
	case WM_ENDSESSION:
		xlprintf(1500)( "ENDING SESSION! (well I guess someone will close this?)" );
		//ThreadTo( DoExit, lParam );
		//BAG_Exit( (int)lParam );
		Return TRUE; // uhmm okay.
	case WM_DESTROY:
#ifdef LOG_DESTRUCTION
		Log( "Destroying a window..." );
#endif
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if (hVideo)
		{
#ifdef LOG_DESTRUCTION
			Log ("Killing the window...");
#endif
			DoDestroy (hVideo);
		}
		break;
#endif
	case WM_ERASEBKGND:
		// LIE! we don't care to ever erase the background...
		// thanx for the invite though.
		//lprintf( "Erase background, and we just say NO" );
		Return TRUE;
	case WM_USER_MOUSE_CHANGE:
		{
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			if( hVideo )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "hVideo..." );
#endif
				// if not idling mouse...
				if( !hVideo->flags.bIdleMouse )
				{
#ifdef LOG_MOUSE_HIDE_IDLE
					lprintf( "No idle mouse invis..." );
#endif
					// if the cursor was off...
					if( !l.flags.mouse_on )
					{
#ifdef LOG_MOUSE_HIDE_IDLE
						lprintf( "Mouse was off... turning on." );
#endif
						ShowCursor( TRUE );
						SetCursor (hCursor);
						l.flags.mouse_on = 1;
					}
				}
			}
		}
		break;
	case WM_TIMER:
		{
			hVideo = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
			if( hVideo && hVideo->top_force_timer_id == wParam )
			{
				lprintf( "tick for top_force_timer" );
				if( hVideo->flags.bAbsoluteTopmost )
				{
					if( GetTopWindow( NULL ) != hVideo->hWndOutput )
						SetWindowPos( hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
				}
			}
		}
		if( l.flags.mouse_on && l.last_mouse_update )
		{
			//lprintf( "tick for mouse_hide" );
#ifdef LOG_MOUSE_HIDE_IDLE
			lprintf( "Pending mouse away... %d", timeGetTime() - ( l.last_mouse_update ) );
#endif
			if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
			{
				int x;
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "OFF!" );
#endif
				l.flags.mouse_on = 0;
				//l.last_mouse_update = 0;
				//lprintf( "showCursor(FALSE)" );
				{
					CURSORINFO  ci;
#ifndef CURSOR_SUPPRESSED
#  define CURSOR_SUPPRESSED 0x00000002
#endif
					ci.cbSize = sizeof( CURSORINFO );
					if( GetCursorInfo( &ci ) ) {
						//lprintf( "cursor info is %d", ci.flags );
						if( (ci.flags & CURSOR_SHOWING) && !(ci.flags & CURSOR_SUPPRESSED) ) {
							int c = ShowCursor( TRUE );
							//lprintf( "cursor count will be %d", c );
							while( c && ( x = ShowCursor( FALSE ) ) )
							{
								//lprintf( "cursor count will be %d", x );
							}
							//lprintf( "cursor count will be %d", x );
						}
					}
					else {
						lprintf( "Cursor info failed? %d", GetLastError() );
					}
				}
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "Show count %d %d", x, GetLastError() );
#endif
				//lprintf( "setCursor(NULL)" );
				SetCursor( NULL );
			}
		}
		// this is a special return, it doesn't decrement the counter... cause WM_TIMER is filtered in OTHER_EVENTS logging
		Return 0;
		//break;
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs;
#ifdef LOG_OPEN_TIMING
			lprintf( "Begin WM_CREATE" );
#endif
			{
				INDEX idx;
				PTHREAD thread;
				HHOOK added;
				PTHREAD me = MakeThread();
				LIST_FORALL( l.threads, idx, PTHREAD, thread )
				{
					if( me == thread )
						break;
				}
				if( thread == NULL )
				{		
					HINSTANCE hInst;
					hInst =  GetModuleHandle(TARGETNAME);
					if( hInst == NULL )
						hInst = (HMODULE)GetPrivateModuleHandle( TARGETNAME );
				// definatly add whatever thread made it to the WM_CREATE.			
					AddLink( &l.threads, me );
					AddIdleProc( (int(CPROC*)(uintptr_t))ProcessDisplayMessages, 0 );
					lprintf( "No thread %x, adding hook and thread.", GetCurrentThreadId() );
#ifdef USE_KEYHOOK

					if( l.flags.bUseLLKeyhook )
						AddLink( &l.ll_keyhooks,
								  added = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
																  , hInst, 0 /*GetCurrentThreadId()*/
																  )
								 );
					else
						AddLink( &l.keyhooks,
								  added = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyHook
																  , NULL /*GetModuleHandle(TARGETNAME)*/, GetCurrentThreadId()
																  )
								 );
#endif
				}
			}
			pcs = (LPCREATESTRUCT) lParam;
#ifndef NO_TOUCH
			if( l.RegisterTouchWindow )
				l.RegisterTouchWindow( hWnd, TWF_WANTPALM|TWF_FINETOUCH );
#endif
#ifndef NO_DRAG_DROP
			DragAcceptFiles( hWnd, TRUE );
#endif
			/* for idle mouse hide.... */
		 /*
VOID DragAcceptFiles(

	 HWND hWnd,	// handle to the registering window
	 BOOL fAccept	// acceptance option
	);	
 

Parameters
hWnd

Identifies the window registering whether it accepts dropped files. 
fAccept
Specifies whether the window identified by the hWnd parameter accepts dropped files. This value is TRUE to accept dropped files; it is FALSE to discontinue accepting dropped files.

 Return Values
This function does not return a value. 

Remarks
An application that calls DragAcceptFiles with the fAccept parameter set to TRUE has identified itself as able to process the WM_DROPFILES message from File Manager. 

See Also

WM_DROPFILES 
			*/
			if (pcs)
			{
				hVideo = (PVIDEO)pcs->lpCreateParams; // user passed param...

				if ((LONG)(uintptr_t)hVideo == 1)
					break;

				if (!hVideo)
				{
					// directly created without parameter (Dialog control
					// probably )... grab a static PVIDEO for this...
					hVideo = &l.hDialogVid[(l.nControlVid++) & 0xf];
				}
				SetLastError( 0 );
				SetWindowLongPtr (hWnd, WD_HVIDEO, (uintptr_t) hVideo);
				if( GetLastError() )
				{
					lprintf( "error setting window long pointer : %d", GetLastError() );
				}

				if (hVideo->flags.bFull)
					hVideo->hDCOutput = GetWindowDC (hWnd);
				else
					hVideo->hDCOutput = GetDC (hWnd);

				GetWindowRect (hWnd, (RECT *) & (hVideo->pWindowPos.x));
				hVideo->pWindowPos.cx -= hVideo->pWindowPos.x;
				hVideo->pWindowPos.cy -= hVideo->pWindowPos.y;
				// should maybe check here... but this better be the same window handle...
				hVideo->hWndOutput = hWnd;
				hVideo->pThreadWnd = MakeThread();

				CreateDrawingSurface (hVideo);
				hVideo->flags.bForceSurfaceUpdate = 0;
				hVideo->flags.bReady = TRUE;
				if( hVideo->thread ) // if someone is waiting...
					WakeThread( hVideo->thread );
			}
#ifdef LOG_OPEN_TIMING
			//lprintf( "Complete WM_CREATE" );
#endif
		}
		break;
	}
	Return DefWindowProc (hWnd, uMsg, wParam, lParam);
#undef Return
}
//#endif
//----------------------------------------------------------------------------

#ifndef HWND_MESSAGE
#define HWND_MESSAGE	  ((HWND)-3)
#endif

#ifdef UNDER_CE
#define WINDOW_STYLE 0
#else
#define WINDOW_STYLE (WS_SYSMENU|WS_OVERLAPPEDWINDOW)
#endif

RENDER_PROC (BOOL, CreateWindowStuffSizedAt) (PVIDEO hVideo, int x, int y,
															 int wx, int wy)
{
#ifndef __NO_WIN32API__
	static HMODULE hMe;
	if( hMe == NULL )
		hMe = GetModuleHandle (TARGETNAME);
	if( hMe == NULL )
		hMe = (HMODULE)GetPrivateModuleHandle( TARGETNAME );
	if( hMe == NULL )
		hMe = GetModuleHandle( NULL );
	//lprintf( "-----Create WIndow Stuff----- %s %s", hVideo->flags.bLayeredWindow?"layered":"solid"
	//		 , hVideo->flags.bChildWindow?"Child(tool)":"user-selectable" );
	//DebugBreak();
	l.bCreatedhWndInstance = 0;
	if (!hVideo->flags.bFull)
	{
		// may need to adjust window frame parameters...
		// used to have this code but we just deleted it!
	}
	if (!l.hWndInstance && !hVideo->hWndContainer)
	{
		l.bCreatedhWndInstance = 1;
		//Log1 ("Creating container window named: %s",
		//	 (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo->pTitle);
		l.hWndInstance = CreateWindowEx (0
#ifndef NO_DRAG_DROP
													| WS_EX_ACCEPTFILES
#endif
#ifdef UNICODE
												  , (LPWSTR)l.aClass
#else
												  , MAKEINTATOM(l.aClass)
#endif
												  , (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo->pTitle
												  , WINDOW_STYLE
												  , 0, 0, 0, 0
												  , HWND_MESSAGE  // (GetDesktopWindow()), // Parent
												  , NULL // Menu
												  , hMe
												  , (void *) 1);
		if( !l.hWndInstance )
		{
			lprintf( "Failed to create instance window %d", GetLastError() );
		}
		{
#ifdef LOG_OPEN_TIMING
			lprintf( "Created Real window...Stuff.." );
#endif
			l.hVidCore = New(VIDEO);
			MemSet (l.hVidCore, 0, sizeof (VIDEO));
			InitializeCriticalSec( &l.hVidCore->cs );
			l.hVidCore->hWndOutput = (HWND)l.hWndInstance;
			l.hVidCore->pThreadWnd = MakeThread();
		}
		//Log ("Created master window...");
	}
	if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
	{
		uint32_t w, h;
		GetDisplaySize (&w, &h);
		wx = w * 7 / 10;
		wy = h * 7 / 10;
	}
	if( hVideo->hWndContainer )
	{
		RECT r;
		GetClientRect( hVideo->hWndContainer, &r );
		x = 0;
		y = 0;
		wx = r.right-r.left + 1;
		wy = r.top-r.bottom + 1;
	}
	if( !hVideo->hWndOutput )
	{
		HWND result;
		// hWndOutput is set within the create window proc...
#ifdef LOG_OPEN_TIMING
		lprintf( "Create Real Window (In CreateWindowStuff).." );
#endif
		result = CreateWindowEx( 0
#ifndef NO_TRANSPARENCY
										| (hVideo->flags.bLayeredWindow?WS_EX_LAYERED:0)
#endif
#ifndef NO_MOUSE_TRANSPARENCY
										| (hVideo->flags.bNoMouse?WS_EX_TRANSPARENT:0)
#endif
										| (hVideo->flags.bChildWindow?WS_EX_TOOLWINDOW:0)
										| (hVideo->flags.bTopmost?WS_EX_TOPMOST:0)
										// | WS_EX_NOPARENTNOTIFY
#ifdef UNICODE
									  , (LPWSTR)l.aClass
#else
									  , MAKEINTATOM(l.aClass)
#endif
									  , (l.gpTitle&&l.gpTitle[0])?l.gpTitle:hVideo->pTitle
									  , (hVideo->hWndContainer)?WS_CHILD:(hVideo->flags.bFull ? (WS_SYSMENU|WS_POPUP|WS_MAXIMIZEBOX|WS_MINIMIZEBOX) : (WINDOW_STYLE))
									  , x, y
									  , hVideo->flags.bFull ?wx:(wx + l.WindowBorder_X)
									  , hVideo->flags.bFull ?wy:(wy + l.WindowBorder_Y)
									  , hVideo->hWndContainer //(HWND)l.hWndInstance  // Parent
									  , NULL	  // Menu
									  , hMe
									  , (void *) hVideo);
		if( !result )
			lprintf( "Failed to create window %d", GetLastError() );
	}
	else
	{

	}
	// save original screen image for initialized state...
	BitBlt ((HDC)hVideo->hDCBitmap, 0, 0, wx, wy
			 , (HDC)hVideo->hDCOutput, 0, 0, SRCCOPY);


#ifdef LOG_OPEN_TIMING
	//lprintf( "Created window stuff..." );
#endif
	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
	//Log ("Created window in module...");
	if (!hVideo->hWndOutput)
		return FALSE;
	//SetParent( hVideo->hWndOutput, l.hWndInstance );
	return TRUE;
#else
	// need a .NET window here...
	return FALSE;
#endif
}

//----------------------------------------------------------------------------

int CPROC VideoEventHandler( uint32_t MsgID, uint32_t *params, uint32_t paramlen )
{
#if defined( LOG_MOUSE_EVENTS ) || defined( OTHER_EVENTS_HERE )
	if( l.flags.bLogMouseEvents || l.flags.bLogMessages )
		lprintf( "Received message %d", MsgID );
#endif
	l.dwEventThreadID = GetCurrentThreadId();
	//LogBinary( (POINTER)params, paramlen );
	switch( MsgID )
	{
	case MSG_DispatchPending:
		{
			INDEX idx;
			PVIDEO hVideo;
			//lprintf( "dispatching outstanding events..." );
			LIST_FORALL( l.pInactiveList, idx, PVIDEO, hVideo )
			{
				if( !hVideo->flags.bReady )
				{
					if( !hVideo->flags.bInDestroy )
					{
						SetWindowLongPtr( hVideo->hWndOutput, WD_HVIDEO, 0 );
						ReleaseEx( hVideo DBG_SRC ); // last event in queue, should be safe to go away now...
					}
					SetLink( &l.pInactiveList, idx, NULL );
				}
			}
			LIST_FORALL( l.pActiveList, idx, PVIDEO, hVideo )
			{
				if( hVideo->flags.mouse_pending )
				{
					InvokeSimpleSurfaceInput( l.mouse_x, l.mouse_y, (l.mouse_b & (~l._mouse_b) ) != 0, (l.mouse_b & l._mouse_b ) != 0 );
					if( hVideo && hVideo->pMouseCallback)
					{
#ifdef LOG_MOUSE_EVENTS
						if( l.flags.bLogMouseEvents )
							lprintf( "Pending dispatch mouse %p (%d,%d) %08x"
									 , hVideo
									 , hVideo->mouse.x
									 , hVideo->mouse.y
									 , hVideo->mouse.b );
#endif
						if( !hVideo->flags.event_dispatched )
						{
							hVideo->flags.mouse_pending = 0;
							hVideo->flags.event_dispatched = 1;
							hVideo->pMouseCallback( hVideo->dwMouseData
														 , hVideo->mouse.x
														 , hVideo->mouse.y
														 , hVideo->mouse.b
														 );
							//lprintf( "Resulted..." );
							hVideo->mouse.x = l.mouse_x;
							hVideo->mouse.y = l.mouse_y;
							hVideo->mouse.b = l.mouse_b;
							hVideo->flags.event_dispatched = 0;
						}
					}
				}
			}
		}
		break;
	case MSG_LoseFocusMethod:
		{
			PVIDEO hVideo = (PVIDEO)((uintptr_t*)params)[0];
			if( l.flags.bLogFocus )
				lprintf( "Got a losefocus for %p at %P", params[0], params[1] );
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo && hVideo->pLoseFocus )
					hVideo->pLoseFocus (hVideo->dwLoseFocus, (PVIDEO)((uintptr_t*)params)[1] );
			}
		}
		break;
	case MSG_RedrawMethod:
		{
			PVIDEO hVideo = (PVIDEO)((uintptr_t*)params)[0];
			//lprintf( "Show video %p", hVideo );
				/* Oh neat a safe window list... we should use this more places! */
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( l.flags.bLogWrites )
					lprintf( "invalidate post" );
				InvalidateRect( hVideo->hWndOutput, NULL, FALSE ); // invokes application draw also... (which if optimalized, will have already drawn if it invoked this, and will not redraw. */
				EnterCriticalSec( &hVideo->cs );
				if( hVideo && hVideo->pRedrawCallback )
				{
					hVideo->flags.event_dispatched = 1;
					hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
					hVideo->flags.event_dispatched = 0;
					UpdateDisplayPortion (hVideo, 0, 0, 0, 0);
					//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
				}
				LeaveCriticalSec( &hVideo->cs );
			}
			else
				lprintf( "Failed to find window to show?" );
		}
		break;
	case MSG_CloseMethod:
		break;
	case MSG_MouseMethod:
		{
			PVIDEO hVideo = (PVIDEO)((uintptr_t*)params)[0];
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
			{
				lprintf( "mouse method... forward to application please..." );
				lprintf( "params %ld %ld %ld", params[1], params[2], params[3] );
			}
#endif
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo->flags.mouse_pending )
				{
					if( params[3] != hVideo->mouse.b ||
						( params[3] & MK_OBUTTON ) )
					{
#ifdef LOG_MOUSE_EVENTS
						if( l.flags.bLogMouseEvents )
							lprintf( "delta dispatching mouse (%d,%d) %08x"
									 , hVideo->mouse.x
									 , hVideo->mouse.y
									 , hVideo->mouse.b );
#endif
						if( !hVideo->flags.event_dispatched )
						{
							hVideo->flags.event_dispatched = 1;
							InvokeSimpleSurfaceInput( l.mouse_x, l.mouse_y, (l.mouse_b & ~l._mouse_b ) != 0, (l.mouse_b & l._mouse_b ) != 0 );
							if( hVideo->pMouseCallback )
								hVideo->pMouseCallback( hVideo->dwMouseData
															 , hVideo->mouse.x
															 , hVideo->mouse.y
															 , hVideo->mouse.b
															 );
							hVideo->mouse.x = l.mouse_x;
							hVideo->mouse.y = l.mouse_y;
							hVideo->mouse.b = l.mouse_b;
							hVideo->flags.event_dispatched = 0;
						}
					}
					if( params[3] & MK_OBUTTON )
					{
						// copy keyboard state table for the client's use...
					}
				}
#ifdef LOG_MOUSE_EVENTS
				if( l.flags.bLogMouseEvents )
					lprintf( "*** Set Mouse Pending on %p %ld,%ld %08x!", hVideo, params[1], params[2], params[3]  );
#endif
				hVideo->flags.mouse_pending = 1;
				hVideo->mouse.x = params[1];
				hVideo->mouse.y = params[2];
				hVideo->mouse.b = params[3];
				// indicate that we want to receive eventdispatch
				return EVENT_WAIT_DISPATCH;
			}
 		}
		break;
	case MSG_KeyMethod:
		{
			int dispatch_handled = 0;
			PVIDEO hVideo = (PVIDEO)((uintptr_t*)params)[0];
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				if( hVideo && hVideo->pKeyProc )
				{
					hVideo->flags.event_dispatched = 1;
					//lprintf( "Dispatched KEY!" );
					if( hVideo->flags.key_dispatched )
						EnqueLink( &hVideo->pInput, (POINTER)((uintptr_t*)params)[1] );
					else
					{
						hVideo->flags.key_dispatched = 1;
						do
						{
							//lprintf( "Dispatching key %08lx", params[1] );
							dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, params[1] );
							//lprintf( "Result of dispatch was %ld", dispatch_handled );
							if( !dispatch_handled )
							{
#ifdef LOG_KEY_EVENTS
								lprintf( "IPC Local Keydefs Dispatch key : %p %08lx", hVideo, params[1] );
#endif
								if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, params[1] ) )
								{
#ifdef LOG_KEY_EVENTS
									lprintf( "IPC Global Keydefs Dispatch key : %08lx", params[1] );
#endif
									if( !HandleKeyEvents( KeyDefs, params[1] ) )
									{
										// previously this would then dispatch the key event...
										// but we want to give priority to handled keys.
									}
								}
							}
							params[1] = (uint32_t)(uintptr_t)DequeLink( &hVideo->pInput );
						} while( params[1] );
						hVideo->flags.key_dispatched = 0;
					}
					hVideo->flags.event_dispatched = 0;
				}
		}
		break;
	default:
		lprintf( "Got a unknown message %d with %d data", MsgID, paramlen );
		break;
	}
	return 0;
}


//----------------------------------------------------------------------------

void HandleDestroyMessage( PVIDEO hVidDestroy )
{
	{
		PVIDEO old_above;// , old_below;
#ifdef LOG_DESTRUCTION
		lprintf( "To destroy! %p %d", 0 /*Msg.lParam*/, hVidDestroy->hWndOutput );
#endif
		// hide the window! then it can't be focused or active or anything!
		//if( hVidDestroy->flags.key_dispatched ) // wait... we can't go away yet...
		//	return;
		//if( hVidDestroy->flags.event_dispatched ) // wait... we can't go away yet...
		//	return;
		//EnableWindow( hVidDestroy->hWndOutput, FALSE );
		old_above = hVidDestroy->pAbove;
		if( old_above )
		{
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "think setting focus to %p (%p %p)", old_above, hVidDestroy->hWndOutput, old_above->hWndOutput );
#endif
			hVidDestroy->pAbove = NULL;
			old_above->pBelow = hVidDestroy->pBelow;
			SafeSetFocus( old_above->hWndOutput );
#ifdef LOG_ORDERING_REFOCUS
			lprintf( "think did set focus to %p (%p)", old_above, old_above->hWndOutput );
#endif
		}
		else
		{
			if( !hVidDestroy->flags.bNoAutoFocus )
				SafeSetFocus( (HWND)GetDesktopWindow() );
		}
#ifdef LOG_DESTRUCTION
		lprintf( "------------ DESTROY! -----------" );
#endif
		if( hVidDestroy->hWndOutput )
 			DestroyWindow (hVidDestroy->hWndOutput);
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( "From destroy" );
#endif
	}
}
//----------------------------------------------------------------------------

static void HandleMessage (MSG Msg)
{
//	lprintf( "%d %d %d %d", Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam );
#ifdef USE_XP_RAW_INPUT
	//#if(_WIN32_WINNT >= 0x0501)
#define WM_INPUT								0x00FF
	//#endif /* _WIN32_WINNT >= 0x0501 */
	if( Msg.message ==  WM_INPUT )
	{
		UINT dwSize;
		WPARAM wParam = Msg.wParam;
		LPARAM lParam = Msg.lParam;
		lprintf( "Raw Input!" );
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
							 sizeof(RAWINPUTHEADER));
		{
			LPBYTE lpb = NewArray( BYTE, dwSize );
			if (lpb == NULL)
			{
				return;
			}

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,
									  sizeof(RAWINPUTHEADER)) != dwSize )
				OutputDebugString (TEXT("GetRawInputData doesn't return correct size !\n"));

			{
				RAWINPUT* raw = (RAWINPUT*)lpb;
				char szTempOutput[256];
				HRESULT hResult;

				if (raw->header.dwType == RIM_TYPEKEYBOARD)
				{
					/*
					snprintf(szTempOutput, sizeof( szTempOutput ), TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
											 raw->data.keyboard.MakeCode,
											 raw->data.keyboard.Flags,
											 raw->data.keyboard.Reserved,
											 raw->data.keyboard.ExtraInformation,
											 raw->data.keyboard.Message,
											 raw->data.keyboard.VKey);
											 lprintf(szTempOutput);
											 */
				}
				else if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					snprintf(szTempOutput,sizeof( szTempOutput ) , TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
											 raw->data.mouse.usFlags,
											 raw->data.mouse.ulButtons,
											 raw->data.mouse.usButtonFlags,
											 raw->data.mouse.usButtonData,
											 raw->data.mouse.ulRawButtons,
											 raw->data.mouse.lLastX,
											 raw->data.mouse.lLastY,
											 raw->data.mouse.ulExtraInformation);

											 lprintf(szTempOutput);

				}
			}
			ReleaseEx( lpb DBG_SRC );
		}
		DispatchMessage (&Msg);
	}

	else
#endif
		if (!Msg.hwnd && (Msg.message == (WM_USER_CREATE_WINDOW)))
	{
		PVIDEO hVidCreate = (PVIDEO) Msg.lParam;
#ifdef LOG_OPEN_TIMING
		//lprintf( "Message Create window stuff..." );
#endif
		CreateWindowStuffSizedAt (hVidCreate, hVidCreate->pWindowPos.x,
										  hVidCreate->pWindowPos.y,
										  hVidCreate->pWindowPos.cx,
										  hVidCreate->pWindowPos.cy);
	}
	else if (!Msg.hwnd && (Msg.message == (WM_USER_DESTROY_WINDOW)))
	{
		HandleDestroyMessage( (PVIDEO) Msg.lParam );
	}
	else if( Msg.message == (WM_USER_HIDE_WINDOW ) )
	{
		PVIDEO hVideo = ((PVIDEO)Msg.lParam);
#ifdef LOG_SHOW_HIDE
		lprintf( "Handling HIDE_WINDOW posted message %p",hVideo->hWndOutput );
#endif
		hVideo->flags.bHidden = 1;
		if( hVideo->pHideCallback )
			hVideo->pHideCallback( hVideo->dwHideData );
		//AnimateWindow( ((PVIDEO)Msg.lParam)->hWndOutput, 0, AW_HIDE );
		ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_HIDE );
#ifdef LOG_SHOW_HIDE
		lprintf( "Handled HIDE_WINDOW posted message %p",hVideo->hWndOutput );
#endif
		
	}
	else if( Msg.message == (WM_USER_SHOW_WINDOW) )
	{
		PVIDEO hVideo = (PVIDEO)Msg.lParam;
		if(  hVideo->flags.bDestroy )
			return;
#ifdef LOG_SHOW_HIDE
		lprintf( "Handling SHOW_WINDOW message! %p", Msg.lParam );
#endif
		//ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_RESTORE );
		//lprintf( "Handling SHOW_WINDOW message! %p", Msg.lParam );
		if( hVideo->flags.bTopmost )
		{
#ifdef LOG_SHOW_HIDE
			lprintf( "Got a posted show - set to topmost.." );
#endif
			SetWindowPos( hVideo->hWndOutput
				, HWND_TOPMOST
				, 0, 0, 0, 0,
				SWP_NOMOVE
				| SWP_NOSIZE
				);
		}
		if( hVideo->flags.bShown )
		{
			hVideo->flags.bRestoring = 1;
			ShowWindow( hVideo->hWndOutput, SW_RESTORE );
			hVideo->flags.bRestoring = 0;
		}
		else
		{
			hVideo->flags.bShown = 1;
			// pretend it's an invalidate to update happens.
#if DEBUG_INVALIDATE
			lprintf( "set Posted Invalidate  (previous:%d)", l.flags.bPostedInvalidate );
#endif
			AddLink( &l.invalidated_windows, hVideo );
			ShowWindow( hVideo->hWndOutput, SW_SHOW );
			if( hVideo->flags.bTopmost )
				SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							SWP_NOMOVE | SWP_NOSIZE);
		}
	}
	else if (Msg.message == WM_QUIT)
		l.bExitThread = TRUE;
	else
	{
		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}
}

//----------------------------------------------------------------------------
int internal;
static int CPROC ProcessDisplayMessages(void )
{
	MSG Msg;
	INDEX idx;
	PTHREAD thread;
	//lprintf( "Checking Thread %Lx", GetThreadID( MakeThread() ) );

	LIST_FORALL( l.threads, idx, PTHREAD, thread )
	{
		//lprintf( "is it %Lx?", GetThreadID( thread ) );
		if( IsThisThread( thread ) )
		{
			if (l.bExitThread)
				return -1;
			if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
			{
				//lprintf( "(E)Got message:%d", Msg.message );
				HandleMessage (Msg);
				if (l.bExitThread)
					return -1;
				return TRUE;
			}
			return FALSE;
		}
	}
	return -1;
}

#ifndef NO_TOUCH
HHOOK prochook;
LRESULT CALLBACK AllWndProc( int code, WPARAM wParam, LPARAM lParam )
{
	PCWPSTRUCT msg  = (PCWPSTRUCT)lParam;
	lprintf( "msg %p %d %d %p", msg->hwnd, msg->message, msg->wParam, msg->lParam );
	if( msg->message == WM_TOUCH )
	{
		lprintf( "TOUCH %d", msg->wParam );
	}

	return CallNextHookEx( prochook, code, wParam, lParam );
}
HHOOK get_prochook;
LRESULT CALLBACK AllGetWndProc( int code, WPARAM wParam, LPARAM lParam )
{
	MSG *msg  = (MSG*)lParam;
	lprintf( "msg %p %d %d %p %d"
		, msg->hwnd
		, msg->message
		, msg->wParam, msg->lParam, msg->time );
	if( msg->message == WM_TOUCH )
	{
		lprintf( "TOUCH %d", msg->wParam );
	}

	return CallNextHookEx( get_prochook, code, wParam, lParam );
}
#endif
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
#ifdef USE_KEYHOOK
#ifndef NO_TOUCH
	if( l.flags.bHookTouchEvents )
	{
		prochook = SetWindowsHookEx(
											 WH_CALLWNDPROC,
											 AllWndProc,
											 GetModuleHandle(TARGETNAME),
											 0);
		get_prochook = SetWindowsHookEx(
												  WH_CALLWNDPROC,
												  AllGetWndProc,
												  GetModuleHandle(TARGETNAME),
												  0);
	}
#endif

	if( l.flags.bUseLLKeyhook )
		AddLink( &l.ll_keyhooks,
				  SetWindowsHookEx (WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
										 ,GetModuleHandle(TARGETNAME), 0 /*GetCurrentThreadId()*/
										 ) );
	else
		AddLink( &l.keyhooks,
				  SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyHook
										, NULL /*GetModuleHandle(TARGETNAME)*/, GetCurrentThreadId()
										)
				 );
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
static void VideoLoadOptions( void )
{
	// don't set this anymore, the new option connection is same version as default
	//SetOptionDatabaseOption( option, TRUE );

#ifndef __NO_OPTIONS__
	PODBC option = NULL;//GetOptionODBC( NULL );
	l.flags.bLogMessages = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log messages", 0, TRUE );
	l.flags.bHookTouchEvents = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/use touch event", 0, TRUE );

	l.flags.bLogMouseEvents = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log mouse event", 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log key event", 0, TRUE );

	l.flags.bOptimizeHide = SACK_GetOptionIntEx( option, "SACK", "Video Render/Optimize Hide with SWP_NOCOPYBITS", 0, TRUE );
#ifndef NO_TRANSPARENCY
	if( l.UpdateLayeredWindow )
		l.flags.bLayeredWindowDefault = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Default windows are layered", 0, TRUE )?TRUE:FALSE;
	else
#endif
		l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogFocus =  SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Log Focus Changes", 0, TRUE );;
	l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Log Video Output", 0, TRUE );
	l.flags.bLogDisplayEnumTest = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Log Display Enumeration", 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Use Low Level Keyhook", 0, TRUE );
   l.flags.bDisableAutoDoubleClickFullScreen = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Disable full-screen mouse auto double-click", 0, TRUE );
   l.flags.bDoNotPreserveAspectOnFullScreen = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Do not preserve aspect streching full-screen", 0, TRUE );
	//DropOptionODBC( option );
#else
#  ifndef UNDER_CE
	if( l.UpdateLayeredWindowIndirect )
	{
		// ug - looks like I have to check if aero enabled?!
		l.flags.bOptimizeHide = 0;
	}
#  endif

#endif

}
//----------------------------------------------------------------------------


RENDER_PROC (int, InitDisplay) (void)
{
#ifndef __NO_WIN32API__
	WNDCLASS wc;
	if (!l.aClass)
	{
		InitializeCriticalSec( &l.csList );

		memset (&wc, 0, sizeof (WNDCLASS));
		wc.style = 0
#ifndef UNDER_CE
			 |	CS_OWNDC 
#endif
			 | CS_GLOBALCLASS
			 ;

		wc.lpfnWndProc = (WNDPROC) VideoWindowProc;
		wc.hInstance = (HINSTANCE)GetModuleHandle (TARGETNAME);
		if( wc.hInstance == NULL )
			wc.hInstance = (HMODULE)GetPrivateModuleHandle( TARGETNAME );
		if( !wc.hInstance )
			wc.hInstance = GetModuleHandle( NULL );
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
#ifdef __cplusplus
		wc.lpszClassName = "VideoOutputClass++";
#else
		wc.lpszClassName = "VideoOutputClass";
#endif
		wc.cbWndExtra = sizeof( uintptr_t );	// one extra pointer

		l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
			lprintf( "Failed to register class %s %d", wc.lpszClassName, GetLastError() );
			return FALSE;
		}

#ifdef __cplusplus
		wc.lpszClassName = "HiddenVideoOutputClass++";
#else
		wc.lpszClassName = "HiddenVideoOutputClass";
#endif
		wc.lpfnWndProc = (WNDPROC)VideoWindowProc2;
		l.aClass2 = RegisterClass (&wc);
		if (!l.aClass2)
		{
			lprintf( "Failed to register class %s %d", wc.lpszClassName, GetLastError() );
			return FALSE;
		}

	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------

#ifdef USE_KEYHOOK
PRIORITY_ATEXIT( RemoveKeyHook, 100 )
{
	// might get the release of the alt while wiating for networks to graceful exit.

	{
		HHOOK hook;
		INDEX idx;
		LIST_FORALL( l.keyhooks, idx, HHOOK, hook )
			UnhookWindowsHookEx (hook );
		LIST_FORALL( l.ll_keyhooks, idx, HHOOK, hook )
			UnhookWindowsHookEx (hook );
	}
	l.bExitThread = TRUE;
	if( l.dwThreadID )
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			d = PostThreadMessage (l.dwThreadID, WM_USER_SHUTDOWN, 0, 0);
			if (!d)
			{
				uint32_t error = GetLastError();
				if( error == ERROR_INVALID_THREAD_ID )
				{
					lprintf( "Have to skip waiting..." );
					PostQuitMessage( 0 );
					l.bThreadRunning = 0;
					break;
				}
				//Log1( "Failed to post shutdown message...%d", error );
				cnt--;
			}
			Relinquish();
		}
		while (!d && cnt);
		if (!d && ( cnt < 25 ) )
		{
			lprintf( "Tried %d times to post thread message... and it alwasy failed.", 25-cnt );
		}
		else
		{
			uint32_t start = timeGetTime() + 100;
			while( ( start > timeGetTime() )&&
				l.bThreadRunning )
			{
				Relinquish();
			}
			//if( l.bThreadRunning )
			//  lprintf( "Had to give up waiting for video thread to exit..." );
		}
	}
	//lprintf( "did we skip waking?" );
}
#endif

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
	if( target_display == - 1)
	{
		if( hVideo->flags.bFullScreen )
		{
			MoveSizeDisplay( hVideo, hVideo->pImage->x, hVideo->pImage->y, hVideo->pImage->width, hVideo->pImage->height );
		}
		hVideo->flags.bFullScreen = FALSE;
	}
	else
	{
		if( !hVideo->flags.bFullScreen )
		{

			hVideo->flags.bFullScreen = TRUE;
			hVideo->full_screen.target_display = target_display;
			GetDisplaySizeEx( target_display, &hVideo->full_screen.x, &hVideo->full_screen.y
					, &hVideo->full_screen.width, &hVideo->full_screen.height );
			{
				int w =  hVideo->pImage->width * hVideo->full_screen.width / hVideo->pImage->width;
				int h =  hVideo->pImage->height * hVideo->full_screen.width / hVideo->pImage->width;
				if( SUS_GT( h, int, hVideo->full_screen.height, uint32_t ) )
				{
					w =  hVideo->pImage->width * hVideo->full_screen.height / hVideo->pImage->height;
					h =  hVideo->pImage->height * hVideo->full_screen.height / hVideo->pImage->height;
				}
				if( l.flags.bDoNotPreserveAspectOnFullScreen )
					MoveSizeDisplay( hVideo
										, hVideo->full_screen.x// + (( hVideo->full_screen.width - w ) / 2)
										, hVideo->full_screen.y// + (( hVideo->full_screen.height - h ) / 2)
										, hVideo->full_screen.width
										, hVideo->full_screen.height );
            else
					MoveSizeDisplay( hVideo
										, hVideo->full_screen.x + (( hVideo->full_screen.width - w ) / 2)
										, hVideo->full_screen.y + (( hVideo->full_screen.height - h ) / 2)
										, w, h );
			}
		}
	}
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

static LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
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

RENDER_PROC (PVIDEO, MakeDisplayFrom) (HWND hWnd) 
{	
	PVIDEO hNextVideo;
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


RENDER_PROC (PVIDEO, OpenDisplaySizedAt) (uint32_t attr, uint32_t wx, uint32_t wy, int32_t x, int32_t y) // if native - we can return and let the messages dispatch...
{
	PVIDEO hNextVideo;
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

RENDER_PROC( void, SetDisplayNoMouse )( PVIDEO hVideo, int bNoMouse )
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

RENDER_PROC (PVIDEO, OpenDisplayAboveSizedAt) (uint32_t attr, uint32_t wx, uint32_t wy,
															  int32_t x, int32_t y, PVIDEO parent)
{
	PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
	if (parent)
		PutDisplayAbove (newvid, parent);
	return newvid;
}

RENDER_PROC (PVIDEO, OpenDisplayAboveUnderSizedAt) (uint32_t attr, uint32_t wx, uint32_t wy,
															  int32_t x, int32_t y, PVIDEO parent, PVIDEO barrier)
{
	PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
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

RENDER_PROC (void, CloseDisplay) (PVIDEO hVideo)
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

RENDER_PROC (void, SizeDisplay) (PVIDEO hVideo, uint32_t w, uint32_t h)
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

RENDER_PROC (void, SizeDisplayRel) (PVIDEO hVideo, int32_t delw, int32_t delh)
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

RENDER_PROC (void, MoveDisplay) (PVIDEO hVideo, int32_t x, int32_t y)
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

RENDER_PROC (void, MoveDisplayRel) (PVIDEO hVideo, int32_t x, int32_t y)
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

RENDER_PROC (void, MoveSizeDisplay) (PVIDEO hVideo, int32_t x, int32_t y, int32_t w,
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

RENDER_PROC (void, MoveSizeDisplayRel) (PVIDEO hVideo, int32_t delx, int32_t dely,
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

RENDER_PROC (void, UpdateDisplayEx) (PVIDEO hVideo DBG_PASS )
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

RENDER_PROC (void, SetMousePosition) (PVIDEO hVid, int32_t x, int32_t y)
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

RENDER_PROC (void, SetCloseHandler) (PVIDEO hVideo,
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

RENDER_PROC (void, SetMouseHandler) (PVIDEO hVideo,
												 MouseCallback pMouseCallback,
												 uintptr_t dwUser)
{
	hVideo->dwMouseData = dwUser;
	hVideo->pMouseCallback = pMouseCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetHideHandler) (PVIDEO hVideo,
												 HideAndRestoreCallback pHideCallback,
												 uintptr_t dwUser)
{
	hVideo->dwHideData = dwUser;
	hVideo->pHideCallback = pHideCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRestoreHandler) (PVIDEO hVideo,
												 HideAndRestoreCallback pRestoreCallback,
												 uintptr_t dwUser)
{
	hVideo->dwRestoreData = dwUser;
	hVideo->pRestoreCallback = pRestoreCallback;
}

//----------------------------------------------------------------------------
#ifndef NO_TOUCH
RENDER_PROC (void, SetTouchHandler) (PVIDEO hVideo,
												 TouchCallback pTouchCallback,
												 uintptr_t dwUser)
{
	hVideo->dwTouchData = dwUser;
	hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------
#ifndef NO_PEN
RENDER_PROC( void, SetPenHandler )( PVIDEO hVideo, PenCallback pPenCallback, uintptr_t dwUser ) {
	hVideo->dwPenData    = dwUser;
	hVideo->pPenCallback = pPenCallback;
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRedrawHandler) (PVIDEO hVideo,
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

RENDER_PROC (void, SetKeyboardHandler) (PVIDEO hVideo, KeyProc pKeyProc,
													 uintptr_t dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetLoseFocusHandler) (PVIDEO hVideo,
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

RENDER_PROC (void, SetRendererTitle) (PVIDEO hVideo, const TEXTCHAR *pTitle)
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

RENDER_PROC (void, MakeTopmost) (PVIDEO hVideo)
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

RENDER_PROC (void, MakeAbsoluteTopmost) (PVIDEO hVideo)
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

RENDER_PROC( int, IsTopmost )( PVIDEO hVideo )
{
	return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, HideDisplay) (PVIDEO hVideo)
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
void RestoreDisplay( PVIDEO hVideo  )
{
	RestoreDisplayEx( hVideo DBG_SRC );
}


void RestoreDisplayEx(PVIDEO hVideo DBG_PASS )
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
			Deallocate( char*, teststring );
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

RENDER_PROC (void, GetDisplayPosition) (PVIDEO hVid, int32_t * x, int32_t * y,
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
RENDER_PROC (LOGICAL, DisplayIsValid) (PVIDEO hVid)
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

RENDER_PROC (ImageFile *,GetDisplayImage )(PVIDEO hVideo)
{
	return hVideo->pImage;
}

//----------------------------------------------------------------------------

RENDER_PROC (PKEYBOARD, GetDisplayKeyboard) (PVIDEO hVideo)
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

RENDER_PROC (void, OwnMouseEx) (PVIDEO hVideo, uint32_t own DBG_PASS)
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
	RENDER_PROC (HWND, GetNativeHandle) (PVIDEO hVideo)
	{
		return hVideo->hWndOutput;
	}

RENDER_PROC (int, BeginCalibration) (uint32_t nPoints)
{
	return 1;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, SyncRender)( PVIDEO hVideo )
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
RENDER_PROC (void, UpdateDisplay) (PVIDEO hVideo )
{
	//DebugBreak();
	UpdateDisplayEx( hVideo DBG_SRC );
}

RENDER_PROC (void, DisableMouseOnIdle) (PVIDEO hVideo, LOGICAL bEnable )
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


RENDER_PROC( void, SetDisplayFade )( PVIDEO hVideo, int level )
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
	return TRUE;
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
#ifndef NO_PEN
       , SetPenHandler
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

LOGICAL IsDisplayHidden( PVIDEO video )
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
