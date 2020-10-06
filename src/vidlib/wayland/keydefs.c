#define _INCLUDE_CLIPBOARD
#define KEYS_DEFINED
#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <keybrd.h>
#include <render.h>
#include <vidlib/keydef.h>
#include "keydefs.h"
// okay yea might leave this on... set l.flags.bLogKey<..>>
#define LOG_KEY_EVENTS

#include "local.h"
extern struct wayland_local_tag wl;

#define l wl


extern int myTypeID;

enum{
   KS_DELETE
};


KEYDEFINE KeyDefs[256];
//----------------------------------------------------------------------------

#if defined( _WIN32 )
int KeystrokePaste( PRENDERER pRenderer )
{
    if( OpenClipboard(NULL) )
    {
        uint32_t format;
        // successful open...
        format = EnumClipboardFormats( 0 );
        while( format )
        {
            //DECLTEXT( msg, "                                     " );
            //msg.data.size = sprintf( msg.data.data, "Format: %d", format );
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
        //DECLTEXT( msg, "Clipboard was not available" );
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
char *ModNames[] = { "shift", "ctrl", "alt"
                   , NULL, "control", NULL
                   , "$", "^", "@" };

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

 PKEYDEFINE wl_CreateKeyBinder( void )
{
	PKEYDEFINE KeyDef = NewArray( KEYDEFINE, 256 );
	MemSet( KeyDef, 0, sizeof( KEYDEFINE ) * 256 );
	return KeyDef;
}

void wl_DestroyKeyBinder ( PKEYDEFINE pKeyDef )
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
int wl_BindEventToKey( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv )
{
	PKEY_FUNCTION keyfunc = New( KEY_FUNCTION );
	MemSet( keyfunc, 0, sizeof( KEY_FUNCTION ) );
	if( !pKeyDefs ) pKeyDefs = KeyDefs;
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
		//data.extended_key_trigger = trigger;
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

//----------------------------------------------------------------------------

int wl_UnbindKey( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier )
{
	if( !pKeyDefs )
		pKeyDefs = KeyDefs;
	pKeyDefs[keycode].mod[modifier].flags.bFunction = FALSE;
   return TRUE;
}


void wl_SetRenderReadCallback ( PRENDERER pRenderer, RenderReadCallback callback, uintptr_t psv )
{
   if( pRenderer )
   {
      pRenderer->ReadComplete = callback;
      pRenderer->psvRead = psv;
   }
}

 int wl_HandleKeyEvents ( PKEYDEFINE pKeyDefs, uint32_t key )
{
	int keycode = KEY_CODE(key);
	int keymod = KEY_MOD(key);
	//l.flags.bLogKeyEvent = 1;
#ifdef LOG_KEY_EVENTS
   if( l.flags.bLogKeyEvent )
		lprintf( "received key %08x %d(%x) %d(%x) %s %s", key, keycode, keycode, keymod,keymod
			, IsKeyExtended( key )?"extended": "",IsKeyPressed( key )? "press": "release" );
#endif
#ifdef LOG_KEY_EVENTS
   if( l.flags.bLogKeyEvent )
		lprintf( "Key event for %08lx ... %d %s %s %s"
				 , key
				 , keycode
				 , keymod&1? "SHIFT": "", keymod&2? "CTRL": "", keymod&4? "ALT": "" );
#endif
	if( pKeyDefs[keycode].mod[keymod].flags.bFunction )
	{
#ifdef LOG_KEY_EVENTS
   if( l.flags.bLogKeyEvent )
		lprintf( "And there is a function... key is %s", IsKeyPressed( key )? "Pressed": "Released" );
#endif
		if( pKeyDefs[keycode].mod[keymod].flags.bAll ||
			( IsKeyPressed( key ) && !pKeyDefs[keycode].mod[keymod].flags.bRelease) ||
			( !IsKeyPressed( key ) && pKeyDefs[keycode].mod[keymod].flags.bRelease) )
		{
         //DebugBreak();
#ifdef LOG_KEY_EVENTS
			if( l.flags.bLogKeyEvent )
				lprintf( "Invoke!" );
#endif
			if( IsKeyExtended( key ) )
			{
				INDEX idx;
				PKEY_FUNCTION keyfunc;
#ifdef LOG_KEY_EVENTS
				if( l.flags.bLogKeyEvent )
					lprintf("extended key method configured" );
#endif
				LIST_FORALL( pKeyDefs[keycode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
				{
					if( keyfunc->data.extended_key_trigger )
					{
#ifdef LOG_KEY_EVENTS
						if( l.flags.bLogKeyEvent )
							lprintf("extended key method configured" );
#endif
						if( keyfunc->data.extended_key_trigger( keyfunc->extended_key_psv
																									  , key ) )
						{
							lprintf( "handled." );
							return 1;
						}
					}
				}
				lprintf( "not handled" );
				return 0;
			}
			else
			{
				INDEX idx;
				PKEY_FUNCTION keyfunc;
#ifdef LOG_KEY_EVENTS
				if( l.flags.bLogKeyEvent )
					lprintf("not extended key method" );
#endif
				LIST_FORALL( pKeyDefs[keycode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
				{
					if( keyfunc->data.trigger )
					{
#ifdef LOG_KEY_EVENTS
						if( l.flags.bLogKeyEvent )
							lprintf("key method configured %p", keyfunc->data.trigger );
#endif
						if( keyfunc->data.trigger( keyfunc->psv, key ) )
						{
							lprintf( "handled." );
							return 1;
						}
					}
				}
				lprintf( "not handled" );
				return 0;
			}
		}
		if( l.flags.bLogKeyEvent )
			lprintf( "Probably handled..." );
		lprintf( "not handled." );
		// for consistancy better just say we handled this key
		return 0;
	}
	if( l.flags.bLogKeyEvent )
		lprintf( "not handled..." );
	return 0;
}

