
#include <stdhdrs.h>
//#include <android/keycodes.h>

#include "local.h"

#include <psi/console.h>
// androi
// requires C99 to compile at this point!
RENDER_NAMESPACE


enum {
    KEY_FUNCTION_NOT_DEFINED = 0
	  , KEYDATA
     , KEYSHIFT
 , KEYDATA_DEFINED // do this stroke?  
 , COMMANDKEY
 , HISTORYKEY
 , CONTROLKEY
 , SPECIALKEY
};  

typedef struct AndroidKeymapKeyDefine {
   CTEXTSTR name1;
   CTEXTSTR name2;
   int flags;
   struct {
      int bFunction;  // true if pStroke is pKeyFunc()
		CTEXTSTR pStroke; // this may be pKeyFunc()
   } op[8];
} PSIKEYDEFINE, *PPSIKEYDEFINE;

#define KEYDATA(a,b) {{ KEYDATA,  a}, {KEYDATA,b }}
#define KEYDATA8(a) {{ KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a}, { KEYDATA, a} }
#define KEYSHIFT  { { KEYSHIFT },{KEYSHIFT},{KEYSHIFT},{KEYSHIFT},{KEYSHIFT},{KEYSHIFT},{KEYSHIFT},{KEYSHIFT}}

//typedef int (CPROC *KeyFunc)( PCONSOLE_INFO pdp );
//typedef int (CPROC *KeyFuncUpDown)( PCONSOLE_INFO pdp, int bDown );

#define KDF_NODEFINE 0x01 // set to disallow key redefinition
//#define KDF_NUMKEY   0x04 // Key is sensitive to numlock state
#define KDF_NOKEY    0x08 // no keydef here...
#define KDF_NOREDEF  0x10
#define KDF_UPACTION 0x20 // action is called on key release

#define KEYMOD_NORMAL 0
#define KEYMOD_SHIFT  KEY_MOD_SHIFT
#define KEYMOD_CTRL   KEY_MOD_CTRL
#define KEYMOD_ALT    KEY_MOD_ALT

static struct keymap_state
{
	struct {
		BIT_FIELD bShifted : 1;
		BIT_FIELD bShowing : 1;
	} flags;
	void (*show_keyboard)(void);
   void (*hide_keyboard)(void);
} keymap_local;

//----------------------------------------------------------------------------

const TEXTCHAR * GetKeyText (int key)
{
	if( l.current_key_text )
		return l.current_key_text;
	return 0;
}


int SACK_Vidlib_SendKeyEvents( int pressed, int key_index, int key_mods )
{
	int used = 0;
	// check current keyboard override...
	int mod = keymap_local.flags.bShifted?1:0;
	{
		int result = 0;
	}
	{
		if( l.hVidVirtualFocused )
		{
			if( l.hVidVirtualFocused->pKeyProc )
			{
				uint32_t normal_key = (pressed?KEY_PRESSED:0)
					| ( key_mods & 7 ) << 28
					| ( key_index & 0xFF ) << 16
					| ( key_index )
					;
				used |= l.hVidVirtualFocused->pKeyProc( l.hVidVirtualFocused->dwKeyData, normal_key );
			}
		}
	}
	return used;
}


void SACK_Vidlib_SetTriggerKeyboard( void (*show)(void), void(*hide)(void))
{
	keymap_local.show_keyboard = show;
	keymap_local.hide_keyboard = hide;
}

void SACK_Vidlib_ShowInputDevice( void )
{
   keymap_local.flags.bShowing = 1;
	if( keymap_local.show_keyboard )
      keymap_local.show_keyboard();
}

void SACK_Vidlib_HideInputDevice( void )
{
   keymap_local.flags.bShowing = 0;
	if( keymap_local.hide_keyboard )
      keymap_local.hide_keyboard();
}

void SACK_Vidlib_ToggleInputDevice( void )
{
   keymap_local.flags.bShowing = !keymap_local.flags.bShowing;
	if( keymap_local.flags.bShowing )
	{
		if( keymap_local.show_keyboard )
			keymap_local.show_keyboard();
	}
   else
	{
		if( keymap_local.hide_keyboard )
			keymap_local.hide_keyboard();
	}
}

RENDER_NAMESPACE_END
