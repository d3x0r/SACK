/* http://www.paulsprojects.net/tutorials/simplebump/simplebump.html */


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


#define NEED_REAL_IMAGE_STRUCTURE
#define USE_IMAGE_INTERFACE l.gl_image_interface

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#ifdef __ANDROID__
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
//#include "../glext.h"
#endif

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
#include <vectlib.h>
#undef StrDup

#ifdef _WIN32
#include <shlwapi.h> // have to include this if shellapi.h is included (for mingw)
#include <shellapi.h> // very last though - this is DragAndDrop definitions...
#endif

// this is safe to leave on.
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
#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT
#include <vidlib/vidstruc.h>
#include <render3d.h>
//#include "vidlib.H"

#include <keybrd.h>


#if defined( __64__ ) && defined( _WIN32 )
#define _SetWindowLong(a,b,c)   SetWindowLongPtr(a,b,(LONG_PTR)(c))
#undef GetWindowLong
#define GetWindowLong   GetWindowLongPtr
#else
#define _SetWindowLong(a,b,c)   SetWindowLong(a,b,(long)(c))
#endif

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
#define WM_USER_OPEN_CAMERAS    WM_USER+518

#define WM_RUALIVE 5000 // lparam = pointer to alive variable expected to set true
#define WD_HVIDEO   0   // WindowData_HVIDEO


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


// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

RENDER_NAMESPACE

extern KEYDEFINE KeyDefs[];

// forward declaration - staticness will probably cause compiler errors.
static int CPROC ProcessDisplayMessages(void );

//----------------------------------------------------------------------------

#ifndef __ANDROID__
/* register and track drag file acceptors - per video surface... attched to hvdieo structure */
struct dropped_file_acceptor_tag {
	dropped_file_acceptor f;
	PTRSZVAL psvUser;
};

void WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
	if( renderer )
	{
#ifdef _WIN32
		struct dropped_file_acceptor_tag *newAcceptor = New( struct dropped_file_acceptor_tag );
		newAcceptor->f = f;
		newAcceptor->psvUser = psvUser;
		AddLink( &renderer->dropped_file_acceptors, newAcceptor );
#endif
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
		lprintf( WIDE( " -- UNLINK Video %p from which is below %p and above %p" ), hVideo, hVideo->pAbove, hVideo->pBelow );
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

	if( !l.bottom )
	{
		if( hAbove )
			lprintf( WIDE( "Failure, no bottom, but somehow a second display is already known?" ) );
		l.bottom = hVideo;
		l.top = hVideo;
		return;
	}

#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "Begin Put Display Above..." ) );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();

	// unlink the video from the stack first.
	if( hVideo->pBelow )
		hVideo->pBelow->pAbove = hVideo->pAbove;
	hVideo->pBelow = NULL;
	if( hVideo->pAbove )
		hVideo->pAbove->pBelow = hVideo->pBelow;
	hVideo->pAbove = NULL;

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
			lprintf( WIDE( "Windwo was over somethign else and now we die." ) );
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
		}
		LeaveCriticalSec( &l.csList );
	}
#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "End Put Display Above..." ) );
#endif
}

void  PutDisplayIn (PVIDEO hVideo, PVIDEO hIn)
{
   lprintf( WIDE( "Relate hVideo as a child of hIn..." ) );
}

//----------------------------------------------------------------------------

LOGICAL CreateDrawingSurface (PVIDEO hVideo)
{
	if (!hVideo)
		return FALSE;

	hVideo->pImage =
		RemakeImage( hVideo->pImage, NULL, hVideo->pWindowPos.cx,
						hVideo->pWindowPos.cy );
	if( !hVideo->transform )
	{
		TEXTCHAR name[64];
		snprintf( name, sizeof( name ), WIDE( "render.display.%p" ), hVideo );
		lprintf( WIDE( "making initial transform" ) );
		hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
	}

	lprintf( WIDE( "Set transform at %d,%d" ), hVideo->pWindowPos.x, hVideo->pWindowPos.y );
	Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
	RotateAbs( hVideo->transform, 0, 0, M_PI );
	// additionally indicate that this is a GL render point
	hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
	return TRUE;
}

void DoDestroy (PVIDEO hVideo)
{
   if (hVideo)
   {
#ifdef _WIN32
      hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
      _SetWindowLong (hVideo->hWndOutput, WD_HVIDEO, 0);
#endif
      if (hVideo->pWindowClose)
      {
         hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
#ifdef _WIN32
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
#endif
		Release (hVideo->pTitle);
		DestroyKeyBinder( hVideo->KeyDefs );
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
		if( l.hCapturedMousePhysical == hVideo )
		{
			l.hCapturedPrior = NULL;
			l.hCapturedMousePhysical = NULL;
		}
		if( l.hCapturedMouseLogical = hVideo )
		{
			l.hCapturedMouseLogical = NULL;
		}
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
#ifndef __ANDROID__

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
					lprintf( WIDE("hwndfocus is something...") );
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
							lprintf( WIDE("already dispatched, delay it.") );
						EnqueLink( &hVideo->pInput, (POINTER)key );
					}
					else
					{
						hVideo->flags.key_dispatched = 1;
						do
						{
							if( l.flags.bLogKeyEvent )
								lprintf( WIDE("Dispatching key %08lx"), key );
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
									if( hVideo && hVideo->KeyDefs && !HandleKeyEvents( hVideo->KeyDefs, key ) )
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
									if( HandleKeyEvents( KeyDefs, key )  )
									{
										dispatch_handled = 1;
									}
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
				HandleKeyEvents( KeyDefs, key ); /* global events, if no keyproc */
			}
		   //else9
			//	lprintf( WIDE( "Failed to find active window..." ) );
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
		//if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hwndfocus is something...") );
#endif
			//hVid = l.hVidFocused;
		}
		//else
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hVid from focus") );
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

		if( l.flags.bLogKeyEvent )
			lprintf( WIDE("hvid is %p"), hVid );
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
#endif

//----------------------------------------------------------------------------

HWND MoveWindowStack( PVIDEO hInChain, HWND hwndInsertAfter, int use_under )
{
#ifdef _WIN32
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
				lprintf( WIDE( "Found we want to be under something... so use that window instead." ) );
#endif
			hwndInsertAfter = check->under->hWndOutput;
		}

		save_current = current;
	while( current )
	{
#ifdef LOG_ORDERING_REFOCUS
		if( l.flags.bLogFocus )
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
#else
	return NULL;
#endif
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
			//if( !SetActiveGLDisplay( hVideo ) )
			{
				// if the opengl failed, dont' let the application draw.
				//return;
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
			//SetActiveGLDisplay( NULL );
			//if( hVideo->flags.bLayeredWindow )
			{
			//	UpdateDisplay( hVideo );
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
#ifdef _WIN32
		InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#endif
	}
}

static void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
    GLfloat m[4][4];
    GLfloat sine, cotangent, deltaZ;
    GLfloat radians=(GLfloat)(fovy/2.0f*M_PI/180.0f);

    /*m[0][0] = 1.0f;*/ m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; /*m[1][1] = 1.0f;*/ m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; /*m[2][2] = 1.0f; m[2][3] = 0.0f;*/
	 m[3][0] = 0.0f; m[3][1] = 0.0f; /*m[3][2] = 0.0f; m[3][3] = 1.0f;*/

    deltaZ=zFar-zNear;
    sine=(GLfloat)sin(radians);
    if ((deltaZ==0.0f) || (sine==0.0f) || (aspect==0.0f))
    {
        return;
    }
    cotangent=(GLfloat)(cos(radians)/sine);

    m[0][0] = cotangent / aspect;
	 m[1][1] = cotangent;
#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER )
    m[2][2] = (zFar + zNear) / deltaZ;
    m[2][3] = 1.0f;
    m[3][2] = -1.0f * zNear * zFar / deltaZ;
#else
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1.0f;
	 m[3][2] = -2.0f * zNear * zFar / deltaZ;
#endif
	 m[3][3] = 0;
#undef m
    glMultMatrixf(&m[0][0]);
}



static void BeginVisPersp( struct display_camera *camera )
{
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	MygluPerspective(90.0f,camera->aspect,1.0f,30000.0f);
	//glGetFloatv( GL_PROJECTION_MATRIX, (GLfloat*)l.fProjection );
	//PrintMatrix( l.fProjection );
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
}


static int InitGL( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !camera->flags.init )
	{
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

		glEnable( GL_ALPHA_TEST );
		glEnable( GL_BLEND );
 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glEnable( GL_TEXTURE_2D );
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		glEnable(GL_NORMALIZE); // glNormal is normalized automatically....
#ifndef __ANDROID__
		//glEnable( GL_POLYGON_SMOOTH );
		//glEnable( GL_POLYGON_SMOOTH_HINT );
		glEnable( GL_LINE_SMOOTH );
		//glEnable( GL_LINE_SMOOTH_HINT );
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
#endif
 		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
 
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 
		//BeginVisPersp( camera );
		lprintf( WIDE("First GL Init Done.") );
		camera->flags.init = 1;
		camera->hVidCore->flags.bReady = TRUE;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}


static void InvokeExtraInit( struct display_camera *camera, PTRANSFORM view_camera )
{
	PTRSZVAL (CPROC *Init3d)(PMatrix,PTRANSFORM,RCOORD*,RCOORD*);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
   lprintf( WIDE("Invoke Init3d") );
	for( name = GetFirstRegisteredName( WIDE("sack/render/puregl/init3d"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		Init3d = GetRegisteredProcedureExx( data,(CTEXTSTR)name,PTRSZVAL,WIDE("ExtraInit3d"),(PMatrix,PTRANSFORM,RCOORD*,RCOORD*));

		if( Init3d )
		{
			struct plugin_reference *reference;
			PTRSZVAL psvInit = Init3d( &l.fProjection, view_camera, &camera->identity_depth, &camera->aspect );
			if( psvInit )
			{
				reference = New( struct plugin_reference );
				reference->psv = psvInit;
				reference->name = name;
				{
					static PCLASSROOT draw3d;
					if( !draw3d )
						draw3d = GetClassRoot( WIDE("sack/render/puregl/draw3d") );
					reference->Update3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("Update3d"),(PTRANSFORM));
					// add one copy of each update proc to update list.
					if( FindLink( &l.update, reference->Update3d ) == INVALID_INDEX )
						AddLink( &l.update, reference->Update3d );
					reference->FirstDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("FirstDraw3d"),(PTRSZVAL));
					reference->ExtraDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraBeginDraw3d"),(PTRSZVAL,PTRANSFORM));
					reference->Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraDraw3d"),(PTRSZVAL));
					reference->Mouse3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("ExtraMouse3d"),(PTRSZVAL,PRAY,S_32,S_32,_32));
					reference->Key3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("ExtraKey3d"),(PTRSZVAL,_32));
				}

				AddLink( &camera->plugins, reference );
			}
		}
	}
}


