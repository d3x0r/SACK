/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *    the first delay is between CreateWindow and WM_NCCREATE
 *    the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/



//#define _OPENGL_ENABLED
/* this must have been done for some other collision in some other bit of code...
 * probably the update queue? the mosue queue ?
 */
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS

#ifdef UNDER_CE
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#define USE_KEYHOOK
#endif

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
//#define OTHER_EVENTS_HERE
//#define LOG_SHOW_HIDE
//#define LOG_DISPLAY_RESIZE
//#define NOISY_LOGGING
// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS
//#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT
#include <vidlib/vidstruc.h>
#include "VidLib.H"

#include "Keybrd.h"

static int stop;
//HWND     hWndLastFocus;

// commands to the video thread for non-windows native ....
#define CREATE_VIEW 1

#define WM_USER_CREATE_WINDOW  WM_USER+512
#define WM_USER_DESTROY_WINDOW  WM_USER+513
#define WM_USER_SHOW_WINDOW  WM_USER+514
#define WM_USER_HIDE_WINDOW  WM_USER+515
#define WM_USER_SHUTDOWN     WM_USER+516
#define WM_USER_MOUSE_CHANGE     WM_USER+517

IMAGE_NAMESPACE

struct saved_location
{
	S_32 x, y;
	_32 w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
	Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h );
	PTRSZVAL psv;
};
IMAGE_NAMESPACE_END

RENDER_NAMESPACE

// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

extern KEYDEFINE KeyDefs[];

// forward declaration - staticness will probably cause compiler errors.
static int CPROC ProcessDisplayMessages(void );

//----------------------------------------------------------------------------

/* register and track drag file acceptors - per video surface... attched to hvdieo structure */
struct dropped_file_acceptor_tag {
	dropped_file_acceptor f;
	PTRSZVAL psvUser;
};

void WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
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
		if( strcmp( classname, WIDE("VideoOutputClass") ) == 0 )
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
	_lprintf(DBG_RELAY)( WIDE("bottommost") );
	while( base )
	{
		TEXTCHAR title[256];
		TEXTCHAR classname[256];
		GetClassName( base->hWndOutput, classname, sizeof( classname ) );
		GetWindowText( base->hWndOutput, title, sizeof( title ) );
		if( base == hVideo )
			_lprintf(DBG_RELAY)( WIDE( "--> %p[%p] %s" ), base, base->hWndOutput, title );
		else
			_lprintf(DBG_RELAY)( WIDE( "    %p[%p] %s" ), base, base->hWndOutput, title );
      base = base->pBelow;
	}
	lprintf( WIDE( "topmost" ) );
#endif
}

