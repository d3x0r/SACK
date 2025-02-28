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
#undef _D3D_ENABLED
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


#define NEED_REAL_IMAGE_STRUCTURE
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#include <d3d9.h>
#include <d3dx9.h>

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
#define NEED_VECTLIB_COMPARE
#include <image.h>
#include <vectlib.h>

#include <shellapi.h> // very last though - this is DragAndDrop definitions...

// this is safe to leave on.
#define LOG_ORDERING_REFOCUS

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
#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_D3D_CONTEXT
#include <vidlib/vidstruc.h>
#include <render3d.h>
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
extern RENDER3D_INTERFACE Render3d;

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
		if( strcmp( classname, "GLVideoOutputClass" ) == 0 )
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
			_lprintf(DBG_RELAY)( "    %p[%p] %s", base, base->hWndOutput, title );
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
										lprintf( "layered... begin update. %d %d %d %d", size.cx, size.cy, topPos.x, topPos.y );
									{
                              // we CAN do indirect...
										l.UpdateLayeredWindow(
																	 hVideo->hWndOutput
																	, bContent?(HDC)hVideo->hDCOutput:NULL
																	, bContent?&topPos:NULL
																	, bContent?&size:NULL
																	, bContent?hVideo->hDCOutput:NULL
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
void  EnableLoggingOutput( LOGICAL bEnable )
{
   l.flags.bLogWrites = bEnable;
}

void  UpdateDisplayPortionEx( PVIDEO hVideo
                                          , S_32 x, S_32 y
                                          , _32 w, _32 h DBG_PASS)
{

   if( hVideo )
		hVideo->flags.bShown = 1;
	l.flags.bUpdateWanted = 1;
}

//----------------------------------------------------------------------------

void
UnlinkVideo (PVIDEO hVideo)
{
	// yes this logging is correct, to say what I am below, is to know what IS above me
	// and to say what I am above means I nkow what IS below me
//#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( " -- UNLINK Video %p from which is below %p and above %p", hVideo, hVideo->pAbove, hVideo->pBelow );
//#endif
	//if( hVideo->pBelow || hVideo->pAbove )
	//   DebugBreak();
	if (hVideo->pBelow)
	{
		hVideo->pBelow->pAbove = hVideo->pAbove;
	}
	else
		l.top = hVideo->pAbove;
	if (hVideo->pAbove)
	{
		hVideo->pAbove->pBelow = hVideo->pBelow;
	}
	else
		l.bottom = hVideo->pBelow;

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

void  PutDisplayAbove (PVIDEO hVideo, PVIDEO hAbove)
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
	if( l.flags.bLogFocus )
		lprintf( "Begin Put Display Above..." );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();

	// if this is already in a list (like it has pBelow things)
	// I want to insert hAbove between hVideo and pBelow...
	// if if above already has things above it, I want to put those above hvideo
	if( hVideo && hAbove )
	{
		if( hVideo->pBelow = hAbove->pBelow )
		{
			hAbove->pBelow->pAbove = hVideo;
		}
		else
			l.top = hVideo;

		if( hVideo->pAbove )
		{
			lprintf( "Window was over somethign else and now we die." );
			DebugBreak();
		}

		hAbove->pBelow = hVideo;
		hVideo->pAbove = hAbove;

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
			if( l.flags.bLogFocus )
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
			if( l.flags.bLogFocus )
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
	if( l.flags.bLogFocus )
		lprintf( "End Put Display Above..." );
#endif
}

void  PutDisplayIn (PVIDEO hVideo, PVIDEO hIn)
{
   lprintf( "Relate hVideo as a child of hIn..." );
}

//----------------------------------------------------------------------------

BOOL CreateDrawingSurface (PVIDEO hVideo)
{
	if (!hVideo)
		return FALSE;

	hVideo->pImage =
		RemakeImage( hVideo->pImage, NULL, hVideo->pWindowPos.cx,
						hVideo->pWindowPos.cy );
	if( !hVideo->transform )
	{
		TEXTCHAR name[64];
		snprintf( name, sizeof( name ), "render.display.%p", hVideo );
		lprintf( "making initial transform" );
		hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
	}

	lprintf( "Set transform at %d,%d", hVideo->pWindowPos.x, hVideo->pWindowPos.y );
	Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );

	// additionally indicate that this is a GL render point
	hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
	return TRUE;
}

void DoDestroy (PVIDEO hVideo)
{
   if (hVideo)
   {
      hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
      SetWindowLong (hVideo->hWndOutput, WD_HVIDEO, 0);
      if (hVideo->pWindowClose)
      {
         hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
		Deallocate(TEXTCHAR*,hVideo->pTitle);
		DestroyKeyBinder( hVideo->KeyDefs );
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
   KeyHook (int code,      // hook code
				WPARAM wParam,    // virtual-key code
				LPARAM lParam     // keystroke-message information
			  )
{
	if( l.flags.bLogKeyEvent )
		lprintf( "KeyHook %d %08lx %08lx", code, wParam, lParam );
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
		if (aThisClass != l.aClass && hWndFocus != ((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance )
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
		{
			INDEX idx;
			struct display_camera *camera;
			LIST_FORALL( l.cameras, idx, struct display_camera *,camera )
			{
				if( hWndFocus == camera->hWndInstance )
					break;
			}
			if( camera )
			{
	#ifdef LOG_FOCUSEVENTS
				if( l.flags.bLogKeyEvent )
					lprintf( "hwndfocus is something..." );
	#endif
				hVid = camera->hVidCore;
			}
			else
			{
				hVid = (PVIDEO) GetWindowLong (hWndFocus, WD_HVIDEO);
				if( hVid )
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
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
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
						EnqueLink( &hVideo->pInput, (POINTER)key );
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
									if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, key ) )
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
									if( HandleKeyEvents( KeyDefs, key )  )
									{
										dispatch_handled = 1;
									}
							}
							if( FindLink( &l.pActiveList, hVideo ) == INVALID_INDEX )
							{
								if( l.flags.bLogKeyEvent )
									lprintf( "lost active window." );
								break;
							}
							key = (_32)DequeLink( &hVideo->pInput );
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
				HandleKeyEvents( KeyDefs, key ); /* global events, if no keyproc */
			}
		   //else9
			//	lprintf( "Failed to find active window..." );
		}
#else
				{
					_32 Msg[2];
					Msg[0] = (_32)hVid;
					Msg[1] = key;
               //lprintf( "Dispatch key from raw handler into event system." );
					SendServiceEvent( 0, l.dwMsgBase + MSG_KeyMethod, Msg, sizeof( Msg ) );
				}
#endif
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
				//   lprintf( "skipping thread %p", check_thread );
			}
		}
	}
	//lprintf( "Finished keyhook..." );
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
		//lprintf( "Received key to %p %p", hWndFocus, hWndFore );
		//lprintf( "Received key %08x %08x", wParam, lParam );
		aThisClass = (ATOM) GetClassLong (hWndFocus, GCW_ATOM);

		if( l.flags.bLogKeyEvent )
			lprintf( "KeyHook2 %d %08lx %d %d %d %d %p"
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
						//lprintf( "Chained to next hook..." );
						return CallNextHookEx ( (HHOOK)GetLink( &l.ll_keyhooks, idx ), code, wParam, lParam);
					}
				}
			}
		}
      */
		//lprintf( "Keyhook mesasage... %08x %08x", wParam, lParam );
		//lprintf( "hWndFocus is %p", hWndFocus );
		//if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( "hwndfocus is something..." );
#endif
			//hVid = l.hVidFocused;
		}
		//else
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( "hVid from focus" );
#endif
			hVid = (PVIDEO) GetWindowLong (hWndFocus, WD_HVIDEO);
			if( hVid )
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
               lprintf( "keydown" );
            l.kbd.key[vkcode] |= 0x80;   // set this bit (pressed)
            l.kbd.key[vkcode] ^= 1;   // toggle this bit...
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

		if( l.flags.bLogKeyEvent )
			lprintf( "hvid is %p", hVid );
		if(hVid)
		{
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			{
				PVIDEO hVideo = hVid;
				//lprintf( "..." );
				if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
					if( hVideo && hVideo->pKeyProc )
					{
						hVideo->flags.event_dispatched = 1;
						//lprintf( "Dispatched KEY!" );
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
								key = (_32)DequeLink( &hVideo->pInput );
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
#else
			{
				_32 Msg[2];
				Msg[0] = (_32)hVid;
				Msg[1] = key;
				//lprintf( "Dispatch key from raw handler into event system." );
				//lprintf( "..." );
				SendServiceEvent( 0, l.dwMsgBase + MSG_KeyMethod, Msg, sizeof( Msg ) );
			}
#endif
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
			if( l.flags.bLogFocus )
				lprintf( "Found we want to be under something... so use that window instead." );
#endif
			hwndInsertAfter = check->under->hWndOutput;
		}

		save_current = current;
	while( current )
	{
#ifdef LOG_ORDERING_REFOCUS
		if( l.flags.bLogFocus )
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
			lprintf( "Access violation - D3D layer at this moment.." );
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
		if( hVideo->flags.bD3D )
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
			if( !SetActiveGLDisplay( hVideo ) )
			{
				// if the opengl failed, dont' let the application draw.
				return;
			}
#endif
		}
		hVideo->flags.event_dispatched = 1;
		//					lprintf( "Disaptched..." );
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
#elif defined( __WATCOMC__ )
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
		lprintf( "..." );
#endif
		SendApplicationDraw( hVideo );
	}
	else
	{
		if( l.flags.bLogWrites )
			lprintf( "Posting invalidate rect..." );
		InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
	}
}


