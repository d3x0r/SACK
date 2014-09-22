#ifndef __KEYS_DEFINED__
#define __KEYS_DEFINED__

// included by consolestruc.h - please use this if you want to include ME

// return values from Func( PUSER_INPUT_BUFFER )
enum {
    UPDATE_NOTHING
 , UPDATE_COMMAND
 , UPDATE_DISPLAY // requires calculate, render display, render history
 , UPDATE_HISTORY
 //, SET_END_HISTORY // special return value - implies UPDATE_DISPLAY
 //, SET_END_COMMAND // special return value - implies UPDATE_DISPLAY
};
// common keys return TRUE if they did an actino which resulted in 
// 0 - no update
// 1 - command line update (cursor or data)
// 2 - display update  (when history starts...)
// 3 - history update only...

CORECON_PROC( int, KeyShift     )( P_32 pKeyState, LOGICAL bState );
CORECON_PROC( int, KeyControl   )( P_32 pKeyState, LOGICAL bState );
CORECON_PROC( int, KeyAlt       )( P_32 pKeyState, LOGICAL bState );

CORECON_PROC( int, KeyLeft )( PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, KeyRight )( PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, KeyInsert )( PUSER_INPUT_BUFFER pci );
//int KeyDelete( PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, CommandKeyUp )( PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, HandleKeyDown )(  PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, KeyHome )( PUSER_INPUT_BUFFER pci );
CORECON_PROC( int, KeyEndCmd )( PUSER_INPUT_BUFFER pci );

CORECON_PROC( int, KeyEndHst )( PHISTORY_BROWSER pht );
CORECON_PROC( int, HistoryPageUp )( PHISTORY_BROWSER pht );
CORECON_PROC( int, HistoryPageDown )( PHISTORY_BROWSER pht );
CORECON_PROC( int, HistoryLineUp )( PHISTORY_BROWSER pht );
CORECON_PROC( int, HistoryLineDown )( PHISTORY_BROWSER pht );
//CPROC KeystrokePaste( PCONSOLE_INFO pht );

enum {
    KEY_FUNCTION_NOT_DEFINED = 0
    , KEYDATA
 , KEYDATA_DEFINED // do this stroke?  
 , COMMANDKEY
 , HISTORYKEY
 , CONTROLKEY
 , SPECIALKEY
};  

typedef struct KeyDefine {
   CTEXTSTR name1;
   CTEXTSTR name2;
   int flags;
   struct {
      int bFunction;  // true if pStroke is pKeyFunc()
      union {
          PTEXT pStroke; // this may be pKeyFunc()
          void (CPROC *ControlKey)( P_32 pKeyState, LOGICAL bState );
          int (CPROC *CommandKey)( PUSER_INPUT_BUFFER pci );
          int (CPROC *HistoryKey)( PHISTORY_BROWSER pct );
          int (CPROC *SpecialKey)( PCONSOLE_INFO pdp ); // PASTE
       } data;
   } op[8];
} PSIKEYDEFINE, *PPSIKEYDEFINE;

typedef int (CPROC *KeyFunc)( PCONSOLE_INFO pdp );
typedef int (CPROC *KeyFuncUpDown)( PCONSOLE_INFO pdp, int bDown );

//#define NONAMES { 0, 0, KDF_NOKEY }
//#define NUM_KEYS ( sizeof( KeyDefs ) / (sizeof(KEYDEFINE)) )
#define KDF_NODEFINE 0x01 // set to disallow key redefinition
#define KDF_NOREDEF  0x10
#define KDF_CAPSKEY  0x02 // key is sensitive to capslock state
#define KDF_NUMKEY   0x04 // key is sensitive to numlock state
// the numpad returns num0 - ...
// or numpad returns home/end/pgdn etc...

//#define KDF_NUMKEY   0x04 // Key is sensitive to numlock state
#define KDF_NOKEY    0x08 // no keydef here...
#define KDF_NOREDEF  0x10
#define KDF_UPACTION 0x20 // action is called on key release

#ifndef KEYS_DEFINED
//CORECON_EXPORT( TEXT, KeyStroke[] );
#ifndef CORECON_SOURCE
CORECON_EXPORT( PSIKEYDEFINE, KeyDefs[] );
#endif
#endif

#define KEYMOD_NORMAL 0
#define KEYMOD_SHIFT  KEY_MOD_SHIFT
#define KEYMOD_CTRL   KEY_MOD_CTRL
#define KEYMOD_ALT    KEY_MOD_ALT
#ifdef __DEKWARE_PLUGIN__
CORECON_PROC( int, KeyBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
CORECON_PROC( int, KeyUnBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
#endif
CORECON_PROC(int, PSI_DoStroke )( PCONSOLE_INFO pdp, PTEXT stroke );
void PSI_KeyPressHandler( PCONSOLE_INFO pdp
												 , _8 key_index
												 , _8 mod
												 , PTEXT characters
												 );

#endif
// $Log: keydefs.h,v $
// Revision 1.20  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.19  2005/01/26 20:00:01  d3x0r
// Okay - need to do something about partial updates - such as command typing should only update that affected area of the screen...
//
// Revision 1.18  2005/01/23 04:07:57  d3x0r
// Hmm somehow between display rendering stopped working.
//
// Revision 1.17  2005/01/20 06:10:19  d3x0r
// One down, 3 to convert... concore library should serve to encapsulate drawing logic and history code...
//
// Revision 1.16  2004/09/29 09:31:32  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.15  2004/09/09 13:41:04  d3x0r
// works much better passing correct structures...
//
// Revision 1.14  2004/08/13 09:29:50  d3x0r
// checkpoint
//
// Revision 1.13  2004/07/30 14:08:40  d3x0r
// More tinkering...
//
// Revision 1.12  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
// Revision 1.11  2004/06/10 10:01:45  d3x0r
// Okay so cursors have input and output characteristics... history and display are run by seperate cursors.
//
// Revision 1.10  2004/06/08 00:23:26  d3x0r
// Display and history combing proceeding...
//
// Revision 1.9  2004/05/12 10:05:20  d3x0r
// checkpoint
//
// Revision 1.8  2004/03/08 09:25:42  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.7  2004/01/21 06:45:23  d3x0r
// Compiles okay - windows.  Test point
//
// Revision 1.6  2004/01/19 23:42:26  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.7  2004/01/19 00:02:06  panther
// Cursecon compat edits
//
// Revision 1.6  2004/01/18 22:02:31  panther
// Merge wincon/psicon/cursecon
//
// Revision 1.5  2003/03/26 02:05:21  panther
// begin updating option handlers to real option handlers
//
// Revision 1.4  2003/03/25 08:59:02  panther
// Added CVS logging
//
