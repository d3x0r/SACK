#include <stdhdrs.h>
#include <sharemem.h>
#include <sack_types.h>

#ifdef DISPLAY_CLIENT
//#define PPANEL PRENDERER
#include "client.h"
#define TOGGLEKEY(display,code)    TOGGLEFLAG( (display)->KeyboardState, (code)*2 )
#define SETTOGGLEKEY(display,code)    SETFLAG( (display)->KeyboardState, (code)*2 )
#define CLEARTOGGLEKEY(display,code)    RESETFLAG( (display)->KeyboardState, (code)*2 )
#define SETKEY(display,code)       SETFLAG( (display)->KeyboardState, (code)*2+1 )
#define CLEARKEY(display,code)     RESETFLAG( (display)->KeyboardState, (code)*2+1 )
#define KEYPRESSED(display, code)  ((display)?TESTFLAG( (display)->KeyboardState, (code)*2+1 ):IsKeyPressed(code))
#define KEYDOWN(display, code)     (display)?TESTFLAG( (display)->KeyboardState, (code)*2 ):0 // toggled down...

#else
#include "global.h"
#endif
#include <keybrd.h>



#define KEY_ALPHA     0x0001
#define KEY_NUM       0x0002
// results in character data
#define KEY_CHARACTER 0x0100
// results in multiple characters (user programmed)
#define KEY_STRING    0x0200
// results in a callback function (user programmed)
#define KEY_FUNCTION  0x0400


typedef struct key_tag
{
	_32 flags;
	union {
		struct {
			unsigned char data[8];
		} character;
		struct {
			void (*Trigger)(PTRSZVAL psv, _32 keycode );
         PTRSZVAL psv;
		} event;
	} data;
} KEY, *PKEY;

