#define NO_UNICODE_C

#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include <idle.h>

#include "local.h"

RENDER_NAMESPACE

extern KEYDEFINE KeyDefs[];
#if defined( UNDER_CE )
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#  if defined( _WIN32 )
#    define USE_KEYHOOK
#  endif
#endif

//#define LOG_STARTUP
//#define OTHER_EVENTS_HERE
//#define LOG_MOUSE_EVENTS
//#define LOG_RECT_UPDATE
//#define LOG_DESTRUCTION
#define LOG_STARTUP
//#define LOG_FOCUSEVENTS
//#define LOG_SHOW_HIDE
//#define LOG_DISPLAY_RESIZE
//#define NOISY_LOGGING
// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS
#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT

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


ATEXIT( ExitTest )
{
	l.bExitThread = 1;
	if( l.bThreadRunning )
	{
		if( l.dwThreadID != GetCurrentThreadId () )
		{
			// just to make sure something wakes it up... it could be in a mouse event.
			PostThreadMessage (l.dwThreadID, WM_USER_SHUTDOWN, 0, 0);
			while(l.bThreadRunning)
			{
				lprintf( WIDE("waiting...") );
            WakeableSleep( 10 );
				//Relinquish();
			}
		}
	}
}


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
			PVIDEO hVideo = (PVIDEO)_GetWindowLong( hWnd, WD_HVIDEO );
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
         hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
							(PVIDEO) _GetWindowLong ((HWND) wParam, WD_HVIDEO);
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
				hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
      hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
			hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
			hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
#ifndef NO_TOUCH
	case WM_TOUCH:
		{
			if( l.GetTouchInputInfo )
			{
				TOUCHINPUT inputs[20];
				struct input_point outputs[20];
				int count = LOWORD(wParam);
				PVIDEO hVideo = (PVIDEO)_GetWindowLong( hWnd, WD_HVIDEO );
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
						handled = hVideo->pTouchCallback( hVideo->dwTouchData, inputs, count );
					}

					if( !handled )
					{
						// this will be like a hvid core
						handled = Handle3DTouches( hVideo->camera, outputs, count );
					}
					if( handled )
						Return 0;
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
		hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
		if (!hVideo)
		{
			Return 0;
		}
		if( l.hCameraCaptured )
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
				if( !l.hCameraCaptured )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( WIDE("Auto release mouse from surface which had the mouse unclicked.") );
				if( !l.hCameraCaptured )
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
			p.x -= (dx =(hVideo)->cursor_bias.x);
			p.y -= (dy=(hVideo)->cursor_bias.y);
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position results %d,%d %d,%d %p"), dx, dy, p.x, p.y, l.hCaptured );
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
			{
				_32 msg[4];
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
			}
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
		hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
		hVideo = (PVIDEO) _GetWindowLong (hWnd, WD_HVIDEO);
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
         ProcessGLDraw( TRUE );
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

				_SetWindowLong(hWnd, WD_HVIDEO, hVideo);

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

#ifdef UNDER_CE
#define WINDOW_STYLE 0
#else
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
#endif

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
				TEXTCHAR szTempOutput[256];
				HRESULT hResult;

				if (raw->header.dwType == RIM_TYPEKEYBOARD)
				{
               /*
					tnprintf(szTempOutput, sizeof( szTempOutput ), TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
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
					tnprintf(szTempOutput,sizeof( szTempOutput ) , TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
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
#if 0
	else if( !Msg.hwnd && (Msg.message == (	WM_USER_OPEN_CAMERAS ) ) )
	{
		if( !l.cameras )
			OpenCameras(); // returns the forward camera
		l.flags.bUpdateWanted = 1;
	}
#endif
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


void OpenWin32Camera( struct display_camera *camera )
{
	static HMODULE hMe;
	TEXTCHAR window_name[128];
	if( !camera->hWndInstance )
	{
		hMe = GetModuleHandle (_WIDE(TARGETNAME));

		if( !camera->display )
			tnprintf( window_name, 128, WIDE("%s:3D View"), GetProgramName() );
		else
			tnprintf( window_name, 128, WIDE("%s:3D View(%d)"), GetProgramName(), camera->display );
		camera->hWndInstance = CreateWindowEx (0
	#ifndef NO_DRAG_DROP
														| WS_EX_ACCEPTFILES
	#endif
#ifndef NO_TRANSPARENCY
															| (camera->hVidCore->flags.bLayeredWindow?WS_EX_LAYERED:0)
#endif
	#ifdef UNICODE
													  , (LPWSTR)l.aClass
	#else
													  , (LPSTR)l.aClass
	#endif
													  , (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : window_name
													  , WS_POPUP //| WINDOW_STYLE
													  , camera->x, camera->y, (int)camera->w, (int)camera->h
													  , NULL // HWND_MESSAGE  // (GetDesktopWindow()), // Parent
													  , NULL // Menu
													  , hMe
													  , (void*)camera );
	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.." ) );
	#endif
			camera->hVidCore->hWndOutput = (HWND)camera->hWndInstance;
			// only need one timer, each camera is draw in the timer. (avoid multithread opengl)
			if( !l.redraw_timer_id )
			{
				l.redraw_timer_id = SetTimer( camera->hWndInstance, (UINT_PTR)1, 16, NULL );
			}
#ifdef _OPENGL_DRIVER
			EnableOpenGL( camera );
#endif
#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER ) || defined( _D3D11_DRIVER )
			EnableD3d( camera );
#endif
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
	}
}

static int CPROC ProcessDisplayMessages( PTRSZVAL psvUnused )
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

#ifndef NO_TOUCH
HHOOK prochook;
LRESULT CALLBACK AllWndProc( int code, WPARAM wParam, LPARAM lParam )
{
	PCWPSTRUCT msg  = (PCWPSTRUCT)lParam;
	//lprintf( WIDE( "msg %p %d %d %p" ), msg->hwnd, msg->message, msg->wParam, msg->lParam );
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
	l.bThreadRunning = TRUE;
	SACK_Vidlib_OpenCameras();
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

static void OnLibraryLoad( WIDE("Video Render PureGL 2") )( void )
{
	if( l.bThreadRunning )
		SACK_Vidlib_OpenCameras();
}

//----------------------------------------------------------------------------


PRELOAD( HostSystem_InitDisplayInfo )
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
		wc.cbWndExtra = sizeof( PVIDEO );   // one extra DWORD

		l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
			lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
		}

#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
		InitMessageService();
		l.dwMsgBase = LoadService( NULL, VideoEventHandler );
#endif
      // need options loaded before thread, because cameras will open.
		LoadOptions();
		AddLink( &l.threads, ThreadTo( VideoThreadProc, 0 ) );
		AddIdleProc( ProcessDisplayMessages, 0 );
	}
#endif
}


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
				tnprintf( teststring, 20, WIDE( "\\\\.\\DISPLAY%s%d" ), (v_test==1)?"V":"", nDisplay );
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
			RECT r;
			GetWindowRect (GetDesktopWindow (), &r);
			//Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
			if (width)
				*width = r.right - r.left;
			if (height)
				*height = r.bottom - r.top;
			if( x )
				(*x)= 0;
			if( y )
				(*y)= 0;
		}

}

RENDER_NAMESPACE_END