static void InvokeExtraInit( struct display_camera *camera, PTRANSFORM view_camera )
{
	PTRSZVAL (CPROC *Init3d)(PMatrix,PTRANSFORM,RCOORD*,RCOORD*);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( "sack/render/puregl/init3d", &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		Init3d = GetRegisteredProcedureExx( data,(CTEXTSTR)name,PTRSZVAL,"ExtraInit3d",(PMatrix,PTRANSFORM,RCOORD*,RCOORD*));

		if( Init3d )
		{
			struct plugin_reference *reference;
			PTRSZVAL psvInit = Init3d( NULL, view_camera, &camera->identity_depth, &camera->aspect );
			if( psvInit )
			{
				reference = New( struct plugin_reference );
				reference->psv = psvInit;
				reference->name = name;
				{
					static PCLASSROOT draw3d;
					if( !draw3d )
						draw3d = GetClassRoot( "sack/render/puregl/draw3d" );
					reference->Update3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"Update3d",(PTRANSFORM));
					// add one copy of each update proc to update list.
					if( FindLink( &l.update, reference->Update3d ) == INVALID_INDEX )
						AddLink( &l.update, reference->Update3d );
					reference->FirstDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"FirstDraw3d",(PTRSZVAL));					reference->ExtraDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"ExtraBeginDraw3d",(PTRSZVAL,PTRANSFORM));					reference->Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"ExtraDraw3d",(PTRSZVAL));					reference->Mouse3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,"ExtraMouse3d",(PTRSZVAL,PRAY,S_32,S_32,_32));
					reference->Key3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,"ExtraKey3d",(PTRSZVAL,_32));				}
				AddLink( &camera->plugins, reference );
			}
		}
	}
}

static void RenderD3D( struct display_camera *camera )
{
	int first_draw;

	// do OpenGL Frame
	if( !SetActiveD3DDisplay( camera->hVidCore ) )  // BeginScene()
		return;

	if( !camera->flags.did_first_draw )
	{
		first_draw = 1;
		camera->flags.did_first_draw = 1;
	}
	else
		first_draw = 0;

	Render3d.current_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER
					, D3DCOLOR_XRGB(0, 40, 100)
					, 1.0f
					, 0);

	{
		INDEX idx;
		struct plugin_reference *reference;

		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			// setup initial state, like every time so it's a known state?
			InitD3D( camera );
			{
				// copy l.origin to the camera
				ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );

				if( first_draw )
				{
					if( reference->FirstDraw3d )
						reference->FirstDraw3d( reference->psv );
				}
				if( reference->ExtraDraw3d )
					reference->ExtraDraw3d( reference->psv, camera->origin_camera );
			}
		}
	}

	switch( camera->type )
	{
	case 0:
		RotateRight( camera->origin_camera, vRight, vForward );
		break;
	case 1:
		RotateRight( camera->origin_camera, vForward, vUp );
		break;
	case 2:
		break;
	case 3:
		RotateRight( camera->origin_camera, vUp, vForward );
		break;
	case 4:
		RotateRight( camera->origin_camera, vForward, vRight );
		break;
	case 5:
		RotateRight( camera->origin_camera, -1, -1 );
		break;
	}

	{
		PC_POINT tmp = GetAxis( camera->origin_camera, 0 );
		{
			D3DXMATRIX out;
			D3DXVECTOR3 eye(tmp[12], tmp[13], tmp[14]);
			D3DXVECTOR3 at(tmp[8], tmp[9], tmp[10]);
			D3DXVECTOR3 up(tmp[4], tmp[5], tmp[6] );
			at += eye;
			D3DXMatrixLookAtLH(&out, &eye, &at, &up);
			camera->d3ddev->SetTransform( D3DTS_WORLD, &out );
		}
	}

	{
		PRENDERER hVideo;

		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( l.flags.bLogMessageDispatch )
				lprintf( "Have a video in stack..." );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( "But it's nto exposed..." );
				continue;
			}


			if( l.flags.bLogWrites )
				lprintf( "------ BEGIN A REAL DRAW -----------" );


			Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );
			ClearImageTo( hVideo->pImage, 0 );
			Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 0 );

			//Render3d.current_device->SetRenderState( D3DRS_BLENDOP, D3DBLEND_SRCALPHA );

			Render3d.current_device->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
			Render3d.current_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			Render3d.current_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			Render3d.current_device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);


			if( hVideo->pRedrawCallback )
				hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );
		}

		Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );

		{
#if 0
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, camera->mouse_ray.o, camera->mouse_ray.n, 1.0 );
				SetPoint( origin, camera->mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,0,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,255,0,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, l.mouse_ray.o, l.mouse_ray.n, 1.0 );
				SetPoint( origin, l.mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,255,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,56,255,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
#endif
		}
		{
			INDEX idx;
			struct plugin_reference *ref;
			PTRSZVAL psvInit;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				if( ref->Draw3d )
					ref->Draw3d( ref->psv );
			}
		}
	}
	SetActiveD3DDisplay( NULL ); // EndScene
}


static HCURSOR hCursor;

//#ifndef __NO_WIN32API__
LRESULT CALLBACK
VideoWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if defined( OTHER_EVENTS_HERE )
	static int level;
#define Return   lprintf( "Finished Message %p %d %d", hWnd, uMsg, level-- ); return 
#else
#define Return   return 
#endif
	PVIDEO hVideo;
	_32 _mouse_b = l.mouse_b;
	//static UINT uLastMouseMsg;