void DumpChainAbove( PVIDEO chain, HWND hWnd )
{
#ifndef UNDER_CE
	int not_mine = 0;
	TEXTCHAR title[256];
   GetWindowText( hWnd, title, sizeof( title ) );
	lprintf( WIDE( "Dumping chain of windows above %p %s" ), hWnd, title );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDPREV ) )
	{
		int ischain;
		TEXTCHAR title[256];
		TEXTCHAR classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		GetWindowText( hWnd, title, sizeof( title ) );
		if( ischain = InMyChain( chain, hWnd ) )
		{
			lprintf( WIDE( "%s %p %s %s" ), ischain==2?WIDE( ">>>" ):WIDE( "^^^" ), hWnd, title, classname );
			not_mine = 0;
		}
		else
		{
			not_mine++;
			if( not_mine < 10 )
				lprintf( WIDE( "... %p %s %s" ), hWnd, title, classname );
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
	_lprintf(DBG_RELAY)( WIDE( "Dumping chain of windows below %p" ), hWnd );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDNEXT ) )
	{
		int ischain;
		TEXTCHAR classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		GetWindowText( hWnd, title, sizeof( title ) );
		if( ischain = InMyChain( chain, hWnd ) )
		{
			lprintf( WIDE( "%s %p %s %s" ), ischain==2?WIDE( ">>>" ):WIDE( "^^^" ), hWnd, title, classname );
			not_mine = 0;
		}
		else
		{
			not_mine++;
			if( not_mine < 10 )
				lprintf( WIDE( "... %p %s %s" ), hWnd, title, classname );
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
   //lprintf( WIDE( "Safe Set Focus %p" ), hWndSetTo );
	SetFocus( hWndSetTo );
	SetActiveWindow( hWndSetTo );
	SetForegroundWindow( hWndSetTo );

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
void IssueUpdateLayeredEx( PVIDEO hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
								if( l.UpdateLayeredWindow )
								{
									SIZE size;// = new Win32.Size(bitmap.Width, bitmap.Height);

									static POINT pointSource;// = new Win32.Point(0, 0);
									POINT topPos;/// = new Win32.Point(Left, Top);
									// DEFINED in WinGDI.h
									BLENDFUNCTION blend;// = new Win32.BLENDFUNCTION();
									blend.BlendOp             = AC_SRC_OVER;
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
									blend.BlendFlags          = 0;
									blend.AlphaFormat         = AC_SRC_ALPHA;
									size.cx = hVideo->pWindowPos.cx;
									size.cy = hVideo->pWindowPos.cy;
									topPos.x = hVideo->pWindowPos.x;
									topPos.y = hVideo->pWindowPos.y;
									// no way to specify just the x/y w/h of the portion we want
									// to update...
									if( l.flags.bLogWrites )
										lprintf( WIDE( "layered... begin update. %d %d %d %d" ), size.cx, size.cy, topPos.x, topPos.y );
									if( bContent
										&& l.UpdateLayeredWindowIndirect
										&& ( x || y || w != hVideo->pWindowPos.cx || h != hVideo->pWindowPos.cy ) )
									{
										// this is Vista+ function.
										RECT rc_dirty;
										UPDATELAYEREDWINDOWINFO ULWInfo;
										rc_dirty.top = y>=topPos.y?y:topPos.y;
										rc_dirty.left = x>=topPos.x?x:topPos.x;
										rc_dirty.right = x + w;
										rc_dirty.bottom = y + h;
										ULWInfo.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
										ULWInfo.hdcDst = bContent?hVideo->hDCOutput:NULL;
										ULWInfo.pptDst = bContent?&topPos:NULL;
										ULWInfo.psize = bContent?&size:NULL;
										ULWInfo.hdcSrc = bContent?hVideo->hDCBitmap:NULL;
										ULWInfo.pptSrc = bContent?&pointSource:NULL;
										ULWInfo.crKey = 0;
										ULWInfo.pblend = &blend;
                              ULWInfo.dwFlags = ULW_ALPHA;
										ULWInfo.prcDirty = bContent?&rc_dirty:NULL;
//#ifdef LOG_RECT_UPDATE
										if( l.flags.bLogWrites )
											_lprintf(DBG_RELAY)( WIDE( "using indirect (with dirty rect %d %d %d %d)" ), x, y, w, h );
//#endif
										if( !l.UpdateLayeredWindowIndirect( hVideo->hWndOutput, &ULWInfo ) )
											lprintf( WIDE( "Error using UpdateLayeredWindowIndirect: %d" ), GetLastError() );
									}
									else
									{
                              // we CAN do indirect...
										l.UpdateLayeredWindow(
																	 hVideo->hWndOutput
																	, bContent?(HDC)hVideo->hDCOutput:NULL
																	, bContent?&topPos:NULL
																	, bContent?&size:NULL
																	, bContent?hVideo->hDCBitmap:NULL
																	, bContent?&pointSource:NULL
																	, 0 // color key
																	, &blend
																	, ULW_ALPHA);
									}
									//lprintf( WIDE( "layered... end update." ) );
								}
}
#endif

//----------------------------------------------------------------------------
RENDER_PROC (void, EnableLoggingOutput)( LOGICAL bEnable )
{
   l.flags.bLogWrites = bEnable;
}

RENDER_PROC (void, UpdateDisplayPortionEx)( PVIDEO hVideo
                                          , S_32 x, S_32 y
                                          , _32 w, _32 h DBG_PASS)
{
   ImageFile *pImage;
   if (hVideo
       && (pImage = hVideo->pImage) && hVideo->hDCBitmap && hVideo->hDCOutput)
	{
#ifdef LOG_RECT_UPDATE
		lprintf( WIDE( "Entering from %s(%d)" ), pFile, nLine );
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
			_xlprintf( 1 DBG_RELAY )( WIDE("Write to Window: %p %d %d %d %d"), hVideo, x, y, w, h );

      if (!hVideo->flags.bShown)
		{
			if( l.flags.bLogWrites )
				lprintf( WIDE("Setting shown...") );
			hVideo->flags.bShown = TRUE;
			hVideo->flags.bHidden = FALSE;
			if( hVideo->pRestoreCallback )
				hVideo->pRestoreCallback( hVideo->dwRestoreData );

			if( l.flags.bLogWrites )
				_xlprintf( 1 DBG_RELAY )( WIDE("Show Window: %d %d %d %d"), x, y, w, h );
			//lprintf( "During an update showing window... %p", hVideo );
				if( hVideo->flags.bTopmost )
				{
#ifdef LOG_ORDERING_REFOCUS

			//DumpMyChain( hVideo );
			//DumpChainBelow( hVideo, hVideo->hWndOutput );
			//DumpChainAbove( hVideo, hVideo->hWndOutput );
			lprintf( WIDE( "Putting window above ... %p %p" ), hVideo->pBelow?hVideo->pBelow->hWndOutput:NULL
							, GetNextWindow( hVideo->hWndOutput, GW_HWNDPREV ) );
			lprintf( WIDE( "Putting should be below ... %p %p" ), hVideo->pAbove?hVideo->pAbove->hWndOutput:NULL, GetNextWindow( hVideo->hWndOutput, GW_HWNDNEXT ) );
#endif
			if( hVideo->flags.bTopmost )
			{
				//lprintf( WIDE( "Initial setup of window -> topmost.... %p" ), HWND_TOPMOST );
				hVideo->pWindowPos.hwndInsertAfter = HWND_TOPMOST; // otherwise we get '0' as our desired 'after'
			}

					SetWindowPos (hVideo->hWndOutput
								 , hVideo->flags.bTopmost?HWND_TOPMOST:HWND_TOP
								 , 0, 0, 0, 0,
								  SWP_NOMOVE
								  | SWP_NOSIZE
								  | SWP_NOACTIVATE
								 );
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
							lprintf( WIDE( "is out of position, attempt to correct during show. Hope we can test this@@" ) );
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
							lprintf( WIDE( "Okay it's already in the right zorder.." ) );
#endif
							hVideo->flags.bIgnoreChanging = 1;
							// the show window generates WM_PAINT, and MOVES THE WINDOW.
							// the window is already in the right place, and knows it.
						}
						if(l.flags.bLogWrites)lprintf( WIDE( "Show WIndow NO Activate... and..." ) );

						ShowWindow (hVideo->hWndOutput, SW_SHOWNA );
						if(l.flags.bLogWrites)lprintf( WIDE( "show window done... so..." ) );
					}
					else
					{
#ifdef LOG_OPEN_TIMING
						lprintf( WIDE( "Showing window..." ) );
#endif

						// SW_shownormal does extra stuff, that I think causes top level windows to fall behind other
                  // topmost windows.
#ifdef UNDER_CE
						ShowWindow (hVideo->hWndOutput, SW_SHOWNORMAL );
#else
						ShowWindow (hVideo->hWndOutput, SW_SHOW );
#endif

#ifdef LOG_OPEN_TIMING
						lprintf( WIDE( "window shown..." ) );
#endif
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE( "Having shown window... should have had a no-event paint ?" ) );
#endif
					}

					if( hVideo->flags.bReady && hVideo->flags.bShown && hVideo->pRedrawCallback )
					{
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
						SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
#else
						//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#endif
						if( l.flags.bLogWrites )
							lprintf( WIDE( "Bitblit out..." ) );
						BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
						  (HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
					}
					// show will generate a paint...
					// and on the paint we will draw, otherwise
					// we'll do it twice.
					//if( bCreatedhWndInstance )
					if( !hVideo->pAbove && !hVideo->flags.bNoAutoFocus )
					{
#ifdef NOISY_LOGGING
						lprintf( WIDE( "Foregrounding?" ) );
#endif
						//SetForegroundWindow (hVideo->hWndOutput);
						// these things should invoke losefocus method messages...
#ifdef NOISY_LOGGING
						lprintf( WIDE( "focusing?" ) );
#endif
						SafeSetFocus( hVideo->hWndOutput );
						if( hVideo->pRedrawCallback )
							hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);

					}
		} // end of if(!shown) else shown....
		else
		{
#ifdef LOG_RECT_UPDATE
			_xlprintf( 1 DBG_RELAY )( WIDE("Draw to Window: %d %d %d %d"), x, y, w, h );
#endif
			if( hVideo->flags.bHidden )
				return;

			{
				PTHREAD thread;

				{
					INDEX idx;
					LIST_FORALL( l.threads, idx, PTHREAD, thread )
					{
						// okay if it's layered, just let the draws through always.
						if( (hVideo->flags.bLayeredWindow) || IsThisThread( thread ) || ( x || y ) )
						{
							if( hVideo->flags.bOpenGL )
								if( l.actual_thread != thread )
                            continue;
							EnterCriticalSec( &hVideo->cs );
							if( hVideo->flags.bDestroy )
							{
								//lprintf( "Saving ourselves from operating a draw while destroyed." );
								// by now we could be in a place where we've been destroyed....
								LeaveCriticalSec( &hVideo->cs );
								return;
							}
#ifdef LOG_RECT_UPDATE
							lprintf( WIDE( "Good thread..." ) ); /* can't be? */
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
								_32 _w, _h;
								PSPRITE_METHOD psm;
								// set these to set clipping for sprite routine
								hVideo->pImage->x = x;
								hVideo->pImage->y = y;
								_w = hVideo->pImage->width;
								hVideo->pImage->width = w;
								_h = hVideo->pImage->height;
								hVideo->pImage->height = h;
#ifdef DEBUG_TIMING
								lprintf( WIDE( "Save screen..." ) );
#endif
								LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
								{
									if( ( psm->original_surface->width != psm->renderer->pImage->width ) ||
										( psm->original_surface->height != psm->renderer->pImage->height ) )
									{
										UnmakeImageFile( psm->original_surface );
										psm->original_surface = MakeImageFile( psm->renderer->pImage->width, psm->renderer->pImage->height );
									}
									//lprintf( WIDE( "save Sprites" ) );
									BlotImageSized( psm->original_surface, psm->renderer->pImage
										, x, y, w, h );
#ifdef DEBUG_TIMING
									lprintf( WIDE( "Render sprites..." ) );
#endif
									if( psm->RenderSprites )
									{
										// if I exported the PSPRITE_METHOD structure to the image library
										// then it could itself short circuit the drawing...
										//lprintf( WIDE( "render Sprites" ) );
										psm->RenderSprites( psm->psv, hVideo, x, y, w, h );
									}
#ifdef DEBUG_TIMING
									lprintf( WIDE( "Done render sprites..." ));
#endif
								}
#ifdef DEBUG_TIMING
								lprintf( WIDE( "Done save screen and update spritess..." ) );
#endif
								hVideo->pImage->x = 0;
								hVideo->pImage->y = 0;
								hVideo->pImage->width = _w;
								hVideo->pImage->height = _h;
							}

							if( l.flags.bLogWrites )
								lprintf( WIDE( "Output %d,%d %d,%d" ), x, y, w, h);

#ifndef NO_TRANSPARENCY
							if( hVideo->flags.bLayeredWindow )
							{
								IssueUpdateLayeredEx( hVideo, TRUE, x, y, w, h DBG_SRC );
							}
							else
#endif
							{
								//lprintf( "non layered... begin update." );
								BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
										  (HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
								//lprintf( WIDE( "non layered... end update." ) );
							}
							if( hVideo->sprites )
							{
								INDEX idx;
								PSPRITE_METHOD psm;
								struct saved_location location;
#ifdef DEBUG_TIMING 
								lprintf( WIDE( "Restore Original" ) );
#endif
								LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
								{
									//BlotImage( psm->renderer->pImage, psm->original_surface
									//			, 0, 0 );
									while( DequeData( &psm->saved_spots, &location ) )
									{
										// restore saved data from image to here...
										//lprintf( WIDE( "Restore %d,%d %d,%d" ), location.x, location.y
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
								lprintf( WIDE( "Restored Original" ) );
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
					if( l.flags.bPostedInvalidate )
					{
						//lprintf( WIDE( "saving from double posting... still processing prior update." ) );
						return;
					}

					r.left = x;
					r.top = y;
					r.right = r.left + w;
					r.bottom = r.top + h;
					//_lprintf(DBG_RELAY)( WIDE( "Posting invalidation rectangle command thing..." ) );
					if( hVideo->flags.event_dispatched )
					{
#ifdef LOG_RECT_UPDATE
						lprintf( WIDE( "No... saving the update... already IN an update..." ) );
#endif
					}
					else
					{
						if( l.flags.bLogWrites )
							lprintf( WIDE( "setting post invalidate..." ) );
						l.flags.bPostedInvalidate = 1;
						InvalidateRect( hVideo->hWndOutput, &r, FALSE );
					}
				}
			}
		}
	}
	else
	{
		//lprintf( WIDE("Rendering surface is not able to be updated (no surface, hdc, bitmap, etc)") );
	}
#if defined( LOG_ORDERING_REFOCUS )
	lprintf( WIDE( "Done with UpdateDisplayPortionEx()" ) );
#endif
}

//----------------------------------------------------------------------------

void
UnlinkVideo (PVIDEO hVideo)
{
	// yes this logging is correct, to say what I am below, is to know what IS above me
   // and to say what I am above means I nkow what IS below me
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( " -- UNLINK Video %p from which is below %p and above %p" ), hVideo, hVideo->pAbove, hVideo->pBelow );
#endif
	//if( hVideo->pBelow || hVideo->pAbove )
   //   DebugBreak();
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
   //   hVideo->pPrior->pNext = hVideo->pNext;

   hVideo->pPrior = hVideo->pNext = hVideo->pAbove = hVideo->pBelow = NULL;
}

//----------------------------------------------------------------------------

void
FocusInLevel (PVIDEO hVideo)
{
	lprintf( WIDE( "Focus IN level" ) );
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
		else        // nothing points to this - therefore we must find the start
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
	lprintf( WIDE( "Begin Put Display Above..." ) );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();
	while( topmost->pBelow )
		topmost = topmost->pBelow;

#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "(actual)Put display above ... %p %p  [%p %p]" )
		, hVideo, topmost
			 , hVideo?hVideo->hWndOutput:NULL
			 , topmost?topmost->hWndOutput: NULL );

	lprintf( WIDE( "hvideo is above %p and below %p" )
			 , original_above?original_above->hWndOutput:NULL
			 , original_below?original_below->hWndOutput:NULL );
	if( hAbove )
		lprintf( WIDE( "hAbove is below %p and above %p" )
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
		lprintf( WIDE( "Putting the above within the list of hVideo... reverse-insert." ) );
		//DumpMyChain( hVideo );
#endif
		if( hAbove->pAbove = hVideo->pAbove )
			hVideo->pAbove->pBelow = hAbove;

		hAbove->pBelow = hVideo;
		hVideo->pAbove = hAbove;

#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "(--- new chain, after linking---)" ) );
		//DumpMyChain( hVideo );
		// hVideo->below stays the same.
		lprintf( WIDE( "Two starting windows %p %p" ),hVideo->hWndOutput
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
		lprintf( WIDE( "------- DONE WITH PutDisplayAbove reorder.." ) );
#endif
		LeaveCriticalSec( &l.csList );
		return;
	}
	{

		EnterCriticalSec( &l.csList );
		UnlinkVideo (hVideo);      // make sure it's isolated...

		if( ( hVideo->pAbove = topmost ) )
		{
			//HWND hWndOver = GetNextWindow( topmost->hWndOutput, GW_HWNDPREV );
			if( hVideo->pBelow = topmost->pBelow )
			{
				hVideo->pBelow->pAbove = hVideo;
			}
			topmost->pBelow = hVideo;
#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE( "Only thing here is... %p after %p or %p so use %p means...below?" )
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
			lprintf( WIDE( "Finished ordering..." ) );
			//DumpChainBelow( hVideo, hVideo->hWndOutput );
			//DumpChainAbove( hVideo, hVideo->hWndOutput );
#endif
		}
#ifdef LOG_ORDERING_REFOCUS
		//lprintf(WIDE( "Put Display Above (this)%p above (below)%p and before %p" ), hVideo->hWndOutput, hAbove->hWndOutput, hAbove->pWindowPos.hwndInsertAfter );
#endif
		LeaveCriticalSec( &l.csList );
	}
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "End Put Display Above..." ) );
#endif
}

RENDER_PROC (void, PutDisplayIn) (PVIDEO hVideo, PVIDEO hIn)
{
	lprintf( WIDE( "Relate hVideo as a child of hIn..." ) );
}

//----------------------------------------------------------------------------

// will remake the image (uses same image structure, swaps bitmap data)
Image CreateImageDIB( _32 w, _32 h, Image original, HBITMAP *phBM, HDC *pHDC)
{
	BITMAPINFO bmInfo;
	HBITMAP tmp;
	Image result;
	HDC tmpdc;
	if( !phBM )
		phBM = &tmp;
	if( !pHDC )
		pHDC = &tmpdc;

	bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = w; // size of window...
	bmInfo.bmiHeader.biHeight = h;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;   // 24, 16, ...
	bmInfo.bmiHeader.biCompression = BI_RGB;
	bmInfo.bmiHeader.biSizeImage = 0;   // zero for BI_RGB
	bmInfo.bmiHeader.biXPelsPerMeter = 0;
	bmInfo.bmiHeader.biYPelsPerMeter = 0;
	bmInfo.bmiHeader.biClrUsed = 0;
	bmInfo.bmiHeader.biClrImportant = 0;
	{
		PCOLOR pBuffer;
		(*phBM) = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer, NULL,   // hVideo (hMemView)
											 0); // offset DWORD multiple
		//lprintf( WIDE("New drawing surface, remaking the image, dispatch draw event...") );
		if (!(*phBM))
		{
			//DWORD dwError = GetLastError();
			// this is normal if window minimizes...
			if (bmInfo.bmiHeader.biWidth || bmInfo.bmiHeader.biHeight)  // both are zero on minimization
			{
				lprintf( WIDE( "Failed to create image %d,%d" ), w, h );
				MessageBox (NULL, WIDE("Failed to create Window DIB"),
								WIDE("ERROR"), MB_OK);
			}
			return NULL;
		}
		//lprintf( "Remake Image to %p %dx%d", pBuffer, bmInfo.bmiHeader.biWidth,
		//					 bmInfo.bmiHeader.biHeight );
		result =
			RemakeImage ( original, pBuffer, bmInfo.bmiHeader.biWidth,
							 bmInfo.bmiHeader.biHeight);
	}
	return result;
}

BOOL CreateDrawingSurface (PVIDEO hVideo)
{
	HBITMAP hBmNew = NULL;
	// can use handle from memory allocation level.....
	if (!hVideo)         // wait......
		return FALSE;

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

		//lprintf( WIDE( "made image %d,%d" ), r.right, r.bottom );
		bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth = r.right; // size of window...
		bmInfo.bmiHeader.biHeight = r.bottom;
		bmInfo.bmiHeader.biPlanes = 1;
		bmInfo.bmiHeader.biBitCount = 32;   // 24, 16, ...
		bmInfo.bmiHeader.biCompression = BI_RGB;
		bmInfo.bmiHeader.biSizeImage = 0;   // zero for BI_RGB
		bmInfo.bmiHeader.biXPelsPerMeter = 0;
		bmInfo.bmiHeader.biYPelsPerMeter = 0;
		bmInfo.bmiHeader.biClrUsed = 0;
		bmInfo.bmiHeader.biClrImportant = 0;

		hBmNew = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer, NULL,   // hVideo (hMemView)
											0); // offset DWORD multiple

		if (!hBmNew)
		{
			// but this will leave our prior bitmap selected... so we should be able to live after this.

			//DWORD dwError = GetLastError();
			// this is normal if window minimizes...
			if (bmInfo.bmiHeader.biWidth || bmInfo.bmiHeader.biHeight)  // both are zero on minimization
	            MessageBox (hVideo->hWndOutput, WIDE("Failed to create Window DIB"),
                        WIDE("ERROR"), MB_OK);
			return FALSE;
		}
		//lprintf( "Remake Image to %p %dx%d", pBuffer, bmInfo.bmiHeader.biWidth,
		//		  bmInfo.bmiHeader.biHeight );
		hVideo->pImage =
			RemakeImage( hVideo->pImage, pBuffer, bmInfo.bmiHeader.biWidth,
							 bmInfo.bmiHeader.biHeight);
	}
	if (!hVideo->hDCBitmap) // first time ONLY...
		hVideo->hDCBitmap = CreateCompatibleDC ((HDC)hVideo->hDCOutput);

	if (hVideo->hBm && hVideo->hWndOutput)
	{
		// if we had an old one, we'll want to delete it.
		if (SelectObject( (HDC)hVideo->hDCBitmap, hBmNew ) != hVideo->hBm)
		{
			Log (WIDE( "Hmm Somewhere we lost track of which bitmap is selected?! bitmap resource not released" ));
		}
		else
		{
			// delete the prior one, we have a new one.
			DeleteObject (hVideo->hBm);
		}
	}
	else // first time through hBm will be NULL... so we save the original bitmap for the display.
		hVideo->hOldBitmap = SelectObject ((HDC)hVideo->hDCBitmap, hBmNew);

	// okay and now this is the bitmap to use for output
	hVideo->hBm = hBmNew;


	if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		{
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			lprintf( WIDE( "Posting invalidate rect..." ) );
			InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#else
			SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
#endif
		}
	}
   //lprintf( WIDE( "And here I might want to update the video, hope someone else does for me." ) );
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
			Log (WIDE( "Don't think we deselected the bm right" ));
		}
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCBitmap);
		if (!DeleteObject (hVideo->hBm))
		{
			Log (WIDE( "Yup this bitmap is expanding..." ));
		}
		ReleaseEx( hVideo->pTitle DBG_SRC );
		DestroyKeyBinder( hVideo->KeyDefs );
		//hVideo->pImage->image = NULL; // cheat this out for now...
		// Image library tracks now that someone else gave it memory
		// and it does not deallocate something it didn't allocate...
		UnmakeImageFile (hVideo->pImage);

#ifdef LOG_DESTRUCTION
		lprintf( WIDE( "In DoDestroy, destroyed a good bit already..." ) );