#ifndef __ANDROID__
#ifndef __WATCOMC__
static int CPROC Handle3DTouches( PRENDERER hVideo, PTOUCHINPUT touches, int nTouches )
{
	static struct touch_event_state
	{
		struct {
			struct {
				BIT_FIELD bDrag : 1;
			} flags;
			int x;
         int y;
		} one;
		struct {
			int x;
         int y;
		} two;
		struct {
			int x;
         int y;
		} three;
	} touch_info;
	if( l.flags.bRotateLock )
	{
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( WIDE( "%d %5d %5d " ), t, touches[t].x, touches[t].y );
		}
		lprintf( WIDE( "touch event" ) );
		if( nTouches == 3 )
		{
			if( touches[2].dwFlags & TOUCHEVENTF_DOWN )
			{
            touch_info.three.x = touches[2].x;
            touch_info.three.y = touches[2].y;
			}
			else if( touches[2].dwFlags & TOUCHEVENTF_UP )
			{

			}
			else
			{
            // move/drag state
			}
		}
		else if( nTouches == 2 )
		{
         // begin rotate lock
			if( touches[1].dwFlags & TOUCHEVENTF_DOWN )
			{
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
			else if( touches[1].dwFlags & TOUCHEVENTF_UP )
			{
			}
			else
			{
				// drag
				int delx, dely;
				int delx2, dely2;
				int delxt, delyt;
				int delx2t, dely2t;
				lprintf( WIDE("drag") );
				delxt = touches[1].x - touches[0].x;
				delyt = touches[1].y - touches[0].y;
				delx2t = touch_info.two.x - touch_info.one.x;
				dely2t = touch_info.two.y - touch_info.one.y;
				delx = -touch_info.one.x + touches[0].x;
				dely = -touch_info.one.y + touches[0].y;
				delx2 = -touch_info.two.x + touches[1].x;
				dely2 = -touch_info.two.y + touches[1].y;
				{
					VECTOR v1,v2/*,vr*/;
					RCOORD delta_x = delx / 40.0f;
					RCOORD delta_y = dely / 40.0f;
					static int toggle;
					v1[vUp] = delyt;
					v1[vRight] = delxt;
					v1[vForward] = 0;
					v2[vUp] = dely2t;
					v2[vRight] = delx2t;
					v2[vForward] = 0;
					normalize( v1 );
					normalize( v2 );
					lprintf( WIDE("angle %g"), atan2( v2[vUp], v2[vRight] ) - atan2( v1[vUp], v1[vRight] ) );
					RotateRel( l.origin, 0, 0, - atan2( v2[vUp], v2[vRight] ) + atan2( v1[vUp], v1[vRight] ) );
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
				}

            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
		}
		else if( nTouches == 1 )
		{
			if( touches[0].dwFlags & TOUCHEVENTF_DOWN )
			{
            lprintf( WIDE("begin") );
				// begin touch
            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
			}
			else if( touches[0].dwFlags & TOUCHEVENTF_UP )
			{
				// release
            lprintf( WIDE("done") );
			}
			else
			{
				// drag
				int delx, dely;
				lprintf( WIDE("drag") );
				delx = -touch_info.one.x + touches[0].x;
				dely = -touch_info.one.y + touches[0].y;
				{
					RCOORD delta_x = -delx / 40.0;
					RCOORD delta_y = -dely / 40.0;
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
				}


				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
			}
		}
		return 1;
	}
	return 0;
}
#endif
#endif

static void WantRenderGL( void )
{
	struct plugin_reference *reference;
	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Begin Render") );

	{
		PRENDERER other = NULL;
		PRENDERER hVideo;
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( other == hVideo )
				DebugBreak();
			other = hVideo;
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("Have a video in stack...") );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( WIDE("But it's nto exposed...") );
				continue;
			}
			if( hVideo->flags.bUpdated )
			{
				// any one window with an update draws all.
				l.flags.bUpdateWanted = 1;
				break;
			}
		}
	}
}

