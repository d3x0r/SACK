#ifndef _OPENGL_DRIVER
#define _OPENGL_DRIVER
#endif
#define _INCLUDE_CLIPBOARD
#define KEYS_DEFINED
#include <stdhdrs.h>
#include <sack_types.h>
#include <sharemem.h>
#include <keybrd.h>
#define _OPENGL_ENABLED

#include <vidlib/vidstruc.h>

//#define LOG_KEY_EVENTS


RENDER_NAMESPACE

extern int myTypeID;

enum{
   KS_DELETE
};

#ifdef __cplusplus
#else
//TEXT DELETE_STROKE = DEFTEXT( "\x7f" );
#endif

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

RENDER_PROC( PKEYDEFINE, CreateKeyBinder )( void )
{
	PKEYDEFINE KeyDef = (PKEYDEFINE)Allocate( sizeof( KEYDEFINE ) * 256 );
	MemSet( KeyDef, 0, sizeof( KEYDEFINE ) * 256 );
   return KeyDef;
}

RENDER_PROC( void, DestroyKeyBinder )( PKEYDEFINE pKeyDef )
{
   Release( pKeyDef );
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
RENDER_PROC( int, BindEventToKeyEx )( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv )
{
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
		pKeyDefs[keycode].mod[modifier&0x7].data.extended_key_trigger = trigger;
		pKeyDefs[keycode].mod[modifier&0x7].extended_key_psv = psv;
	}
	else
	{
		pKeyDefs[keycode].mod[modifier&0x7].data.trigger = trigger;
		pKeyDefs[keycode].mod[modifier&0x7].psv = psv;
	}
	return TRUE;
}

RENDER_PROC( int, BindEventToKey )( PRENDERER pRenderer, uint32_t keycode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv )
{
	return BindEventToKeyEx( pRenderer?pRenderer->KeyDefs:KeyDefs
								  , keycode, modifier
								  , trigger, psv );
}
//----------------------------------------------------------------------------

RENDER_PROC( int, UnbindKey )( PRENDERER pRenderer, uint32_t keycode, uint32_t modifier )
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


RENDER_PROC( void, SetRenderReadCallback )( PRENDERER pRenderer, RenderReadCallback callback, uintptr_t psv )
{
   if( pRenderer )
   {
      pRenderer->ReadComplete = callback;
      pRenderer->psvRead = psv;
   }
}

RENDER_PROC( int, HandleKeyEvents )( PKEYDEFINE pKeyDefs, uint32_t key )
{
	int keycode = KEY_CODE(key);
	int keymod = KEY_MOD(key);
#ifdef LOG_KEY_EVENTS

	lprintf( "Key event for %08lx ... %d %s %s %s", key, keycode, keymod&1?"SHIFT":"", keymod&2?"CTRL":"", keymod&4?"ALT":"" );
#endif
	if( pKeyDefs[keycode].mod[keymod].flags.bFunction )
	{
#ifdef LOG_KEY_EVENTS
		lprintf( "And there is a function... key is %s", IsKeyPressed( key )?"Pressed":"Released" );
#endif
		if( pKeyDefs[keycode].mod[keymod].flags.bAll ||
			( IsKeyPressed( key ) && !pKeyDefs[keycode].mod[keymod].flags.bRelease) ||
			( !IsKeyPressed( key ) && pKeyDefs[keycode].mod[keymod].flags.bRelease) )
		{
         //DebugBreak();
#ifdef LOG_KEY_EVENTS
			lprintf( "Invoke!" );
#endif
			if( IsKeyExtended( key ) )
			{
				if( pKeyDefs[keycode].mod[keymod].data.extended_key_trigger )
				{
					pKeyDefs[keycode].mod[keymod].data.extended_key_trigger( pKeyDefs[keycode].mod[keymod].extended_key_psv
																							, key );
				}
				else
               return 0;
			}
			else
			{
				if( pKeyDefs[keycode].mod[keymod].data.trigger )
				{
					pKeyDefs[keycode].mod[keymod].data.trigger( pKeyDefs[keycode].mod[keymod].psv, key );
				}
				else
               return 0;
			}
		}
		// for consistancy better just say we handled this key
		return 1;
	}
   return 0;
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