#endif

		// this will be cleared at the next statement....
		// which indicates we will be ready to be released anyhow...
		//hVideo->flags.bReady = FALSE;
		// unlink from the stack of windows...
		UnlinkVideo (hVideo);
		if( l.hCaptured == hVideo )
			l.hCaptured = NULL;
		//Log (WIDE( "Cleared hVideo - is NOW !bReady" ));
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
   KeyHook (int code,      // hook code
				WPARAM wParam,    // virtual-key code
				LPARAM lParam     // keystroke-message information
			  )
{
	if( l.flags.bLogKeyEvent )
		lprintf( WIDE( "KeyHook %d %08lx %08lx" ), code, wParam, lParam );
	{
		int dispatch_handled = 0;
		PVIDEO hVid;
		int key, scancode, keymod = 0;
		HWND hWndFocus = GetFocus ();
		HWND hWndFore = GetForegroundWindow();
		ATOM aThisClass;
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
							lprintf( WIDE( "skipping thread %p" ), check_thread );
						}
				}
			}
			return 0;
		}
		if( l.flags.bLogKeyEvent )
		{
			lprintf( WIDE( "%x Received key to %p %p" ), GetCurrentThreadId(), hWndFocus, hWndFore );
			lprintf( WIDE( "Received key %d %08x %08x" ), code, wParam, lParam );
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
							lprintf( WIDE( "Chained to next hook..." ) );
						}
						return CallNextHookEx ( (HHOOK)GetLink( &l.keyhooks, idx ), code, wParam, lParam);
					}
				}
			}
		}
		if( l.flags.bLogKeyEvent )
			lprintf( WIDE("Keyhook mesasage... %08x %08x"), wParam, lParam );
		//lprintf( WIDE("hWndFocus is %p"), hWndFocus );
		if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			if( l.flags.bLogKeyEvent )
				lprintf( WIDE("hwndfocus is something...") );
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
			lprintf( WIDE("hvid is %p"), hVid );
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
				| ((lParam & 0x1000000) >> 16)   // extended key
				| (lParam & 0x80FF0000) // repeat count? and down status
				^ (0x80000000) // not's the single top bit... becomes 'press'
				;
			// lparam MSB is keyup status (strange)

			if (key & 0x80000000)   // test keydown...
			{
				l.kbd.key[wParam & 0xFF] |= 0x80;   // set this bit (pressed)
				l.kbd.key[wParam & 0xFF] ^= 1;   // toggle this bit...
			}
			else
			{
				l.kbd.key[wParam & 0xFF] &= ~0x80;  //(unpressed)
			}
         //lprintf( WIDE("Set local keyboard %d to %d"), wParam& 0xFF, l.kbd.key[wParam&0xFF]);
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
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			{
				PVIDEO hVideo = hVid;
				if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				{
					if( hVideo && hVideo->pKeyProc )
					{
						hVideo->flags.event_dispatched = 1;
						if( l.flags.bLogKeyEvent )
							lprintf( WIDE("Dispatched KEY!") );
						if( hVideo->flags.key_dispatched )
						{
							if( l.flags.bLogKeyEvent )
								lprintf( WIDE( "already dispatched, delay it." ) );
							EnqueLink( &hVideo->pInput, (POINTER)key );
						}
						else
						{
							hVideo->flags.key_dispatched = 1;
							do
							{
								if( l.flags.bLogKeyEvent )
									lprintf( WIDE( "Dispatching key %08lx" ), key );
								if( keymod & 6 )
									if( HandleKeyEvents( KeyDefs, key )  )
									{
										lprintf( WIDE( "Sent global first." ) );
										dispatch_handled = 1;
									}

								if( !dispatch_handled )
								{
									// previously this would then dispatch the key event...
									// but we want to give priority to handled keys.
									dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, key );
									if( l.flags.bLogKeyEvent )
										lprintf( WIDE( "Result of dispatch was %ld" ), dispatch_handled );
									if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
										break;

									if( !dispatch_handled )
									{
										if( l.flags.bLogKeyEvent )
											lprintf( WIDE("Local Keydefs Dispatch key : %p %08lx"), hVideo, key );
										if( hVideo && !( dispatch_handled = HandleKeyEvents( hVideo->KeyDefs, key ) ) )
										{
											if( l.flags.bLogKeyEvent )
												lprintf( WIDE("Global Keydefs Dispatch key : %08lx"), key );
											if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
											{
												if( l.flags.bLogKeyEvent )
													lprintf( WIDE( "lost window..." ) );
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
										lprintf( WIDE( "lost active window." ) );
									break;
								}
								key = (_32)DequeLink( &hVideo->pInput );
								if( l.flags.bLogKeyEvent )
									lprintf( WIDE( "key from deque : %p" ), key );
							} while( key );
							if( l.flags.bLogKeyEvent )
								lprintf( WIDE( "completed..." ) );
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
						lprintf( WIDE( "Not active window?" ) );
				}
			}
#else
			{
				_32 Msg[2];
				Msg[0] = (_32)hVid;
				Msg[1] = key;
				//lprintf( WIDE("Dispatch key from raw handler into event system.") );
				SendServiceEvent( 0, l.dwMsgBase + MSG_KeyMethod, Msg, sizeof( Msg ) );
			}
#endif
		}
		// do we REALLY have to call the next hook?!
		// I mean windows will just fuck us in the next layer....
		//lprintf( WIDE( "%d %d" ), code, dispatch_handled );
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
						lprintf( WIDE( "Chained to next hook...(2)" ) );
					result = CallNextHookEx ( (HHOOK)GetLink( &l.keyhooks, idx ), code, wParam, lParam);
					if( l.flags.bLogKeyEvent )
						lprintf( WIDE( "and result is %d" ), result );
					return result;
				}
				//else
				//   lprintf( WIDE( "skipping thread %p" ), check_thread );
			}
		}
	}
	//lprintf( WIDE( "Finished keyhook..." ) );
	return 1; // stop handling - zero allows continuation...
}

LRESULT CALLBACK
   KeyHook2 (int code,      // hook code
				WPARAM wParam,    // virtual-key code
				LPARAM lParam     // keystroke-message information
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
		ATOM aThisClass;
		//LogBinary( kbhook, sizeof( *kbhook ) );
		//lprintf( WIDE( "Received key to %p %p" ), hWndFocus, hWndFore );
		//lprintf( WIDE( "Received key %08x %08x" ), wParam, lParam );
		aThisClass = (ATOM) GetClassLong (hWndFocus, GCW_ATOM);

		if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "KeyHook2 %d %08lx %d %d %d %d %p" )
					 , code, wParam
					 , kbhook->vkCode, kbhook->scanCode, kbhook->flags, kbhook->time, kbhook->dwExtraInfo );

      /*
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
						//lprintf( WIDE( "Chained to next hook..." ) );
						return CallNextHookEx ( (HHOOK)GetLink( &l.ll_keyhooks, idx ), code, wParam, lParam);
					}
				}
			}
		}
      */
		//lprintf( WIDE("Keyhook mesasage... %08x %08x"), wParam, lParam );
		//lprintf( WIDE("hWndFocus is %p"), hWndFocus );
		if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hwndfocus is something...") );
#endif
			hVid = l.hVidFocused;
		}
		else
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hVid from focus") );
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
				| (( kbhook->flags&LLKHF_EXTENDED)?0x0100:0)   // extended key
				| (scancode << 16)
				| (( kbhook->flags & LLKHF_UP )?0:0x80000000)
            ;
			// lparam MSB is keyup status (strange)

			if (key & 0x80000000)   // test keydown...
			{
				if( l.flags.bLogKeyEvent )
					lprintf( WIDE( "keydown" ) );
				l.kbd.key[vkcode] |= 0x80;   // set this bit (pressed)
				l.kbd.key[vkcode] ^= 1;   // toggle this bit...
			}
			else
			{
				if( l.flags.bLogKeyEvent )
					lprintf( WIDE( "keyup" ) );
				l.kbd.key[vkcode] &= ~0x80;  //(unpressed)
			}

			l.kbd.key[KEY_SHIFT] = l.kbd.key[VK_RSHIFT]|l.kbd.key[VK_LSHIFT];
			l.kbd.key[KEY_CONTROL] = l.kbd.key[VK_RCONTROL]|l.kbd.key[VK_LCONTROL];
			//lprintf( WIDE("Set local keyboard %d to %d"), wParam& 0xFF, l.kbd.key[wParam&0xFF]);
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
				lprintf( WIDE( "keymod is %d from (%d,%d,%d)" ), keymod
						 , (l.kbd.key[VK_LMENU]|l.kbd.key[VK_RMENU]|l.kbd.key[KEY_ALT])
						 , (l.kbd.key[VK_LCONTROL]|l.kbd.key[VK_RCONTROL]|l.kbd.key[KEY_CTRL])
						 , (l.kbd.key[VK_LSHIFT]|l.kbd.key[VK_RSHIFT]|l.kbd.key[KEY_SHIFT])
						 );
		}

		//lprintf( WIDE("hvid is %p"), hVid );
		if(hVid)
		{
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			{
				PVIDEO hVideo = hVid;
				//lprintf( WIDE( "..." ) );
				if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
					if( hVideo && hVideo->pKeyProc )
					{
						hVideo->flags.event_dispatched = 1;
						//lprintf( WIDE("Dispatched KEY!") );
						if( hVideo->flags.key_dispatched )
							EnqueLink( &hVideo->pInput, (POINTER)key );
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
										lprintf( WIDE( "Dispatching key %08lx" ), key );
									dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, key );
								}
								if( l.flags.bLogKeyEvent )
									lprintf( WIDE( "Result of dispatch was %ld" ), dispatch_handled );
								if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
									break;

								if( !dispatch_handled )
								{
									if( l.flags.bLogKeyEvent )
										lprintf( WIDE("Local Keydefs Dispatch key : %p %08lx"), hVideo, key );
									if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, key ) )
									{
										if( l.flags.bLogKeyEvent )
											lprintf( WIDE("Global Keydefs Dispatch key : %08lx"), key );
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
								key = (_32)DequeLink( &hVideo->pInput );
							} while( key );
							hVideo->flags.key_dispatched = 0;
						}
						hVideo->flags.event_dispatched = 0;
					}
					else
					{
						//lprintf( WIDE( "calling global events." ) );
						dispatch_handled = HandleKeyEvents( KeyDefs, key ); /* global events, if no keyproc */
					}
				//else
				//	lprintf( WIDE( "Failed to find active window..." ) );
			}
#else
			{
				_32 Msg[2];
				Msg[0] = (_32)hVid;
				Msg[1] = key;
				//lprintf( WIDE("Dispatch key from raw handler into event system.") );
				//lprintf( WIDE( "..." ) );
				SendServiceEvent( 0, l.dwMsgBase + MSG_KeyMethod, Msg, sizeof( Msg ) );
			}
#endif
		}
		else
		{
			dispatch_handled = HandleKeyEvents( KeyDefs, key );
		}
		//lprintf( WIDE( "code:%d handled:%d" ), code, dispatch_handled );
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
						lprintf( WIDE( "Chained to next hook... %08x %08x" ), wParam, lParam );
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
			lprintf( WIDE( "Found we want to be under something... so use that window instead." ) );
#endif
			hwndInsertAfter = check->under->hWndOutput;
		}

		save_current = current;
	while( current )
	{
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "Add defered pos put %p after %p" )
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
		current = current->pAbove;
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
			lprintf( WIDE( "Access violation - OpenGL layer at this moment.." ) );
	return EXCEPTION_EXECUTE_HANDLER;
	default:
		if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Filter unknown : %08X" ), n );

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
	lprintf( WIDE( "Application should redraw... %p" ), hVideo );
#endif
	if( hVideo && hVideo->pRedrawCallback )
	{
		if( !hVideo->flags.bShown || hVideo->flags.bHidden )
		{
#ifdef LOG_SHOW_HIDE
			lprintf(WIDE( " hidden." ) );
#endif
         // oh - opps, it's not allowed to draw.
			return;
		}
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( WIDE( "Auto-enable window GL." ) );
#endif
			//if( hVideo->flags.event_dispatched )
			{
				//lprintf( WIDE( "Fatality..." ) );
				//Return 0;
			}
			//lprintf( WIDE( "Allowed to draw..." ) );
#ifdef _OPENGL_ENABLED
			if( !SetActiveGLDisplay( hVideo ) )
			{
				// if the opengl failed, dont' let the application draw.
				return;
			}
#endif
		}
		hVideo->flags.event_dispatched = 1;
		//					lprintf( WIDE( "Disaptched..." ) );
#ifdef _MSC_VER
		__try
		{
			//try
#elif defined( __WATCOMC__ )
#ifndef __cplusplus
			_try
			{
#endif
#endif
				//if( !hVideo->flags.bShown || !hVideo->flags.bLayeredWindow )
				{
					//HDWP hDeferWindowPos = BeginDeferWindowPos( 1 );
#ifdef NOISY_LOGGING
					lprintf( WIDE( "redraw... WM_PAINT (sendapplicationdraw)" ) );
					lprintf( WIDE( "%p %p %p"), hVideo->pRedrawCallback, hVideo->dwRedrawData, (PRENDERER) hVideo );
#endif
					hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
				}
				//catch(...)
				{
					//lprintf( WIDE( "Unknown exception during Redraw Callback" ) );
				}
#ifdef _MSC_VER
			}
			__except( EvalExcept( GetExceptionCode() ) )
			{
				lprintf( WIDE( "Caught exception in video output window" ) );
				;
			}
#elif defined( __WATCOMC__ )
#ifndef __cplusplus
		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
			lprintf( WIDE( "Caught exception in video output window" ) );
			;
		}
#endif
#endif
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( WIDE( "Auto disable (swap) window GL" ) );
#endif
#ifdef _OPENGL_ENABLED
			SetActiveGLDisplay( NULL );
			if( hVideo->flags.bLayeredWindow )
			{
				UpdateDisplay( hVideo );
			}
#endif
		}
		// might have 'controls' over the open...
		// these would need to be updated seperately?
		hVideo->flags.event_dispatched = 0;
		if( hVideo->flags.bShown )
		{
#ifdef NOISY_LOGGING
			lprintf( WIDE( "painting... shown... %p" ), hVideo );
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
	//lprintf( WIDE( "Application should have redrawn..." ) );
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
			int line = (int)(PTRSZVAL)GetRegisteredValueEx( (CTEXTSTR)root, "Source Line", TRUE );
			lprintf( "Surface input event handler %s at %s(%d)", name, file, line );
		}

		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (int,PINPUT_POINT) );
		if( f )
			f( nPoints, points );
	}
}

static void InvokeSimpleSurfaceInput( S_32 x, S_32 y, int new_touch, int last_touch )
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

	for( name = GetFirstRegisteredName( WIDE( "SACK/System/Begin Shutdown" ), &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(void);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX WIDE( "/common/save common/%s" ), name );
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
	if( IsThisThread( hVideo->pThreadWnd ) )
		//if( IsVidThread() )
	{
#ifdef LOG_RECT_UPDATE
		lprintf( WIDE( "..." ) );
#endif
		SendApplicationDraw( hVideo );
	}
	else
	{
		if( l.flags.bLogWrites )
			lprintf( WIDE( "Posting invalidate rect..." ) );
		InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
	}
}

