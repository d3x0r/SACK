#define NO_UNICODE_C
#define FIX_RELEASE_COM_COLLISION

#ifdef UNDER_CE
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#define USE_KEYHOOK
#endif

#include <stdhdrs.h>

#include "local.h"

RENDER_NAMESPACE

// key_events has this

const TEXTCHAR*  GetKeyText (int key)
{
	static int c;
	static char ch[5];
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
#ifdef UNICODE
	{
		static wchar_t *out;
		if( out ) Deallocate( wchar_t *, out );
		out = DupCStr( ch );
		return out;
	}
#else
	return ch;
#endif
}

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

			dispatch_handled = DispatchKeyEvent( hVid, key );
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
			dispatch_handled = DispatchKeyEvent( hVid, key );
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


void SACK_Vidlib_ShowInputDevice( void )
{
//	if( keymap_local.show_keyboard )
//      keymap_local.show_keyboard();
}

void SACK_Vidlib_HideInputDevice( void )
{
//	if( keymap_local.hide_keyboard )
//      keymap_local.hide_keyboard();
}

RENDER_NAMESPACE_END