static void RenderGL( struct display_camera *camera )
{
	INDEX idx;
	PRENDERER hVideo;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Begin Render") );

	if( !camera->flags.did_first_draw )
	{
		first_draw = 1;
		camera->flags.did_first_draw = 1;
	}
	else
		first_draw = 0;

	// do OpenGL Frame
	SetActiveGLDisplay( camera->hVidCore );

	InitGL( camera );

	LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
	{
		// setup initial state, like every time so it's a known state?
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

	BeginVisPersp( camera );
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
	glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );

	//BeginVisPersp( camera );
	//glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );

	for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
	{
		if( l.flags.bLogMessageDispatch )
			lprintf( WIDE("Have a video in stack...") );
		if( hVideo->flags.bDestroy )
			continue;
		if( hVideo->flags.bHidden || !hVideo->flags.bShown )
		{
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("But it's nto exposed...") );
			continue;
		}

		hVideo->flags.bUpdated = 0;

		if( l.flags.bLogWrites )
			lprintf( WIDE("------ BEGIN A REAL DRAW -----------") );

		glEnable( GL_DEPTH_TEST );
		// put out a black rectangle
		// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
		ClearImageTo( hVideo->pImage, 0 );
		glDisable(GL_DEPTH_TEST);							// Enables Depth Testing
		if( hVideo->pRedrawCallback )
			hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );

		{
			INDEX idx;
			PSPRITE_METHOD psm;
			LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
			{
				psm->RenderSprites( psm->psv, hVideo, 0, 0, 0, 0 );
			}
		}
		// allow draw3d code to assume depth testing
		glEnable( GL_DEPTH_TEST );
		glColor3d( 1.0, 1.0, 1.0 );
	}

	LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
	{
		if( reference->Draw3d )
			reference->Draw3d( reference->psv );
	}


	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Done external drawing") );
	SetActiveGLDisplay( NULL );
	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Done output.") );
}



#ifdef _WIN32
static HCURSOR hCursor;

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
		Return MA_ACTIVATE;
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
				DragQueryFile( hDrop, iFile, buffer, (UINT)sizeof( buffer ) );
				//lprintf( WIDE( "Accepting file drop [%s]" ), buffer );
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
				PVIDEO hVidPrior = l.hVidPhysicalFocused;
				hVideo = camera->hVidCore;
				if( hVidPrior )
				{
#ifdef LOG_ORDERING_REFOCUS
					if( l.flags.bLogFocus )
					{
						lprintf( WIDE("-------------------------------- GOT FOCUS --------------------------") );
						lprintf( WIDE("Instance is %p"), hWnd );
#ifdef _WIN32
						lprintf( WIDE("prior is %p"), hVidPrior->hWndOutput );
#endif
					}
#endif
					if( hVidPrior->pBelow )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior was below...") );
#endif
#ifdef _WIN32
						SetFocus( hVidPrior->pBelow->hWndOutput );
#endif
					}
					else if( hVidPrior->pNext )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior last interrupted..") );
#endif
#ifdef _WIN32
						SetFocus( hVidPrior->pNext->hWndOutput );
#endif
					}
					else if( hVidPrior->pPrior )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to window prior which interrupted us...") );
#endif
#ifdef _WIN32
						SetFocus( hVidPrior->pPrior->hWndOutput );
