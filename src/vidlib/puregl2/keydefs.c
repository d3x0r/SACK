#define NO_UNICODE_C
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#define _INCLUDE_CLIPBOARD
#define KEYS_DEFINED
#include <stdhdrs.h>

// okay yea might leave this on... set l.flags.bLogKey<..>>
#define LOG_KEY_EVENTS

#include "local.h"

RENDER_NAMESPACE


extern int myTypeID;

enum{
   KS_DELETE
};

#ifdef __cplusplus
#else
//TEXT DELETE_STROKE = DEFTEXT( WIDE("\x7f") );
#endif

KEYDEFINE KeyDefs[256];
//----------------------------------------------------------------------------

#if defined( _WIN32 )
int KeystrokePaste( PRENDERER pRenderer )
{
    if( OpenClipboard(NULL) )
    {
        _32 format;
        // successful open...
        format = EnumClipboardFormats( 0 );
        while( format )
        {
            //DECLTEXT( msg, WIDE("                                     ") );
            //msg.data.size = sprintf( msg.data.data, WIDE("Format: %d"), format );
            //EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate( (PTEXT)&msg ) );
            if( format == CF_TEXT )
            {
                HANDLE hData = GetClipboardData( CF_TEXT );
                LPVOID pData = GlobalLock( hData );
                PTEXT pStroke = SegCreateFromText( (CTEXTSTR)pData );
                int ofs, n;
                GlobalUnlock( hData );
                n = ofs = 0;
                while( pStroke->data.data[n] )
                {
                    pStroke->data.data[ofs] = pStroke->data.data[n];
                    if( pStroke->data.data[n] == '\r' ) // trash extra returns... keep newlines
                    {
                        n++;
                        continue;
                    }
                    else
                    {
                        ofs++;
                        n++;
                    }           
                }
                pStroke->data.size = ofs;
                pStroke->data.data[ofs] = pStroke->data.data[n];
                EnqueLink( &pRenderer->pInput, SegDuplicate(pStroke) );
                break;
            }
            format = EnumClipboardFormats( format );
        }
        CloseClipboard();
    }
    else
    {
        //DECLTEXT( msg, WIDE("Clipboard was not available") );
        //EnqueLink( &pdp->ps->Command->Output, &msg );
    }
    return 0;

}

#endif

//----------------------------------------------------------------------------
// Extensions and usage of keybinding data
// -- so far seperated so that perhaps it could be a seperate module...
//----------------------------------------------------------------------------

#define NUM_MODS ( sizeof( ModNames ) / sizeof( char * ) )
#if 0
char *ModNames[] = { "shift", WIDE("ctrl"), WIDE("alt")
                   , NULL, WIDE("control"), NULL
                   , WIDE("$"), WIDE("^"), WIDE("@") };

int FindMod( PTEXT pMod )
{
   int i;
   for( i = 0; i < NUM_MODS; i++ )
   {
      if( ModNames[i] )
         if( TextLike( pMod, ModNames[ i ] ) )
            break;
   }
   if( i < NUM_MODS )
      return ( 1 << ( i % 3 ) );
   return 0;
}