static PTRSZVAL CPROC DoExit( PTHREAD thread )
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
#define Return   lprintf( WIDE("Finished Message %p %d %d"), hWnd, uMsg, level-- ); return 
#else
#define Return   return 
#endif
	PVIDEO hVideo;
	_32 _mouse_b = l.mouse_b;
	//static UINT uLastMouseMsg;
#if defined( OTHER_EVENTS_HERE )
   if( uMsg != 13 && uMsg != WM_TIMER ) // get window title?
   {
		lprintf( WIDE("Got message %p %d(%04x) %p %p %d"), hWnd, uMsg, uMsg, wParam, lParam, ++level );
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
		level++;
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
		lprintf( WIDE( "No, thanx, you don't need to activate." ) );
#endif
#ifndef UNDER_CE
		Return MA_NOACTIVATE;
#endif
#ifndef UNDER_CE

	case WM_NCHITTEST:
		{
#if defined( OTHER_EVENTS_HERE )
			POINTS p = MAKEPOINTS( lParam );
			lprintf( WIDE( "%d,%d Hit Test is Client" ), p.x,p.y );
#endif
		}
		Return HTCLIENT;
		//Return HTNOWHERE;
#endif
#ifndef UNDER_CE

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
			INDEX nFiles = DragQueryFile( hDrop, INVALID_INDEX, NULL, 0 );
			INDEX iFile;
			POINT pt;
			// AND we should...
			DragQueryPoint( hDrop, &pt );
			for( iFile = 0; iFile < nFiles; iFile++ )
			{
				INDEX idx;
				struct dropped_file_acceptor_tag *callback;
				//_32 namelen = DragQueryFile( hDrop, iFIle, NULL, 0 );
				DragQueryFile( hDrop, (UINT)iFile, buffer, sizeof( buffer ) );
				//lprintf( WIDE( "Accepting file drop [%s]" ), buffer );
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

DragQueryPoint 		   */
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
						lprintf( WIDE("-------------------------------- GOT FOCUS --------------------------") );
						lprintf( WIDE("Instance is %p"), hWnd );
						lprintf( WIDE("prior is %p"), hVidPrior->hWndOutput );
					}
#endif
					if( hVidPrior->pBelow )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior was below...") );
#endif
						SetFocus( hVidPrior->pBelow->hWndOutput );
					}
					else if( hVidPrior->pNext )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior last interrupted..") );
#endif
						SetFocus( hVidPrior->pNext->hWndOutput );
					}
					else if( hVidPrior->pPrior )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior which interrupted us...") );
#endif
						SetFocus( hVidPrior->pPrior->hWndOutput );
					}
					else if( hVidPrior->pAbove )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to a window which prior was above.") );
#endif
						SetFocus( hVidPrior->pAbove->hWndOutput );
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
						if( l.flags.bLogFocus )
							lprintf( WIDE("prior window is not around anythign!?") );
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
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
					{
						if( l.flags.bLogFocus )
							lprintf( WIDE("Got a losefocus for %p at %P"), hVideo, NULL );
						if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
						{
							if( hVideo && hVideo->pLoseFocus )
								hVideo->pLoseFocus (hVideo->dwLoseFocus, NULL );
						}
					}
#else
					{
						static POINTER _NULL = NULL;
						SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
													, &hVideo, sizeof( hVideo )
													, &_NULL, sizeof( _NULL ) );
					}
#endif
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
							//lprintf( WIDE("killfocus window thing %d"), hVidRecv );
							while( me )
							{
								if( me && me->pAbove == hVidRecv )
								{
#ifdef LOG_ORDERING_REFOCUS
									lprintf( WIDE("And - we need to stop this from happening, I'm stacked on the window getting the focus... restore it back to me") );
#endif
									//SetFocus( hVideo->hWndOutput );
									Return 0;
								}
								me = me->pAbove;
							}
						}
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Dispatch lose focus callback....") );
#endif
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
						{
							if( l.flags.bLogFocus )
								lprintf( WIDE("Got a losefocus for %p at %P"), hVideo, hVidRecv );
							if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
							{
								if( hVideo && hVideo->pLoseFocus )
									hVideo->pLoseFocus (hVideo->dwLoseFocus, hVidRecv );
							}
						}
#else
						SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
													, &hVideo, sizeof( hVideo )
													, &hVidRecv, sizeof( hVidRecv ) );
#endif
						//hVideo->pLoseFocus (hVideo->dwLoseFocus, hVidRecv);
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
                  lprintf( WIDE("Hidden window or lose focus was not active.") );
					}
#endif
				}
#ifdef LOG_ORDERING_REFOCUS
				else
				{
					lprintf( WIDE("this video window was not focused.") );
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
						Log (WIDE( "Within same level do focus..." ));
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
      if (hVideo && hVideo->flags.bFull)   // do not allow system draw...
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
				lprintf( WIDE( "Too early, hVideo is not setup yet to reference." ) );
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

			{
				if( !( pwp->flags & SWP_NOSIZE ) )
				{
               //lprintf( "Got resizing poschange..." );
					if( ( pwp->cx != hVideo->pWindowPos.cx )
						|| ( pwp->cy != hVideo->pWindowPos.cy ) )
					{
						lprintf( WIDE( "Forced size of display to %dx%d back to %dx%d" ), pwp->cx, pwp->cy, hVideo->pWindowPos.cx, hVideo->pWindowPos.cy );
						pwp->cx = hVideo->pWindowPos.cx;
						pwp->cy = hVideo->pWindowPos.cy;
					}
					//else
					//   lprintf( "Target is correct." );
				}
			}
			if( hVideo->flags.bDeferedPos )
			{
				if( !(pwp->flags & SWP_NOZORDER ) )	
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE( "PosChanging ? %p %p" ), hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
					pwp->hwndInsertAfter = hVideo->hDeferedAfter;
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE( "PosChanging ? %p %p" ), hVideo->hWndOutput, pwp->hwndInsertAfter );
					lprintf( WIDE( "Someone outside knew something about the ordering and this is a mass-reorder, must be correct? %p %p" ), hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
				}
				Return 0;
			}

			if( !(pwp->flags & SWP_NOZORDER ) )	
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE( "Include set Z-Order" ) );
#endif
            // being moved in zorder...
				if( !hVideo->pBelow && !hVideo->pAbove && !hVideo->under )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE( "... not above below or under, who cares. return. %p" ), pwp->hwndInsertAfter );							
#endif
					if( hVideo->flags.bAbsoluteTopmost )
					{
						if( pwp->hwndInsertAfter != HWND_TOPMOST )
						{
#ifdef LOG_ORDERING_REFOCUS
							lprintf( WIDE( "Just make sure we're the TOP TOP TOP window.... (more than1?!)" ) );
#endif
							pwp->hwndInsertAfter = HWND_TOPMOST;
						}
						else
							Return 0;
					}
					else
						Return 0;
				}
				if( !pwp->hwndInsertAfter )				
				{											
					//lprintf( WIDE( "..." ) );
					pwp->hwndInsertAfter = MoveWindowStack( hVideo, pwp->hwndInsertAfter,1  );

				}

				if( hVideo->flags.bIgnoreChanging )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE( "Ignoreing the change generated by draw.... %p " ), hVideo->pWindowPos.hwndInsertAfter );
#endif
					pwp->hwndInsertAfter = hVideo->pWindowPos.hwndInsertAfter;
					hVideo->flags.bIgnoreChanging = 0; // one shot. ... set by ShowNA
					Return 1;
				}
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Window pos Changing %p(used to be %p) (want to be after %p and before %p) %d,%d %d,%d")
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
						lprintf( WIDE( "Moving myself below what I'm expected to be below." ) );
						lprintf( WIDE( "This action stops all reordering with other windows." ) );
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
								lprintf( WIDE( "Fixup for being the top window, but a parent is under something." ) );
#endif
								pwp->hwndInsertAfter = check->under->hWndOutput;
							}
#ifdef LOG_ORDERING_REFOCUS
							else
								lprintf( WIDE( "uhmm... well it's going away.." ) );
#endif
						}
					}
				}
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Window pos Changing %p %d,%d %d,%d %08x")
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
	   Return 1;
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
   //   break;
			{
			// global hVideo is pPrimary Video...
			LPWINDOWPOS pwp;
			pwp = (LPWINDOWPOS) lParam;
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
 #ifdef LOG_ORDERING_REFOCUS
			if( !pwp->hwndInsertAfter )
			{
            //WakeableSleep( 500 );
				lprintf( WIDE( "..." ) );
			}
			lprintf( WIDE("Being inserted after %x %x"), pwp->hwndInsertAfter, hWnd );
#endif
			if (!hVideo)      // not attached to anything...
			{
				Return 0;
			}
			if( hVideo->flags.bDestroy )
			{
#ifdef NOISY_LOGGING
				lprintf( WIDE( "Oh - don't care what is done with ordering to destroy." ) );
#endif
				Return 0;
			}
			if( hVideo->flags.bHidden )
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Hmm don't really care about the motion of a hidden window...") );
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
            hVideo->flags.bForceSurfaceUpdate = 0;
				hVideo->pWindowPos.cx = pwp->cx;
				hVideo->pWindowPos.cy = pwp->cy;
#ifdef LOG_DISPLAY_RESIZE
				lprintf( WIDE( "Resize happened, recreate drawing surface..." ) );
#endif
				CreateDrawingSurface (hVideo);
            // ??
			}
			LeaveCriticalSec( &hVideo->cs );

#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE("window pos changed - new ordering includes %p for %p(%p)"), pwp->hwndInsertAfter, pwp->hwnd, hWnd );
#endif
			// maybe maintain my link according to outside changes...
			// tried to make it work the other way( and it needs to work the other way)

			//// don't save always, only parts are valid at times.
			//hVideo->pWindowPos = *pwp; // save always....!!!

			if( (!(pwp->flags & SWP_NOZORDER ))
				&& ( !hVideo->pWindowPos.hwndInsertAfter )
				&& hVideo->flags.bTopmost 
				)
				hVideo->pWindowPos.hwndInsertAfter = HWND_TOPMOST;
			{
				RECT pwp2;
				RECT pwp3;
				GetWindowRect( hWnd, &pwp2 );
				GetClientRect( hWnd, &pwp3 );
				hVideo->cursor_bias.x = pwp2.left;// - l.WindowBorder_X + 5;
				hVideo->cursor_bias.y = pwp2.top;// - l.WindowBorder_Y + 7;
			}
#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE("Window pos %p %d,%d %d,%d (bias %d,%d)")
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
		Return 0;         // indicate handled message... no WM_MOVE/WM_SIZE generated.
#ifndef NO_TOUCH
	case WM_TOUCH:
		{
			if( l.GetTouchInputInfo )
			{
				TOUCHINPUT inputs[100];
				int count = LOWORD(wParam);
				PVIDEO hVideo = (PVIDEO)GetWindowLongPtr( hWnd, WD_HVIDEO );
				if( count > 100 )
					count = 100;
				l.GetTouchInputInfo( (HTOUCHINPUT)lParam, count, inputs, sizeof( TOUCHINPUT ) );
				//lprintf( "touch event with %d", count );
				l.CloseTouchInputHandle( (HTOUCHINPUT)lParam );
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
						hVideo->pTouchCallback( hVideo->dwTouchData, inputs, count );
					}
				}
			}
		}
		Return 0;
#endif
#ifndef UNDER_CE
	case WM_NCMOUSEMOVE:
#endif
      // normal fall through without processing button states
		if (0)
		{
			S_16 wheel;
	case WM_MOUSEWHEEL:
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			wheel = (S_16)(( wParam & 0xFFFF0000 ) >> 16);
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
				lprintf( WIDE("Captured mouse already - don't do anything?") );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
				if( l.flags.bLogMouseEvents )
					lprintf( WIDE("Auto owning mouse to surface which had the mouse clicked DOWN.") );
#endif
				if( !l.hCaptured )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( WIDE("Auto release mouse from surface which had the mouse unclicked.") );
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
				lprintf( WIDE("Mouse position %d,%d"), p.x, p.y );
#endif
			p.x -= (dx =(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.x);
			p.y -= (dy=(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.y);
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( WIDE("Mouse position results %d,%d %d,%d"), dx, dy, p.x, p.y );
#endif
			if (!(l.hCaptured?l.hCaptured:hVideo)->flags.bFull)
			{
				p.x -= l.WindowBorder_X;
				p.y -= l.WindowBorder_Y;
			}
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( WIDE("Mouse position results %d,%d %d,%d"), dx, dy, p.x, p.y );
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
				lprintf( WIDE( "Mouse Moved" ) );
			if( (!hVideo->flags.mouse_on || !l.flags.mouse_on ) && !hVideo->flags.bNoMouse)
			{
				int x;
				if (!hCursor)
					hCursor = LoadCursor (NULL, IDC_ARROW);
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "cursor on." ) );
#endif
				x = ShowCursor( TRUE );
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "cursor count %d %d" ), x, hCursor );
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
				lprintf( WIDE( "Tick ... %d" ), l.last_mouse_update );
#endif
			}
			l.mouse_last_vid = hVideo;
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
				lprintf( WIDE("Generate mouse message %p(%p?) %d %d,%08x %d+%d=%d"), l.hCaptured, hVideo, l.mouse_x, l.mouse_y, l.mouse_b, l.dwMsgBase, MSG_MouseMethod, l.dwMsgBase + MSG_MouseMethod );
#endif
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			{
				_32 msg[4];
				msg[0] = (_32)(l.hCaptured?l.hCaptured:hVideo);
				msg[1] = l.mouse_x;
				msg[2] = l.mouse_y;
				msg[3] = l.mouse_b;
				SendServiceEvent( 0, l.dwMsgBase + MSG_MouseMethod
									 , msg
									 , sizeof( msg ) );
			}
