#define CORECON_SOURCE
#define NO_LOGGING
#define KEYS_DEFINED
#include <stdhdrs.h>
#include <keybrd.h>
// included to have pending struct available to pass to
// command line update....
//#include "WinLogic.h"

#include "chat_control_internal.h"


extern int myTypeID;

enum{
   KS_DELETE
};

static void CPROC _Key_KeystrokePaste( PCONSOLE_INFO pmdp )
{
	if( pmdp->KeystrokePaste )
      pmdp->KeystrokePaste( pmdp );
}

DECLTEXT( KeyStroke, WIDE("\x7f") ); // DECLTEXT implies 'static'

#if defined( GCC ) || defined( __LINUX__ )
CORECON_EXPORT( PSIKEYDEFINE, ConsoleKeyDefs[256] ) =
#ifdef __cplusplus
{{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {WIDE("esc"),WIDE("escape"),0|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("1"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("2"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("3"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("4"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("5"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("6"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("7"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("8"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("9"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("0"),0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("-"),WIDE("dash"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("="),WIDE("equal"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("back"),WIDE("backspace"),0|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("tab"),0,0|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("q"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("w"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("e"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("r"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("t"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("y"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("u"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("i"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("o"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("p"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("["),WIDE("lbracket"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("]"),WIDE("rbracket"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
,{0}
, {WIDE("a"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("s"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("d"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("f"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("g"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("h"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("j"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("k"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("l"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE(";"),WIDE("semicolon"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("'"),WIDE("quote"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("`"),WIDE("accent"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
, {WIDE("\\"),WIDE("backslash"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("z"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("x"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("c"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("v"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{}}}
, {WIDE("b"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("n"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE("m"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {WIDE(","),WIDE("comma"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("."),WIDE("period"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {WIDE("/"),WIDE("slash"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
, {WIDE("mult"),WIDE("mulitply"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
, {WIDE("space"),WIDE("blank"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA}}}
,{0}
, {WIDE("f1"),0,0|0|0|0|0|0,{}}
, {WIDE("f2"),0,0|0|0|0|0|0,{}}
, {WIDE("f3"),0,0|0|0|0|0|0,{}}
, {WIDE("f4"),0,0|0|0|0|0|0,{}}
, {WIDE("f5"),0,0|0|0|0|0|0,{}}
, {WIDE("f6"),0,0|0|0|0|0|0,{}}
, {WIDE("f7"),0,0|0|0|0|0|0,{}}
, {WIDE("f8"),0,0|0|0|0|0|0,{}}
, {WIDE("f9"),0,0|0|0|0|0|0,{}}
, {WIDE("f10"),0,0|0|0|0|0|0,{}}
,{0}
,{0}
, {WIDE("num7"),WIDE("pad7"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num8"),WIDE("pad8"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num9"),WIDE("pad9"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("subtract"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num4"),WIDE("pad4"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num5"),WIDE("pad5"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num6"),WIDE("pad6"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("add"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("separator"),0,KDF_NODEFINE|0|0|0|0|0,{}}
, {WIDE("num2"),WIDE("pad2"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num3"),WIDE("pad3"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("num0"),WIDE("pad0"),KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {WIDE("decimal"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
,{0}
,{0}
, {WIDE("f11"),0,0|0|0|0|0|0,{}}
, {WIDE("f12"),0,0|0|0|0|0|0,{}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {WIDE("insert"),0,0|0|0|0|0|0,{{}}}
, {WIDE("delete"),0,0|0|0|0|0|0,{{}}}
, {WIDE("help"),0,0|0|0|0|0|0,{}}
,{0}
,{0}
,{0}
, {WIDE("divide"),0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {WIDE("return"),WIDE("enter"),0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA}}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {WIDE("next"),WIDE("pgdn"),0|0|0|0|0|0,{{}}}
, {WIDE("down"),0,0|0|0|0|0|0,{{},{}}}
, {WIDE("end"),0,0|0|0|0|0|0,{{}}}
, {WIDE("right"),0,0|0|0|0|0|0,{{}}}
,{0}
, {WIDE("left"),0,0|0|0|0|0|0,{{}}}
, {WIDE("prior"),WIDE("pgup"),0|0|0|0|0|0,{{}}}
, {WIDE("up"),0,0|0|0|0|0|0,{{},{}}}
, {WIDE("home"),0,0|0|0|0|0|0,{{}}}
,{0}
,{0}
,{0}};
#else
{ [KEY_BACKSPACE]={WIDE("back"),WIDE("backspace"),0,{{KEYDATA}} }
                      , [KEY_TAB]={WIDE("tab"),0,0,{{KEYDATA}} }
                      , [KEY_ENTER]={WIDE("return"), WIDE("enter"),0,{{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}} }
                      //, [KEY_PAUSE]={WIDE("pause"),0,KDF_NODEFINE }
                      , [KEY_ESCAPE]={WIDE("esc"), WIDE("escape"), 0, {{KEYDATA}}} // 0x1b
                      , [KEY_SPACE]={WIDE("space"), WIDE("blank"), 0, {{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}} } //0x20
                      , [KEY_PGUP]={WIDE("prior"), WIDE("pgup"), 0, {{HISTORYKEY,(PTEXT)HistoryPageUp}} }
                      , [KEY_PGDN]={WIDE("next"), WIDE("pgdn"), 0, {{HISTORYKEY,(PTEXT)HistoryPageDown}} }
                      , [KEY_END]={WIDE("end"), 0, 0, {{COMMANDKEY, (PTEXT)KeyEndCmd}}}
                      , [KEY_HOME]={WIDE("home"), 0, 0, {{COMMANDKEY, (PTEXT)KeyHome}}}
                      , [KEY_LEFT]={WIDE("left"), 0, 0, {{COMMANDKEY, (PTEXT)KeyLeft}}}

                      , [KEY_UP]={"up" , 0, 0, { {COMMANDKEY, (PTEXT)CommandKeyUp}
                                       , {HISTORYKEY, (PTEXT)HistoryLineUp}}}
                      , [KEY_RIGHT]={WIDE("right"), 0, 0, {{COMMANDKEY, (PTEXT)KeyRight}}}
                      , [KEY_DOWN]={WIDE("down"), 0, 0, {{COMMANDKEY, (PTEXT)HandleKeyDown}
                                       , {HISTORYKEY, (PTEXT)HistoryLineDown}}}
                      //, {"select"}
                      //, [KEY_PRINT]={"print"}
                      //, {"execute"}
                      //, {"snapshot"}
                      , [KEY_GREY_INSERT]={WIDE("insert"), 0, 0, {{COMMANDKEY, (PTEXT)KeyInsert}}}
                      , [KEY_GREY_DELETE]={WIDE("delete"), 0, 0, {{KEYDATA_DEFINED, (PTEXT)&KeyStroke}}}
                      , {"help"}
                      , [KEY_0]={WIDE("0"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}      //0x30
                      , [KEY_1]={WIDE("1"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_2]={WIDE("2"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_3]={WIDE("3"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_4]={WIDE("4"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_5]={WIDE("5"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_6]={WIDE("6"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_7]={WIDE("7"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_8]={WIDE("8"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_9]={WIDE("9"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
                      //..... to hmm 39
                      , [KEY_A] = {WIDE("a"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}} // 0x41 'A'
                      , [KEY_B] = {WIDE("b"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_C] = {WIDE("c"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_D] = {WIDE("d"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_E] = {WIDE("e"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_F] = {WIDE("f"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_G] = {WIDE("g"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_H] = {WIDE("h"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_I] = {WIDE("i"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_J] = {WIDE("j"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_K] = {WIDE("k"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_L] = {WIDE("l"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_M] = {WIDE("m"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_N] = {WIDE("n"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_O] = {WIDE("o"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_P] = {WIDE("p"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Q] = {WIDE("q"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_R] = {WIDE("r"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_S] = {WIDE("s"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_T] = {WIDE("t"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_U] = {WIDE("u"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_V] = {WIDE("v"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}
                                           ,{0}
                                           ,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}}}
                      , [KEY_W] = {WIDE("w"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_X] = {WIDE("x"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Y] = {WIDE("y"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Z] = {WIDE("z"),0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_PAD_0] = {WIDE("num0"), WIDE("pad0"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_1] = {WIDE("num1"), WIDE("pad1"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_2] = {WIDE("num2"), WIDE("pad2"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_3] = {WIDE("num3"), WIDE("pad3"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_4] = {WIDE("num4"), WIDE("pad4"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_5] = {WIDE("num5"), WIDE("pad5"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_6] = {WIDE("num6"), WIDE("pad6"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_7] = {WIDE("num7"), WIDE("pad7"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_8] = {WIDE("num8"), WIDE("pad8"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_9] = {WIDE("num9"), WIDE("pad9"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_MULT] = {WIDE("mult"), WIDE("mulitply"),KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_PLUS] = {WIDE("add"),0,KDF_NODEFINE,{{KEYDATA} }}
                      , {WIDE("separator"), 0, KDF_NODEFINE }
                      , [KEY_PAD_MINUS] = {WIDE("subtract"),0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_DOT] = {WIDE("decimal"),0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_DIV] = {WIDE("divide"),0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_F1]={"f1" }   // 0x70
                      , [KEY_F2]={"f2" }
                      , [KEY_F3]={"f3" }
                      , [KEY_F4]={"f4" }
                      , [KEY_F5]={"f5" }
                      , [KEY_F6]={"f6" }
                      , [KEY_F7]={"f7" }
                      , [KEY_F8]={"f8" }
                      , [KEY_F9]={"f9" }
                      , [KEY_F10]={"f10" }
                      , [KEY_F11]={"f11" }
                      , [KEY_F12]={"f12" }
                      //, {WIDE("numlock"),0,KDF_NODEFINE} // 0x90
                      //, {WIDE("scroll"),0,KDF_NODEFINE}

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
                      //, NONAMES // {WIDE("lshift"),0,KDF_NODEFINE}  // 0xa0
                      //, NONAMES // {WIDE("rshift"),0,KDF_NODEFINE}
                      //, NONAMES // {WIDE("lctrl"), WIDE("lcontrol"),0,KDF_NODEFINE}
                      //, NONAMES // {WIDE("rctrl"), WIDE("rcontrol"),0,KDF_NODEFINE}
                      //, NONAMES // {WIDE("lmenu"), WIDE("lalt"),0,KDF_NODEFINE }
                      //, NONAMES // {WIDE("rmenu"), WIDE("ralt"),0,KDF_NODEFINE }
                      , [KEY_SEMICOLON]={WIDE(";"), WIDE("semicolon"),0,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , [KEY_EQUAL]={WIDE("="), WIDE("equal"),0    ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_COMMA]={WIDE(","), WIDE("comma"),0    ,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , [KEY_DASH]={WIDE("-"), WIDE("dash"),0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_STOP]={WIDE("."), WIDE("period"),0   ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_SLASH]={WIDE("/"),WIDE("slash"),0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_ACCENT]={WIDE("`"), WIDE("accent"),0   ,{{KEYDATA}
                                            ,{KEYDATA}} } // 0xc0
                      , [KEY_LEFT_BRACKET]={ WIDE("["), WIDE("lbracket"),0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_BACKSLASH]={ WIDE("\\"), WIDE("backslash"),0,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_RIGHT_BRACKET]={ WIDE("]"), WIDE("rbracket"),0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_QUOTE]={ WIDE("'"), WIDE("quote"),0     ,{{KEYDATA}
                                              ,{KEYDATA}} }
};
#endif
#ifndef __cplusplus
#  if 0
PRELOAD( WriteSymbols)
{
	FILE *junk = fopen( WIDE("out.keysyms"), "wt" );
	if( junk )
	{
		int n;
		for( n = 0; n <256; n++ )
		{
         int m;
			if( ConsoleKeyDefs[n].name1 || ConsoleKeyDefs[n].name2 )
			{
				TEXTCHAR tmp[4];
				TEXTCHAR tmp2[4];
				TEXTSTR n1 = ConsoleKeyDefs[n].name1;
				TEXTSTR n2 = ConsoleKeyDefs[n].name2;
				if( StrCmp( ConsoleKeyDefs[n].name1, "\"" ) == 0 )
				{
               n1 = tmp;
               tmp[0] = '\\';
               tmp[1] = '\"';
					tmp[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name2, "\"" ) == 0 )
				{
               n2 = tmp2;
               tmp2[0] = '\\';
               tmp2[1] = '\"';
					tmp2[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name1, "\\" ) == 0 )
				{
               n1 = tmp;
               tmp[0] = '\\';
               tmp[1] = '\\';
					tmp[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name2, "\\" ) == 0 )
				{
               n2 = tmp2;
               tmp2[0] = '\\';
               tmp2[1] = '\\';
					tmp2[2] = 0;
				}
				fprintf( junk, ", {%s%s%s,%s%s%s,%s|%s|%s|%s|%s|%s,{"
						 , ConsoleKeyDefs[n].name1?"WIDE(\"":""
						 , ConsoleKeyDefs[n].name1?n1:"NULL"
						 , ConsoleKeyDefs[n].name1?"\")":""
						 , ConsoleKeyDefs[n].name2?"WIDE(\"":""
						 , ConsoleKeyDefs[n].name2?n2:"NULL"
						 , ConsoleKeyDefs[n].name2?"\")":""
						 , ConsoleKeyDefs[n].flags&KDF_NODEFINE?"KDF_NODEFINE":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NOREDEF?"KDF_NOREDEF":"0"
						 , ConsoleKeyDefs[n].flags&KDF_CAPSKEY?"KDF_CAPSKEY":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NUMKEY?"KDF_NUMKEY":"0"
						 , ConsoleKeyDefs[n].flags&KDF_UPACTION?"KDF_UPACTION":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NOKEY?"KDF_NOKEY":"0"
						 );
				for( m = 0; m < 8; m++ )
				{
					if( ConsoleKeyDefs[n].op[m].bFunction )
					{
						fprintf( junk, WIDE("%s"), m?",":"" );
						fprintf( junk, "{%s}"
								 , (ConsoleKeyDefs[n].op[m].bFunction==KEYDATA)?"KEYDATA":""
								 );
					}

				}
				fprintf( junk, "}}\n" );
			}
			else
			{
				fprintf( junk, ",{0}\n");
			}
		}
		fclose( junk );
	}
}
#  endif

#endif
#else
#define NONAMES {NULL,NULL,0}
PSIKEYDEFINE ConsoleKeyDefs[] = { NONAMES
                      , {WIDE("lbutton"),0,0 }
                      , {WIDE("rbutton"),0,0 }
                      , {WIDE("cancel"),0,0}
                      , {WIDE("mbutton"),0,0} // 0x04
                      , NONAMES, NONAMES, NONAMES
                      , {WIDE("back"),WIDE("backspace"),0,{{KEYDATA}} }
                      , {WIDE("tab"),0,0,{{KEYDATA}} }
                      , NONAMES, NONAMES
                      , {WIDE("clear"),0,0 }   // 0x0c
                      , {WIDE("return"), WIDE("enter"),0,{{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}} }
                      , NONAMES, NONAMES
                      , {WIDE("shift"),0,0, {{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}} } // 0x10
                      , {WIDE("control"),0,0, {{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}} }
                      , {WIDE("menu"),0,0, {{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}} }
                      , {WIDE("pause"),0,0 }
                      , {WIDE("captial"),0,0 }
                      , NONAMES, NONAMES, NONAMES, NONAMES, NONAMES, NONAMES
                      , {WIDE("esc"), WIDE("escape"), 0, {{KEYDATA}}} // 0x1b
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , {WIDE("space"), WIDE("blank"), 0, {{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}} } //0x20
                      , {WIDE("prior"), WIDE("pgup"), 0, {{HISTORYKEY,(PTEXT)HistoryPageUp}} }
                      , {WIDE("next"), WIDE("pgdn"), 0, {{HISTORYKEY,(PTEXT)HistoryPageDown}} }
                      , {WIDE("end"), 0, 0, {{COMMANDKEY, (PTEXT)KeyEndCmd}}}
                      , {WIDE("home"), 0, 0, {{COMMANDKEY, (PTEXT)KeyHome}}}
                      , {WIDE("left"), 0, 0, {{COMMANDKEY, (PTEXT)KeyLeft}}}

                      , {WIDE("up") , 0, 0, { {COMMANDKEY, (PTEXT)CommandKeyUp}
                                       , {HISTORYKEY, (PTEXT)HistoryLineUp}}}
                      , {WIDE("right"), 0, 0, {{COMMANDKEY, (PTEXT)KeyRight}}}
                      , {WIDE("down"), 0, 0, {{COMMANDKEY, (PTEXT)HandleKeyDown}
                                       , {HISTORYKEY, (PTEXT)HistoryLineDown}}}
                      , {WIDE("select")}
                      , {WIDE("print")}
                      , {WIDE("execute")}
                      , {WIDE("snapshot")}
                      , {WIDE("insert"), 0, 0, {{COMMANDKEY, (PTEXT)KeyInsert}}}
                      , {WIDE("delete"), 0, 0, {{KEYDATA_DEFINED, (PTEXT)&KeyStroke}}}
                      , {WIDE("help")}
                      , {WIDE("0"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}      //0x30
                      , {WIDE("1"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("2"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("3"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("4"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("5"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("6"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("7"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("8"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {WIDE("9"), 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , NONAMES
/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
                      //..... to hmm 39
                      , {WIDE("a"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}} // 0x41 'A'
                      , {WIDE("b"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("c"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("d"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("e"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("f"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("g"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("h"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("i"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("j"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("k"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("l"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("m"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("n"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("o"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("p"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("q"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("r"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("s"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("t"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("u"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("v"),0,0,{{KEYDATA}
											,{KEYDATA}
											,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}
											,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}
											//,{KEYDATA}
							 }}
                      , {WIDE("w"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("x"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("y"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("z"),0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {WIDE("lwin") }
                      , {WIDE("rwin") }
                      , {WIDE("apps") }
                      , NONAMES, NONAMES
                      , {WIDE("num0"), WIDE("pad0"),0,{{KEYDATA}} }
                      , {WIDE("num1"), WIDE("pad1"),0,{{KEYDATA}} }
                      , {WIDE("num2"), WIDE("pad2"),0,{{KEYDATA}} }
                      , {WIDE("num3"), WIDE("pad3"),0,{{KEYDATA}} }
                      , {WIDE("num4"), WIDE("pad4"),0,{{KEYDATA}} }
                      , {WIDE("num5"), WIDE("pad5"),0,{{KEYDATA}} }
                      , {WIDE("num6"), WIDE("pad6"),0,{{KEYDATA}} }
                      , {WIDE("num7"), WIDE("pad7"),0,{{KEYDATA}} }
                      , {WIDE("num8"), WIDE("pad8"),0,{{KEYDATA}} }
                      , {WIDE("num9"), WIDE("pad9"),0,{{KEYDATA}} }
                      , {WIDE("mult"), WIDE("mulitply"),0,{{KEYDATA}} }
                      , {WIDE("add"),0,0,{{KEYDATA} }}
                      , {WIDE("separator"), 0, 0 }
                      , {WIDE("subtract"),0,0,{{KEYDATA}} }
                      , {WIDE("decimal"),0,0,{{KEYDATA}} }
                      , {WIDE("divide"),0,0,{{KEYDATA}} }
                      , {WIDE("f1") }   // 0x70
                      , {WIDE("f2") }
                      , {WIDE("f3") }
                      , {WIDE("f4") }
                      , {WIDE("f5") }
                      , {WIDE("f6") }
                      , {WIDE("f7") }
                      , {WIDE("f8") }
                      , {WIDE("f9") }
                      , {WIDE("f10") }
                      , {WIDE("f11") }
                      , {WIDE("f12") }
                      , {WIDE("f13") }
                      , {WIDE("f14") }
                      , {WIDE("f15") }
                      , {WIDE("f16") }
                      , {WIDE("f17") }
                      , {WIDE("f18") }
                      , {WIDE("f19") }
                      , {WIDE("f20") }
                      , {WIDE("f21") }
                      , {WIDE("f22") }
                      , {WIDE("f23") }
                      , {WIDE("f24") } // 0x87
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , {WIDE("numlock"),0,0} // 0x90
                      , {WIDE("scroll"),0,0}

                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
                      , NONAMES // {WIDE("lshift"),0,0}  // 0xa0
                      , NONAMES // {WIDE("rshift"),0,0}
                      , NONAMES // {WIDE("lctrl"), WIDE("lcontrol"),0,0}
                      , NONAMES // {WIDE("rctrl"), WIDE("rcontrol"),0,0}
                      , NONAMES // {WIDE("lmenu"), WIDE("lalt"),0,0 }
                      , NONAMES // {WIDE("rmenu"), WIDE("ralt"),0,0 }
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xb0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , {WIDE(";"), WIDE("semicolon"),0,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , {WIDE("="), WIDE("equal"),0    ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {WIDE(","), WIDE("comma"),0    ,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , {WIDE("-"), WIDE("dash"),0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {WIDE("."), WIDE("period"),0   ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {WIDE("/"),WIDE("slash"),0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {WIDE("`"), WIDE("accent"),0   ,{{KEYDATA}
                                            ,{KEYDATA}} } // 0xc0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xd0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , { WIDE("["), WIDE("lbracket"),0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { WIDE("\\"), WIDE("backslash"),0,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { WIDE("]"), WIDE("rbracket"),0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { WIDE("'"), WIDE("quote"),0     ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , NONAMES
                      , NONAMES // 0xe0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xf0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES  // 0xff
                       };
#endif
/*
#if(WINVER >= 0x0400)
#define VK_PROCESSKEY     0xE5
#endif

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE
*/
//----------------------------------------------------------------------------

int CommandKeyUp( PUSER_INPUT_BUFFER pci )
{
   RecallUserInput( pci, TRUE );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int HandleKeyDown( PUSER_INPUT_BUFFER pci )
{
   RecallUserInput( pci, FALSE );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyHome( PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, 0, COMMAND_POS_SET );
   return UPDATE_COMMAND; 
}

//----------------------------------------------------------------------------

int KeyEndCmd( PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, -1, COMMAND_POS_SET );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyInsert( PUSER_INPUT_BUFFER pci )
{
	SetUserInputInsert( pci, -1 );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyRight( PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, 1, COMMAND_POS_CUR );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyLeft( PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, -1, COMMAND_POS_CUR );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyShift( P_32 pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_SHIFT;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_SHIFT;
   }
   return FALSE;
}

//----------------------------------------------------------------------------

int KeyControl( P_32 pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_CTRL;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_CTRL;
   }
   return FALSE;
}

//----------------------------------------------------------------------------

int KeyAlt( P_32 pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_ALT;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_ALT;
   }
   return FALSE;
}

//----------------------------------------------------------------------------
// Extensions and usage of keybinding data
// -- so far seperated so that perhaps it could be a seperate module...
//----------------------------------------------------------------------------

#define NUM_MODS ( sizeof( ModNames ) / sizeof( char * ) )

CTEXTSTR ModNames[] = { WIDE("shift"), WIDE("ctrl"), WIDE("alt")
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
		//if( ( !ConsoleKeyDefs[i].flags ) ||
		//    ( ConsoleKeyDefs[i].flags & (KDF_NODEFINE) ) )
      //   continue;
      if( ConsoleKeyDefs[i].name1 && TextLike( pKey, ConsoleKeyDefs[i].name1 ) )
      {
         return i;
      }
      else if( ConsoleKeyDefs[i].name2 && TextLike( pKey, ConsoleKeyDefs[i].name2 ) )
      {
         return i;
      }
   }
   return 0;
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
static void DestroyKeyMacro( PMACRO pm )
{
   PTEXT temp;
   INDEX idx;
   if( pm->flags.un.macro.bUsed )
   {
      pm->flags.un.macro.bDelete = TRUE;
      return;
   }
   LineRelease( pm->pArgs );
   LineRelease( pm->pName );
   LineRelease( pm->pDescription );
   LIST_FORALL( pm->pCommands, idx, PTEXT, temp )
   {
      LineRelease( temp );
   }
   DeleteList( &pm->pCommands );
   Release( pm );
}
#endif

// Usage: /KeyBind shift-F1
//        ... #commands
//        /endmac
// Usage: /KeyBind shift-F1 kill
// Usage: /KeyBind $F1 ... ^F1 $^F1
//  if parameters follow the keybind key-def, those params
//  are taken as keystrokes to type...
//  if no parameters follow, the definition is assumed to
//  be a macro definition, and the macro is invoked by
//  the processing entity...
#ifdef __DEKWARE_PLUGIN__

CORECON_NPROC( int, KeyBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PTEXT pKey;
   int ModVal;
   int Mod = 0;
   int KeyVal;
	PVARTEXT vt;
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;

   do{
      pKey = GetParam( ps, &parameters );
      if( pKey )
      {
         ModVal = FindMod( pKey );
         Mod |= ModVal;
      }
      else
         break;
   }while( ModVal );
   if( pKey )
      KeyVal = FindKey( pKey );
   else
   {
      DECLTEXT( msg, "Not enough parameters to KeyBind..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( !KeyVal )
   {
      DECLTEXT( msg, "First parameters to KeyBind were not a known key..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   else
   {
      //DECLTEXTSZ( msg, 256 );
      PMACRO pm;
      PTEXT pStroke;
      if( pmdp->Keyboard[KeyVal][Mod].flags.bMacro )
      {
         DECLTEXT( msg, "Destroyed prior key macro" );
         EnqueLink( &ps->Command->Output, &msg );
         DestroyKeyMacro( pmdp->Keyboard[KeyVal][Mod].data.macro );
      }
      if( pmdp->Keyboard[KeyVal][Mod].flags.bStroke )
      {
         DECLTEXT( msg, "Destroyed prior key stroke" );
         EnqueLink( &ps->Command->Output, &msg );
         LineRelease( pmdp->Keyboard[KeyVal][Mod].data.stroke );
      }
      {
         PTEXT pSubst;
         pSubst = MacroDuplicateEx( ps, parameters, FALSE, TRUE );
         if( pSubst )
         {
            pStroke = BuildLine( pSubst );
            LineRelease( pSubst );
         }
         else
            pStroke = NULL;
      }
      if( pStroke )
      {
         pmdp->Keyboard[KeyVal][Mod].flags.bStroke = TRUE;
         pmdp->Keyboard[KeyVal][Mod].flags.bMacro = FALSE;
         pmdp->Keyboard[KeyVal][Mod].data.stroke = pStroke;
         return 0;
      }
      vt = VarTextCreate();
      if( Mod & KEYMOD_SHIFT )
      {
         if( Mod & KEYMOD_CTRL )
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "shift-ctrl-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "shift-ctrl-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
         else
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "shift-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "shift-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
      }
      else
      {
         if( Mod & KEYMOD_CTRL )
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "ctrl-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "ctrl-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
         else
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
      }
      pm = (PMACRO)Allocate( sizeof( MACRO ) );
      MemSet( pm, 0, sizeof( MACRO ) );
      pm->flags.bMacro = TRUE;
      pm->pName = VarTextGet( vt );
      pm->pDescription = NULL;
      pm->nArgs = 0;
      pm->pArgs = NULL;
      ps->pRecord = pm;
      pmdp->Keyboard[KeyVal][Mod].flags.bMacro = TRUE;
      pmdp->Keyboard[KeyVal][Mod].flags.bStroke = FALSE;
      pmdp->Keyboard[KeyVal][Mod].data.macro = pm;
		VarTextDestroy( &vt );
   }
   return 0;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
CORECON_NPROC( int, KeyUnBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PTEXT pKey;
   int ModVal;
   int Mod = 0;
   int KeyVal;
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;

   do{
      pKey = GetParam( ps, &parameters );
      if( pKey )
      {
         ModVal = FindMod( pKey );
         Mod |= ModVal;
      }
      else
         break;
   }while( ModVal );
   if( pKey )
      KeyVal = FindKey( pKey );
   else
   {
      DECLTEXT( msg, "Not enough parameters to KeyUnBind..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( !KeyVal )
   {
      DECLTEXT( msg, "First parameters to KeyUnBind were not a known key..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   else
   {
      if( pmdp->Keyboard[KeyVal][Mod].flags.bMacro )
      {
         DECLTEXT( msg, "Destroyed prior key macro" );
         EnqueLink( &ps->Command->Output, &msg );
         DestroyKeyMacro( pmdp->Keyboard[KeyVal][Mod].data.macro );
         pmdp->Keyboard[KeyVal][Mod].data.macro = NULL;
         pmdp->Keyboard[KeyVal][Mod].flags.bMacro = FALSE;

      }
      else if( pmdp->Keyboard[KeyVal][Mod].flags.bStroke )
      {
         DECLTEXT( msg, "Destroyed prior key stroke" );
         EnqueLink( &ps->Command->Output, &msg );
         LineRelease( pmdp->Keyboard[KeyVal][Mod].data.stroke );
         pmdp->Keyboard[KeyVal][Mod].data.stroke = NULL;
         pmdp->Keyboard[KeyVal][Mod].flags.bStroke = FALSE;
      }
      else
      {
         DECLTEXT( msg, "Key was not bound..." );
         EnqueLink( &ps->Command->Output, &msg );
      }
   }

   return 0;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int KeyList( PSENTIENT ps, PTEXT parameters )
{
	// somewhere I should have macro dump 
	// this would be useful for this thing also..
	// header's a little different though
	return 0;
}
#endif
//----------------------------------------------------------------------------

int Widget_DoStroke( PCHAT_LIST list, PTEXT stroke )
{
   INDEX i;
   int bOutput = FALSE;
   DECLTEXT( key, WIDE(" ") );
   //Log1( WIDE("Do Stroke with %c"), stroke->data.data[0] );
   while( stroke )
   {
      for( i = 0; i < stroke->data.size; i++ )
      {
         switch( key.data.data[0] = stroke->data.data[i] )
         {
         case '\r':
            // output is always prefix linefed...
            key.data.data[0] = '\n'; // carriage return = linefeed
            goto normal_process;  // do not output return... extra lines otherwise
         case 9:
            key.data.data[0] = ' ';
         case 27:
         case '\b':
            //pdp->bUpdateToEnd = FALSE; // need update ALL not just to end
         default:
         normal_process:
            {
				PTEXT pLine;
				{
					if( ( pLine =
							GatherUserInput( list->CommandInfo
											, (PTEXT)&key ) ) )
					{
                     // I dunno - maybe we shouldn't enque the
                     // command... maybe we should count on whatever
                     // the destination is to echo it
                     // or - perhaps this should be another
                     // option only effective while bDirect...
					if( list->InputData )
						  list->InputData( list->psvInputData, pLine );
                  }
                  else // didn't get a linefeed... so.. safe echo
                  {
                     //Log( "Failed to Gather Line..." );
                  }
               }
            }
            bOutput = TRUE;
            break;
         }
      }
      stroke = NEXTLINE( stroke );
   }
   return bOutput;
}

//----------------------------------------------------------------------------
void Widget_WinLogicDoStroke( PCHAT_LIST list, PTEXT stroke )
{
	//EnterCriticalSec( &pdp->Lock );
	if( Widget_DoStroke( list, stroke ) )
	{
		if( !list->phb_Input->pBlock )
		{
			list->phb_Input->pBlock = list->phb_Input->region->pHistory.root.next;
			list->phb_Input->pBlock->nLinesUsed = 1;
			list->phb_Input->pBlock->pLines[0].flags.deleted = 0;
			list->phb_Input->nLine = 1;
		}
		list->phb_Input->pBlock->pLines[0].nLineLength = LineLengthExEx( list->CommandInfo->CollectionBuffer, FALSE, 8, NULL );
		list->phb_Input->pBlock->pLines[0].pLine = list->CommandInfo->CollectionBuffer;
		//if( !pdp->flags.bDirect && pdp->flags.bWrapCommand )
		BuildDisplayInfoLines( list->phb_Input, list->input_font );
	}

}

//----------------------------------------------------------------------------
typedef struct penging_rectangle_tag
{
	struct {
		_32 bHasContent : 1;
		_32 bTmpRect : 1;
	} flags;
   CRITICALSECTION cs;
	S_32 x, y;
   _32 width, height;
} PENDING_RECT, *PPENDING_RECT;

void Widget_KeyPressHandler( PCHAT_LIST list
						  , _8 key_index
						  , _8 mod
						  , PTEXT characters
						  )
{
//cpg26dec2006 console\keydefs.c(1409): Warning! W202: Symbol 'result' has been defined, but not referenced
//cpg26dec2006    int result;
	int bOutput = 0;
	// check current keyboard override...
   if( ConsoleKeyDefs[key_index].flags & KDF_CAPSKEY )
   {
      //if( event.dwControlKeyState & CAPSLOCK_ON )
      //{
      //   mod ^= KEYMOD_SHIFT;
      //}
   }

#ifdef programmable_keys
	if( pdp->Keyboard[key_index][mod].flags.bStroke ||
		pdp->Keyboard[key_index][mod].flags.bMacro )
	{
		if( pdp->Keyboard[key_index][mod].flags.bStroke )
		{
			extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
			PSI_WinLogicDoStroke(pdp, pdp->Keyboard[key_index][mod].data.stroke);
			SmudgeCommon( pdp->psicon.frame );
		}
#ifdef __DEKWARE_PLUGIN__
		else if( pdp->Keyboard[key_index][mod].flags.bMacro )
		{
			if( pdp->common.Owner->pRecord != pdp->Keyboard[key_index][mod].data.macro )
				InvokeMacro( pdp->common.Owner
							  , pdp->Keyboard[key_index][mod].data.macro
							  , NULL );
		}
#endif
	}
	else // key was not overridden
#endif
	{
		int result = 0;
		//Log1( WIDE("Keyfunc = %d"), KeyDefs[key_index].op[mod].bFunction );
		switch( ConsoleKeyDefs[key_index].op[mod].bFunction )
		{
		case KEYDATA_DEFINED:
			//Log( "Key data_defined" );
			{
				//extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
				//PSI_WinLogicDoStroke( pp, (PTEXT)&ConsoleKeyDefs[key_index].op[mod].data.pStroke );
				//SmudgeCommon( pdp->psicon.frame );
			}
			result = UPDATE_NOTHING; // unsure about this - recently added.
			// well it would appear that the stroke results in whether to update
			// the command prompt or not.
			break;
		case KEYDATA:
			if( GetTextSize( characters ) )
			{
				//extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
				Widget_WinLogicDoStroke( list, characters );
				//SmudgeCommon( list->psicon.frame );
			}
			result = UPDATE_NOTHING; // already taken care of?!
			break;
		case COMMANDKEY:
			result = ConsoleKeyDefs[key_index].op[mod].data.CommandKey( list->CommandInfo );
			//SmudgeCommon( pdp->psicon.frame );
			break;
		case HISTORYKEY:
			//result = ConsoleKeyDefs[key_index].op[mod].data.HistoryKey( list->pHistoryDisplay );
			break;
		case CONTROLKEY:
			//ConsoleKeyDefs[key_index].op[mod].data.ControlKey( &pdp->dwControlKeyState, TRUE );
			result = UPDATE_NOTHING;
			break;
		case SPECIALKEY:
		    list->InputPaste( list->psvInputPaste );
			//result = ConsoleKeyDefs[key_index].op[mod].data.SpecialKey( (PCONSOLE_INFO)list );
			break;
		}
		switch( result )
		{
		case UPDATE_COMMAND:
			{
				PENDING_RECT upd;
				//extern void RenderCommandLine( PCONSOLE_INFO pdp, POINTER region );
				upd.flags.bHasContent = 0;
				upd.flags.bTmpRect = 1;
				//PSI_RenderCommandLine( pdp, &upd );
				/*
				{
					RECT r;
					r.left = upd.x;
					r.right = upd.x + upd.width;
					r.top = upd.y;
					r.bottom = upd.y + upd.height;
					//pdp->Update( pdp, &r );
				}
				*/
			}

			bOutput = TRUE;
			break;
		case UPDATE_HISTORY:
			{
				/*
				extern int CPROC PSI_UpdateHistory( PCONSOLE_INFO pdp );
				if( PSI_UpdateHistory( pdp ) )
				{
					extern void CPROC PSI_RenderConsole( PCONSOLE_INFO pdp );
					PSI_RenderConsole( pdp );
				}
				*/
			}
			break;
		case UPDATE_DISPLAY:
			{
				/*
				extern void CPROC PSI_ConsoleCalculate( PCONSOLE_INFO pdp );
				PSI_ConsoleCalculate( pdp );
				*/
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------