int FindKey( PTEXT pKey )
{
   int i;
   for( i = 0; i < NUM_KEYS; i++ )
   {
      //if( ( !KeyDefs[i].flags ) ||
      //  ( KeyDefs[i].flags & (KDF_NODEFINE) ) )
      // continue;
      if( KeyDefs[i].name1 && TextLike( pKey, KeyDefs[i].name1 ) )
      {
         return i;
      }
      else if( KeyDefs[i].name2 && TextLike( pKey, KeyDefs[i].name2 ) )
      {
         return i;
      }
   }
   return 0;
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC( PKEYDEFINE, CreateKeyBinder )( void )
{
	PKEYDEFINE KeyDef = NewArray( KEYDEFINE, 256 );
	MemSet( KeyDef, 0, sizeof( KEYDEFINE ) * 256 );
	return KeyDef;
}

RENDER_PROC( void, DestroyKeyBinder )( PKEYDEFINE pKeyDef )
{
   Deallocate( PKEYDEFINE, pKeyDef );
}

// Usage: /KeyBind shift-F1
//      ... #commands
//      /endmac
// Usage: /KeyBind shift-F1 kill
// Usage: /KeyBind $F1 ... ^F1 $^F1
//  if parameters follow the keybind key-def, those params
//  are taken as keystrokes to type...
//  if no parameters follow, the definition is assumed to
//  be a macro definition, and the macro is invoked by
//  the processing entity...
RENDER_PROC( int, BindEventToKeyEx )( PKEYDEFINE pKeyDefs, _32 keycode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	PKEY_FUNCTION keyfunc;
	keyfunc = New( KEY_FUNCTION );
	MemSet( keyfunc, 0, sizeof( KEY_FUNCTION ) );
	if( modifier & KEY_MOD_ALL_CHANGES )
	{
		pKeyDefs[keycode].mod[modifier&0x7].flags.bAll = TRUE;
	}
	else if( modifier & KEY_MOD_RELEASE )
		pKeyDefs[keycode].mod[modifier&0x7].flags.bRelease = TRUE;
	else
		pKeyDefs[keycode].mod[modifier&0x7].flags.bRelease = FALSE;
	pKeyDefs[keycode].mod[modifier&0x7].flags.bFunction = TRUE;
	if( modifier & KEY_MOD_EXTENDED )
	{
		keyfunc->data.extended_key_trigger = trigger;
		keyfunc->extended_key_psv = psv;
		AddLink( &pKeyDefs[keycode].mod[modifier&0x7].key_procs, keyfunc );
		//pKeyDefs[keycode].mod[modifier&0x7].data.extended_key_trigger = trigger;
		//pKeyDefs[keycode].mod[modifier&0x7].extended_key_psv = psv;
	}
	else
	{
      keyfunc->data.trigger = trigger;
      keyfunc->psv = psv;
		AddLink( &pKeyDefs[keycode].mod[modifier&0x7].key_procs, keyfunc );
		//pKeyDefs[keycode].mod[modifier&0x7].data.trigger = trigger;
		//pKeyDefs[keycode].mod[modifier&0x7].psv = psv;
	}
	return TRUE;
}

RENDER_PROC( int, BindEventToKey )( PRENDERER pRenderer, _32 keycode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	return BindEventToKeyEx( pRenderer?pRenderer->KeyDefs:KeyDefs
								  , keycode, modifier
								  , trigger, psv );
}
//----------------------------------------------------------------------------

RENDER_PROC( int, UnbindKey )( PRENDERER pRenderer, _32 keycode, _32 modifier )
{
   if( pRenderer )
   {
      pRenderer->KeyDefs[keycode].mod[modifier].flags.bFunction = FALSE;
   }
   else
   {
      KeyDefs[keycode].mod[modifier].flags.bFunction = FALSE;
   }
   return TRUE;
}


RENDER_PROC( void, SetRenderReadCallback )( PRENDERER pRenderer, RenderReadCallback callback, PTRSZVAL psv )
{
   if( pRenderer )
   {
      pRenderer->ReadComplete = callback;
      pRenderer->psvRead = psv;
   }
}

RENDER_PROC( int, HandleKeyEvents )( PKEYDEFINE pKeyDefs, _32 key )
{
	int keycode = KEY_CODE(key);
	int keymod = KEY_MOD(key);
#ifdef LOG_KEY_EVENTS
   if( l.flags.bLogKeyEvent )
		lprintf( WIDE("Key event for %08lx ... %d %s %s %s")
				 , key
				 , keycode
				 , keymod&1?"SHIFT":"", keymod&2?"CTRL":"", keymod&4?"ALT":"" );
#endif
	if( pKeyDefs[keycode].mod[keymod].flags.bFunction )
	{
#ifdef LOG_KEY_EVENTS
   if( l.flags.bLogKeyEvent )
		lprintf( WIDE("And there is a function... key is %s"), IsKeyPressed( key )?"Pressed":"Released" );
#endif
		if( pKeyDefs[keycode].mod[keymod].flags.bAll ||
			( IsKeyPressed( key ) && !pKeyDefs[keycode].mod[keymod].flags.bRelease) ||
			( !IsKeyPressed( key ) && pKeyDefs[keycode].mod[keymod].flags.bRelease) )
		{
			PKEY_FUNCTION keyfunc;
         INDEX idx;
			//DebugBreak();
#ifdef LOG_KEY_EVENTS
			if( l.flags.bLogKeyEvent )
				lprintf( WIDE("Invoke!") );
#endif
			if( IsKeyExtended( key ) )
			{
#ifdef LOG_KEY_EVENTS
				if( l.flags.bLogKeyEvent )
					lprintf(WIDE( "extended key method configured" ) );
#endif
				LIST_FORALL( pKeyDefs[keycode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
				{
					if( keyfunc->data.extended_key_trigger )
					{
#ifdef LOG_KEY_EVENTS
						if( l.flags.bLogKeyEvent )
							lprintf(WIDE( "extended key method configured" ) );
#endif
						if( keyfunc->data.extended_key_trigger( keyfunc->extended_key_psv
																		  , key ) )
							return 1;
					}
				}
				return 0;
			}
			else
			{
#ifdef LOG_KEY_EVENTS
				if( l.flags.bLogKeyEvent )
					lprintf(WIDE( "not extended key method" ) );
#endif
				LIST_FORALL( pKeyDefs[keycode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
				{
					if( keyfunc->data.trigger )
					{
#ifdef LOG_KEY_EVENTS
						if( l.flags.bLogKeyEvent )
							lprintf(WIDE( "extended key method configured" ) );
#endif
						if( keyfunc->data.trigger( keyfunc->psv, key ) )
                     return 1;
					}
				}
				return 0;
			}
		}
		if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Probably handled..." ) );
		// for consistancy better just say we handled this key
		return 1;
	}
	if( l.flags.bLogKeyEvent )
		lprintf( WIDE( "not handled..." ) );
	return 0;
}

int DispatchKeyEvent( PRENDERER hVideo, _32 key )
{

   int dispatch_handled;
   int keymod = 0;

	// this really will be calling OpenGLKey above....
			if (key & 0x80000000)   // test keydown...
			{
				l.kbd.key[KEY_CODE(key)] |= 0x80;   // set this bit (pressed)
				l.kbd.key[KEY_CODE(key)] ^= 1;   // toggle this bit...
			}
			else
			{
				l.kbd.key[KEY_CODE(key)] &= ~0x80;  //(unpressed)
			}
			//lprintf( WIDE("Set local keyboard %d to %d"), wParam& 0xFF, l.kbd.key[wParam&0xFF]);
			if( hVideo )
				hVideo->kbd.key[KEY_CODE(key)] = l.kbd.key[KEY_CODE(key)];

			if( (l.kbd.key[KEY_LEFT_SHIFT]|l.kbd.key[KEY_RIGHT_SHIFT]|l.kbd.key[KEY_SHIFT]) & 0x80)
			{
				key |= ( KEY_MOD_SHIFT << 28 );
				l.mouse_b |= MK_SHIFT;
				keymod |= 1;
			}
			else
				l.mouse_b &= ~MK_SHIFT;
			if ((l.kbd.key[KEY_LEFT_CONTROL]|l.kbd.key[KEY_RIGHT_CONTROL]|l.kbd.key[KEY_CTRL]) & 0x80)
			{
				key |= ( KEY_MOD_CTRL << 28 );
				l.mouse_b |= MK_CONTROL;
				keymod |= 2;
			}
			else
				l.mouse_b &= ~MK_CONTROL;
			if((l.kbd.key[KEY_LEFT_ALT]|l.kbd.key[KEY_RIGHT_ALT]|l.kbd.key[KEY_ALT]) & 0x80)
			{
				key |= ( KEY_MOD_ALT << 28 );
				l.mouse_b |= MK_ALT;
				keymod |= 4;
			}
			else
				l.mouse_b &= ~MK_ALT;

	if( dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, key ) )
	{
		return 1;
	}

   // start with 'focused virtual panel...'
	hVideo = l.hVidVirtualFocused;
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
					if( KEY_MOD( key ) & 6 )
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
						if( !(KEY_MOD( key ) & 6) )
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
		else  // no keyproc on the hrenderer; might be mouse-based; but give global a shot...
		{
			if( !HandleKeyEvents( KeyDefs, key ) ) /* global events, if no keyproc */
			{
				if( hVideo->pMouseCallback )
					hVideo->pMouseCallback( hVideo->dwMouseData, hVideo->_mouse_x, hVideo->_mouse_y
									, hVideo->mouse_b | IsKeyPressed( key )?MK_OBUTTON:MK_OBUTTON_UP );
			}
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

	// dispatch a mouse event!
   lprintf( "Missing dispatch for no key method to mouse event" );
   return dispatch_handled;
}


RENDER_NAMESPACE_END

//----------------------------------------------------------------------------
//
// $Log: keydefs.c,v $
// Revision 1.10  2005/04/09 00:45:23  panther
// Fix key invokation...
//
// Revision 1.9  2005/04/08 12:08:13  panther
// Fix key handling to trigger on key down instead of key release.
//
// Revision 1.8  2004/12/04 01:22:04  panther
// Destroy key binder associated with hvideo
//
// Revision 1.7  2004/08/24 11:15:15  d3x0r
// Checkpoint Visual studio mods.
//
// Revision 1.6  2004/08/11 11:40:23  d3x0r
// Begin seperation of key and render
//
// Revision 1.5  2004/06/21 07:46:45  d3x0r
// Account for newly moved structure files.
//
// Revision 1.4  2004/05/04 03:36:47  d3x0r
// Cleanup compile of keydefs...
//
// Revision 1.3  2004/05/03 06:15:39  d3x0r
// Define buffered render read
//
// Revision 1.2  2004/05/02 05:44:38  d3x0r
// Implement  BindEventToKey and UnbindKey
//
// Revision 1.1  2004/04/27 09:55:11  d3x0r
// Add keydef to keyhandler path
//
//