#endif
			InvokeSimpleSurfaceInput( l.mouse_x, l.mouse_y, (l.mouse_b & ~l._mouse_b ) != 0, (l.mouse_b & l._mouse_b ) != 0 );
			if (hVideo->pMouseCallback)
			{
				hVideo->pMouseCallback (hVideo->dwMouseData,
												l.mouse_x, l.mouse_y, l.mouse_b);
			}
			l._mouse_x = l.mouse_x;
			l._mouse_y = l.mouse_y;
			// clear scroll buttons...
			// otherwise circumstances of mouse wheel followed by any other event
			// continues to generate scroll clicks.
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			l._mouse_b = l.mouse_b;
		}
		Return 0;         // don't allow windows to think about this...
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
		//lprintf( WIDE("Show Window Message! %p"), hVideo );
		if( !hVideo->flags.bShown )
		{
			//lprintf( "Window is hiding? that is why we got it?" );
			Return 0;
		}
		//lprintf( "Fall through to WM_PAINT..." );
   case WM_PAINT:
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if( l.flags.bLogWrites )
			lprintf( WIDE("Paint Message! %p"), hVideo );
		if( hVideo->flags.event_dispatched )
		{
			ValidateRect( hWnd, NULL );
//#ifdef NOISY_LOGGING
			lprintf( WIDE( "Validated rect... will you stop calling paint!?" ) );
//#endif
			Return 0;
		}
		{
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				EnterCriticalSec( &hVideo->cs );
				if( hVideo->flags.bDestroy )
				{
					lprintf( WIDE( "Aborting WM_PAINT (is bDestroy'ed)" ) );
					LeaveCriticalSec( &hVideo->cs );
					Return 0;
				}
				//lprintf( WIDE( "Found window - send draw" ) );
				SendApplicationDraw( hVideo );
				LeaveCriticalSec( &hVideo->cs );
			}
			else
				lprintf( WIDE("Failed to find window to show?") );
			//UpdateDisplayPortion (hVideo, 0, 0, 0, 0);
			//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#else
			SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
#endif
		}
		//lprintf( "redraw... WM_PAINT" );
		ValidateRect (hWnd, NULL);
		//lprintf( "redraw... WM_PAINT" );
		/// allow a second invalidate to post.
#ifdef NOISY_LOGGING
		lprintf( WIDE( "Finished and clearing post Invalidate." ) );
#endif
		l.flags.bPostedInvalidate = 0;
		break;
   case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_CLOSE:

			//DestroyWindow( hWnd );
			Return TRUE;
		}
		break;
#ifndef _ARM_ // maybe win32_CE?
	case WM_QUERYENDSESSION:
		xlprintf(1500)( WIDE( "QUERY ENDING SESSION! (respond OK)" ) );
#ifdef WIN32
		SetEvent( CreateEvent( NULL, TRUE, FALSE, WIDE( "Windows Is Shutting Down" ) ) );
#endif
		InvokeBeginShutdown();
		Return TRUE; // uhmm okay.
	case WM_ENDSESSION:
		xlprintf(1500)( WIDE( "ENDING SESSION! (well I guess someone will close this?)" ) );
      //ThreadTo( DoExit, lParam );
		//BAG_Exit( (int)lParam );
		Return TRUE; // uhmm okay.
	case WM_DESTROY:
#ifdef LOG_DESTRUCTION
		Log( WIDE("Destroying a window...") );
#endif
		hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
		if (hVideo)
		{
#ifdef LOG_DESTRUCTION
			Log (WIDE( "Killing the window..." ));
#endif
			DoDestroy (hVideo);
		}
		break;
#endif
	case WM_ERASEBKGND:
		// LIE! we don't care to ever erase the background...
		// thanx for the invite though.
		//lprintf( WIDE("Erase background, and we just say NO") );
		Return TRUE;
	case WM_USER_MOUSE_CHANGE:
		{
			hVideo = (PVIDEO) GetWindowLongPtr (hWnd, WD_HVIDEO);
			if( hVideo )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "hVideo..." ) );
#endif
				// if not idling mouse...
				if( !hVideo->flags.bIdleMouse )
				{
#ifdef LOG_MOUSE_HIDE_IDLE
					lprintf( WIDE( "No idle mouse invis..." ) );
#endif
					// if the cursor was off...
					if( !l.flags.mouse_on )
					{
#ifdef LOG_MOUSE_HIDE_IDLE
						lprintf( WIDE( "Mouse was off... turning on." ) );
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
		if( l.flags.mouse_on && l.last_mouse_update )
		{
#ifdef LOG_MOUSE_HIDE_IDLE
			lprintf( WIDE( "Pending mouse away... %d" ), timeGetTime() - ( l.last_mouse_update ) );
#endif
			if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
			{
				int x;
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "OFF!" ) );
#endif
				l.flags.mouse_on = 0;
				//l.last_mouse_update = 0;
				while( x = ShowCursor( FALSE ) )
				{
				}
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "Show count %d %d" ), x, GetLastError() );
#endif
				SetCursor( NULL );
			}
		}
      // this is a special return, it doesn't decrement the counter... cause WM_TIMER is filtered in OTHER_EVENTS logging
		return 0;
      //break;
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs;
#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Begin WM_CREATE" ) );
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
// definatly add whatever thread made it to the WM_CREATE.			
					AddLink( &l.threads, me );
					AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
					lprintf( WIDE( "No thread %x, adding hook and thread." ), GetCurrentThreadId() );
#ifdef USE_KEYHOOK

					if( l.flags.bUseLLKeyhook )
						AddLink( &l.ll_keyhooks,
								  added = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
																  , GetModuleHandle(_WIDE(TARGETNAME)), 0 /*GetCurrentThreadId()*/
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

				if ((LONG) hVideo == 1)
					break;

				if (!hVideo)
				{
					// directly created without parameter (Dialog control
					// probably )... grab a static PVIDEO for this...
					hVideo = &l.hDialogVid[(l.nControlVid++) & 0xf];
				}
				SetLastError( 0 );
				SetWindowLongPtr (hWnd, WD_HVIDEO, (PTRSZVAL) hVideo);
				if( GetLastError() )
				{
					lprintf( WIDE("error setting window long pointer : %d"), GetLastError() );
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

            hVideo->flags.bForceSurfaceUpdate = 0;
				CreateDrawingSurface (hVideo);
				hVideo->flags.bReady = TRUE;
				WakeThreadID( hVideo->thread );
			}
#ifdef LOG_OPEN_TIMING
			//lprintf( WIDE( "Complete WM_CREATE" ) );
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
#define HWND_MESSAGE     ((HWND)-3)
#endif

#ifdef UNDER_CE
#define WINDOW_STYLE 0
#else
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
#endif

RENDER_PROC (BOOL, CreateWindowStuffSizedAt) (PVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
#ifndef __NO_WIN32API__
	static HMODULE hMe;
	if( hMe == NULL )
		hMe = GetModuleHandle (_WIDE(TARGETNAME));
	//lprintf( WIDE( "-----Create WIndow Stuff----- %s %s" ), hVideo->flags.bLayeredWindow?WIDE( "layered" ):WIDE( "solid" )
	//		 , hVideo->flags.bChildWindow?WIDE( "Child(tool)" ):WIDE( "user-selectable" ) );
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
		//Log1 (WIDE( "Creating container window named: %s" ),
		//    (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo->pTitle);
		l.hWndInstance = CreateWindowEx (0
#ifndef NO_DRAG_DROP
													| WS_EX_ACCEPTFILES
#endif
#ifdef UNICODE
												  , (LPWSTR)l.aClass
#else
												  , (LPSTR)l.aClass
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
         lprintf( WIDE( "Failed to create instance window %d" ), GetLastError() );
		}
		{
#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.." ) );
#endif
			l.hVidCore = New(VIDEO);
			MemSet (l.hVidCore, 0, sizeof (VIDEO));
         InitializeCriticalSec( &l.hVidCore->cs );
			l.hVidCore->hWndOutput = (HWND)l.hWndInstance;
			l.hVidCore->pThreadWnd = MakeThread();
		}
		//Log (WIDE( "Created master window..." ));
	}
	if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
	{
		_32 w, h;
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
		lprintf( WIDE( "Create Real Window (In CreateWindowStuff).." ) );
#endif
		result = CreateWindowEx( 0
#ifndef NO_TRANSPARENCY
										| (hVideo->flags.bLayeredWindow?WS_EX_LAYERED:0)
#endif
#ifndef NO_MOUSE_TRANSPARENCY
										| (hVideo->flags.bNoMouse?WS_EX_TRANSPARENT:0)
#endif
										| (hVideo->flags.bChildWindow?WS_EX_TOOLWINDOW:0)
										// | WS_EX_NOPARENTNOTIFY
#ifdef UNICODE
									  , (LPWSTR)l.aClass
#else
									  , (LPSTR)l.aClass
#endif
									  , (l.gpTitle&&l.gpTitle[0])?l.gpTitle:hVideo->pTitle
									  , (hVideo->hWndContainer)?WS_CHILD:(hVideo->flags.bFull ? (WS_POPUP) : (WINDOW_STYLE))
									  , x, y
									  , hVideo->flags.bFull ?wx:(wx + l.WindowBorder_X)
									  , hVideo->flags.bFull ?wy:(wy + l.WindowBorder_Y)
									  , hVideo->hWndContainer //(HWND)l.hWndInstance  // Parent
									  , NULL     // Menu
									  , hMe
									  , (void *) hVideo);
		if( !result )
			lprintf( WIDE( "Failed to create window %d" ), GetLastError() );
	}
	else
	{

	}
	// save original screen image for initialized state...
	BitBlt ((HDC)hVideo->hDCBitmap, 0, 0, wx, wy
			 , (HDC)hVideo->hDCOutput, 0, 0, SRCCOPY);


#ifdef LOG_OPEN_TIMING
	//lprintf( WIDE( "Created window stuff..." ) );
#endif
	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
	SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
#endif
	//Log (WIDE( "Created window in module..." ));
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

int CPROC VideoEventHandler( _32 MsgID, _32 *params, _32 paramlen )
{
#if defined( LOG_MOUSE_EVENTS ) || defined( OTHER_EVENTS_HERE )
	if( l.flags.bLogMouseEvents )
		lprintf( WIDE("Received message %d"), MsgID );
#endif
	l.dwEventThreadID = GetCurrentThreadId();
	//LogBinary( (POINTER)params, paramlen );
	switch( MsgID )
	{
	case MSG_DispatchPending:
		{
			INDEX idx;
			PVIDEO hVideo;
         //lprintf( WIDE("dispatching outstanding events...") );
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
							lprintf( WIDE("Pending dispatch mouse %p (%d,%d) %08x")
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
			PVIDEO hVideo = (PVIDEO)params[0];
			if( l.flags.bLogFocus )
				lprintf( WIDE("Got a losefocus for %p at %P"), params[0], params[1] );
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo && hVideo->pLoseFocus )
					hVideo->pLoseFocus (hVideo->dwLoseFocus, (PVIDEO)params[1] );
			}
		}
		break;
	case MSG_RedrawMethod:
		{
			PVIDEO hVideo = (PVIDEO)params[0];
			//lprintf( WIDE("Show video %p"), hVideo );
            /* Oh neat a safe window list... we should use this more places! */
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( l.flags.bLogWrites )
					lprintf( WIDE( "invalidate post" ) );
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
            lprintf( WIDE("Failed to find window to show?") );
		}
		break;
	case MSG_CloseMethod:
		break;
	case MSG_MouseMethod:
		{
			PVIDEO hVideo = (PVIDEO)params[0];
#ifdef LOG_MOUSE_EVENTS
			if( l.flags.bLogMouseEvents )
			{
				lprintf( WIDE("mouse method... forward to application please...") );
				lprintf( WIDE( "params %ld %ld %ld" ), params[1], params[2], params[3] );
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
							lprintf( WIDE("delta dispatching mouse (%d,%d) %08x")
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
					lprintf( WIDE( "*** Set Mouse Pending on %p %ld,%ld %08x!" ), hVideo, params[1], params[2], params[3]  );
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
			PVIDEO hVideo = (PVIDEO)params[0];
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				if( hVideo && hVideo->pKeyProc )
				{
					hVideo->flags.event_dispatched = 1;
					//lprintf( WIDE("Dispatched KEY!") );
					if( hVideo->flags.key_dispatched )
						EnqueLink( &hVideo->pInput, (POINTER)params[1] );
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
								lprintf( WIDE("IPC Local Keydefs Dispatch key : %p %08lx"), hVideo, params[1] );
#endif
								if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, params[1] ) )
								{
#ifdef LOG_KEY_EVENTS
									lprintf( WIDE("IPC Global Keydefs Dispatch key : %08lx"), params[1] );
#endif
									if( !HandleKeyEvents( KeyDefs, params[1] ) )
									{
										// previously this would then dispatch the key event...
										// but we want to give priority to handled keys.
									}
								}
							}
							params[1] = (_32)DequeLink( &hVideo->pInput );
						} while( params[1] );
						hVideo->flags.key_dispatched = 0;
					}
					hVideo->flags.event_dispatched = 0;
				}
		}
		break;
	default:
		lprintf( WIDE("Got a unknown message %d with %d data"), MsgID, paramlen );
		break;
	}
	return 0;
}


//----------------------------------------------------------------------------

void HandleDestroyMessage( PVIDEO hVidDestroy )
{
	{
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("To destroy! %p %d"), 0 /*Msg.lParam*/, hVidDestroy->hWndOutput );
#endif
		// hide the window! then it can't be focused or active or anything!
		//if( hVidDestroy->flags.key_dispatched ) // wait... we can't go away yet...
      //   return;
		//if( hVidDestroy->flags.event_dispatched ) // wait... we can't go away yet...
      //   return;
		//EnableWindow( hVidDestroy->hWndOutput, FALSE );
		SafeSetFocus( (HWND)GetDesktopWindow() );
#ifdef asdfasdfsdfa
		if( GetActiveWindow() == hVidDestroy->hWndOutput)
		{
#ifdef LOG_DESTRUCTION
			lprintf( WIDE("Set ourselves inactive...") );
#endif
			//SetActiveWindow( l.hWndInstance );
#ifdef LOG_DESTRUCTION
			lprintf( WIDE("Set foreground to instance...") );
#endif
		}
		if( GetFocus() == hVidDestroy->hWndOutput)
		{
#ifdef LOG_DESTRUCTION
			lprintf( WIDE("Fixed focus away from ourselves before destroy.") );
#endif
			AttachThreadInput( GetWindowThreadProcessId(
									GetForegroundWindow()
									,NULL)
					,GetCurrentThreadId()
					,TRUE);

			AttachThreadInput( GetWindowThreadProcessId(
									GetDesktopWindow()
									,NULL)
					,GetCurrentThreadId()
					,TRUE);
//Detach the attached thread

			SetFocus( GetDesktopWindow() );

			AttachThreadInput(
				    GetWindowThreadProcessId(
							GetForegroundWindow(),NULL)
					,GetCurrentThreadId()
					,FALSE);
		}
#endif
		//ShowWindow( hVidDestroy->hWndOutput, SW_HIDE );
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("------------ DESTROY! -----------") );
#endif
 		DestroyWindow (hVidDestroy->hWndOutput);
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("From destroy") );
#endif
	}
}
//----------------------------------------------------------------------------

