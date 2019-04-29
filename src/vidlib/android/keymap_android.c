
#include <stdhdrs.h>
#include <idle.h>
#include <android/keycodes.h>
#include "Android_local.h"

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



PSIKEYDEFINE AndroidKeyDefs[256] =
                     { [0]={"wide key","wide key",0,KEYDATA("","") }
                      ,[AKEYCODE_DEL]={"back","backspace",0,KEYDATA("\b","\b") }
                      , [KEY_TAB]={"tab",0,0,KEYDATA("\t","\t") }
                      , [AKEYCODE_ENTER]={"return", "enter",0,KEYDATA8("\n") }
                      //, [AKEYCODE_PAUSE]={"pause",0,KDF_NODEFINE }
                      , [AKEYCODE_ESCAPE]={"esc", "escape", 0, {{KEYDATA}}} // 0x1b
                      , [AKEYCODE_SPACE]={"space", "blank", 0, KEYDATA8(" ") } //0x20
                      , [AKEYCODE_PAGE_UP]={"prior", "pgup", 0 }
                      , [AKEYCODE_PAGE_DOWN]={"next", "pgdn", 0 }
                      //, [AKEYCODE_END]={"end", 0, 0, {{COMMANDKEY, (PTEXT)KeyEndCmd}}}
							, [AKEYCODE_MOVE_HOME]={"home", 0, 0 }
							, [AKEYCODE_MOVE_END]={"end", 0, 0 }

                      , [AKEYCODE_DPAD_LEFT]={"left", 0, 0 }

                      , [AKEYCODE_DPAD_UP]={"up" , 0, 0 }
                      , [AKEYCODE_DPAD_RIGHT]={"right", 0, 0 }
                      , [AKEYCODE_DPAD_DOWN]={"down", 0, 0 }
                      , [AKEYCODE_DPAD_CENTER]={"center", 0, 0 }
                      //, {"select"}
                      //, [AKEYCODE_PRINT]={"print"}
                      //, {"execute"}
                      //, {"snapshot"}
                      , [AKEYCODE_0]={"0", 0, 0, KEYDATA("0",")")}//0x30
                      , [AKEYCODE_1]={"1", 0, 0, KEYDATA("1","!")}
							, [AKEYCODE_2]={"2", 0, 0, KEYDATA("2","@")}
							, [AKEYCODE_3]={"3", 0, 0, KEYDATA("3","#")}
                      , [AKEYCODE_4]={"4", 0, 0, KEYDATA("4","$")}
                      , [AKEYCODE_5]={"5", 0, 0, KEYDATA("5","%")}
                      , [AKEYCODE_6]={"6", 0, 0, KEYDATA("6","^")}
                      , [AKEYCODE_7]={"7", 0, 0, KEYDATA("7","&")}
                      , [AKEYCODE_8]={"8", 0, 0, KEYDATA("8","*")}
                      , [AKEYCODE_9]={"9", 0, 0, KEYDATA("9","(")}

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
                      //..... to hmm 39
                      , [AKEYCODE_A] = {"a",0,KDF_NODEFINE,KEYDATA("a","A")}// 0x41 'A'
                      , [AKEYCODE_B] = {"b",0,KDF_NODEFINE,KEYDATA("b","B")}
                      , [AKEYCODE_C] = {"c",0,KDF_NODEFINE,KEYDATA("c","C")}
                      , [AKEYCODE_D] = {"d",0,KDF_NODEFINE,KEYDATA("d","D")}
                      , [AKEYCODE_E] = {"e",0,KDF_NODEFINE,KEYDATA("e","E")}
                      , [AKEYCODE_F] = {"f",0,KDF_NODEFINE,KEYDATA("f","F")}
                      , [AKEYCODE_G] = {"g",0,KDF_NODEFINE,KEYDATA("g","G")}
                      , [AKEYCODE_H] = {"h",0,KDF_NODEFINE,KEYDATA("h","H")}
                      , [AKEYCODE_I] = {"i",0,KDF_NODEFINE,KEYDATA("i","I")}
                      , [AKEYCODE_J] = {"j",0,KDF_NODEFINE,KEYDATA("j","J")}
                      , [AKEYCODE_K] = {"k",0,KDF_NODEFINE,KEYDATA("k","K")}
                      , [AKEYCODE_L] = {"l",0,KDF_NODEFINE,KEYDATA("l","L")}
                      , [AKEYCODE_M] = {"m",0,KDF_NODEFINE,KEYDATA("m","M")}
                      , [AKEYCODE_N] = {"n",0,KDF_NODEFINE,KEYDATA("n","N")}
                      , [AKEYCODE_O] = {"o",0,KDF_NODEFINE,KEYDATA("o","O")}
                      , [AKEYCODE_P] = {"p",0,KDF_NODEFINE,KEYDATA("p","P")}
                      , [AKEYCODE_Q] = {"q",0,KDF_NODEFINE,KEYDATA("q","Q")}
                      , [AKEYCODE_R] = {"r",0,KDF_NODEFINE,KEYDATA("r","R")}
                      , [AKEYCODE_S] = {"s",0,KDF_NODEFINE,KEYDATA("s","S")}
                      , [AKEYCODE_T] = {"t",0,KDF_NODEFINE,KEYDATA("t","T")}
                      , [AKEYCODE_U] = {"u",0,KDF_NODEFINE,KEYDATA("u","U")}
                      , [AKEYCODE_V] = {"v",0,KDF_NODEFINE,KEYDATA("v","V")}
                      , [AKEYCODE_W] = {"w",0,KDF_NODEFINE,KEYDATA("w","W")}
                      , [AKEYCODE_X] = {"x",0,KDF_NODEFINE,KEYDATA("x","X")}
                      , [AKEYCODE_Y] = {"y",0,KDF_NODEFINE,KEYDATA("y","Y")}
                      , [AKEYCODE_Z] = {"z",0,KDF_NODEFINE,KEYDATA("z","Z")}

                      //, {"numlock",0,KDF_NODEFINE} // 0x90
                      //, {"scroll",0,KDF_NODEFINE}

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
                      , [AKEYCODE_SHIFT_LEFT] = {"lshift",0,KDF_NODEFINE,KEYSHIFT}  // 0xa0
                      , [AKEYCODE_SHIFT_RIGHT] ={"rshift",0,KDF_NODEFINE,KEYSHIFT}
                      //, NONAMES // {"lctrl", "lcontrol",0,KDF_NODEFINE}
                      //, NONAMES // {"rctrl", "rcontrol",0,KDF_NODEFINE}
                      //, NONAMES // {"lmenu", "lalt",0,KDF_NODEFINE }
                      //, NONAMES // {"rmenu", "ralt",0,KDF_NODEFINE }
							, [AKEYCODE_SEMICOLON]={";", "semicolon",0,KEYDATA(";",":")}
                      , [AKEYCODE_EQUALS]={"=", "equal",0    ,KEYDATA("=","+")}
                      , [AKEYCODE_COMMA]={",", "comma",0    ,KEYDATA(",","<")}
                      , [AKEYCODE_MINUS]={"-", "dash",0     ,KEYDATA("-","_")}
                      , [AKEYCODE_PERIOD]={".", "period",0   ,KEYDATA(".",">")}
                      , [AKEYCODE_SLASH]={"/","slash",0     ,KEYDATA("/","?")}
                      , [AKEYCODE_GRAVE]={"`", "accent",0   ,KEYDATA("`","~")}
							, [AKEYCODE_LEFT_BRACKET]={ "[", "lbracket",0  ,KEYDATA("[","{")}
							, [AKEYCODE_BACKSLASH]={ "\\", "backslash",0,KEYDATA("\\","|")}
							, [AKEYCODE_RIGHT_BRACKET]={ "]", "rbracket",0  ,KEYDATA("]","}")}
                      , [AKEYCODE_APOSTROPHE]={ "'", "apostrophe",0     ,KEYDATA("`","~")}

                     // I get AKEYCODE_SHIFT_LEFT (mod 65), AKEYCODE_APOSTROPHE( 65) (up?), left_hisft(up) mod (0)
                      //, [AKEYCODE_APOSTROPHE]={ "'", "quote",0     ,{{KEYDATA}
                      //                        ,{KEYDATA}} }
};