#if defined( OTHER_EVENTS_HERE )
   if( uMsg != 13 && uMsg != WM_TIMER ) // get window title?
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
			lprintf( "%d,%d Hit Test is Client", p.x,p.y );
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
			PVIDEO hVideo = (PVIDEO)GetWindowLong( hWnd, WD_HVIDEO );
			INDEX nFiles = DragQueryFile( hDrop, INVALID_INDEX, NULL, 0 );
			INDEX iFile;
			POINT pt;
			if( hVideo )
			{
			// AND we should...
			DragQueryPoint( hDrop, &pt );
			for( iFile = 0; iFile < nFiles; iFile++ )
			{
				INDEX idx;
				struct dropped_file_acceptor_tag *callback;
				//_32 namelen = DragQueryFile( hDrop, iFIle, NULL, 0 );
				DragQueryFile( hDrop, iFile, buffer, sizeof( buffer ) );
				//lprintf( "Accepting file drop [%s]", buffer );
				LIST_FORALL( hVideo->dropped_file_acceptors, idx, struct dropped_file_acceptor_tag*, callback )
				{
					callback->f( callback->psvUser, buffer, pt.x, pt.y );
				}
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
		  INDEX idx;
		  struct display_camera * camera;
		  LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
		  {
			  if( camera->hWndInstance == hWnd )
				  break;
		  }
			if( camera )
			{
				PVIDEO hVidPrior = l.hVidFocused;
				hVideo = camera->hVidCore;
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
			//hVideo = l.top;
			l.hVidFocused = camera->hVidCore;
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
							lprintf( "Got a losefocus for %p at %P", hVideo, NULL );
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
			if( l.flags.bLogFocus )
				lprintf("Got Killfocus new focus to %p %p", hWnd, wParam);
#endif
         l.hVidFocused = NULL;
         hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
							(PVIDEO) GetWindowLong ((HWND) wParam, WD_HVIDEO);
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
#ifndef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
						{
							if( l.flags.bLogFocus )
								lprintf( "Got a losefocus for %p at %P", hVideo, hVidRecv );
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
		lprintf( "activate app on this window? %d", wParam );
#endif
	   break;
#endif
   case WM_ACTIVATE:
		if( hWnd == ((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance ) {
#ifdef LOG_ORDERING_REFOCUS
			Log2 ("Activate: %08x %08x", wParam, lParam);
#endif
			if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
			{
				hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
      hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
			hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
			if( !hVideo )
			{
#ifdef NOISY_LOGGING
				lprintf( "Too early, hVideo is not setup yet to reference." );
#endif
				Return 1;
			}
			pwp = (LPWINDOWPOS) lParam;

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
					if( hVideo->flags.bAbsoluteTopmost )
					{
						if( pwp->hwndInsertAfter != HWND_TOPMOST )
						{
#ifdef LOG_ORDERING_REFOCUS
							lprintf( "Just make sure we're the TOP TOP TOP window.... (more than1?!)" );
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
			}
	   }
	   Return 1;
		//break;
#endif
	case WM_WINDOWPOSCHANGED:
   //   break;
			{
			// global hVideo is pPrimary Video...
			LPWINDOWPOS pwp;
			pwp = (LPWINDOWPOS) lParam;
			hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
 #ifdef LOG_ORDERING_REFOCUS
			if( !pwp->hwndInsertAfter )
			{
            //WakeableSleep( 500 );
				lprintf( "..." );
			}
			lprintf( "Being inserted after %x %x", pwp->hwndInsertAfter, hWnd );
#endif
			if (!hVideo)      // not attached to anything...
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
			if( pwp->cx != hVideo->pWindowPos.cx ||
				pwp->cy != hVideo->pWindowPos.cy)
			{
				hVideo->pWindowPos.cx = pwp->cx;
				hVideo->pWindowPos.cy = pwp->cy;
#ifdef LOG_DISPLAY_RESIZE
				lprintf( "Resize happened, recreate drawing surface..." );
#endif
				CreateDrawingSurface (hVideo);
            // ??
			}
			LeaveCriticalSec( &hVideo->cs );

#ifdef LOG_ORDERING_REFOCUS
			lprintf( "window pos changed - new ordering includes %p for %p(%p)", pwp->hwndInsertAfter, pwp->hwnd, hWnd );
#endif
			// maybe maintain my link according to outside changes...
         // tried to make it work the other way( and it needs to work the other way)
			hVideo->pWindowPos = *pwp; // save always....!!!
			if( !hVideo->pWindowPos.hwndInsertAfter && hVideo->flags.bTopmost )
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
			hVideo->flags.bReady = 1;
		}
		Return 0;         // indicate handled message... no WM_MOVE/WM_SIZE generated.

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
		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
		if (!hVideo)
		{
			Return 0;
		}
		if( l.hCaptured )
		{
#ifdef LOG_MOUSE_EVENTS
			lprintf( "Captured mouse already - don't do anything?" );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
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
			lprintf( "Mouse position %d,%d", p.x, p.y );
#endif
			p.x -= (dx =(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.x);
			p.y -= (dy=(l.hCaptured?l.hCaptured:hVideo)->cursor_bias.y);
#ifdef LOG_MOUSE_EVENTS
			lprintf( "Mouse position results %d,%d %d,%d", dx, dy, p.x, p.y );
#endif

//			if (!(l.hCaptured?l.hCaptured:hVideo)->flags.bFull)
			{
//				p.x -= l.WindowBorder_X;
//				p.y -= l.WindowBorder_Y;
			}
#ifdef LOG_MOUSE_EVENTS
			lprintf( "Mouse position results %d,%d %d,%d", dx, dy, p.x, p.y );
#endif
         //l.real_mouse_x =
			l.mouse_x = p.x;
			l.mouse_y = p.y;
			// save now, so idle timer can hide cursor.
		}
		if( l.last_mouse_update )
		{
			if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
			{
            if( !l.flags.mouse_on ) // mouse is off...
					l.last_mouse_update = 0;
			}
		}
		if( l.mouse_x != l._mouse_x ||
			l.mouse_y != l._mouse_y ||
			l.mouse_b != l._mouse_b ||
		   l.mouse_last_vid != hVideo ) // this hvideo!= last hvideo?
		{
			_32 msg[4];
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
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "Tick ... %d", l.last_mouse_update );
#endif
			}
			l.mouse_last_vid = hVideo;
			msg[0] = (_32)(l.hCaptured?l.hCaptured:hVideo);
			msg[1] = l.mouse_x;
			msg[2] = l.mouse_y;
			msg[3] = l.mouse_b;
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
#ifdef LOG_MOUSE_EVENTS
			lprintf( "Generate mouse message %p(%p?) %d %d,%08x %d+%d=%d", l.hCaptured, hVideo, l.mouse_x, l.mouse_y, l.mouse_b, l.dwMsgBase, MSG_MouseMethod, l.dwMsgBase + MSG_MouseMethod );
#endif
			SendServiceEvent( 0, l.dwMsgBase + MSG_MouseMethod
								 , msg
								 , sizeof( msg ) );
#endif
			if( l.flags.bRotateLock )
			{
				RCOORD delta_x = l.mouse_x - (hVideo->pWindowPos.cx/2);
				RCOORD delta_y = l.mouse_y - (hVideo->pWindowPos.cy/2);
				lprintf( "mouse came in we're at %d,%d %g,%g", l.mouse_x, l.mouse_y, delta_x, delta_y );
				if( delta_y && delta_y )
				{
					static int toggle;
					delta_x /= hVideo->pWindowPos.cx;
					delta_y /= hVideo->pWindowPos.cy;
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
					l.mouse_x = hVideo->pWindowPos.cx/2;
					l.mouse_y = hVideo->pWindowPos.cy/2;
					lprintf( "Set curorpos.." );
					SetCursorPos( hVideo->pWindowPos.cx/2, hVideo->pWindowPos.cy / 2 );
					lprintf( "Set curorpos Done.." );
				}
			}
			else if (hVideo->pMouseCallback)
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
		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
		if( hVideo )
		{
			//lprintf( "Show Window hidden = %d (%d)", wParam, !wParam );
			hVideo->flags.bHidden = !wParam;
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
		}
      Return 0;
      //lprintf( "Fall through to WM_PAINT..." );
	case WM_PAINT:
#if 0 // in puregl mode... maybe just set dirty

		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
		if( l.flags.bLogWrites )
			lprintf( "Paint Message! %p", hVideo );
		if( hVideo->flags.event_dispatched )
		{
			ValidateRect( hWnd, NULL );
//#ifdef NOISY_LOGGING
			lprintf( "Validated rect... will you stop calling paint!?" );
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
			//UpdateDisplayPortion (hVideo, 0, 0, 0, 0);
			//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#else
			SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
#endif
		}
#endif
		//lprintf( "redraw... WM_PAINT" );
		ValidateRect (hWnd, NULL);
		//lprintf( "redraw... WM_PAINT" );
		/// allow a second invalidate to post.
//#ifdef NOISY_LOGGING
		lprintf( "Finished and clearing post Invalidate." );
//#endif
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
		lprintf( "ENDING SESSION!" );
		BAG_Exit( (int)lParam );
		Return TRUE; // uhmm okay.
	case WM_DESTROY:
#ifdef LOG_DESTRUCTION
		Log( "Destroying a window..." );
#endif
		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
			hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
		if( l.redraw_timer_id  == wParam )
		{
			Move( l.origin );

			{
				INDEX idx;
				Update3dProc proc;
				LIST_FORALL( l.update, idx, Update3dProc, proc )
					proc( l.origin );
			}

			if( l.flags.bUpdateWanted || 1 )
			{
				struct display_camera *camera;
				INDEX idx;
				l.flags.bUpdateWanted = 0;
				LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
				{
					if( !camera->hVidCore || !camera->hVidCore->flags.bReady )
						continue;
					// drawing may cause subsequent draws; so clear this first
					RenderD3D( camera );
				}
			}
		}
		if( l.mouse_timer_id == wParam )
		{
			if( l.flags.mouse_on && l.last_mouse_update )
			{
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
					while( x = ShowCursor( FALSE ) )
					{
					}
#ifdef LOG_MOUSE_HIDE_IDLE
					lprintf( "Show count %d %d", x, GetLastError() );
#endif
					SetCursor( NULL );
				}
			}
		}
      // this is a special return, it doesn't decrement the counter... cause WM_TIMER is filtered in OTHER_EVENTS logging
		return 0;
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
// definatly add whatever thread made it to the WM_CREATE.			
					AddLink( &l.threads, me );
					AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
					lprintf( "No thread %x, adding hook and thread.", GetCurrentThreadId() );
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
				struct display_camera *camera = (struct display_camera *)pcs->lpCreateParams; // user passed param...

				if( camera )
				{
					if( l.redraw_timer_id == 0 )
					{
						lprintf( "Setting up redraw timer.." );
						l.redraw_timer_id = SetTimer( hWnd, (UINT_PTR)1, 16, NULL );
						lprintf( "Setting up redraw timer.. result %d", l.redraw_timer_id );
					}
					hVideo = camera->hVidCore;
				}

				if ( !camera )
				{
					// directly created without parameter (Dialog control
					// probably )... grab a static PVIDEO for this...
					hVideo = &l.hDialogVid[(l.nControlVid++) & 0xf];
				}

				SetWindowLong (hWnd, WD_HVIDEO, (long) hVideo);

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
				//hVideo->flags.bReady = TRUE;
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

static void LoadOptions( void )
{
	_32 average_width, average_height;
	PODBC option = ConnectToDatabase( GetDefaultOptionDatabaseDSN() );
	SetOptionDatabaseOption( option, TRUE );
	int some_width;
	int some_height;
#ifndef __NO_OPTIONS__
	{
		int nDisplays = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Number of Displays", 1, TRUE );
		int n;
		_32 screen_w, screen_h;
		GetDisplaySizeEx( 0, NULL, NULL, &screen_w, &screen_h );

		switch( nDisplays )
		{
		default:
		case 0:
			average_width = screen_w;
			average_height = screen_h;
			break;
		case 6:
			average_width = screen_w/4;
			average_height = screen_h/3;
			break;
		}

		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Custom Position", n+1 );
			custom_pos = SACK_GetOptionIntEx( option, GetProgramName(), tmp, 0, TRUE );
			if( custom_pos )
			{
				camera->display = -1;
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/x", n+1 );
				camera->x = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?400:n==2?0:n==3?400:0
					:nDisplays==6
						?n==0?0:n==1?300:n==2?300:n==3?300:n==4?600:n==5?900:0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/y", n+1 );
				camera->y = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?0:n==2?300:n==3?300:0
					:nDisplays==6
						?n==0?240:n==1?0:n==2?240:n==3?480:n==4?240:n==5?240:0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/width", n+1 );
				camera->w = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?400
					:nDisplays==6
						?300
					:nDisplays==1
						?800
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/height", n+1 );
				camera->h = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?300
					:nDisplays==6
						?240
					:nDisplays==1
						?600
					:0), TRUE );
				/*
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/Direction", n+1 );
				camera->direction = SACK_GetOptionIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?1:n==2?2:n==3?3:0
					:nDisplays==6
						?n // this is natural 0=left, 2=forward, 1=up, 3=down, 4=right, 5=back
					:nDisplays==1
						?0
					:0), TRUE );
					*/
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Display", n+1 );
				camera->display = SACK_GetOptionIntEx( option, GetProgramName(), tmp, nDisplays>1?n+1:0, TRUE );
			}
			camera->origin_camera = CreateTransform();
			if( n == 0 )
			{
				Translate( camera->origin_camera, camera->w/2, camera->h/2, camera->w/2 );
				//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
				//ShowTransform( l.origin );
				// flip over
				RotateAbs( camera->origin_camera, M_PI, 0, 0 );
			}
			else if( n == 1 )
			{
				Translate( camera->origin_camera, camera->w/2, camera->h/2, camera->w/2 );
				//Translate( camera->origin_camera, -100, 150, 150 );
				//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
				//ShowTransform( l.origin );
				// flip over
				RotateAbs( camera->origin_camera, M_PI, M_PI/16, 0 );
			}
			else if( n == 2 )
			{
				Translate( camera->origin_camera, camera->w/2, camera->h/2, camera->w/2 );
				//Translate( camera->origin_camera, 900, 150, 150 );
				//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
				//ShowTransform( l.origin );
				// flip over
				RotateAbs( camera->origin_camera, M_PI, -M_PI/16, 0 );
			}
			else if( n == 3 )
			{
				Translate( camera->origin_camera, camera->w/2, camera->h/2, camera->w/2 );
				//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
				//ShowTransform( l.origin );
				// flip over
				RotateAbs( camera->origin_camera, M_PI, 0, 0 );
				RotateRel( camera->origin_camera, 0, 0, M_PI/32 );
			}
			camera->identity_depth = camera->w/2;
			InvokeExtraInit( camera, camera->origin_camera );			

			AddLink( &l.cameras, camera );
		}
	}
	l.flags.bView360 = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log focus event", 0, TRUE );
	l.flags.bLogFocus = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log focus event", 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log key event", 0, TRUE );
	l.flags.bLogKeyEvent = 1;
#ifndef NO_TRANSPARENCY
	if( l.UpdateLayeredWindow )
		l.flags.bLayeredWindowDefault = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Default windows are layered", 0, TRUE )?TRUE:FALSE;
	else
#endif
		l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Log Video Output", 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Use Low Level Keyhook", 0, TRUE );
#else
#endif
	if( !l.origin )
	{
		static MATRIX m;
		l.origin = CreateNamedTransform( "render.camera" );

		Translate( l.origin, average_width/2, average_height/2, average_height/2 );
		RotateAbs( l.origin, M_PI, 0, 0 );


		//Translate( l.origin, 200, 150, 150 );
		//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
		//ShowTransform( l.origin );
		// flip over
		//RotateAbs( l.origin, M_PI, 0, 0 );
		//ShowTransform( l.origin );
		CreateTransformMotion( l.origin ); // some things like rotate rel
		{
			VECTOR v;
			v[0] = 0;
			v[1] = 0; //3.14/2;
			v[2] = 3.14/4;
			//SetRotation( l.origin, v );
		}
	}
}


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

//   v � w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v � w)/(|v| |w|) = cos �     

   a = dotproduct( pv1, pv2 );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
	{
#ifdef DEBUG_PLANE_INTERSECTION
		Log( "Planes are not parallel" );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   lprintf( " a: %g b: %g c: %g cos: %g \n", a, b, c, cosTheta );
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

//   v � w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v � w)/(|v| |w|) = cos �     

	//cosPhi = CosAngle( Slope, n );

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
	{
		//Log1( DBG_FILELINEFMT "Bad choice - slope vs normal is 0" DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( "Slope and or n are near 0" );
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
		Log1( "Parallel... %g\n", cosPhi );
		PrintVector( Slope );
		PrintVector( n );
      // plane and line are parallel if slope and normal are perpendicular
//      lprintf("Parallel...\n");
		return 0;
	}
}



// h
static int CPROC InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y )
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
		ApplyInverse( l.origin, v1, v2 );

		//lprintf( "%g,%g,%g  from %g,%g,%g ", v1[0], v1[1], v1[2], v2[0], v2[1] , v2[2] );

		// so this puts the point back in world space
		{
			RCOORD t;
			RCOORD cosphi;
			VECTOR v4;
			SetPoint( v4, _0 );
			v4[2] = 1.0;

			cosphi = IntersectLineWithPlane( v2, _0, _Z, v4, &t );
			//lprintf( "t is %g  cosph = %g", t, cosphi );
			if( cosphi != 0 )
				addscaled( v2, _0, v1, t );

			//lprintf( "%g,%g,%g  ", v1[0], v1[1],v1[2] );

			//surface_x * l.viewport[2] +l.viewport[2] =   ((float)l.mouse_x-((float)l.viewport[2]/2.0f) )*0.25f/(float)l.viewport[2]
		}

		//lprintf( "%g,%g became like %g,%g,%g or %g,%g", x, y
   		//		 , v1[0], v1[1], v1[2]
		//		 , (v1[0]/2.5 * l.viewport[2]) + (l.viewport[2]/2)
		//		 , (l.viewport[3]/2) - (v1[1]/(2.5/l.aspect) * l.viewport[3])
		//   	 );
#define BEGIN_SCALE 1
#define COMMON_SCALE ( 2*camera->aspect)
#define END_SCALE 1000
#define tmp_param1 (END_SCALE*COMMON_SCALE)
		if( result_x )
			(*result_x) = (int)((v2[0]/COMMON_SCALE * camera->viewport[2]) + (camera->viewport[2]/2));
		if( result_y )
			(*result_y) = (camera->viewport[3]/2) - (v2[1]/(COMMON_SCALE/camera->aspect) * camera->viewport[3]);
	}
	return 1;
}