static void HandleMessage (MSG Msg)
{
//   lprintf( "%d %d %d %d", Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam );
#ifdef USE_XP_RAW_INPUT
	//#if(_WIN32_WINNT >= 0x0501)
#define WM_INPUT                        0x00FF
	//#endif /* _WIN32_WINNT >= 0x0501 */
	if( Msg.message ==  WM_INPUT )
	{
		UINT dwSize;
		WPARAM wParam = Msg.wParam;
		LPARAM lParam = Msg.lParam;
		lprintf( WIDE( "Raw Input!" ) );
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
		//lprintf( WIDE( "Message Create window stuff..." ) );
#endif
		CreateWindowStuffSizedAt (hVidCreate, hVidCreate->pWindowPos.x,
                                hVidCreate->pWindowPos.y,
                                hVidCreate->pWindowPos.cx,
                                hVidCreate->pWindowPos.cy);
	}
	else if (!Msg.hwnd && (Msg.message == (WM_USER + 513)))
	{
		HandleDestroyMessage( (PVIDEO) Msg.lParam );
	}
	else if( Msg.message == (WM_USER_HIDE_WINDOW ) )
	{
		PVIDEO hVideo = ((PVIDEO)Msg.lParam);
#ifdef LOG_SHOW_HIDE
		lprintf( WIDE( "Handling HIDE_WINDOW posted message %p" ),hVideo->hWndOutput );
#endif
		hVideo->flags.bHidden = 1;
		if( hVideo->pHideCallback )
			hVideo->pHideCallback( hVideo->dwHideData );
      //AnimateWindow( ((PVIDEO)Msg.lParam)->hWndOutput, 0, AW_HIDE );
		ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_HIDE );
#ifdef LOG_SHOW_HIDE
		lprintf( WIDE( "Handled HIDE_WINDOW posted message %p" ),hVideo->hWndOutput );
#endif
		
	}
	else if( Msg.message == (WM_USER_SHOW_WINDOW ) )
	{
		PVIDEO hVideo = (PVIDEO)Msg.lParam;
		lprintf( WIDE( "Handling SHOW_WINDOW message! %p" ), Msg.lParam );
		//ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_RESTORE );
		//lprintf( WIDE( "Handling SHOW_WINDOW message! %p" ), Msg.lParam );
		if( hVideo->flags.bTopmost )
			SetWindowPos( hVideo->hWndOutput
							, HWND_TOPMOST
							, 0, 0, 0, 0,
							 SWP_NOMOVE
							 | SWP_NOSIZE
							);
		if( hVideo->flags.bShown )
		{
			hVideo->flags.bRestoring = 1;
			ShowWindow( hVideo->hWndOutput, SW_RESTORE );
			hVideo->flags.bRestoring = 0;
		}
		else
		{
			hVideo->flags.bShown = 1;
			ShowWindow( hVideo->hWndOutput, SW_SHOW );
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
				//lprintf( WIDE("(E)Got message:%d"), Msg.message );
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
	lprintf( WIDE( "msg %p %d %d %p" ), msg->hwnd, msg->message, msg->wParam, msg->lParam );
	if( msg->message == WM_TOUCH )
	{
		lprintf( WIDE( "TOUCH %d" ), msg->wParam );
	}

	return CallNextHookEx( prochook, code, wParam, lParam );
}
HHOOK get_prochook;
LRESULT CALLBACK AllGetWndProc( int code, WPARAM wParam, LPARAM lParam )
{
	MSG *msg  = (MSG*)lParam;
	lprintf( WIDE( "msg %p %d %d %p %d" )
		, msg->hwnd
		, msg->message
		, msg->wParam, msg->lParam, msg->time );
	if( msg->message == WM_TOUCH )
	{
		lprintf( WIDE( "TOUCH %d" ), msg->wParam );
	}

	return CallNextHookEx( get_prochook, code, wParam, lParam );
}
#endif
//----------------------------------------------------------------------------
PTRSZVAL CPROC VideoThreadProc (PTHREAD thread)
{
#ifdef LOG_STARTUP
	Log( WIDE("Video thread...") );
#endif
	if (l.bThreadRunning)
	{
#ifdef LOG_STARTUP
		Log( WIDE("Already exists - leaving.") );
#endif
      return 0;
	}
#ifdef LOG_STARTUP
	lprintf( WIDE( "Video Thread Proc %x, adding hook and thread." ), GetCurrentThreadId() );
#endif
#ifdef USE_KEYHOOK
#ifndef NO_TOUCH
	if( l.flags.bHookTouchEvents )
	{
		prochook = SetWindowsHookEx(
											 WH_CALLWNDPROC,
											 AllWndProc,
											 GetModuleHandle(_WIDE(TARGETNAME)),
											 0);
		get_prochook = SetWindowsHookEx(
												  WH_CALLWNDPROC,
												  AllGetWndProc,
												  GetModuleHandle(_WIDE(TARGETNAME)),
												  0);
	}
#endif

	if( l.flags.bUseLLKeyhook )
		AddLink( &l.ll_keyhooks,
				  SetWindowsHookEx (WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
										 ,GetModuleHandle(_WIDE(TARGETNAME)), 0 /*GetCurrentThreadId()*/
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
		Log( WIDE("reading a message to create a message queue") );
#endif
		PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	}
	l.actual_thread = thread;
	l.dwThreadID = GetCurrentThreadId ();
	//l.pid = (_32)l.dwThreadID;
	l.bThreadRunning = TRUE;
	AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
	//AddIdleProc ( ProcessClientMessages, 0);
#ifdef LOG_STARTUP
	Log( WIDE("Registered Idle, and starting message loop") );
#endif
	{
		MSG Msg;
		while( !l.bExitThread && GetMessage (&Msg, NULL, 0, 0) )
		{
			//lprintf( "Dispatched... %d", Msg.message );
			HandleMessage (Msg);
			//lprintf( "Finish Dispatched... %d", Msg.message );
		}
	}
	l.bThreadRunning = FALSE;
	lprintf( WIDE( "Video Exited volentarily" ) );
	//ExitThread( 0 );
	return 0;
}

//----------------------------------------------------------------------------
static void VideoLoadOptions( void )
{
	// don't set this anymore, the new option connection is same version as default
	//SetOptionDatabaseOption( option, TRUE );

#ifndef __NO_OPTIONS__
	PODBC option = GetOptionODBC( GetDefaultOptionDatabaseDSN(), 0 );
	l.flags.bHookTouchEvents = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/use touch event" ), 0, TRUE );

	l.flags.bLogMouseEvents = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/log mouse event" ), 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/log key event" ), 0, TRUE );

	l.flags.bOptimizeHide = SACK_GetOptionIntEx( option, WIDE( "SACK" ), WIDE( "Video Render/Optimize Hide with SWP_NOCOPYBITS" ), 0, TRUE );
#ifndef NO_TRANSPARENCY
	if( l.UpdateLayeredWindow )
		l.flags.bLayeredWindowDefault = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/Default windows are layered" ), 0, TRUE )?TRUE:FALSE;
	else
#endif
		l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/Log Video Output" ), 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/Use Low Level Keyhook" ), 0, TRUE );
	DropOptionODBC( option );
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
		wc.hInstance = GetModuleHandle (_WIDE(TARGETNAME));
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
#ifdef __cplusplus
		wc.lpszClassName = WIDE( "VideoOutputClass++" );
#else
		wc.lpszClassName = WIDE( "VideoOutputClass" );
#endif
		wc.cbWndExtra = sizeof( PTRSZVAL );   // one extra pointer

		l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
			lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
			return FALSE;
		}

#ifdef __cplusplus
		wc.lpszClassName = WIDE( "HiddenVideoOutputClass++" );
#else
		wc.lpszClassName = WIDE( "HiddenVideoOutputClass" );
#endif
		wc.lpfnWndProc = (WNDPROC)VideoWindowProc2;
		l.aClass2 = RegisterClass (&wc);
		if (!l.aClass2)
		{
			lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
			return FALSE;
		}

#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
		InitMessageService();
		l.dwMsgBase = LoadService( NULL, VideoEventHandler );
#endif
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
				_32 error = GetLastError();
				
				if( error == ERROR_INVALID_THREAD_ID )
				{
					l.bThreadRunning = 0;
					break;
				}
				Log1( WIDE("Failed to post shutdown message...%d" ), error );
				cnt--;
			}
			Relinquish();
		}
		while (!d && cnt);
		if (!d && ( cnt < 25 ) )
		{
			lprintf( WIDE( "Tried %d times to post thread message... and it alwasy failed." ), 25-cnt );
		}
		else
		{
			_32 start = timeGetTime() + 100;
			while( ( start > timeGetTime() )&&
				l.bThreadRunning )
			{
				Relinquish();
			}
			if( l.bThreadRunning )
				lprintf( WIDE( "Had to give up waiting for video thread to exit..." ) );
		}
	}
}
#endif

RENDER_PROC (TEXTCHAR, GetKeyText) (int key)
{
	int c;
	char ch[5];
	if( key & KEY_MOD_DOWN )
      return 0;
	key ^= 0x80000000;

	c =  
#ifndef UNDER_CE
      ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
               l.kbd.key, (unsigned short *) ch, 0);
#else
	   key;
#endif
	if (!c)
	{
		// check prior key bindings...
		//printf( WIDE("no translation\n") );
		return 0;
	}
	else if (c == 2)
	{
      //printf( WIDE("Key Translated: %d %d\n"), ch[0], ch[1] );
      return 0;
   }
   else if (c < 0)
   {
      //printf( WIDE("Key Translation less than 0\n") );
      return 0;
   }
   //printf( WIDE("Key Translated: %d(%c)\n"), ch[0], ch[0] );
   return ch[0];
}

//----------------------------------------------------------------------------

static LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
{
	InitDisplay ();
	if (!l.flags.bThreadCreated && !hNextVideo->hWndContainer)
	{
		int failcount = 0;
		l.flags.bThreadCreated = 1;
		//Log( WIDE("Starting video thread...") );
		AddLink( &l.threads, ThreadTo (VideoThreadProc, 0) );
#ifdef LOG_STARTUP
		Log( WIDE("Started video thread...") );
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
			AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
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
			Rid[0].dwFlags = 0 /*RIDEV_NOLEGACY*/;   // adds HID mouse and also ignores legacy mouse messages

			Rid[1].usUsagePage = 0x01;
			Rid[1].usUsage = 0x06;
			Rid[1].dwFlags = 0 /*RIDEV_NOLEGACY*/;   // adds HID keyboard and also ignores legacy keyboard messages

			if (RegisterRawInputDevices(Rid, 2, sizeof (Rid [0])) == FALSE)
			{
				lprintf( WIDE("Registration failed!?") );
				//registration failed. Call GetLastError for the cause of the error
			}
		}
#endif
	}

	AddLink( &l.pActiveList, hNextVideo );
	//hNextVideo->pid = l.pid;
	hNextVideo->KeyDefs = CreateKeyBinder();
#ifdef LOG_OPEN_TIMING
	lprintf( WIDE( "Doing open of a display..." ) );
#endif
	if( ( GetCurrentThreadId () == l.dwThreadID ) || hNextVideo->hWndContainer )
	{
#ifdef LOG_OPEN_TIMING
		lprintf( WIDE( "Allowed to create my own stuff..." ) );
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
				Log1( WIDE("Failed to post create new window message...%d" ),
						GetLastError ());
				cnt--;
			}
#ifdef LOG_STARTUP
			else
			{
				lprintf( WIDE("Posted create new window message...") );
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
			_32 timeout = timeGetTime() + 5000;
			hNextVideo->thread = GetMyThreadID();
			while (!hNextVideo->flags.bReady && timeout > timeGetTime())
			{
				// need to do this on the possibility that
				// thIS thread did create this window...
				if( !Idle() )
				{
#ifdef NOISY_LOGGING
					lprintf( WIDE("Sleeping until the window is created.") );
#endif
					WakeableSleep( SLEEP_FOREVER );
					//Relinquish();
				}
			}
			if( !hNextVideo->flags.bReady )
			{
				CloseDisplay( hNextVideo );
				lprintf( WIDE("Fatality.. window creation did not complete in a timely manner.") );
				// hnextvideo is null anyhow, but this is explicit.
				return FALSE;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( WIDE("Resulting new window %p %d"), hNextVideo, hNextVideo->hWndOutput );
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


RENDER_PROC (PVIDEO, OpenDisplaySizedAt) (_32 attr, _32 wx, _32 wy, S_32 x, S_32 y) // if native - we can return and let the messages dispatch...
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
	//lprintf( WIDE("Hardcoded right here for FULL window surfaces, no native borders.") );
	hNextVideo->flags.bFull = TRUE;
#ifndef UNDER_CE
   if( l.UpdateLayeredWindow )
		hNextVideo->flags.bLayeredWindow = (attr & DISPLAY_ATTRIBUTE_LAYERED)?1:(l.flags.bLayeredWindowDefault);
   else
#endif
		hNextVideo->flags.bLayeredWindow = 0;
	hNextVideo->flags.bNoAutoFocus = (attr & DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS)?TRUE:FALSE;
	hNextVideo->flags.bChildWindow = (attr & DISPLAY_ATTRIBUTE_CHILD)?TRUE:FALSE;
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
				SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
				SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
#endif
		}

	}
}