#if ( defined( __LINUX__ ) || defined( GCC ) ) && !defined( __cplusplus )
static KEY keymap[256] =
{ [KEY_A] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'a', 'A',  1,  1 }}} }
, [KEY_B] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'b', 'B',  2,  2 }}} }
, [KEY_C] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'c', 'C',  3,  3 }}} }
, [KEY_D] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'd', 'D',  4,  4 }}} }
, [KEY_E] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'e', 'E',  5,  5 }}} }
, [KEY_F] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'f', 'F',  6,  6 }}} }
, [KEY_G] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'g', 'G',  7,  7 }}} }
, [KEY_H] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'h', 'H',  8,  8 }}} }
, [KEY_I] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'i', 'I',  9,  9 }}} }
, [KEY_J] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'j', 'J', 10, 10 }}} }
, [KEY_K] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'k', 'K', 11, 11 }}} }
, [KEY_L] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'l', 'L', 12, 12 }}} }
, [KEY_M] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'm', 'M', 13, 13 }}} }
, [KEY_N] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'n', 'N', 14, 14 }}} }
, [KEY_O] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'o', 'O', 15, 15 }}} }
, [KEY_P] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'p', 'P', 16, 16 }}} }
, [KEY_Q] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'q', 'Q', 17, 17 }}} }
, [KEY_R] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'r', 'R', 18, 18 }}} }
, [KEY_S] = { KEY_ALPHA|KEY_CHARACTER, {{{ 's', 'S', 19, 19 }}} }
, [KEY_T] = { KEY_ALPHA|KEY_CHARACTER, {{{ 't', 'T', 20, 20 }}} }
, [KEY_U] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'u', 'U', 21, 21 }}} }
, [KEY_V] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'v', 'V', 22, 22 }}} }
, [KEY_W] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'w', 'W', 23, 23 }}} }
, [KEY_X] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'x', 'X', 24, 24 }}} }
, [KEY_Y] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'y', 'Y', 25, 25 }}} }
, [KEY_Z] = { KEY_ALPHA|KEY_CHARACTER, {{{ 'z', 'Z', 26, 26 }}} }
, [KEY_1] = { KEY_CHARACTER, {{{ '1', '!', 0, 0 }}} }
, [KEY_2] = { KEY_CHARACTER, {{{ '2', '@', 0xFF, 0xFF }}} } // something different than NO data...
, [KEY_3] = { KEY_CHARACTER, {{{ '3', '#', 0, 0 }}} }
, [KEY_4] = { KEY_CHARACTER, {{{ '4', '$', 0, 0 }}} }
, [KEY_5] = { KEY_CHARACTER, {{{ '5', '%', 0, 0 }}} }
, [KEY_6] = { KEY_CHARACTER, {{{ '6', '^', 0, 0 }}} }
, [KEY_7] = { KEY_CHARACTER, {{{ '7', '&', 0, 0 }}} }
, [KEY_8] = { KEY_CHARACTER, {{{ '8', '*', 0, 0 }}} }
, [KEY_9] = { KEY_CHARACTER, {{{ '9', '(', 0, 0 }}} }
, [KEY_0] = { KEY_CHARACTER, {{{ '0', ')', 0, 0 }}} }
, [KEY_TAB] = { KEY_CHARACTER, {{{ '\t', 0, 0, 0 }}} }
, [KEY_BACKSPACE] = { KEY_CHARACTER, {{{ '\b', '\b' }}} }
, [KEY_COMMA] = { KEY_CHARACTER, {{{ ',', '<' }}} }
, [KEY_PERIOD] = { KEY_CHARACTER, {{{ '.', '>' }}} }
, [KEY_SLASH] = { KEY_CHARACTER, {{{ '/', '?' }}} }
, [KEY_SEMICOLON] = { KEY_CHARACTER, {{{ ';', ':' }}} }
, [KEY_QUOTE] = { KEY_CHARACTER, {{{ '\'', '\"' }}} }
, [KEY_LEFT_BRACKET] = { KEY_CHARACTER, {{{ '[', '{' }}} }
, [KEY_RIGHT_BRACKET] = { KEY_CHARACTER, {{{ ']', '}' }}} }
, [KEY_BACKSLASH] = { KEY_CHARACTER, {{{ '\\', '|' }}} }
, [KEY_DASH] = { KEY_CHARACTER, {{{ '-', '_' }}} }
, [KEY_EQUAL] = { KEY_CHARACTER, {{{ '=', '+' }}} }
, [KEY_ACCENT] = { KEY_CHARACTER, {{{ '`', '~' }}} }
, [KEY_SPACE] = { KEY_CHARACTER, {{{ ' ', ' ', ' ', ' ' }}} }
, [KEY_ESCAPE] = { KEY_CHARACTER, {{{ '\x1b','\x1b' }}}}
, [KEY_ENTER] = { KEY_CHARACTER, {{{ '\r','\r','\r','\r' }}}}

};
#else
static KEY keymap[256] =
{
{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{KEY_CHARACTER, {{{0x08,0x08 }}} }
,{KEY_CHARACTER, {{{0x09 }}} }
,{0} //10
,{0}
,{0}
,{KEY_CHARACTER, {{{0x0D,0x0D,0x0D,0x0D }}} }
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}   //20
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{KEY_CHARACTER, {{{0x1B,0x1B }}} }
,{0}
,{0}
,{0}    //30
,{0}
,{KEY_CHARACTER, {{{' ',' ',' ',' ' }}} }
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{KEY_CHARACTER, {{{'\'','\"' }}} }
,{0}    //40
,{0}
,{0}
,{0}
,{0}
,{KEY_CHARACTER, {{{'-','_' }}} }
,{0}
,{KEY_CHARACTER, {{{'/','?' }}} }
,{KEY_CHARACTER, {{{'0',')' }}} }
,{KEY_CHARACTER, {{{'1','!' }}} }
,{KEY_CHARACTER, {{{'2','@',0xFF,0xFF }}} } //50
,{KEY_CHARACTER, {{{'3','#' }}} }
,{KEY_CHARACTER, {{{'4','$' }}} }
,{KEY_CHARACTER, {{{'5','%' }}} }
,{KEY_CHARACTER, {{{'6','^' }}} }
,{KEY_CHARACTER, {{{'7','&' }}} }
,{KEY_CHARACTER, {{{'8','*' }}} }
,{KEY_CHARACTER, {{{'9','(' }}} }
,{0}
,{KEY_CHARACTER, {{{';',':' }}} }
,{0}                                      //60
,{KEY_CHARACTER, {{{'=','+' }}} }
,{0}
,{0}
,{0}
,{KEY_ALPHA|KEY_CHARACTER, {{{'a','A',0x01,0x01 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'b','B',0x02,0x02 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'c','C',0x03,0x03 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'d','D',0x04,0x04 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'e','E',0x05,0x05 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'f','F',0x06,0x06 }}} } //70
,{KEY_ALPHA|KEY_CHARACTER, {{{'g','G',0x07,0x07 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'h','H',0x08,0x08 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'i','I',0x09,0x09 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'j','J',0x0A,0x0A }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'k','K',0x0B,0x0B }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'l','L',0x0C,0x0C }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'m','M',0x0D,0x0D }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'n','N',0x0E,0x0E }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'o','O',0x0F,0x0F }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'p','P',0x10,0x10 }}} }  //80
,{KEY_ALPHA|KEY_CHARACTER, {{{'q','Q',0x11,0x11 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'r','R',0x12,0x12 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'s','S',0x13,0x13 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'t','T',0x14,0x14 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'u','U',0x15,0x15 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'v','V',0x16,0x16 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'w','W',0x17,0x17 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'x','X',0x18,0x18 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'y','Y',0x19,0x19 }}} }
,{KEY_ALPHA|KEY_CHARACTER, {{{'z','Z',0x1A,0x1A }}} }  //90
,{KEY_CHARACTER, {{{'[','{' }}} }
,{KEY_CHARACTER, {{{'\\','|' }}} }
,{KEY_CHARACTER, {{{']','}' }}} }
,{0}
,{0}
,{KEY_CHARACTER, {{{'`','~' }}} }
,{0}
,{0}
,{0}
,{0}                                                  //100             
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                 //110      
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                 //120      
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                 //130      
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                //140       
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                //150       
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                   
,{0}                                                  //160     
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                            
,{0}                                                   //170    
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                  //180      
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{KEY_CHARACTER, {{{';',':' }}} }//{0}
,{KEY_CHARACTER, {{{'=','+' }}} }
,{KEY_CHARACTER, {{{',','<' }}} }                                
,{KEY_CHARACTER, {{{'-','_' }}} }
,{KEY_CHARACTER, {{{'.','>' }}} }                   //190       
,{KEY_CHARACTER, {{{'/','?' }}} }
,{KEY_CHARACTER, {{{'`','~' }}} } //{0}
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                
,{0}                                                //200                 
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                 
,{0}                                               //210         
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{KEY_CHARACTER, {{{'[','{' }}} }
,{KEY_CHARACTER, {{{'\\','|' }}} }              //220
,{KEY_CHARACTER, {{{']','}' }}} }
,{KEY_CHARACTER, {{{'\'','\"' }}} }
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                           //230             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                          //240                                       
,{0}                                                             
,{0}                                                             
,{0}                                                             
,{0}                                                       
,{0}                                                             
,{0}                                                             
,{0}                                                                       
,{0}
,{0}
,{0}                                          //250
,{0}
,{0}
,{0}
,{0}
,{0}                                         //255
};
;
#endif