static int CPROC OpenGLMouse( PTRSZVAL psvMouse, S_32 x, S_32 y, _32 b )
{
	int used = 0;
	PRENDERER check;
	struct display_camera *camera = (struct display_camera *)psvMouse;
	if( camera->origin_camera )
	{
		VECTOR mouse_ray_slope;
		VECTOR mouse_ray_origin;
		VECTOR mouse_ray_target;
		VECTOR tmp1, tmp2;
		static PTRANSFORM t;
		if( !t )
			t = CreateTransform();

		// camera origin is already relative to l.origin 
		//   as l.origin rotates, the effetive camera origin also rotates.

		ApplyT( camera->origin_camera, t, l.origin );
		//GetOriginV( camera->origin_camera, tmp1 );
		//InvertVector( tmp1 );
		//ApplyRotation( l.origin, tmp1, GetOrigin( camera->origin_camera ) );
		//add( (P_POINT)GetOrigin( t ), tmp1, GetOrigin( l.origin  ) );

		//ApplyT( camera->origin_camera, t, l.origin );
		/*
		addscaled( mouse_ray_origin, GetOrigin( t ), GetAxis( t, vForward ), BEGIN_SCALE );
		addscaled( mouse_ray_origin, mouse_ray_origin, GetAxis( t, vRight ), -((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( mouse_ray_origin, mouse_ray_origin, GetAxis( t, vUp ), -((float)l.mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );
		PrintVector( mouse_ray_origin );
		addscaled( mouse_ray_target, GetOrigin( t ), GetAxis( t, vForward ), END_SCALE );
		addscaled( mouse_ray_target, mouse_ray_target, GetAxis( t, vRight ), -tmp_param1*((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( mouse_ray_target, mouse_ray_target, GetAxis( t, vUp ), -(tmp_param1/camera->aspect)*((float)l.mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );
		PrintVector( mouse_ray_target );
		*/
		ShowTransform( l.origin );
		ShowTransform( camera->origin_camera );
		ShowTransform( t );
		addscaled( mouse_ray_origin, _0, _Z, BEGIN_SCALE );
		addscaled( mouse_ray_origin, mouse_ray_origin, _X, ((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( mouse_ray_origin, mouse_ray_origin, _Y, -((float)l.mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );
		addscaled( mouse_ray_target, _0, _Z, END_SCALE );
		addscaled( mouse_ray_target, mouse_ray_target, _X, tmp_param1*((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( mouse_ray_target, mouse_ray_target, _Y, -(tmp_param1/camera->aspect)*((float)l.mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );

		// this is definaly the correct rotation
		ApplyInverseRotation( t, tmp1, mouse_ray_origin );
		ApplyTranslation( t, mouse_ray_origin, tmp1 );
		ApplyInverseRotation( t, tmp1, mouse_ray_target );
		ApplyTranslation( t, mouse_ray_target, tmp1 );

		//ApplyInverseRotation( t, tmp1, mouse_ray_origin );
		//ApplyTranslation( t, mouse_ray_origin, tmp1 );
		//ApplyInverseRotation( t, tmp1, mouse_ray_target );
		//ApplyTranslation( t, mouse_ray_target, tmp1 );
		sub( mouse_ray_slope, mouse_ray_target, mouse_ray_origin );
		PrintVector( mouse_ray_origin );
		PrintVector( mouse_ray_target );
		
		SetPoint( camera->mouse_ray.n, mouse_ray_slope );
		SetPoint( camera->mouse_ray.o, mouse_ray_origin );
		SetRay( &l.mouse_ray, &camera->mouse_ray );

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				if( ref->Mouse3d )
				{
					used = ref->Mouse3d( ref->psv, &camera->mouse_ray, x, y, b );
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
				cosphi = IntersectLineWithPlane( mouse_ray_slope, mouse_ray_origin
												, GetAxis( check->transform, vForward )
												, GetOrigin( check->transform ), &t );
				if( cosphi != 0 )
					addscaled( target_point, mouse_ray_origin, mouse_ray_slope, t );
				//PrintVector( target_point );
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

				l.real_mouse_x = target_point[0];
				l.real_mouse_y = target_point[1];

				ApplyInverse( check->transform, target_surface_point, target_point );
				//PrintVector( target_surface_point );
				newx = (int)target_surface_point[0];
				newy = (int)target_surface_point[1];
				//lprintf( "Is %d,%d in %d,%d %dx%d"
				 //  	 ,newx, newy
				 //  	 ,check->pWindowPos.x, check->pWindowPos.y
				 //  	 , check->pWindowPos.x+ check->pWindowPos.cx
				 //  	 , check->pWindowPos.y+ check->pWindowPos.cy );

				if( check == l.hCaptured ||
					( ( newx >= 0 && newx < (check->pWindowPos.cx ) )
					 && ( newy >= 0 && newy < (check->pWindowPos.cy ) ) ) )
				{
					if( check && check->pMouseCallback)
					{
						lprintf( "Sent Mouse Proper. %d,%d %08x", newx, newy, l.mouse_b );
						//InverseOpenGLMouse( camera, check, newx, newy, NULL, NULL );
						used = check->pMouseCallback( check->dwMouseData
													, newx
													, newy
													, l.mouse_b );
						if( used )
							break;
					}
				}
			}
		}
	}
	return used;
}


static void HandleMessage (MSG Msg);

#ifndef HWND_MESSAGE
#define HWND_MESSAGE     ((HWND)-3)
#endif

#ifdef UNDER_CE
#define WINDOW_STYLE 0
#else
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
#endif

BOOL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
#ifndef __NO_WIN32API__
	static HMODULE hMe;
	struct display_camera *camera;
	INDEX idx;
	if( hMe == NULL )
		hMe = GetModuleHandle (_WIDE(TARGETNAME));
	//lprintf( "-----Create WIndow Stuff----- %s %s", hVideo->flags.bLayeredWindow?"layered":"solid"
	//		 , hVideo->flags.bChildWindow?"Child(tool)":"user-selectable" );
	LoadOptions();

	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		lprintf( "camera instance %p", camera->hWndInstance );
		if( !camera->hWndInstance )
		{
			int UseCoords = camera->display == -1;
			int DefaultScreen = camera->display;
			S_32 x, y;
			_32 w, h;
			// if we don't do it this way, size_t overflows in camera definition.
			if( !UseCoords )
			{
				GetDisplaySizeEx( DefaultScreen, &x, &y, &w, &h );
				camera->x = x;
				camera->y = y;
				camera->w = w;
				camera->h = h;
			}
			else
			{
				w = (_32)camera->w;
				h = (_32)camera->h;
			}


			x = camera->x;
			y = camera->y;

			camera->aspect = ((float)w/(float)h);
			lprintf( "aspect is %g", camera->aspect );

			lprintf("Creating container window named: %s",
					(l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo?hVideo->pTitle:"No Name");

	#ifdef LOG_OPEN_TIMING
			lprintf( "Created Real window...Stuff.. %d,%d %dx%d",x,y,w,h );
	#endif
			camera->hVidCore = New(VIDEO);
			MemSet (camera->hVidCore, 0, sizeof (VIDEO));
			camera->hVidCore->camera = camera;
			camera->hVidCore->pMouseCallback = OpenGLMouse;
			camera->hVidCore->dwMouseData = (PTRSZVAL)camera;
			camera->viewport[0] = x;
			camera->viewport[1] = y;
			camera->viewport[2] = (int)w;
			camera->viewport[3] = (int)h;
			camera->hWndInstance = CreateWindowEx (0
	#ifndef NO_DRAG_DROP
														| WS_EX_ACCEPTFILES
	#endif
	#ifdef UNICODE
													  , (LPWSTR)l.aClass
	#else
													  , (LPSTR)l.aClass
	#endif
													  , (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo?hVideo->pTitle:""
													  , WS_POPUP //| WINDOW_STYLE
													  , x, y, (int)w, (int)h
													  , NULL // HWND_MESSAGE  // (GetDesktopWindow()), // Parent
													  , NULL // Menu
													  , hMe
													  , (void*)camera );
	#ifdef LOG_OPEN_TIMING
			lprintf( "Created Real window...Stuff.." );
	#endif
			camera->hVidCore->hWndOutput = (HWND)camera->hWndInstance;
			EnableD3D( camera );
			ShowWindow( camera->hWndInstance, SW_SHOWNORMAL );
			lprintf( "ShowWindow error: %d", GetLastError() );
			while( !camera->hVidCore->flags.bReady )
			{
				MSG Msg;
				if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
				{
					if( l.flags.bLogMessageDispatch )
						lprintf( "(E)Got message:%d", Msg.message );
					HandleMessage (Msg);
				}
			}
			if (!camera->hWndInstance)
			{
				return FALSE;
			}
		}
	}

		if( hVideo )
		{
			if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
			{
				wx = camera->viewport[2] * 7 / 10;
				wy = camera->viewport[3] * 7 / 10;
			}
			if( x == CW_USEDEFAULT )
				x = 10;
			if( y == CW_USEDEFAULT )
				y = 10;
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
				// hWndOutput is set within the create window proc...
		#ifdef LOG_OPEN_TIMING
				lprintf( "Create Real Window (In CreateWindowStuff).." );
		#endif

				hVideo->hWndOutput = (HWND)1;
				hVideo->pWindowPos.x = x;
				hVideo->pWindowPos.y = y;
				hVideo->pWindowPos.cx = wx;
				hVideo->pWindowPos.cy = wy;
				//lprintf( "%d %d", x, y );
				CreateDrawingSurface (hVideo);

				hVideo->flags.bReady = 1;
				WakeThread( hVideo->thread );
			  //CreateWindowEx used to be here
			}
			else
			{

			}


		#ifdef LOG_OPEN_TIMING
			lprintf( "Created window stuff..." );
		#endif
			// generate an event to dispatch pending...
			// there is a good chance that a window event caused a window
			// and it will be sleeping until the next event...
		#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
			SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
		#endif
			//Log ("Created window in module...");

		}

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
						SetWindowLong( hVideo->hWndOutput, WD_HVIDEO, 0 );
						Deallocate( PVIDEO, hVideo ); // last event in queue, should be safe to go away now...
					}
					SetLink( &l.pInactiveList, idx, NULL );
				}
			}
			LIST_FORALL( l.pActiveList, idx, PVIDEO, hVideo )
			{
            if( hVideo->flags.mouse_pending )
				{
					if( hVideo && hVideo->pMouseCallback)
					{
#ifdef LOG_MOUSE_EVENTS
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
			PVIDEO hVideo = (PVIDEO)params[0];
			if( l.flags.bLogFocus )
				lprintf( "Got a losefocus for %p at %P", params[0], params[1] );
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
			PVIDEO hVideo = (PVIDEO)params[0];
#ifdef LOG_MOUSE_EVENTS
			lprintf( "mouse method... forward to application please..." );
         lprintf( "params %ld %ld %ld", params[1], params[2], params[3] );
#endif
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo->flags.mouse_pending )
				{
					if( params[3] != hVideo->mouse.b ||
						( params[3] & MK_OBUTTON ) )
					{
#ifdef LOG_MOUSE_EVENTS
						lprintf( "delta dispatching mouse (%d,%d) %08x"
								 , hVideo->mouse.x
								 , hVideo->mouse.y
								 , hVideo->mouse.b );
#endif
						if( !hVideo->flags.event_dispatched )
						{
							hVideo->flags.event_dispatched = 1;
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
			PVIDEO hVideo = (PVIDEO)params[0];
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				if( hVideo && hVideo->pKeyProc )
				{
					hVideo->flags.event_dispatched = 1;
					//lprintf( "Dispatched KEY!" );
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
							params[1] = (_32)DequeLink( &hVideo->pInput );
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
#ifdef LOG_DESTRUCTION
		lprintf( "To destroy! %p %d", 0 /*Msg.lParam*/, hVidDestroy->hWndOutput );
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
			lprintf( "Set ourselves inactive..." );
#endif
			//SetActiveWindow( l.hWndInstance );
#ifdef LOG_DESTRUCTION
			lprintf( "Set foreground to instance..." );
#endif
		}
		if( GetFocus() == hVidDestroy->hWndOutput)
		{
#ifdef LOG_DESTRUCTION
			lprintf( "Fixed focus away from ourselves before destroy." );
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
		lprintf( "------------ DESTROY! -----------" );
#endif
 		DestroyWindow (hVidDestroy->hWndOutput);
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( "From destroy" );
#endif
	}
}
//----------------------------------------------------------------------------

void HandleMessage (MSG Msg)
{
#ifdef USE_XP_RAW_INPUT
	//#if(_WIN32_WINNT >= 0x0501)
#define WM_INPUT                        0x00FF
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
			LPBYTE lpb = Allocate( dwSize );
			if (lpb == NULL)
			{
				return;
			}

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,
									  sizeof(RAWINPUTHEADER)) != dwSize )
				OutputDebugString (TEXT("GetRawInputData doesn't return correct size !\n"));

			{
				RAWINPUT* raw = (RAWINPUT*)lpb;
				TEXTCHAR szTempOutput[256];
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
			Release( lpb );
		}
		DispatchMessage (&Msg);
	}

	else
#endif
		if (!Msg.hwnd && (Msg.message == (WM_USER_CREATE_WINDOW)))
	{
		PVIDEO hVidCreate = (PVIDEO) Msg.lParam;
#ifdef LOG_OPEN_TIMING
		lprintf( "Message Create window stuff..." );
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
#ifdef LOG_SHOW_HIDE
		PVIDEO hVideo = ((PVIDEO)Msg.lParam);
		lprintf( "Handling HIDE_WINDOW posted message %p",hVideo->hWndOutput );
#endif
		((PVIDEO)Msg.lParam)->flags.bHidden = 1;
      //AnimateWindow( ((PVIDEO)Msg.lParam)->hWndOutput, 0, AW_HIDE );
		ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_HIDE );
#ifdef LOG_SHOW_HIDE
		lprintf( "Handled HIDE_WINDOW posted message %p",hVideo->hWndOutput );
#endif
		
	}
	else if( Msg.message == (WM_USER_SHOW_WINDOW ) )
	{
      PVIDEO hVideo = (PVIDEO)Msg.lParam;
      lprintf( "Handling SHOW_WINDOW message! %p", Msg.lParam );
      //ShowWindow( ((PVIDEO)Msg.lParam)->hWndOutput, SW_RESTORE );
      //lprintf( "Handling SHOW_WINDOW message! %p", Msg.lParam );
		if( hVideo->flags.bTopmost )
			SetWindowPos( hVideo->hWndOutput
							, HWND_TOPMOST
							, 0, 0, 0, 0,
							 SWP_NOMOVE
							 | SWP_NOSIZE
							);
		if( hVideo->flags.bShown )
			ShowWindow( hVideo->hWndOutput, SW_RESTORE );
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
				if( l.flags.bLogMessageDispatch )
					lprintf( "(E)Got message:%d", Msg.message );
				HandleMessage (Msg);
				if( l.flags.bLogMessageDispatch )
					lprintf( "(X)Got message:%d", Msg.message );
				if (l.bExitThread)
					return -1;
				return TRUE;
			}
			return FALSE;
		}
	}
	return -1;
}

//----------------------------------------------------------------------------
PTRSZVAL CPROC VideoThreadProc (PTHREAD thread)
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

   if( l.flags.bUseLLKeyhook )
		AddLink( &l.ll_keyhooks,
				  SetWindowsHookEx (WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
										 ,GetModuleHandle(_WIDE(TARGETNAME)), 0 /*GetCurrentThreadId()*/
										 ) );
   else
		AddLink( &l.keyhooks,
				  SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyHook
										, NULL /*GetModuleHandle(_WIDE(TARGETNAME)*/, GetCurrentThreadId()
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
	//l.pid = (_32)l.dwThreadID;
	l.bThreadRunning = TRUE;
	AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
	//AddIdleProc ( ProcessClientMessages, 0);
#ifdef LOG_STARTUP
	Log( "Registered Idle, and starting message loop" );
#endif
	{
		MSG Msg;
		while( !l.bExitThread && GetMessage (&Msg, NULL, 0, 0) )
		{
         if( l.flags.bLogMessageDispatch )
				lprintf( "Dispatched... %d", Msg.message );
			HandleMessage (Msg);
			if( l.flags.bLogMessageDispatch )
				lprintf( "Finish Dispatched... %d", Msg.message );
		}
	}
	l.bThreadRunning = FALSE;
	lprintf( "Video Exited volentarily" );
	//ExitThread( 0 );
	return 0;
}

//----------------------------------------------------------------------------

int  InitDisplay (void)
{
#ifndef __NO_WIN32API__
	WNDCLASS wc;
	if (!l.aClass)
	{
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
		wc.lpszClassName = "GLVideoOutputClass";
		wc.cbWndExtra = 4;   // one extra DWORD

		l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
			lprintf( "Failed to register class %s %d", wc.lpszClassName, GetLastError() );
			return FALSE;
		}

#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
		InitMessageService();
		l.dwMsgBase = LoadService( NULL, VideoEventHandler );
#endif

		AddLink( &l.threads, ThreadTo( VideoThreadProc, 0 ) );
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
				Log1( "Failed to post shutdown message...%d", error );

				if( error == ERROR_INVALID_THREAD_ID )
               break;
				cnt--;
			}
			Relinquish();
		}
		while (!d && cnt);
		if (!d)
		{
			lprintf( "Tried %d times to post thread message... and it alwasy failed.", 25-cnt );
			//DebugBreak ();
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
				lprintf( "Had to give up waiting for video thread to exit..." );
		}
	}
}
#endif