//----------------------------------------------------------------------------

RENDER_PROC (PVIDEO, OpenDisplayAboveSizedAt) (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PVIDEO parent)
{
   PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

RENDER_PROC (PVIDEO, OpenDisplayAboveUnderSizedAt) (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PVIDEO parent, PVIDEO barrier)
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
	if (!hVideo)         // must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log (WIDE( "Unlinking destroyed window..." ));
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	hVideo->flags.bDestroy = 1;
	// this isn't the therad to worry about...
	bEventThread = ( l.dwEventThreadID == GetCurrentThreadId() );
	if (GetCurrentThreadId () == l.dwThreadID )
	{
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("Scheduled for deletion") );
#endif
      HandleDestroyMessage( hVideo );
	}
	else
	{
		int d = 1;
#ifdef LOG_DESTRUCTION
		Log (WIDE( "Dispatching destroy and resulting..." ));
#endif
		//SendServiceEvent( l.pid, WM_USER + 513, &hVideo, sizeof( hVideo ) );
		d = PostThreadMessage (l.dwThreadID, WM_USER + 513, 0, (LPARAM) hVideo);
		if (!d)
		{
			Log (WIDE( "Failed to post create new window message..." ));
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
				lprintf( WIDE("Wait for window to go unready.") );
				logged = 1;
			}
			Idle();
			//Sleep (0);
		}
#else
		{
			_32 fail_time = timeGetTime() + 500;
			while (l.bThreadRunning && hVideo->flags.bReady && !bEventThread && (fail_time < timeGetTime()))
				Idle();
			if(!(fail_time < timeGetTime()))
			{
			//lprintf( WIDE( "Failed to wait for close of display....(well we might have caused it somewhere in back hsack... return now k?" ) );
			}
		}
#endif
		hVideo->flags.bInDestroy = 0;
		// the scan of inactive windows releases the hVideo...
		AddLink( &l.pInactiveList, hVideo );
		// generate an event to dispatch pending...
		// there is a good chance that a window event caused a window
		// and it will be sleeping until the next event...
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
		SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
#endif
	}
	return;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SizeDisplay) (PVIDEO hVideo, _32 w, _32 h)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Size Display..." ) );
#endif
	if( w == hVideo->pWindowPos.cx && h == hVideo->pWindowPos.cy )
		return;
	// if this isn't updated, this determines a forced size during changing.
	hVideo->pWindowPos.cx = w;
	hVideo->pWindowPos.cy = h;
	if( hVideo->flags.bLayeredWindow )
	{
		// need to remake image surface too...
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
					, SWP_NOMOVE|SWP_NOACTIVATE);
		CreateDrawingSurface (hVideo);
		if( hVideo->flags.bShown )
			UpdateDisplay( hVideo );
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

RENDER_PROC (void, SizeDisplayRel) (PVIDEO hVideo, S_32 delw, S_32 delh)
{
	if (delw || delh)
	{
		S_32 cx, cy;
		cx = (hVideo->pWindowPos.cx += delw);
		cy = (hVideo->pWindowPos.cy += delh);
		if (hVideo->pWindowPos.cx < 50)
			cx = hVideo->pWindowPos.cx = 50;
		if (hVideo->pWindowPos.cy < 20)
			cy = hVideo->pWindowPos.cy = 20;
#ifdef LOG_RESIZE
		Log2 (WIDE( "Resized display to %d,%d" ), hVideo->pWindowPos.cx,
            hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "size display relative" ) );
#endif
		hVideo->flags.bForceSurfaceUpdate = 1;
		SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
                    SWP_NOZORDER | SWP_NOMOVE);
   }
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveDisplay) (PVIDEO hVideo, S_32 x, S_32 y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Move display %d,%d" ), x, y );
#endif
	stop = 1;
	if( hVideo->flags.bLayeredWindow )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
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

RENDER_PROC (void, MoveDisplayRel) (PVIDEO hVideo, S_32 x, S_32 y)
{
	if (x || y)
	{
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "Move display relative" ) );
#endif
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
					, hVideo->pWindowPos.x
                    , hVideo->pWindowPos.y
					, 0, 0
					, SWP_NOZORDER | SWP_NOSIZE);
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplay) (PVIDEO hVideo, S_32 x, S_32 y, S_32 w,
                                     S_32 h)
{
	S_32 cx, cy;
	if( !hVideo )
		return;
	hVideo->pWindowPos.x = x;
	hVideo->pWindowPos.y = y;
	hVideo->pWindowPos.cx = w;
	hVideo->pWindowPos.cy = h;
	cx = w;
	cy = h;
	if (cx < 50)
		cx = 50;
	if (cy < 20)
		cy = 20;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size display." ) );
#endif
	hVideo->flags.bForceSurfaceUpdate = 1;
	SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
				, hVideo->pWindowPos.x
				, hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				 , SWP_NOZORDER|SWP_NOACTIVATE );
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplayRel) (PVIDEO hVideo, S_32 delx, S_32 dely,
                                        S_32 delw, S_32 delh)
{
	S_32 cx, cy;
	hVideo->pWindowPos.x += delx;
	hVideo->pWindowPos.y += dely;
	cx = (hVideo->pWindowPos.cx += delw);
	cy = (hVideo->pWindowPos.cy += delh);
	if (cx < 50)
		cx = 50;
	if (cy < 20)
		cy = 20;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size relative %d,%d %d,%d" ), delx, dely, delw, delh );
#endif
	hVideo->flags.bForceSurfaceUpdate = 1;
	SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
				, hVideo->pWindowPos.x
                , hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				, SWP_NOZORDER );
}

//----------------------------------------------------------------------------

RENDER_PROC (void, UpdateDisplayEx) (PVIDEO hVideo DBG_PASS )
{
	// copy hVideo->lpBuffer to hVideo->hDCOutput
	if (hVideo && hVideo->hWndOutput && hVideo->hBm)
	{
		UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
	}
	return;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, ClearDisplay) (PVIDEO hVideo)
{
	ClearImage (hVideo->pImage);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetMousePosition) (PVIDEO hVid, S_32 x, S_32 y)
{
	if (hVid->flags.bFull)
		SetCursorPos (x + hVid->cursor_bias.x, y + hVid->cursor_bias.y);
	else
		SetCursorPos (x + l.WindowBorder_X + hVid->cursor_bias.x,
                      y + l.WindowBorder_Y + hVid->cursor_bias.y);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetMousePosition) (S_32 * x, S_32 * y)
{
	POINT p;
	GetCursorPos (&p);
	if (x)
		(*x) = p.x;
	if (y)
		(*y) = p.y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
	GetMousePosition( x, y );
	if( b )
      (*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetCloseHandler) (PVIDEO hVideo,
                                     CloseCallback pWindowClose,
                                     PTRSZVAL dwUser)
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
                                     PTRSZVAL dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetHideHandler) (PVIDEO hVideo,
                                     HideAndRestoreCallback pHideCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwHideData = dwUser;
   hVideo->pHideCallback = pHideCallback;
}
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRestoreHandler) (PVIDEO hVideo,
                                     HideAndRestoreCallback pRestoreCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwRestoreData = dwUser;
   hVideo->pRestoreCallback = pRestoreCallback;
}

