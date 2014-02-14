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
extern KEYDEFINE KeyDefs[];


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
			hVid->kbd.key[wParam & 0xFF] = l.kbd.key[wParam & 0xFF];

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


         // this really will be calling OpenGLKey above....
			if( dispatch_handled = hVid->pKeyProc( hVid->dwKeyData, key ) )
			{
				return 1;
			}

			if( !dispatch_handled )
			{
				PVIDEO hVideo = l.hVidVirtualFocused;

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
										if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, key ) )
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