#endif
					}
					else if( hVidPrior->pAbove )
					{
#ifdef LOG_ORDERING_REFOCUS
						if( l.flags.bLogFocus )
							lprintf( WIDE("Set to a window which prior was above.") );
#endif
#ifdef _WIN32
						SetFocus( hVidPrior->pAbove->hWndOutput );
#endif
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
				Log (WIDE("Got setfocus..."));
#endif
			//SetWindowPos( l.hWndInstance, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			//SetWindowPos( hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			//hVideo = l.top;
			l.hVidPhysicalFocused = camera->hVidCore;
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
			if( l.flags.bLogFocus )
				lprintf(WIDE("Got Killfocus new focus to %p %p"), hWnd, wParam);
#endif
         l.hVidPhysicalFocused = NULL;
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
		lprintf( WIDE("activate app on this window? %d"), wParam );
#endif
	   break;
#endif
   case WM_ACTIVATE:
		if( hWnd == ((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance ) {
#ifdef LOG_ORDERING_REFOCUS
			Log2 (WIDE("Activate: %08x %08x"), wParam, lParam);
#endif
			if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
			{
				hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
				if (hVideo)
				{
#ifdef LOG_ORDERING_REFOCUS
					Log2 (WIDE("Window %08x is below %08x"), hVideo, hVideo->pBelow);
#endif
					if (hVideo->pBelow && hVideo->pBelow->flags.bShown)
					{
#ifdef LOG_ORDERING_REFOCUS
						Log (WIDE("Setting active window the the lower(upper?) one..."));
#endif
#ifdef _WIN32
						SetActiveWindow (hVideo->pBelow->hWndOutput);
#endif
					}
					else
					{
#ifdef LOG_ORDERING_REFOCUS
						Log (WIDE("Within same level do focus..."));
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
				lprintf( WIDE("Too early, hVideo is not setup yet to reference.") );
#endif
				Return 1;
			}
			pwp = (LPWINDOWPOS) lParam;

			if( hVideo->flags.bDeferedPos )
			{
				if( !(pwp->flags & SWP_NOZORDER ) )	
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("PosChanging ? %p %p"), hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
					pwp->hwndInsertAfter = hVideo->hDeferedAfter;
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("PosChanging ? %p %p"), hVideo->hWndOutput, pwp->hwndInsertAfter );
					lprintf( WIDE("Someone outside knew something about the ordering and this is a mass-reorder, must be correct? %p %p"), hVideo->hWndOutput, pwp->hwndInsertAfter );
#endif
				}
				Return 0;
			}

			if( !(pwp->flags & SWP_NOZORDER ) )	
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Include set Z-Order") );
#endif
            // being moved in zorder...
				if( !hVideo->pBelow && !hVideo->pAbove && !hVideo->under )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("... not above below or under, who cares. return. %p"), pwp->hwndInsertAfter );							
#endif
					if( hVideo->flags.bAbsoluteTopmost )
					{
						if( pwp->hwndInsertAfter != HWND_TOPMOST )
						{
#ifdef LOG_ORDERING_REFOCUS
							lprintf( WIDE("Just make sure we're the TOP TOP TOP window.... (more than1?!)") );
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
					lprintf( WIDE("Ignoreing the change generated by draw.... %p "), hVideo->pWindowPos.hwndInsertAfter );
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
						lprintf( WIDE("Moving myself below what I'm expected to be below.") );
						lprintf( WIDE("This action stops all reordering with other windows.") );
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
								lprintf( WIDE("Fixup for being the top window, but a parent is under something.") );
#endif
								pwp->hwndInsertAfter = check->under->hWndOutput;
							}
#ifdef LOG_ORDERING_REFOCUS
							else
								lprintf( WIDE("uhmm... well it's going away..") );
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
				lprintf( WIDE( "..." ) );
			}
			lprintf( WIDE( "Being inserted after %x %x" ), pwp->hwndInsertAfter, hWnd );
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
			if( pwp->cx != hVideo->pWindowPos.cx ||
				pwp->cy != hVideo->pWindowPos.cy)
			{
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
			hVideo->flags.bReady = 1;
		}
		Return 0;         // indicate handled message... no WM_MOVE/WM_SIZE generated.
#if !(defined __WATCOMC__ || defined __WATCOMCPP__ || defined __GNUC__ )
	case WM_TOUCH:
		{
			if( l.GetTouchInputInfo )
			{
				TOUCHINPUT inputs[100];
				struct input_point outputs[20];

				int count = LOWORD(wParam);
				PVIDEO hVideo = (PVIDEO)GetWindowLong( hWnd, WD_HVIDEO );
				if( count > 100 )
					count = 100;
				l.GetTouchInputInfo( (HTOUCHINPUT)lParam, count, inputs, sizeof( TOUCHINPUT ) );
				//lprintf( "touch event with %d", count );
				l.CloseTouchInputHandle( (HTOUCHINPUT)lParam );
				{
					int n;
					for( n = 0; n < count; n++ )
					{
						// windows coordiantes some in in hundreths of pixesl as a long
						lprintf( WIDE("input point %d,%d %08x  %s %s %s %s %s %s %s")
								 , inputs[n].x, inputs[n].y
								 , inputs[n].dwFlags
								 , ( inputs[n].dwFlags & TOUCHEVENTF_MOVE)?WIDE("MOVE"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_DOWN)?WIDE("DOWN"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_UP)?WIDE("UP"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_INRANGE)?WIDE("InRange"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_PRIMARY)?WIDE("Primary"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_NOCOALESCE)?WIDE("NoCoales"):WIDE("")
								 , ( inputs[n].dwFlags & TOUCHEVENTF_PALM)?WIDE("PALM"):WIDE("")

								 );
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
					int handled = 0;

					if( hVideo->pTouchCallback )
					{
						handled = hVideo->pTouchCallback( hVideo->dwTouchData, outputs, count );
					}

					if( !handled )
					{
						// this will be like a hvid core
						handled = Handle3DTouches( hVideo, inputs, count );
					}
					if( handled )
						Return 0;
					Return 1;
				}
			}
		}
      break;
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
		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
		if (!hVideo)
		{
			Return 0;
		}
		if( l.hCapturedMousePhysical )
		{
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Captured mouse already - don't do anything?") );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
				lprintf( WIDE("Auto owning mouse to surface which had the mouse clicked DOWN.") );
#endif
				if( !l.hCapturedMousePhysical )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( WIDE("Auto release mouse from surface which had the mouse unclicked.") );
				if( !l.hCapturedMousePhysical )
					ReleaseCapture();
			}
		}

		{
			POINT p;
			int dx, dy;
			GetCursorPos (&p);
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position %d,%d"), p.x, p.y );
#endif
			p.x -= (dx =(l.hCapturedMousePhysical?l.hCapturedMousePhysical:hVideo)->cursor_bias.x);
			p.y -= (dy=(l.hCapturedMousePhysical?l.hCapturedMousePhysical:hVideo)->cursor_bias.y);
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position results %d,%d %d,%d"), dx, dy, p.x, p.y );
#endif

//			if (!(l.hCaptured?l.hCaptured:hVideo)->flags.bFull)
			{
//				p.x -= l.WindowBorder_X;
//				p.y -= l.WindowBorder_Y;
			}
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position results %d,%d %d,%d"), dx, dy, p.x, p.y );
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
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
			msg[0] = (_32)(l.hCaptured?l.hCaptured:hVideo);
			msg[1] = l.mouse_x;
			msg[2] = l.mouse_y;
			msg[3] = l.mouse_b;
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Generate mouse message %p(%p?) %d %d,%08x %d+%d=%d"), l.hCaptured, hVideo, l.mouse_x, l.mouse_y, l.mouse_b, l.dwMsgBase, MSG_MouseMethod, l.dwMsgBase + MSG_MouseMethod );
#endif
			SendServiceEvent( 0, l.dwMsgBase + MSG_MouseMethod
								 , msg
								 , sizeof( msg ) );
#endif
			if( l.flags.bRotateLock  )
			{
				RCOORD delta_x = l.mouse_x - (hVideo->pWindowPos.cx/2);
				RCOORD delta_y = l.mouse_y - (hVideo->pWindowPos.cy/2);
				//lprintf( WIDE("mouse came in we're at %d,%d %g,%g"), l.mouse_x, l.mouse_y, delta_x, delta_y );
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
					//lprintf( WIDE("Set curorpos..") );
					SetCursorPos( hVideo->pWindowPos.x + hVideo->pWindowPos.cx/2, hVideo->pWindowPos.y + hVideo->pWindowPos.cy / 2 );
					//lprintf( WIDE("Set curorpos Done..") );
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
			//lprintf( WIDE("Show Window Message! %p"), hVideo );
			if( !hVideo->flags.bShown )
			{
				//lprintf( "Window is hiding? that is why we got it?" );
				Return 0;
			}
		}
      Return 0;
      //lprintf( "Fall through to WM_PAINT..." );
	case WM_PAINT:
		ValidateRect (hWnd, NULL);
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
		lprintf( WIDE( "ENDING SESSION!" ) );
		BAG_Exit( (int)lParam );
		Return TRUE; // uhmm okay.
	case WM_DESTROY:
#ifdef LOG_DESTRUCTION
		Log( WIDE("Destroying a window...") );
#endif
		hVideo = (PVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
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
		if( l.bExitThread )
			return 0;
		if( l.redraw_timer_id  == wParam )
		{
			if( l.flags.bLogRenderTiming )
				lprintf( WIDE("Begin Draw Tick") );
			Move( l.origin );

			{
				INDEX idx;
				int hadone = 0;
				Update3dProc proc;
				LIST_FORALL( l.update, idx, Update3dProc, proc )
				{
					hadone = 1;
					if( proc( l.origin ) )
						l.flags.bUpdateWanted = TRUE;
				}
				// for simple plugins that don't know any better
				if( l.update && !hadone )
					l.flags.bUpdateWanted = TRUE;

			}

         // no reason to check this if an update is already wanted.
			if( !l.flags.bUpdateWanted )
			{
				// set l.flags.bUpdateWanted for window surfaces.
				WantRenderGL();
			}

			if( l.flags.bUpdateWanted )
			{
				struct display_camera *camera;
				INDEX idx;
				l.flags.bUpdateWanted = 0;
				LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
				{
					// if plugins or want update, don't continue.
					if( !camera->plugins && !l.flags.bUpdateWanted )
						continue;
					
					//if( !camera->hVidCore || !camera->hVidCore->flags.bReady )
					//	continue;

					// drawing may cause subsequent draws; so clear this first
					RenderGL( camera );
				}
			}
		}
		if( l.mouse_timer_id == wParam )
		{
			if( l.flags.mouse_on && l.last_mouse_update )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE("Pending mouse away... %d"), timeGetTime() - ( l.last_mouse_update ) );
#endif
				if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
				{
					int x;
#ifdef LOG_MOUSE_HIDE_IDLE
					lprintf( WIDE("OFF!") );
#endif
					l.flags.mouse_on = 0;
					//l.last_mouse_update = 0;
					while( x = ShowCursor( FALSE ) )
					{
					}
#ifdef LOG_MOUSE_HIDE_IDLE
					lprintf( WIDE("Show count %d %d"), x, GetLastError() );
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
#if !defined( __WATCOMC__ ) && !defined( __GNUC__ )
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
				struct display_camera *camera = (struct display_camera *)pcs->lpCreateParams; // user passed param...

				if( camera )
				{
					hVideo = camera->hVidCore;
				}

				if ( !camera )
				{
					// directly created without parameter (Dialog control
					// probably )... grab a static PVIDEO for this...
					hVideo = &l.hDialogVid[(l.nControlVid++) & 0xf];
				}
				_SetWindowLong (hWnd, WD_HVIDEO, hVideo);

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
#endif
//----------------------------------------------------------------------------

static void LoadOptions( void )
{
	_32 average_width, average_height;
	//int some_width;
	//int some_height;
#ifndef __NO_OPTIONS__
	PODBC option = GetOptionODBC( NULL, 0 );
	l.flags.bLogRenderTiming = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Log Render Timing"), 0, TRUE );
	l.flags.bView360 = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/360 view"), 0, TRUE );

	l.scale = (RCOORD)SACK_GetOptionInt( option, GetProgramName(), WIDE("SACK/Image Library/Scale"), 10 );
	if( l.scale == 0.0 )
	{
		l.scale = (RCOORD)SACK_GetOptionInt( option, GetProgramName(), WIDE("SACK/Image Library/Inverse Scale"), 2 );
		if( l.scale == 0.0 )
			l.scale = 1;
	}
	else
		l.scale = 1.0 / l.scale;
	if( !l.cameras )
	{
		struct display_camera *default_camera = NULL;
		_32 screen_w, screen_h;
		int nDisplays = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Number of Displays"), l.flags.bView360?6:1, TRUE );
		int n;
		l.flags.bForceUnaryAspect = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Force Aspect 1.0"), (nDisplays==1)?0:1, TRUE );
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
		SetLink( &l.cameras, 0, (POINTER)1 ); // set default here 
		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Display is topmost"), n+1 );
         camera->flags.topmost = SACK_GetOptionIntEx( option, GetProgramName(), tmp, 0, TRUE );
			snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Custom Position"), n+1 );
			custom_pos = SACK_GetOptionIntEx( option, GetProgramName(), tmp, l.flags.bView360?1:0, TRUE );
			if( custom_pos )
			{
				camera->display = -1;
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/x"), n+1 );
				camera->x = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?400:n==2?0:n==3?400:0
					:nDisplays==6
						?n==0?((screen_w * 0)/4):n==1?((screen_w * 1)/4):n==2?((screen_w * 1)/4):n==3?((screen_w * 1)/4):n==4?((screen_w * 2)/4):n==5?((screen_w * 3)/4):0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/y"), n+1 );
				camera->y = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?0:n==2?300:n==3?300:0
					:nDisplays==6
						?n==0?((screen_h * 1)/3):n==1?((screen_h * 0)/3):n==2?((screen_h * 1)/3):n==3?((screen_h * 2)/3):n==4?((screen_h * 1)/3):n==5?((screen_h * 1)/3):0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/width"), n+1 );
				camera->w = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?400
					:nDisplays==6
						?( screen_w / 4 )
					:nDisplays==1
						?800
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/height"), n+1 );
				camera->h = SACK_GetOptionIntEx( option, GetProgramName(), tmp, (
					nDisplays==4
						?300
					:nDisplays==6
						?( screen_h / 3 )
					:nDisplays==1
						?600
					:0), TRUE );
				camera->identity_depth = camera->w/2;
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
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Display"), n+1 );
				camera->display = SACK_GetOptionIntEx( option, GetProgramName(), tmp, nDisplays>1?n+1:0, TRUE );
				GetDisplaySizeEx( camera->display, &camera->x, &camera->y, &camera->w, &camera->h );
			}

			if( l.flags.bForceUnaryAspect )
				camera->aspect = 1.0;
			else
			{
				camera->aspect = ((float)camera->w/(float)camera->h);
			}

			camera->origin_camera = CreateTransform();
			switch( nDisplays )
			{
			default:
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Camera Type"), n+1 );
				camera->type = SACK_GetOptionIntEx( option, GetProgramName(), tmp, 2, TRUE );
				if( !default_camera )
					default_camera = camera;
				break;
			case 6: // default left, top, middle, bottom, right, back
				camera->type = n;
				if( n == 2 )
					default_camera = camera;
				break;
			}
			if( camera != default_camera )
				AddLink( &l.cameras, camera );
		}
		SetLink( &l.cameras, 0, default_camera );
	}
	l.flags.bLogMessageDispatch = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log message dispatch"), 0, TRUE );
	l.flags.bLogFocus = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log focus event"), 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log key event"), 0, TRUE );
	l.flags.bLogMouseEvent = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log mouse event"), 0, TRUE );
	l.flags.bOptimizeHide = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Optimize Hide with SWP_NOCOPYBITS"), 0, TRUE );
	l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Log Video Output"), 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Use Low Level Keyhook"), 0, TRUE );

#else
#  ifndef UNDER_CE
#  endif
#endif
	if( !l.origin )
	{
		static MATRIX m;
		// create initial origin camera so a window at x,y = 0,0  will show the same 
		// as a normal display 0,0.
		l.origin = CreateNamedTransform( WIDE("render.camera") );
		Translate( l.origin, l.scale * average_width/(2), l.scale * average_height/(2), l.scale * average_height/(2) );
		RotateAbs( l.origin, M_PI, 0, 0 );
		CreateTransformMotion( l.origin ); // some things like rotate rel
	}

	{
		INDEX idx;
		struct display_camera * camera;
		LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
		{	
			if( !camera->flags.extra_init )
			{
				camera->flags.extra_init = 1;
				camera->identity_depth = camera->w/2;
				InvokeExtraInit( camera, camera->origin_camera );
			}
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

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

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

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

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

void UpdateMouseRay( struct display_camera * camera )
{
#define BEGIN_SCALE 1
#define COMMON_SCALE ( 2*camera->aspect)
#define END_SCALE (1000*l.scale)
#define tmp_param1 (END_SCALE*COMMON_SCALE)
	if( camera->origin_camera )
	{
		VECTOR tmp1;
		PTRANSFORM t = camera->origin_camera;

		addscaled( l.mouse_ray_origin, _0, _Z, BEGIN_SCALE );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _X, ((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _Y, -((float)l.mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );

		addscaled( l.mouse_ray_target, _0, _Z, END_SCALE );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _X, tmp_param1*((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _Y, -(tmp_param1/camera->aspect)*((float)l.mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );

		// this is definaly the correct rotation
		Apply( t, tmp1, l.mouse_ray_origin );
		SetPoint( l.mouse_ray_origin, tmp1 );
		Apply( t, tmp1, l.mouse_ray_target );
		SetPoint( l.mouse_ray_target, tmp1 );

		sub( l.mouse_ray_slope, l.mouse_ray_target, l.mouse_ray_origin );
		normalize( l.mouse_ray_slope );
		
		SetPoint( camera->mouse_ray.n, l.mouse_ray_slope );
		SetPoint( camera->mouse_ray.o, l.mouse_ray_origin );
		SetRay( &l.mouse_ray, &camera->mouse_ray );
	}
}

void UpdateMouseRays( void )
{
	struct display_camera *camera;
	INDEX idx;
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		UpdateMouseRay( camera );
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
		if( hVideo )
			Apply( hVideo->transform, v1, v2 );
		else
			SetPoint( v1, v2 );
		ApplyInverse( camera->origin_camera, v2, v1 );
		// so this puts the point back in world space

		// compute intersection of the point in the view, with a plane 1.0 units into the monitor
		// and then 
		{
			RCOORD t;
			RCOORD cosphi;
			VECTOR v4;
			SetPoint( v4, _0 );
			v4[2] = 1.0;

			cosphi = IntersectLineWithPlane( v2, _0, _Z, v4, &t );
			if( cosphi != 0 )
				addscaled( v1, _0, v2, t );
		}


		if( result_x )
			(*result_x) = (int)((camera->viewport[2]/2) + (v1[0]/COMMON_SCALE * camera->viewport[2]));
		if( result_y )
			(*result_y) = (camera->viewport[3]/2) - (v1[1]/(COMMON_SCALE/camera->aspect) * camera->viewport[3]);
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
		UpdateMouseRay( camera );


		if( !used )
		for( check = l.top; check ;check = check->pAbove)
		{
			VECTOR target_point;
			if( l.hCapturedMouseLogical )
				if( check != l.hCapturedMouseLogical )
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
					addscaled( target_point, l.mouse_ray_origin, l.mouse_ray_slope, t );
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

				if( check == l.hCapturedMouseLogical ||
					( ( newx >= 0 && newx < (check->pWindowPos.cx ) )
					 && ( newy >= 0 && newy < (check->pWindowPos.cy ) ) ) )
				{
					if( check && check->pMouseCallback)
					{
						if( l.flags.bLogMouseEvent )
							lprintf( WIDE("Sent Mouse Proper. %d,%d %08x"), newx, newy, l.mouse_b );
						l.current_mouse_event_camera = camera;
						used = check->pMouseCallback( check->dwMouseData
													, newx
													, newy
													, l.mouse_b );
						l.current_mouse_event_camera = NULL;
						if( used )
							break;
					}
				}
			}
		}
		if( !used )
		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				if( ref->Mouse3d )
				{
					//lprintf( "send mouse : %d %d", x, y );
					used = ref->Mouse3d( ref->psv, &camera->mouse_ray, x, y, b );
					if( used )
						break;
				}
			}
		}
	}
	return used;
}

int CPROC OpenGLKey( PTRSZVAL psv, _32 keycode )
{
	struct display_camera *camera = (struct display_camera *)psv;
	int used = 0;
	PRENDERER check = NULL;
	INDEX idx;
	struct plugin_reference *ref;
	if( l.hPluginKeyCapture )
	{
		used = l.hPluginKeyCapture->Key3d( l.hPluginKeyCapture->psv, keycode );
		if( !used )
			l.hPluginKeyCapture = NULL;
		else
			return 1;
	}

	LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
	{
		if( ref->Key3d )
		{
			used = ref->Key3d( ref->psv, keycode );
			if( used )
			{
				// if a thing uses a key, lock to that plugin for future keys until it doesn't want a key
				// (thing like a chat module, first key would lock to it, and it could claim all events;
				// maybe should implement an interface to manually clear this
				l.hPluginKeyCapture = ref;
				break;
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

// returns the forward view camera (or default camera)
static struct display_camera *OpenCameras( void )
{
	struct display_camera *default_camera = NULL;
	static HMODULE hMe;
	struct display_camera *camera;
	INDEX idx;

 	hMe = GetModuleHandle (_WIDE(TARGETNAME));
	//lprintf( WIDE( "-----Create WIndow Stuff----- %s %s" ), hVideo->flags.bLayeredWindow?WIDE( "layered" ):WIDE( "solid" )
		//		 , hVideo->flags.bChildWindow?WIDE( "Child(tool)" ):WIDE( "user-selectable" ) );
	LoadOptions();

	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		lprintf( WIDE("camera instance %p"), camera->hWndInstance );
		if( !camera->hWndInstance )
		{
			TEXTCHAR window_name[128];
			int UseCoords = camera->display == -1;
			int DefaultScreen = camera->display;
			S_32 x, y;
			_32 w, h;

			// if we don't do it this way, size_t overflows in camera definition.
			if( !UseCoords && !l.flags.bView360 )
			{
				GetDisplaySizeEx( DefaultScreen, &camera->x, &camera->y, &w, &h );
 
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


	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.. %d,%d %dx%d" ),x,y,w,h );
	#endif
			camera->hVidCore = New( VIDEO );
			AddLink( &l.pActiveList, camera->hVidCore );
			MemSet (camera->hVidCore, 0, sizeof (VIDEO));
			InitializeCriticalSec( &camera->hVidCore->cs );
			camera->hVidCore->camera = camera;
			camera->hVidCore->pKeyProc = OpenGLKey;
			camera->hVidCore->dwKeyData = (PTRSZVAL)camera;
			camera->hVidCore->pMouseCallback = OpenGLMouse;
			camera->hVidCore->dwMouseData = (PTRSZVAL)camera;
			camera->identity_depth = camera->w/2;
			camera->viewport[0] = x;
			camera->viewport[1] = y;
			camera->viewport[2] = (int)w;
			camera->viewport[3] = (int)h;
         if( !idx )
				snprintf( window_name, 128, WIDE("%s:3D View"), GetProgramName() );
         else
				snprintf( window_name, 128, WIDE("%s:3D View(%d)"), GetProgramName(), idx );

			camera->hWndInstance = CreateWindowEx (0
	#ifndef NO_DRAG_DROP
														| WS_EX_ACCEPTFILES
	#endif
	#ifdef UNICODE
													  , (LPWSTR)l.aClass
	#else
													  , (LPSTR)l.aClass
	#endif
													  , (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : window_name
													  , WS_POPUP //| WINDOW_STYLE
													  , x, y, (int)w, (int)h
													  , NULL // HWND_MESSAGE  // (GetDesktopWindow()), // Parent
													  , NULL // Menu
													  , hMe
													  , (void*)camera );
	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.." ) );
	#endif
			camera->hVidCore->hWndOutput = (HWND)camera->hWndInstance;
			EnableOpenGL( camera->hVidCore );
			ShowWindow( camera->hWndInstance, SW_SHOWNORMAL );
			camera->hVidCore->flags.bTopmost = camera->flags.topmost;
			if( camera->flags.topmost )
			{
				SetWindowPos( camera->hWndInstance
								, HWND_TOPMOST
								, 0, 0, 0, 0,
								 SWP_NOMOVE
								 | SWP_NOSIZE
								);
			}
			/*
			while( !camera->hVidCore->flags.bReady )
			{
				MSG Msg;
				if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
				{
					if( l.flags.bLogMessageDispatch )
						lprintf( WIDE("(E)Got message:%d"), Msg.message );
					HandleMessage (Msg);
				}
			}
			*/
			if (!camera->hWndInstance)
			{
				return FALSE;
			}
		}
	}

	if( l.redraw_timer_id == 0 )
	{
		struct display_camera *camera = (struct display_camera *)GetLink( &l.cameras, 0 );
		lprintf( WIDE("Setting up redraw timer..") );
		l.redraw_timer_id = SetTimer( camera->hWndInstance, (UINT_PTR)1, 1, NULL );
		lprintf( WIDE("Setting up redraw timer.. result %d"), l.redraw_timer_id );
	}

	return (struct display_camera *)GetLink( &l.cameras, 0 );
}

BOOL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
#ifndef __NO_WIN32API__
	struct display_camera *camera;
	lprintf(WIDE( "Creating container window named: %s" ),
			(l.gpTitle && l.gpTitle[0]) ? l.gpTitle : (hVideo&&hVideo->pTitle)?hVideo->pTitle:WIDE("No Name"));
	if( !l.cameras )
		camera = OpenCameras(); // returns the forward camera
	else
		camera = (struct display_camera *)GetLink( &l.cameras, 0 );


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
				lprintf( WIDE( "Create Real Window (In CreateWindowStuff).." ) );
		#endif

				hVideo->hWndOutput = (HWND)1;
				hVideo->pWindowPos.x = x;
				hVideo->pWindowPos.y = y;
				hVideo->pWindowPos.cx = wx;
				hVideo->pWindowPos.cy = wy;
				lprintf( WIDE("%d %d"), x, y );
				CreateDrawingSurface (hVideo);

				hVideo->flags.bReady = 1;
				WakeThreadID( hVideo->thread );
			  //CreateWindowEx used to be here
			}
			else
			{

			}


		#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created window stuff..." ) );
		#endif
			// generate an event to dispatch pending...
			// there is a good chance that a window event caused a window
			// and it will be sleeping until the next event...
		#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
			SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
		#endif
			//Log (WIDE( "Created window in module..." ));

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
						_SetWindowLong( hVideo->hWndOutput, WD_HVIDEO, 0 );
						Release( hVideo ); // last event in queue, should be safe to go away now...
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
			lprintf( WIDE("mouse method... forward to application please...") );
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
						lprintf( WIDE("delta dispatching mouse (%d,%d) %08x")
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
		//SafeSetFocus( (HWND)GetDesktopWindow() );
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
		lprintf( WIDE( "Raw Input!" ) );
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
		lprintf( WIDE( "Message Create window stuff..." ) );
#endif
		CreateWindowStuffSizedAt (hVidCreate, hVidCreate->pWindowPos.x,
                                hVidCreate->pWindowPos.y,
                                hVidCreate->pWindowPos.cx,
                                hVidCreate->pWindowPos.cy);
	}
	else if( !Msg.hwnd && (Msg.message == (	WM_USER_OPEN_CAMERAS ) ) )
	{
		if( !l.cameras )
			OpenCameras(); // returns the forward camera
		l.flags.bUpdateWanted = 1;
	}
	else if (!Msg.hwnd && (Msg.message == (WM_USER + 513)))
	{
      HandleDestroyMessage( (PVIDEO) Msg.lParam );
	}
	else if( Msg.message == (WM_USER_HIDE_WINDOW ) )
	{
#ifdef LOG_SHOW_HIDE
		PVIDEO hVideo = ((PVIDEO)Msg.lParam);
		lprintf( WIDE( "Handling HIDE_WINDOW posted message %p" ),hVideo->hWndOutput );
#endif
		((PVIDEO)Msg.lParam)->flags.bHidden = 1;
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
					lprintf( WIDE("(E)Got message:%d"), Msg.message );
				HandleMessage (Msg);
				if( l.flags.bLogMessageDispatch )
					lprintf( WIDE("(X)Got message:%d"), Msg.message );
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
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("Dispatched... %p %d(%04x)"), Msg.hwnd, Msg.message, Msg.message );
			HandleMessage (Msg);
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("Finish Dispatched... %p %d(%04x)"), Msg.hwnd, Msg.message, Msg.message );
		}
	}
	l.bThreadRunning = FALSE;
	lprintf( WIDE( "Video Exited volentarily" ) );
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
		wc.lpszClassName = WIDE( "GLVideoOutputClass" );
		wc.cbWndExtra = sizeof(PVIDEO);   // one extra DWORD

		l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
			lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
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
				Log1( WIDE("Failed to post shutdown message...%d" ), error );

				if( error == ERROR_INVALID_THREAD_ID )
               break;
				cnt--;
			}
			Relinquish();
		}
		while (!d && cnt);
		if (!d)
		{
			lprintf( WIDE( "Tried %d times to post thread message... and it alwasy failed." ), 25-cnt );
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
				lprintf( WIDE( "Had to give up waiting for video thread to exit..." ) );
		}
	}
}
#endif

TEXTCHAR  GetKeyText (int key)
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

LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
{
	InitDisplay();
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
#if 0 // this id added in the thread anyhow
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

	PutDisplayAbove( hNextVideo, l.top );

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
		lprintf( WIDE("about to Create some window stuff") );
		CreateWindowStuffSizedAt( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										, hNextVideo->pWindowPos.cy);
		lprintf( WIDE("Created some window stuff") );
	}
	else
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			lprintf( WIDE("posting create to thred.") );
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
			DebugBreak ();
		}
		if( hNextVideo )
		{
			_32 timeout = timeGetTime() + 500000;
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
					//WakeableSleep( SLEEP_FOREVER );
					WakeableSleep( 250 );
					//Relinquish();
				}
			}
			//if( !hNextVideo->flags.bReady )
			{
				//CloseDisplay( hNextVideo );
				//lprintf( WIDE("Fatality.. window creation did not complete in a timely manner.") );
				// hnextvideo is null anyhow, but this is explicit.
				//return FALSE;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( WIDE("Resulting new window %p %d"), hNextVideo, hNextVideo->hWndOutput );
#endif
	return TRUE;
}