//----------------------------------------------------------------------------
#ifndef NO_TOUCH
RENDER_PROC (void, SetTouchHandler) (PVIDEO hVideo,
                                     TouchCallback pTouchCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwTouchData = dwUser;
   hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (void, SetRedrawHandler) (PVIDEO hVideo,
                                      RedrawCallback pRedrawCallback,
                                      PTRSZVAL dwUser)
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
                                        PTRSZVAL dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetLoseFocusHandler) (PVIDEO hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         PTRSZVAL dwUser)
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
		if( hVideo->flags.bShown )
		{
			//lprintf( WIDE( "Forcing topmost" ) );
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							  SWP_NOMOVE | SWP_NOSIZE);
		}
		else
		{
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
		if( hVideo->flags.bShown )
		{
			lprintf( WIDE( "Forcing topmost" ) );
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
		lprintf(WIDE( "Hiding the window! %p %p %p" ), hVideo->hWndOutput, hVideo->pAbove, hVideo->pBelow );
	else
		lprintf( WIDE( "Someone tried to hide NULL window!" ) );
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
					//lprintf( WIDE( "..." ) );
					if( hVideo->hWndOutput == GetActiveWindow() ||
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

					lprintf(WIDE( " Think this thead is a videothread?!" ) );
					ShowWindow (hVideo->hWndOutput, SW_HIDE);
					hVideo->flags.bHidden = TRUE;
					if( hVideo->pHideCallback )
						hVideo->pHideCallback( hVideo->dwHideData );
					if (hVideo->pAbove)
					{
						lprintf( WIDE("Focusing and activating my parent window.") );
						{
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
							static POINTER _NULL = NULL;
							SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
								, &hVideo, sizeof( hVideo )
								, &hVideo->pAbove, sizeof( hVideo->pAbove ) );
							SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
								, &hVideo->pAbove, sizeof( hVideo->pAbove )
								, &_NULL, sizeof( _NULL ) );
#endif
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
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
							static POINTER _NULL = NULL;
							SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
								, &hVideo, sizeof( hVideo )
								, &hVideo->pBelow, sizeof( hVideo->pBelow ) );
							SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
								, &hVideo->pBelow, sizeof( hVideo->pBelow )
								, &_NULL, sizeof( _NULL ) );
#endif
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
			_32 shortwait = timeGetTime() + 250;
			while( !hVideo->flags.bHidden && shortwait > timeGetTime() )
			{
				IdleFor( 10 );
			}
			if( !hVideo->flags.bHidden )
			{
				lprintf( WIDE( "window did not hide." ) );
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
	_lprintf(DBG_RELAY)( WIDE( "Restore display. %p" ), hVideo->hWndOutput );
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
					isthread = TRUE;
			{
				if( ( GetCurrentThreadId () == l.dwThreadID ) && isthread )
				{
#if defined( OTHER_EVENTS_HERE )
					lprintf( WIDE( "Sending SHOW_WINDOW to window thread.. %p" ), hVideo  );
#endif
					PostThreadMessage (l.dwThreadID, WM_USER_SHOW_WINDOW, 0, (LPARAM)hVideo );
				}
				else
				{
#if defined( OTHER_EVENTS_HERE )
					lprintf( WIDE( "Doing the show window." ) );
#endif
					if( hVideo->flags.bShown )
					{
#if defined( OTHER_EVENTS_HERE )
						lprintf( WIDE( "window was shown, use restore." ) );
#endif
						hVideo->flags.bRestoring = 1;
						ShowWindow( hVideo->hWndOutput, SW_RESTORE );
						hVideo->flags.bRestoring = 0;
					}
					else
					{
						hVideo->flags.bShown = 1;
#if defined( OTHER_EVENTS_HERE )
						lprintf( WIDE( "Generating initial show (restore display, never shown)" ) );
#endif
						ShowWindow( hVideo->hWndOutput, SW_SHOW );
					}
					if( hVideo->flags.bTopmost )
					{
#if defined( OTHER_EVENTS_HERE )
						lprintf( WIDE( "Setting possition topmost..." ) );
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
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
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
				snprintf( teststring, 20, WIDE( "\\\\.\\DISPLAY%s%d" ), (v_test==1)?"V":"", nDisplay );
				for( i = 0;
					 !found && EnumDisplayDevices( NULL // all devices
														  , i
														  , &dev
														  , 0 // dwFlags
														  ); i++ )
				{
					if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
					{
						lprintf( WIDE( "display %s is at %d,%d %dx%d" ), dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					else
						lprintf( WIDE( "Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					lprintf( WIDE( "[%s] might be [%s]" ), teststring, dev.DeviceName );
					if( StrCaseCmp( teststring, dev.DeviceName ) == 0 )
					{
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
							lprintf( WIDE( "Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					}
					else
					{
						//lprintf( WIDE( "[%s] is not [%s]" ), teststring, dev.DeviceName );
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

RENDER_PROC (void, GetDisplaySize) (_32 * width, _32 * height)
{
   RECT r;
   GetWindowRect (GetDesktopWindow (), &r);
   //Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
   if (width)
      *width = r.right - r.left;
   if (height)
      *height = r.bottom - r.top;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetDisplayPosition) (PVIDEO hVid, S_32 * x, S_32 * y,
                                        _32 * width, _32 * height)
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
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetDisplaySize) (_32 width, _32 height)
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
                                       GeneralCallback general, PTRSZVAL psv)
{
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (void, OwnMouseEx) (PVIDEO hVideo, _32 own DBG_PASS)
{
	if (own)
	{
		lprintf( WIDE("Capture is set on %p"),hVideo );
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
				lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
				l.hCaptured = hVideo;
				hVideo->flags.bCaptured = 1;
				SetCapture (hVideo->hWndOutput);
			}
			else
			{
				if( !hVideo->flags.bCaptured )
				{
					lprintf( WIDE("This should NEVER happen!") );
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
			lprintf( WIDE("No more capture.") );
			//ReleaseCapture ();
			hVideo->flags.bCaptured = 0;
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

RENDER_PROC (int, BeginCalibration) (_32 nPoints)
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
   //lprintf( WIDE( "... 3 step?" ) );
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
      lprintf( WIDE("Window Pos set failed: %d"), GetLastError() );
   //if( !SetActiveWindow( pRender->hWndOutput ) )
   //   lprintf( WIDE("active window failed: %d"), GetLastError() );

   //if( !SetForegroundWindow( pRender->hWndOutput ) )
   //   lprintf( WIDE("okay well foreground failed...?") );
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayBack )( PRENDERER pRender )
{
	// uhmm...
   lprintf( WIDE( "Force display backward." ) );
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
			SetTimer( (HWND)hVideo->hWndOutput, 100, 100, NULL );
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
				lprintf( WIDE( "Mouse was off... want it on..." ) );
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
			lprintf( WIDE( "Output fade %d %p" ), hVideo->fade_alpha, hVideo->hWndOutput );
#ifndef UNDER_CE
		IssueUpdateLayeredEx( hVideo, FALSE, 0, 0, 0, 0 DBG_SRC );
#endif
	}
}

LOGICAL RequiresDrawAll ( void )
{
	return FALSE;
}

void MarkDisplayUpdated( PRENDERER renerer )
{

}


#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void (CPROC*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , ProcessDisplayMessages
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32)) OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) CloseDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32, _32, _32 DBG_PASS)) UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , (void (CPROC*)(PRENDERER)) ClearDisplay
                                       , GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, _32, _32)) SizeDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) GetDisplayImage
                                       , SetCloseHandler
                                       , SetMouseHandler
                                       , SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, PTRSZVAL)) SetKeyboardHandler
													,  SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(S_32 *, S_32 *)) GetMousePosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SetMousePosition
                                       , HasFocus  // has focus
                                       , NULL         // SendMessage
													, NULL         // CrateMessage
                                       , GetKeyText
                                       , IsKeyDown
                                       , KeyDown
                                       , DisplayIsValid
                                       , OwnMouseEx
                                       , BeginCalibration
													, SyncRender   // sync
#ifdef _OPENGL_ENABLED
													, EnableOpenGL
                                       , SetActiveGLDisplay
#else
                                       , NULL
                                       , NULL
#endif
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

static LOGICAL CPROC DefaultExit( PTRSZVAL psv, _32 keycode )
{
   lprintf( WIDE( "Default Exit..." ) );
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
		lprintf( WIDE("Regstering video interface...") );
#ifndef UNDER_CE
	l.UpdateLayeredWindow = ( BOOL (WINAPI *)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD))LoadFunction( WIDE( "user32.dll" ), WIDE( "UpdateLayeredWindow" ) );
	// this is Vista+ function.
	l.UpdateLayeredWindowIndirect = ( BOOL (WINAPI *)(HWND,const UPDATELAYEREDWINDOWINFO *))LoadFunction( WIDE( "user32.dll" ), WIDE( "UpdateLayeredWindowIndirect" ) );
#ifndef NO_TOUCH
	l.GetTouchInputInfo = (BOOL (WINAPI *)( HTOUCHINPUT, UINT, PTOUCHINPUT, int) )LoadFunction( WIDE( "user32.dll" ), WIDE( "GetTouchInputInfo" ) );
	l.CloseTouchInputHandle =(BOOL (WINAPI *)( HTOUCHINPUT ))LoadFunction( WIDE( "user32.dll" ), WIDE( "CloseTouchInputHandle" ) );
	l.RegisterTouchWindow = (BOOL (WINAPI *)( HWND, ULONG  ))LoadFunction( WIDE( "user32.dll" ), WIDE( "RegisterTouchWindow" ) );
#endif
#endif
	RegisterInterface( 
#ifdef SACK_BAG_EXPORTS  // symbol defined by visual studio sack_bag.vcproj
#  ifdef __cplusplus
#    ifdef __cplusplus_cli
	   WIDE("sack.render")
#    else
	   WIDE("sack.render++")
#    endif
#  else
	   WIDE("sack.render")
#  endif
#else
#  ifdef UNDER_CE
		WIDE("render")
#  else
#    ifdef __cplusplus
#      ifdef __cplusplus_cli
	     WIDE("sack.render")
#      else
	     WIDE("sack.render++")
#      endif
#    else
	   WIDE("sack.render")
#    endif
#  endif
#endif
	   , GetDisplayInterface, DropDisplayInterface );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	//EnableLoggingOutput( TRUE );
	VideoLoadOptions();
}


//typedef struct sprite_method_tag *PSPRITE_METHOD;

RENDER_PROC( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
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
static void CPROC SavePortion( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h )
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

PRELOAD( InitSetSavePortion )
{
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

//--------------------------------------------------------------------------
//
// $Log: vidlib.c,v $
// Revision 1.99  2005/06/29 09:07:58  d3x0r
// No opengl version compiled... may be selected by editing interface.conf
//
// Revision 1.98  2005/06/19 07:28:25  d3x0r
// Fix local event handling (actually it's a bug in the msgsvr client library.  Also update to new msgevent_handling protocol which one can return wait_dispatch_pending....
//
// Revision 1.97  2005/05/25 16:50:31  d3x0r
// Synch with working repository.
//
// Revision 1.123  2005/05/13 00:39:50  jim
// Quick hack on windows vidlib code for sendeventmessage (local queue)
//
// Revision 1.122  2005/05/12 21:05:08  jim
// Implement (sorta) OkaySyncRender to comply with changes to display
//
// Revision 1.121  2005/04/26 18:54:54  jim
// Fix SyncRender implementation
//
// Revision 1.120  2005/04/12 23:57:16  jim
// Remove noisy logging by ifdeffing the sections.
//
// Revision 1.119  2005/04/11 22:29:56  jim
// Oh - we DO care about motions of hidden windows, at last to save their new dimensions.
//
// Revision 1.118  2005/04/05 11:56:04  panther
// Adding sprite support - might have added an extra draw callback...
//
// Revision 1.117  2005/03/28 09:44:17  panther
// Use single surface to project surround-o-vision.  This btw has the benefit of uniform output.
//
// Revision 1.116  2005/03/23 12:31:23  panther
// Remove noisy messages...
//
// Revision 1.115  2005/03/23 12:21:58  panther
// Remove more noisy messages.
//
// Revision 1.114  2005/03/22 12:11:28  panther
// Save window surface into the buffer before resulting to the application.
//
// Revision 1.113  2005/03/17 02:23:53  panther
// Checkpoint - working on message server abstraction interface... some of this seems to work quite well, some of this is still broken very badly...
//
// Revision 1.112  2005/03/14 16:31:14  panther
// Hmm WS_EX_LAYERED was bad... made window disssappear.
//
// Revision 1.111  2005/03/14 11:07:00  panther
// Link all windows above something... so all windows of an application promote in application order.
//
// Revision 1.110  2005/03/13 23:34:35  panther
// Focus and mouse capture issues resolved for windows libraries... need to tinker with this same function within Linux.
//
// Revision 1.109  2005/03/12 23:22:30  panther
// Added support to reorder in-level windows based on the windowposchanged message.... focus issues are still isues for popups.
//
// Revision 1.108  2005/02/27 00:59:42  panther
// option out noisy logging.
//
// Revision 1.107  2005/02/18 19:43:10  panther
// Added some meaningful logging about hidden window updates.
//
// Revision 1.106  2005/01/17 12:28:57  panther
// Remove noisy logging
//
// Revision 1.105  2004/12/20 19:37:09  panther
// Fixes for re-typed event proc handler
//
// Revision 1.104  2004/12/18 22:34:08  panther
// Protect against focus changes while destroying...
//
// Revision 1.103  2004/12/16 06:53:44  panther
// Fixes for setting topmost
//
// Revision 1.102  2004/12/15 03:17:12  panther
// Minor remanants of fixes already commited.
//
// Revision 1.101  2004/12/13 11:08:17  panther
// Checkpoint, minor tweaks
//
// Revision 1.100  2004/12/05 15:32:06  panther
// Some minor cleanups fixed a couple memory leaks
//
// Revision 1.99  2004/12/05 14:31:52  panther
// Improve handling of closing video surfaces... something should be learned here about easy methods of deep protection involved between 3 input events and timers and the application....
//
// Revision 1.98  2004/12/04 01:22:04  panther
// Destroy key binder associated with hvideo
//
// Revision 1.97  2004/12/04 01:09:13  panther
// Set destroy flag to avoid dispatch killfocus during destruction
//
// Revision 1.96  2004/10/31 17:22:29  d3x0r
// Minor fixes to control library...
//
// Revision 1.95  2004/10/22 09:27:02  d3x0r
// Checkpoint.  Stability is approaching a limit.
//
// Revision 1.94  2004/10/08 01:21:17  d3x0r
// checkpoint
//
// Revision 1.93  2004/10/04 20:08:39  d3x0r
// Minor adjustments for static linking
//
// Revision 1.92  2004/10/03 01:25:36  d3x0r
// Stop a parent focus from getting focus... have to do things about events thereto.
//
// Revision 1.91  2004/10/02 19:49:12  d3x0r
// Fix logging... trying to track down multiple update display issues.... keys are queued, events are locally queued...
//
// Revision 1.90  2004/09/30 22:02:43  d3x0r
// checkpoing
//
// Revision 1.89  2004/09/30 01:14:48  d3x0r
// Cleaned up consistancy of PID and thread ID... extended message service a bit to supply event PID both ways.
//
// Revision 1.88  2004/09/29 00:49:50  d3x0r
// Added fancy wait for PSI frames which allows non-polling sleeping... Extended Idle() to result in meaningful information.
//
// Revision 1.87  2004/09/24 00:07:19  d3x0r
// Minor adjustments for video stability...
//
// Revision 1.86  2004/09/23 00:35:33  d3x0r
// Implement key and mouse messages across message system... soon to allow applications to catch these directly instead of callback methods.
//
// Revision 1.85  2004/09/22 20:25:20  d3x0r
// Begin implementation of message queues to handle events from video to application
//
// Revision 1.84  2004/09/22 03:11:16  d3x0r
// It WORKs!
//
// Revision 1.83  2004/09/21 00:48:52  d3x0r
// checkpoint.
//
// Revision 1.82  2004/09/12 02:07:53  d3x0r
// Checkpoint...
//
// Revision 1.81  2004/09/01 03:27:21  d3x0r
// Control updates video display issues?  Image blot message go away...
//
// Revision 1.80  2004/08/29 14:56:24  d3x0r
// Protect against hiding a NULL window
//
// Revision 1.79  2004/08/17 16:45:57  d3x0r
// When registering the draw event, do a draw... otherwise surface is not initialized.  Only draw dispatch is when size changes.
//
// Revision 1.78  2004/08/16 06:34:37  d3x0r
// Begin toying with raw input avaialble on XP
//
// Revision 1.77  2004/08/11 11:40:23  d3x0r
// Begin seperation of key and render
//
// Revision 1.76  2004/06/21 07:46:45  d3x0r
// Account for newly moved structure files.
//
// Revision 1.75  2004/06/21 07:39:36  d3x0r
// use moved include files for image structures
//
// Revision 1.74  2004/06/16 03:02:50  d3x0r
// checkpoint
//
// Revision 1.73  2004/06/15 08:32:35  d3x0r
// Added bind events to interface table
//
// Revision 1.72  2004/06/14 11:15:10  d3x0r
// Okay force foreground movement in all ways - else it was just shy of the top...
//
// Revision 1.71  2004/06/14 10:52:49  d3x0r
// Implement new force methods for order and focus
//
// Revision 1.70  2004/05/25 19:11:33  d3x0r
// Fix redundant mouse click issue... ONLY generate mouse messages when there is a change
//
// Revision 1.69  2004/05/02 05:44:38  d3x0r
// Implement  BindEventToKey and UnbindKey
//
// Revision 1.68  2004/05/02 05:06:23  d3x0r
// Sweeping changes to logging which by default release was very very noisy...
//
// Revision 1.67  2004/04/29 11:16:41  d3x0r
// Reformat, collect global data into a structure
//
// Revision 1.66  2004/04/27 21:14:30  d3x0r
// Fix nasty thread problem
//
// Revision 1.65  2004/04/27 09:55:11  d3x0r
// Add keydef to keyhandler path
//
// Revision 1.64  2004/04/27 03:18:27  d3x0r
// Fix race condition when starting window
//
// Revision 1.63  2004/04/26 20:15:31  d3x0r
// Include idle.h, return keys...
//
// Revision 1.62  2004/04/19 14:04:27  d3x0r
// Okay this seems to layout the whole project tree - just not reassembling the parts right...
//
// Revision 1.61  2004/03/04 01:09:51  d3x0r
// Modifications to force slashes to wlib.  Updates to Interfaces to be gotten from common interface manager.
//
// Revision 1.60  2003/12/14 06:35:27  panther
// Added syncrender call
//
// Revision 1.59  2003/11/30 08:17:39  panther
// Put more functions in interface table
//
// Revision 1.58  2003/11/10 03:59:27  panther
// Fix idle to call idle()
//
// Revision 1.57  2003/11/03 09:20:44  panther
// Grabbed the threadID from the wrong place...
//
// Revision 1.56  2003/10/27 16:41:26  panther
// Go to standard abstract ThreadTO instead of CreateThread
//
// Revision 1.55  2003/09/26 14:20:41  panther
// PSI DumpFontFile, fix hide/restore display
//
// Revision 1.54  2003/09/25 09:18:11  panther
// Add newly defined methods...
//
// Revision 1.53  2003/08/01 23:52:57  panther
// Updates for msvc build
//
// Revision 1.52  2003/07/25 08:33:37  panther
// Heck - always focus the newest window created
//
// Revision 1.51  2003/07/22 15:16:59  panther
// Add AddIdleProc
//
// Revision 1.50  2003/07/21 16:31:19  panther
// Generate useful information on lose focus
//
// Revision 1.49  2003/07/21 15:27:13  panther
// Active window initially created on win98 (xp was easy)
//
// Revision 1.48  2003/07/17 10:13:53  panther
// Set newly created windows to be the active window.
//
// Revision 1.47  2003/06/24 11:05:59  panther
// define calibration, include displaylib on windows project list
//
// Revision 1.46  2003/04/24 00:03:49  panther
// Added ColorAverage to image... Fixed a couple macros
//
// Revision 1.45  2003/04/21 20:01:20  panther
// Remove some logging, add support for hasfocus
//
// Revision 1.44  2003/03/29 17:21:06  panther
// Focus problems, mouse message problems resolved... Focus works through to the client side now
//
// Revision 1.43  2003/03/29 15:56:53  panther
// Simplify focus handling
//
// Revision 1.42  2003/03/29 15:53:07  panther
// Fix windows vidlib focus handling
//
// Revision 1.41  2003/03/27 19:26:03  panther
// Implement OwnMouseEx and DropDisplayInterface
//
// Revision 1.40  2003/03/25 23:36:27  panther
// Added SizeDisplayRel and MoveSizeDisplayRel.
//
// Revision 1.39  2003/03/25 08:45:58  panther
// Added CVS logging tag
//