const TEXTCHAR*  GetKeyText (int key)
{
	static int c;
	static TEXTCHAR ch[5];
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
		//printf( "no translation\n" );
		return 0;
	}
	else if (c == 2)
	{
		//printf( "Key Translated: %d %d\n", ch[0], ch[1] );
		return 0;
	}
	else if (c < 0)
	{
		//printf( "Key Translation less than 0\n" );
		return 0;
	}
	//printf( "Key Translated: %d(%c)\n", ch[0], ch[0] );
	return ch;
}

//----------------------------------------------------------------------------

LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
{
	InitDisplay();
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
			AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );
#if 0 // this id added in the thread anyhow
#ifdef USE_KEYHOOK
			if( l.flags.bUseLLKeyhook )
				AddLink( &l.ll_keyhooks, added =
						  SetWindowsHookEx (WH_KEYBOARD_LL, (HOOKPROC)KeyHook2
												 ,GetModuleHandle(_WIDE(TARGETNAME)), 0/*GetCurrentThreadId()*/
												 ) );
         else
				AddLink( &l.keyhooks,
						  added = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyHook
														  , NULL /*GetModuleHandle(_WIDE(TARGETNAME))*/, GetCurrentThreadId()
														  )
						 );
#endif
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
				lprintf( "Registration failed!?" );
				//registration failed. Call GetLastError for the cause of the error
			}
		}