PVIDEO  MakeDisplayFrom (HWND hWnd) 
{
	
	PVIDEO hNextVideo;
	hNextVideo = New( VIDEO );
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
	{
		lprintf( WIDE("New bottom is %p"), l.bottom );
		return hNextVideo;
	}
	Release( hNextVideo );
	return NULL;
#if 0
	_SetWindowLong( hWnd, GWL_WNDPROC, (DWORD)VideoWindowProc );
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
	//lprintf( "open display..." );
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
		lprintf( WIDE("New bottom is %p"), l.bottom );
		return hNextVideo;
	}
	Release( hNextVideo );
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
				_SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
				_SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
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

			if( newvid->pBelow )
				newvid->pBelow->pAbove = newvid->pAbove;
			if( newvid->pAbove )
				newvid->pAbove->pBelow = newvid->pBelow;

			newvid->pBelow = barrier;
			if( newvid->pAbove = barrier->pAbove )
				barrier->pAbove->pBelow = newvid;
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
	// just kills this video handle....
	if (!hVideo)         // must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log (WIDE( "Unlinking destroyed window..." ));
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	UnlinkVideo( hVideo );
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
	lprintf( WIDE( "Size Display..." ) );
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
		Log2 (WIDE( "Resized display to %d,%d" ), hVideo->pWindowPos.cx,
            hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "size display relative" ) );