#undef KEYSHIFT

static struct keymap_state
{
	struct {
		BIT_FIELD bShifted : 1;
		BIT_FIELD bShowing : 1;
	} flags;
	void (*show_keyboard)(void);
	void (*hide_keyboard)(void);
	int (*get_status_metric)(void);
	int (*get_keyboard_metric)(void);
   char *(*get_key_text)( void);
	CTEXTSTR current_key_text;
} keymap_local;

//----------------------------------------------------------------------------

const TEXTCHAR * AndroidANW_GetKeyText(int key)
{
   if( keymap_local.current_key_text )
		return keymap_local.current_key_text;
   return 0;
}

// This is a callback from the native hook code....
int SACK_Vidlib_SendKeyEvents( int pressed, int key_index, int key_mods )
{
   int used = 0;
	int bOutput = 0;
	// check current keyboard override...
	int mod = keymap_local.flags.bShifted?1:0;
	{
		int result = 0;
		Log1( "Keyfunc = %d", AndroidKeyDefs[key_index].op[mod].bFunction );
		switch( AndroidKeyDefs[key_index].op[mod].bFunction )
		{
#undef KEYSHIFT
		case KEYSHIFT:
         if( pressed )
				keymap_local.flags.bShifted = 1;
         else
				keymap_local.flags.bShifted = 0;
			used = 1;
         bOutput = 0;
         break;
		case KEYDATA:
			if( pressed )
			{
				//keymap_local.current_key_text = AndroidKeyDefs[key_index].op[mod].pStroke;
				keymap_local.current_key_text = keymap_local.get_key_text();
				lprintf( "key text becomes :%s", keymap_local.current_key_text );
			}
			used = 1;
			bOutput = 1;
         break;
		}
	}
   if( bOutput )
	{
		if( l.hVidVirtualFocused )
		{
			if( l.hVidVirtualFocused->key_callback )
			{
				uint32_t normal_key = (pressed?KEY_PRESSED:0)
					| ( key_mods & 7 ) << 28
					| ( key_index & 0xFF ) << 16
					| ( key_index )
					;
            lprintf( "send app key %08x", normal_key );
				used |= l.hVidVirtualFocused->key_callback( l.hVidVirtualFocused->psv_key_callback, normal_key );
			}
		}
	}
   return used;
}


void SACK_Vidlib_SetTriggerKeyboard( void (*show)(void), void(*hide)(void)
											  , int(*get_status_metric)(void)
											  , int(*get_keyboard_metric)(void)
											  , char *(*get_key_text)( void)
                                    , int(*process_events)(uintptr_t psv)
											  )
{
	keymap_local.show_keyboard = show;
	keymap_local.hide_keyboard = hide;
   keymap_local.get_status_metric = get_status_metric;
	keymap_local.get_keyboard_metric = get_keyboard_metric;
	keymap_local.get_key_text = get_key_text;
	//keymap_local.process_events = process_events;
   AddIdleProc( process_events, 1 );
}

int SACK_Vidlib_GetStatusMetric( void )
{
	if(  keymap_local.get_status_metric )
		return keymap_local.get_status_metric();
	return 50;
}


/*
 char * SACK_Vidlib_NewGetKeyText( void )
{
	if(  keymap_local.get_key_text )
		return keymap_local.get_key_text( );
	return ;
}
*/

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