#endif
	}
	if( !l.bottom )
		l.bottom = hNextVideo;
	if( !l.top )
		l.top = hNextVideo;
	AddLink( &l.pActiveList, hNextVideo );
	//hNextVideo->pid = l.pid;
	hNextVideo->KeyDefs = CreateKeyBinder();
#ifdef LOG_OPEN_TIMING
	lprintf( "Doing open of a display..." );
#endif
	if( ( GetCurrentThreadId () == l.dwThreadID ) || hNextVideo->hWndContainer )
	{
#ifdef LOG_OPEN_TIMING
		lprintf( "Allowed to create my own stuff..." );
#endif
		lprintf( "about to Create some window stuff" );
		CreateWindowStuffSizedAt( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										, hNextVideo->pWindowPos.cy);
		lprintf( "Created some window stuff" );
	}
	else
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			lprintf( "posting create to thred." );
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
			DebugBreak ();
		}
		if( hNextVideo )
		{
			_32 timeout = timeGetTime() + 500000;
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
			//if( !hNextVideo->flags.bReady )
			{
				//CloseDisplay( hNextVideo );
				//lprintf( "Fatality.. window creation did not complete in a timely manner." );
				// hnextvideo is null anyhow, but this is explicit.
				//return FALSE;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( "Resulting new window %p %d", hNextVideo, hNextVideo->hWndOutput );
#endif
	return TRUE;
}

PVIDEO  MakeDisplayFrom (HWND hWnd) 
{
	
	PVIDEO hNextVideo;
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
	hNextVideo->hWndContainer = hWnd;
	hNextVideo->flags.bFull = TRUE;
   // should check styles and set rendered according to hWnd
   hNextVideo->flags.bLayeredWindow = l.flags.bLayeredWindowDefault;
	if( DoOpenDisplay( hNextVideo ) )
	{
		lprintf( "New bottom is %p", l.bottom );
		return hNextVideo;
	}
	Deallocate( PVIDEO, hNextVideo );
	return NULL;
#if 0
	SetWindowLong( hWnd, GWL_WNDPROC, (DWORD)VideoWindowProc );
	{
		CREATESTRUCT cs;
      cs.lpCreateParams = (void*)hNextVideo;
		SendMessage( hWnd, WM_CREATE, 0, (LPARAM)&cs );
	}
#endif
	//return hNextVideo;
}


PVIDEO  OpenDisplaySizedAt (_32 attr, _32 wx, _32 wy, S_32 x, S_32 y) // if native - we can return and let the messages dispatch...
{
	PVIDEO hNextVideo;
   lprintf( "open display..." );
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
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
	hNextVideo->flags.bNoMouse = (attr & DISPLAY_ATTRIBUTE_NO_MOUSE)?TRUE:FALSE;
	hNextVideo->pWindowPos.x = x;
	hNextVideo->pWindowPos.y = y;
	hNextVideo->pWindowPos.cx = wx;
	hNextVideo->pWindowPos.cy = wy;

	if( DoOpenDisplay( hNextVideo ) )
	{
      lprintf( "New bottom is %p", l.bottom );
      return hNextVideo;
	}
	Deallocate( PVIDEO, hNextVideo );
	return NULL;
}

 void  SetDisplayNoMouse ( PVIDEO hVideo, int bNoMouse )
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

PVIDEO  OpenDisplayAboveSizedAt (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PVIDEO parent)
{
   PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

PVIDEO  OpenDisplayAboveUnderSizedAt (_32 attr, _32 wx, _32 wy,
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
		if( barrier )
		{
			if( l.bottom == barrier )
			{
				l.bottom = newvid;
			}
			newvid->pBelow = barrier;
			newvid->pAbove = barrier->pAbove;
			barrier->pAbove = newvid;
		}
		//lprintf( "Opening window behind another." );
		//lprintf( "--- before SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );

		//SetWindowPos( newvid->hWndOutput, barrier->hWndOutput, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	   //lprintf( "--- after SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );
   }
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

//----------------------------------------------------------------------------

void  CloseDisplay (PVIDEO hVideo)
{
	lprintf( "close display %p", hVideo );
	// just kills this video handle....
	if (!hVideo)         // must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log ("Unlinking destroyed window...");
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	UnlinkVideo( hVideo );
	lprintf( "and we should be ok?" );
	hVideo->flags.bDestroy = 1;

	// the scan of inactive windows releases the hVideo...
	AddLink( &l.pInactiveList, hVideo );
	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
	SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
#endif
   return;
}

//----------------------------------------------------------------------------

void  SizeDisplay (PVIDEO hVideo, _32 w, _32 h)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "Size Display..." );
#endif
	if( w == hVideo->pWindowPos.cx && h == hVideo->pWindowPos.cy )
		return;
	if( hVideo->flags.bLayeredWindow )
	{
		// need to remake image surface too...
		hVideo->pWindowPos.cx = w;
		hVideo->pWindowPos.cy = h;
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
		SetWindowPos (hVideo->hWndOutput, hVideo->pWindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
					, SWP_NOMOVE|SWP_NOACTIVATE);
}


//----------------------------------------------------------------------------

void  SizeDisplayRel (PVIDEO hVideo, S_32 delw, S_32 delh)
{
	if (delw || delh)
	{
		S_32 cx, cy;
		cx = hVideo->pWindowPos.cx + delw;
		cy = hVideo->pWindowPos.cy + delh;
		if (hVideo->pWindowPos.cx < 50)
			hVideo->pWindowPos.cx = 50;
		if (hVideo->pWindowPos.cy < 20)
			hVideo->pWindowPos.cy = 20;
#ifdef LOG_RESIZE
		Log2 ("Resized display to %d,%d", hVideo->pWindowPos.cx,
            hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( "size display relative" );
#endif
		SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
                    SWP_NOZORDER | SWP_NOMOVE);
   }
}

//----------------------------------------------------------------------------

void  MoveDisplay (PVIDEO hVideo, S_32 x, S_32 y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( "Move display %d,%d", x, y );
#endif
   if( hVideo )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
			Translate( hVideo->transform, x, y, 0.0 );
			if( hVideo->flags.bShown )
			{
				// layered window requires layered output to be called to move the display.
				UpdateDisplay( hVideo );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  MoveDisplayRel (PVIDEO hVideo, S_32 x, S_32 y)
{
   if (x || y)
   {
		lprintf( "Moving display %d,%d", x, y );
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
		Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
#ifdef LOG_ORDERING_REFOCUS
		lprintf( "Move display relative" );
#endif
   }
}

//----------------------------------------------------------------------------

void  MoveSizeDisplay (PVIDEO hVideo, S_32 x, S_32 y, S_32 w,
                                     S_32 h)
{
   S_32 cx, cy;
   hVideo->pWindowPos.x = x;
	hVideo->pWindowPos.y = y;
   cx = w;
   cy = h;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->pWindowPos.cx = cx;
   hVideo->pWindowPos.cy = cy;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( "move and size display." );
#endif
   // updates window translation
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  MoveSizeDisplayRel (PVIDEO hVideo, S_32 delx, S_32 dely,
                                        S_32 delw, S_32 delh)
{
	S_32 cx, cy;
   hVideo->pWindowPos.x += delx;
   hVideo->pWindowPos.y += dely;
   cx = hVideo->pWindowPos.cx + delw;
   cy = hVideo->pWindowPos.cy + delh;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->pWindowPos.cx = cx;
   hVideo->pWindowPos.cy = cy;
//fdef LOG_DISPLAY_RESIZE
	lprintf( "move and size relative %d,%d %d,%d", delx, dely, delw, delh );
//ndif
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  UpdateDisplayEx (PVIDEO hVideo DBG_PASS )
{
   // copy hVideo->lpBuffer to hVideo->hDCOutput
   if (hVideo )
   {
      UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
   }
   return;
}

//----------------------------------------------------------------------------

void  ClearDisplay (PVIDEO hVideo)
{
   lprintf( "Who's calling clear display? it's assumed clear" );
   //ClearImage( hVideo->pImage );
}

//----------------------------------------------------------------------------

void  SetMousePosition (PVIDEO hVid, S_32 x, S_32 y)
{
	if( !hVid )
	{
		int newx, newy;
		lprintf( "TAGHERE" );
		InverseOpenGLMouse( hVid->camera, hVid, (RCOORD)x, (RCOORD)y, &newx, &newy );
		lprintf( "%d,%d became %d,%d", x, y, newx, newy );
		SetCursorPos( newx, newy );
	}
	else
	{
		if (hVid->flags.bFull)
		{
			int newx, newy;
			//lprintf( "TAGHERE" );
			InverseOpenGLMouse( hVid->camera, hVid, x+ hVid->cursor_bias.x, y, &newx, &newy );
			//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
			SetCursorPos (newx,newy);
		}
		else
		{
			SetCursorPos (x + l.WindowBorder_X + hVid->cursor_bias.x,
							  y + l.WindowBorder_Y + hVid->cursor_bias.y);
		}
	}
}

//----------------------------------------------------------------------------

void  GetMousePosition (S_32 * x, S_32 * y)
{
	lprintf( "This is really relative to what is looking at it " );
	DebugBreak();
	if (x)
		(*x) = l.real_mouse_x;
	if (y)
		(*y) = l.real_mouse_y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
	GetMousePosition( x, y );
	if( b )
		(*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

void  SetCloseHandler (PVIDEO hVideo,
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

void  SetMouseHandler (PVIDEO hVideo,
                                     MouseCallback pMouseCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}

void  SetHideHandler (PVIDEO hVideo,
                                     HideAndRestoreCallback pHideCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwHideData = dwUser;
   hVideo->pHideCallback = pHideCallback;
}

void  SetRestoreHandler (PVIDEO hVideo,
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

void  SetRedrawHandler (PVIDEO hVideo,
                                      RedrawCallback pRedrawCallback,
                                      PTRSZVAL dwUser)
{
	hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		//lprintf( "Sending redraw for %p", hVideo );
		if( hVideo->flags.bShown )
		{
			Redraw( hVideo );
			//lprintf( "Invalida.." );
			//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
			//SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
		}
		//hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
	}

}

//----------------------------------------------------------------------------

void  SetKeyboardHandler (PVIDEO hVideo, KeyProc pKeyProc,
                                        PTRSZVAL dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

void  SetLoseFocusHandler (PVIDEO hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         PTRSZVAL dwUser)
{
	hVideo->dwLoseFocus = dwUser;
	hVideo->pLoseFocus = pLoseFocus;
}

//----------------------------------------------------------------------------

void  SetApplicationTitle (const TEXTCHAR *pTitle)
{
	l.gpTitle = pTitle;
	if (l.cameras)
	{
      //DebugBreak();
		SetWindowText((((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance), l.gpTitle);
	}
}

//----------------------------------------------------------------------------

void  SetRendererTitle (PVIDEO hVideo, const TEXTCHAR *pTitle)
{
	//l.gpTitle = pTitle;
	//if (l.hWndInstance)
	{
		if( hVideo->pTitle )
			Deallocate( TEXTCHAR *, hVideo->pTitle );
		hVideo->pTitle = StrDup( pTitle );
		SetWindowText( hVideo->hWndOutput, pTitle );
	}
}

//----------------------------------------------------------------------------

void  SetApplicationIcon (ImageFile * hIcon)
{
#ifdef _WIN32
   //HICON hIcon = CreateIcon();
#endif
}

//----------------------------------------------------------------------------

void  MakeTopmost (PVIDEO hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		if( hVideo->flags.bShown )
		{
			//lprintf( "Forcing topmost" );
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							  SWP_NOMOVE | SWP_NOSIZE);
		}
		else
		{
		}
	}
}

//----------------------------------------------------------------------------

void  MakeAbsoluteTopmost (PVIDEO hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		hVideo->flags.bAbsoluteTopmost = 1;
		if( hVideo->flags.bShown )
		{
			lprintf( "Forcing topmost" );
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
							  SWP_NOMOVE | SWP_NOSIZE);
		}
	}
}

//----------------------------------------------------------------------------

 int  IsTopmost ( PVIDEO hVideo )
{
   return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
void  HideDisplay (PVIDEO hVideo)
{
//#ifdef LOG_SHOW_HIDE
	lprintf("Hiding the window! %p %p %p %p", hVideo, hVideo->hWndOutput, hVideo->pAbove, hVideo->pBelow );
//#endif
	if( hVideo )
	{
		if( l.hCaptured == hVideo )
		{
			l.hCapturedPrior = NULL;
			l.hCaptured = NULL;
		}
		hVideo->flags.bHidden = 1;
		/* handle lose focus */
	}
}

//----------------------------------------------------------------------------
#undef RestoreDisplay
void RestoreDisplay( PVIDEO hVideo  )
{
   lprintf( "We should not be here!" );
   RestoreDisplayEx( hVideo DBG_SRC );
}


void RestoreDisplayEx(PVIDEO hVideo DBG_PASS )
{
//#ifdef LOG_SHOW_HIDE
	lprintf( "Restore display. %p %p", hVideo, hVideo->hWndOutput );
//#endif
	if( hVideo )
	{
		hVideo->flags.bHidden = 0;
		hVideo->flags.bShown = 1;
	}
}

//----------------------------------------------------------------------------

void  GetDisplaySizeEx ( int nDisplay
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
			dm.dmSize = sizeof( DEVMODE );
			dev.cb = sizeof( DISPLAY_DEVICE );
			for( v_test = 0; !found && ( v_test < 2 ); v_test++ )
			{
            // go ahead and try to find V devices too... not sure what they are, but probably won't get to use them.
				snprintf( teststring, 20, "\\\\.\\DISPLAY%s%d", (v_test==1)?"V":"", nDisplay );
				for( i = 0;
					 !found && EnumDisplayDevices( NULL // all devices
														  , i
														  , &dev
														  , 0 // dwFlags
														  ); i++ )
				{
					if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
					{
						lprintf( "display %s is at %d,%d %dx%d", dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					else
						lprintf( "Found display name, but enum current settings failed? %s %d", teststring, GetLastError() );
					lprintf( "[%s] might be [%s]", teststring, dev.DeviceName );
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
         // Ex version should return real screen size for screen 0.
			if( x )
				(*x)= 0;
			if( y )
				(*y)= 0;
			{
				RECT r;
				GetWindowRect (GetDesktopWindow (), &r);
				//Log4( "Desktop rect is: %d, %d, %d, %d", r.left, r.right, r.top, r.bottom );
				if (width)
					*width = r.right - r.left;
				if (height)
					*height = r.bottom - r.top;
			}
		}

}

void  GetDisplaySize (_32 * width, _32 * height)
{
	if( width )
		(*width) = 65535;
	if( height )
      (*height) = 65535;
#if 0
   RECT r;
   GetWindowRect (GetDesktopWindow (), &r);
   //Log4( "Desktop rect is: %d, %d, %d, %d", r.left, r.right, r.top, r.bottom );
   if (width)
      *width = r.right - r.left;
   if (height)
		*height = r.bottom - r.top;
#endif
}

//----------------------------------------------------------------------------

void  GetDisplayPosition (PVIDEO hVid, S_32 * x, S_32 * y,
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
LOGICAL  DisplayIsValid (PVIDEO hVid)
{
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

void  SetDisplaySize (_32 width, _32 height)
{
   SizeDisplay (l.hVideoPool, width, height);
}

//----------------------------------------------------------------------------

ImageFile * GetDisplayImage (PVIDEO hVideo)
{
   return hVideo->pImage;
}

//----------------------------------------------------------------------------

PKEYBOARD  GetDisplayKeyboard (PVIDEO hVideo)
{
   return &hVideo->kbd;
}

//----------------------------------------------------------------------------

LOGICAL  HasFocus (PRENDERER hVideo)
{
   return hVideo->flags.bFocused;
}

//----------------------------------------------------------------------------

#if ACTIVE_MESSAGE_IMPLEMENTED
int  SendActiveMessage (PRENDERER dest, PACTIVEMESSAGE msg)
{
   return 0;
}

PACTIVEMESSAGE  CreateActiveMessage (int ID, int size,...)
{
   return NULL;
}

void  SetDefaultHandler (PRENDERER hVideo,
                                       GeneralCallback general, PTRSZVAL psv)
{
}
#endif
//----------------------------------------------------------------------------

void  OwnMouseEx (PVIDEO hVideo, _32 own DBG_PASS)
{
	if (own)
	{
		lprintf( "Capture is set on %p",hVideo );
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
			lprintf( "No more capture." );
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
	HWND  GetNativeHandle (PVIDEO hVideo)
	{
		return hVideo->hWndOutput;
	}

int  BeginCalibration (_32 nPoints)
{
   return 1;
}

//----------------------------------------------------------------------------
void  SyncRender( PVIDEO hVideo )
{
   // sync has no consequence...
   return;
}

//----------------------------------------------------------------------------

 void  ForceDisplayFocus ( PRENDERER pRender )
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

 void  ForceDisplayFront ( PRENDERER pRender )
{
	if( pRender != l.top )
		PutDisplayAbove( pRender, l.top );

}

//----------------------------------------------------------------------------

 void  ForceDisplayBack ( PRENDERER pRender )
{
	// uhmm...
   lprintf( "Force display backward." );
   SetWindowPos( pRender->hWndOutput, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
void  UpdateDisplay (PVIDEO hVideo )
{
   //DebugBreak();
   UpdateDisplayEx( hVideo DBG_SRC );
}

void  DisableMouseOnIdle (PVIDEO hVideo, LOGICAL bEnable )
{
	if( hVideo->flags.bIdleMouse != bEnable )
	{
		if( bEnable )
		{
			l.redraw_timer_id = SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)1, 33, NULL );
			l.mouse_timer_id = SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)2, 100, NULL );
			hVideo->idle_timer_id = SetTimer( hVideo->hWndOutput, (UINT_PTR)3, 100, NULL );
			l.last_mouse_update = timeGetTime(); // prime the hider.
			hVideo->flags.bIdleMouse = bEnable;
		}
		else // disabling...
		{
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


 void  SetDisplayFade ( PVIDEO hVideo, int level )
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

#undef GetRenderTransform
PTRANSFORM GetRenderTransform       ( PRENDERER r )
{
	return r->transform;
}

LOGICAL RequiresDrawAll ( void )
{
	return TRUE;
}

void MarkDisplayUpdated( PRENDERER r )
{
   l.flags.bUpdateWanted = 1;
	if( r )
      r->flags.bUpdated = 1;
}


#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void (CPROC*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32)) OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) CloseDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32, _32, _32 DBG_PASS)) UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, _32, _32)) SizeDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) GetDisplayImage
                                       , (void (CPROC*)(PRENDERER, CloseCallback, PTRSZVAL)) SetCloseHandler
                                       , (void (CPROC*)(PRENDERER, MouseCallback, PTRSZVAL)) SetMouseHandler
                                       , (void (CPROC*)(PRENDERER, RedrawCallback, PTRSZVAL)) SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, PTRSZVAL)) SetKeyboardHandler
													,  SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(S_32 *, S_32 *)) GetMousePosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SetMousePosition
                                       , HasFocus  // has focus
                                       , GetKeyText
                                       , IsKeyDown
                                       , KeyDown
                                       , DisplayIsValid
                                       , OwnMouseEx
                                       , BeginCalibration
													, SyncRender   // sync
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
									   , NULL
									   , NULL
									   , NULL 
};

RENDER3D_INTERFACE Render3d = {
	GetRenderTransform

};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  GetDisplayInterface (void)
{
	InitDisplay();
   return (POINTER)&VidInterface;
}

void  DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER  GetDisplay3dInterface (void)
{
	InitDisplay();
	return (POINTER)&Render3d;
}

void  DropDisplay3dInterface (POINTER p)
{
}

static LOGICAL CPROC DefaultExit( PTRSZVAL psv, _32 keycode )
{
   lprintf( "Default Exit..." );
	BAG_Exit(0);
   return 1;
}

static LOGICAL CPROC EnableRotation( PTRSZVAL psv, _32 keycode )
{
	lprintf( "Enable Rotation..." );
	if( IsKeyPressed( keycode ) )
	{
		l.flags.bRotateLock = 1 - l.flags.bRotateLock;
		if( l.flags.bRotateLock )
		{
			struct display_camera *defaulorigin_camera = (struct display_camera *)GetLink( &l.cameras, 0 );
			l.mouse_x = defaulorigin_camera->hVidCore->pWindowPos.cx/2;
			l.mouse_y = defaulorigin_camera->hVidCore->pWindowPos.cy/2;
			SetCursorPos( defaulorigin_camera->hVidCore->pWindowPos.cx/2, defaulorigin_camera->hVidCore->pWindowPos.cy / 2 );
		}
		lprintf( "ALLOW ROTATE" );
	}
	else
		lprintf( "DISABLE ROTATE" );
	if( l.flags.bRotateLock )
		lprintf( "lock rotate" );
	else
		lprintf( "unlock rotate" );
   return 1;
}

static LOGICAL CPROC CameraForward( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
#define SPEED_CONSTANT 250
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
				Forward( l.origin, -SPEED_CONSTANT );
			else
				Forward( l.origin, SPEED_CONSTANT );
		}
      else
			Forward( l.origin, 0.0 );
//      return 1;
	}
	//   return 0;
   return 1;
}

static LOGICAL CPROC CameraLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
			{
				Right( l.origin, SPEED_CONSTANT );
			}
			else
			{
				Right( l.origin, -SPEED_CONSTANT );
			}
		}
      else
			Right( l.origin, 0.0 );
//      return 1;
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
      if( IsKeyPressed( keycode ) )
			Right( l.origin, SPEED_CONSTANT );
      else
			Right( l.origin, 0.0 );
//      return 1;
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRollRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			VECTOR tmp;
			scale( tmp, _Z, -1.0 );
			SetRotation( l.origin, tmp );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRollLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			//VECTOR tmp;
         //scale(tmp,_Z,1.0 );
			SetRotation( l.origin, _Z );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
	}
	//   return 0;
   return 1;
}

static LOGICAL CPROC CameraDown( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
 			if( keycode & KEY_SHIFT_DOWN )
				Up( l.origin, SPEED_CONSTANT );
			else
				Up( l.origin, -SPEED_CONSTANT );
		}
		else
			Up( l.origin, 0.0 );
//      return 1;
	}
//   return 0;
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
#endif
	RegisterInterface( 
	   "d3d.render"
	   , GetDisplayInterface, DropDisplayInterface );
	RegisterInterface( 
	   "d3d.render.3d"
	   , GetDisplay3dInterface, DropDisplay3dInterface );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	BindEventToKey( NULL, KEY_SCROLL_LOCK, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_F12, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_A, KEY_MOD_ALL_CHANGES, CameraLeft, 0 );
	BindEventToKey( NULL, KEY_S, KEY_MOD_ALL_CHANGES, CameraDown, 0 );
	BindEventToKey( NULL, KEY_D, KEY_MOD_ALL_CHANGES, CameraRight, 0 );
	BindEventToKey( NULL, KEY_W, KEY_MOD_ALL_CHANGES, CameraForward, 0 );
	BindEventToKey( NULL, KEY_Q, KEY_MOD_ALL_CHANGES, CameraRollLeft, 0 );
	BindEventToKey( NULL, KEY_E, KEY_MOD_ALL_CHANGES, CameraRollRight, 0 );
	//EnableLoggingOutput( TRUE );
}

//typedef struct sprite_method_tag *PSPRITE_METHOD;

PSPRITE_METHOD  EnableSpriteMethod (PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = New(struct sprite_method_tag);
	psm->renderer = render;
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

RENDER_NAMESPACE_END