#endif
		SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
                    SWP_NOZORDER | SWP_NOMOVE);
   }
}

//----------------------------------------------------------------------------

void  MoveDisplay (PVIDEO hVideo, S_32 x, S_32 y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Move display %d,%d" ), x, y );
#endif
   if( hVideo )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
			Translate( hVideo->transform, -x, -y, 0.0 );
			//if( hVideo->flags.bShown )
			//{
				// layered window requires layered output to be called to move the display.
			//	UpdateDisplay( hVideo );
			//}
		}
	}
}

//----------------------------------------------------------------------------

void  MoveDisplayRel (PVIDEO hVideo, S_32 x, S_32 y)
{
   if (x || y)
   {
		lprintf( WIDE("Moving display %d,%d"), x, y );
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
		Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "Move display relative" ) );
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
	lprintf( WIDE( "move and size display." ) );
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
	lprintf( WIDE( "move and size relative %d,%d %d,%d" ), delx, dely, delw, delh );
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

void  SetMousePosition (PVIDEO hVid, S_32 x, S_32 y)
{
	if( !hVid )
	{
		int newx, newy;
		hVid = l.mouse_last_vid;
			
		//lprintf( WIDE("TAGHERE") );
		if( hVid )
		{
			//InverseOpenGLMouse( hVid->camera, hVid, (RCOORD)x, (RCOORD)y, &newx, &newy );
			newx = x + hVid->pWindowPos.x;
			newy = y + hVid->pWindowPos.y;
		}
		else
		{
			newx = x;
			newy = y;
		}
		//lprintf( WIDE("%d,%d became %d,%d"), x, y, newx, newy );
		SetCursorPos( newx, newy );
	}
	else
	{
		if( hVid->camera && hVid->flags.bFull)
		{
			int newx, newy;
			//lprintf( "TAGHERE" );
			InverseOpenGLMouse( hVid->camera, hVid, x+ hVid->cursor_bias.x, y, &newx, &newy );
			//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
			SetCursorPos (newx,newy);
		}
		else
		{
			if( l.current_mouse_event_camera )
			{
				int newx, newy;
				//lprintf( "TAGHERE" );
				InverseOpenGLMouse( l.current_mouse_event_camera, hVid, x, y, &newx, &newy );
				//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
				SetCursorPos (newx + l.current_mouse_event_camera->x ,
								  newy + l.current_mouse_event_camera->y );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  GetMousePosition (S_32 * x, S_32 * y)
{
	lprintf( WIDE("This is really relative to what is looking at it ") );
	//DebugBreak();
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
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		if( hVideo->flags.bShown )
		{
			Redraw( hVideo );
			//lprintf( WIDE( "Invalida.." ) );
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
         Release( hVideo->pTitle );
		hVideo->pTitle = StrDupEx( pTitle DBG_SRC );
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

void  MakeAbsoluteTopmost (PVIDEO hVideo)
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

 int  IsTopmost ( PVIDEO hVideo )
{
   return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
void  HideDisplay (PVIDEO hVideo)
{
//#ifdef LOG_SHOW_HIDE
	lprintf(WIDE( "Hiding the window! %p %p %p %p" ), hVideo, hVideo->hWndOutput, hVideo->pAbove, hVideo->pBelow );
//#endif
	if( hVideo )
	{
		if( l.hCapturedMouseLogical == hVideo )
		{
			l.hCapturedPrior = NULL;
			l.hCapturedMouseLogical = NULL;
		}
		if( l.hCapturedMousePhysical == hVideo )
		{
			l.hCapturedPrior = NULL;
			l.hCapturedMousePhysical = NULL;
		}
		hVideo->flags.bHidden = 1;
		if( hVideo->pHideCallback )
         hVideo->pHideCallback( hVideo->dwHideData );
		/* handle lose focus */
	}
}

//----------------------------------------------------------------------------
#undef RestoreDisplay
void RestoreDisplay( PVIDEO hVideo  )
{
   lprintf( WIDE("We should not be here!") );
   RestoreDisplayEx( hVideo DBG_SRC );
}


void RestoreDisplayEx(PVIDEO hVideo DBG_PASS )
{
	PostThreadMessage (l.dwThreadID, WM_USER_OPEN_CAMERAS, 0, 0 );

#ifdef LOG_SHOW_HIDE
	lprintf( WIDE( "Restore display. %p %p" ), hVideo, hVideo?hVideo->hWndOutput:0 );
#endif

	if( hVideo )
	{
		hVideo->flags.bHidden = 0;
		hVideo->flags.bShown = 1;
		if( hVideo->pRestoreCallback )
         hVideo->pRestoreCallback( hVideo->dwRestoreData );
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
			int ndisplay_enum = 0;
			DISPLAY_DEVICE dev;
			DEVMODE dm;
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
						lprintf( WIDE("display %s is at %d,%d %dx%d"), dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					else
						lprintf( WIDE("Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					ndisplay_enum++;
					lprintf( WIDE( "[%s] might be [%s]" ), teststring, dev.DeviceName );
					if( StrCaseCmp( teststring, dev.DeviceName ) == 0 || ndisplay_enum )
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
							lprintf( WIDE("Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
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
				//Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
				if (width)
					*width = r.right - r.left;
				if (height)
					*height = r.bottom - r.top;
			}
		}

}

void  GetDisplaySize (_32 * width, _32 * height)
{
	{
		INDEX idx;
		struct display_camera * camera;
		LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
		{	
			if( (struct display_camera*)1 == camera )
			{
				Relinquish();
				idx == -1;
				continue;
			}
			if( width )
				(*width) = camera->w;
			if( height )
				(*height) = camera->h;
			break;
		}
	}
	//GetDisplaySizeEx( 0, width, height );
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
		if( hVid && ( hVid->hWndOutput != (HWND)1 ) )
		{
			WINDOWINFO wi;
			wi.cbSize = sizeof( wi);
			
			if( GetWindowInfo( hVid->hWndOutput, &wi ) )
			{
				posx += wi.rcClient.left;
				posy += wi.rcClient.top;
			}
		}
		else
		{
			posx = hVid->pWindowPos.x;
			posy = hVid->pWindowPos.y;
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
		if( !hVideo )
		{
			hVideo = l.mouse_last_vid;
			l.hCapturedMousePhysical = hVideo;
			hVideo->flags.bCaptured = 1;
			SetCapture (hVideo->hWndOutput);
		}
		else
		{
			lprintf( WIDE("Capture is set on %p"),hVideo );
			if( !l.hCapturedMouseLogical )
			{
				l.hCapturedMouseLogical = hVideo;
				hVideo->flags.bCaptured = 1;
			}
			else
			{
				if( l.hCapturedMouseLogical != hVideo )
				{
					lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
					l.hCapturedMouseLogical = hVideo;
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
	}
	else
	{
		if( !hVideo )
		{
			l.hCapturedMousePhysical = NULL;
			ReleaseCapture();
		}
		else
		{
			if( l.hCapturedMouseLogical == hVideo )
			{
				//lprintf( WIDE("No more capture.") );
				//ReleaseCapture ();
				hVideo->flags.bCaptured = 0;
				l.hCapturedPrior = NULL;
				l.hCapturedMouseLogical = NULL;
			}
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
	//lprintf( WIDE( "... 3 step?" ) );
	//SetActiveWindow( pRender->hWndOutput );
	//SetForegroundWindow( pRender->hWndOutput );
	//if( pRender )
	//	SafeSetFocus( pRender->hWndOutput );
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
   lprintf( WIDE( "Force display backward." ) );
   SetWindowPos( pRender->hWndOutput, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
void  UpdateDisplay (PRENDERER hVideo )
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
			l.mouse_timer_id = (_32)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)2, 100, NULL );
			hVideo->idle_timer_id = (_32)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)3, 100, NULL );
			l.last_mouse_update = timeGetTime(); // prime the hider.
			hVideo->flags.bIdleMouse = bEnable;
		}
		else // disabling...
		{
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
			lprintf( WIDE( "Output fade %d %p" ), hVideo->fade_alpha, hVideo->hWndOutput );
	}
}

#undef GetRenderTransform
PTRANSFORM CPROC GetRenderTransform       ( PRENDERER r )
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
#ifdef WIN32
													, GetNativeHandle
#endif
                                       , GetDisplaySizeEx
													, LockRenderer
													, UnlockRenderer
													, NULL  //IssueUpdateLayeredEx
                                       , RequiresDrawAll
#ifndef NO_TOUCH
													, SetTouchHandler
#endif
													, MarkDisplayUpdated
													, SetHideHandler
                                       , SetRestoreHandler
                                       , RestoreDisplayEx
};

RENDER3D_INTERFACE Render3d = {
	GetRenderTransform
	, NULL
};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  CPROC GetDisplayInterface (void)
{
	InitDisplay();
   return (POINTER)&VidInterface;
}

void  CPROC DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER CPROC GetDisplay3dInterface (void)
{
	InitDisplay();
	return (POINTER)&Render3d;
}

void  CPROC DropDisplay3dInterface (POINTER p)
{
}

static LOGICAL CPROC DefaultExit( PTRSZVAL psv, _32 keycode )
{
   lprintf( WIDE( "Default Exit..." ) );
	BAG_Exit(0);
   return 1;
}

static LOGICAL CPROC EnableRotation( PTRSZVAL psv, _32 keycode )
{
	lprintf( WIDE("Enable Rotation...") );
	if( IsKeyPressed( keycode ) )
	{
		l.flags.bRotateLock = 1 - l.flags.bRotateLock;
		if( l.flags.bRotateLock )
		{
			struct display_camera *default_camera = (struct display_camera *)GetLink( &l.cameras, 0 );
			l.mouse_x = default_camera->hVidCore->pWindowPos.cx/2;
			l.mouse_y = default_camera->hVidCore->pWindowPos.cy/2;
			SetCursorPos( default_camera->hVidCore->pWindowPos.x 
				+ default_camera->hVidCore->pWindowPos.cx/2
				, default_camera->hVidCore->pWindowPos.y 
				+ default_camera->hVidCore->pWindowPos.cy / 2 );
		}
		lprintf( WIDE("ALLOW ROTATE") );
	}
	else
		lprintf( WIDE("DISABLE ROTATE") );
	if( l.flags.bRotateLock )
		lprintf( WIDE("lock rotate") );
	else
		lprintf(WIDE("unlock rotate") );
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
      UpdateMouseRays( );
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
      UpdateMouseRays( );
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
      UpdateMouseRays( );
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
      UpdateMouseRays( );
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
		UpdateMouseRays( );
	}
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
		UpdateMouseRays( );
	}
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

PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	if( l.flags.bLogRegister )
		lprintf( WIDE("Regstering video interface...") );
#ifndef __ANDROID__
#ifndef UNDER_CE
#if !defined( __WATCOMC__ ) && !defined( __GNUC__ )
	l.GetTouchInputInfo = (BOOL (WINAPI *)( HTOUCHINPUT, UINT, PTOUCHINPUT, int) )LoadFunction( WIDE("user32.dll"), WIDE("GetTouchInputInfo") );
	l.CloseTouchInputHandle =(BOOL (WINAPI *)( HTOUCHINPUT ))LoadFunction( WIDE("user32.dll"), WIDE("CloseTouchInputHandle") );
	l.RegisterTouchWindow = (BOOL (WINAPI *)( HWND, ULONG  ))LoadFunction( WIDE("user32.dll"), WIDE("RegisterTouchWindow") );
#endif
#endif
#endif
	RegisterInterface( 
	   WIDE("puregl.render")
	   , GetDisplayInterface, DropDisplayInterface );
	RegisterInterface( 
	   WIDE("puregl.render.3d")
	   , GetDisplay3dInterface, DropDisplay3dInterface );

	l.gl_image_interface = (PIMAGE_INTERFACE)GetInterface( WIDE("image") );

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
	PSPRITE_METHOD psm = New( struct sprite_method_tag );
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
	//lprintf( WIDE("Save Portion %d,%d %d,%d"), x, y, w, h );
	EnqueData( &psm->saved_spots, &location );
	//lprintf( WIDE("Save Portion %d,%d %d,%d"), x, y, w, h );
}

PRELOAD( InitSetSavePortion )
{
   //SetSavePortion( SavePortion );
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