RENDER_NAMESPACE

RENDER_PROC( PKEYDEFINE, CreateKeyBinder )( void )
{
	PKEYDEFINE KeyDef = (PKEYDEFINE)Allocate( sizeof( KEYDEFINE ) * 256 );
	MemSet( KeyDef, 0, sizeof( KEYDEFINE ) * 256 );
   return KeyDef;
}

RENDER_PROC( void, DestroyKeyBinder )( PKEYDEFINE KeyDef )
{
   Release( KeyDef );
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
RENDER_PROC( int, BindEventToKeyEx )( PKEYDEFINE KeyDefs, _32 keycode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	if( modifier & KEY_MOD_RELEASE )
		KeyDefs[keycode].mod[modifier&0x7].flags.bRelease = TRUE;
	else
		KeyDefs[keycode].mod[modifier&0x7].flags.bRelease = FALSE;
	KeyDefs[keycode].mod[modifier&0x7].flags.bFunction = TRUE;
#if 0
	{
	KeyDefs[keycode].mod[modifier&0x7].data.trigger = trigger;
	KeyDefs[keycode].mod[modifier&0x7].psv = psv;
	}
#endif
	return TRUE;
}

RENDER_PROC( int, BindEventToKey )( PRENDERER pRenderer, _32 keycode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
   if( !g.KeyDefs )
		g.KeyDefs = CreateKeyBinder();
	return BindEventToKeyEx( pRenderer?pRenderer->KeyDefs:g.KeyDefs
								  , keycode, modifier
								  , trigger, psv );
}

RENDER_PROC( int, HandleKeyEvents )( PKEYDEFINE KeyDefs, _32 key )
{
#if 0
	int keycode = KEY_CODE(key);
	int keymod = KEY_MOD(key);
   //lprintf( WIDE("Got key %08") _32fx WIDE(""), key );
	if( KeyDefs[keycode].mod[keymod].flags.bFunction )
	{
      //lprintf( "a function of some sort..." );
		if( ( IsKeyPressed( key ) ) ||
			( !IsKeyPressed( key ) && KeyDefs[keycode].mod[keymod].flags.bRelease) )
		{
         //lprintf( "dispatch trigger..." );
			KeyDefs[keycode].mod[keymod].data.trigger( KeyDefs[keycode].mod[keymod].psv, key );
		}
		// for consistancy better just say we handled this key
		return 1;
	}
#endif
   return 0;
}

RENDER_PROC( int, UnbindKey )( PRENDERER pRenderer, _32 keycode, _32 modifier )
{
	return TRUE;
}

RENDER_PROC( char, GetKeyText)         ( int key )
{
#if 0
	INDEX keyindex = KEY_CODE(key);
	_32 keymod = KEY_MOD(key);
   // check for alpha-shift use, toggle shift appropriately.
	if( ( keymap[keyindex].flags & KEY_ALPHA ) &&
       ( key & KEY_ALPHA_LOCK_ON ) &&
	    !( keymod & (KEY_MOD_CTRL|KEY_MOD_ALT) ) )
		keymod ^= KEY_MOD_SHIFT;

   // check for num-shift use, toggle shift appropriately.
	if( ( keymap[keyindex].flags & KEY_NUM ) &&
       ( key & KEY_NUM_LOCK_ON ) &&
	    !( keymod & (KEY_MOD_CTRL|KEY_MOD_ALT) ) )
		keymod ^= KEY_MOD_SHIFT;
	//Log4( WIDE("Looking up key %d (%08lx %d %c)")
	//	 , keyindex
	//	 , keymap[keyindex].flags
	//	 , keymod
	//	 , keymap[keyindex].data.character.data[keymod] );
	if( keymap[keyindex].flags & KEY_CHARACTER )
	{
      return keymap[keyindex].data.character.data[keymod];
	}
#endif
   return 0;
}

RENDER_PROC( _32, IsKeyDown )( PRENDERER display, int key )
{
	//Log3( WIDE("Testing display %p for key %d (%d)"), display, key, KEYPRESSED(display,key ) );
#if 0
   if( display )
		return KEYPRESSED( display, key );
#endif
   return 0;
}

RENDER_PROC( _32, KeyDown )( PRENDERER display, int key )
{
	if( !display )
		return 0;
#if 0
	// down and was down - stay down...
	if( KEYPRESSED( display, key ) && TESTFLAG( display->KeyboardMetaState,key) )
	{
      return FALSE;
	}
	else
	{
		if( KEYPRESSED( display, key ) )
		{
			SETFLAG( display->KeyboardMetaState, key );
		}
		else
		{
         RESETFLAG(  display->KeyboardMetaState, key );
		}
      return TESTFLAG(  display->KeyboardMetaState, key );
	}
#endif
   return 0;
}

RENDER_NAMESPACE_END

#ifdef UPDATE_TABLE_FROM_GCC_TABLE
void DumpTable( void )
{

	int firstkey = 1;
	int nkey;
   for( nkey = 0; nkey < 256; nkey++ )
	{
		int firstflag, firstchar;
		firstflag = 1;
		firstchar = 1;

		if( !firstkey )
			printf( WIDE(",") );
      firstkey = 0;
		printf( WIDE("{") );
		if( keymap[nkey].flags & KEY_ALPHA )
		{
			if( !firstflag )
				printf( WIDE("|") );
			firstflag = 0;
			printf( WIDE("KEY_ALPHA") );
		}
		if( keymap[nkey].flags & KEY_CHARACTER )
		{
			if( !firstflag )
				printf( WIDE("|") );
			firstflag = 0;
			printf( WIDE("KEY_CHARACTER") );
		}
		if( firstflag )
		{
			printf( WIDE("0") );
         firstflag = 0;
		}
		{
			int n;
			for( n = 0; n < 8; n++ )
			{
				if( keymap[nkey].data.character.data[n] )
				{
					if( !firstchar )
						printf("," );
					else
						printf( WIDE(", {{{") );
					firstchar = 0;

					if( keymap[nkey].data.character.data[n] < 32 || keymap[nkey].data.character.data[n] > 127)
					{
						printf( WIDE("0x%02X"), keymap[nkey].data.character.data[n] );
					}
					else
					{
						switch( keymap[nkey].data.character.data[n] )
						{
						case '\\':
						case '\'':
						case '\"':
							printf( WIDE("\'\\%c\'"), keymap[nkey].data.character.data[n] );
							break;
						default:
							printf( WIDE("\'%c\'"), keymap[nkey].data.character.data[n] );
							break;
						}
					}
				}
			}
		}
      if( !firstchar )
			printf( WIDE(" }}} ") );
		printf( WIDE("}\n") );
	}
	printf( WIDE("};") );
}

int main( void )
{
   DumpTable();
}
#endif
