#ifndef __KEYPAD2_WIDGET_DEFINED__
#define __KEYPAD2_WIDGET_DEFINED__
#include <controls.h>
#include <configscript.h>
#ifdef KEYPAD_SOURCE
#define KEYPAD_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define KEYPAD_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

//-- keypad.c --------------------

#ifdef __cplusplus
#define KEYPAD_NAMESPACE SACK_NAMESPACE namespace widgets { namespace buttons {
#define KEYPAD_NAMESPACE_END } } SACK_NAMESPACE_END
#else
#define KEYPAD_NAMESPACE
#define KEYPAD_NAMESPACE_END
#endif

SACK_NAMESPACE
#ifdef __cplusplus
	namespace widgets {
		namespace buttons {
#endif

#ifndef KEYPAD_STRUCTURE_DEFINED
typedef PSI_CONTROL PKEYPAD;
#else
typedef struct keypad_struct *PKEYPAD;
#endif

#define KEYPAD_FLAG_DISPLAY 1
#define KEYPAD_FLAG_PASSWORD 2 // 2 and 1
#define KEYPAD_FLAG_ENTRY 4
#define KEYPAD_FLAG_ALPHANUM 8
#define KEYPAD_FLAG_DISPLAY_LEFT_JUSTIFY  KEYPAD_DISPLAY_ALIGN_LEFT
#define KEYPAD_FLAG_DISPLAY_CENTER_JUSTIFY  KEYPAD_DISPLAY_ALIGN_CENTER

KEYPAD_PROC( void, SetNewKeypadFlags )( int newflags );
// if you call SetNewKeypadFlags with the desired flags, and
// then invoke a normal MakeNamedControl( WIDE("Keypad Control") ) you will
// get the desired results - though not thread safe...
KEYPAD_PROC(PSI_CONTROL, MakeKeypad )( PSI_CONTROL parent
												 , int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t ID, uint32_t flags
												 , CTEXTSTR accumulator_name );
KEYPAD_PROC(void, SetKeypadAccumulator )( PSI_CONTROL keypad, char *accumulator_name );
KEYPAD_PROC(int64_t, GetKeyedValue )( PSI_CONTROL keypad );
KEYPAD_PROC(int, GetKeyedText )( PSI_CONTROL keypad, TEXTSTR buffer, int buffersize );
KEYPAD_PROC(void, ClearKeyedEntry )( PSI_CONTROL keypad );
KEYPAD_PROC( void, ClearKeyedEntryOnNextKey )( PSI_CONTROL pc );

KEYPAD_PROC(int, WaitForKeypadResult) ( PSI_CONTROL keypad );
KEYPAD_PROC( void, CancelKeypadWait )( PSI_CONTROL keypad );
//KEYPAD_PROC(void, HideKeypad )( PSI_CONTROL keypad );
//KEYPAD_PROC(void, ShowKeypad )( PSI_CONTROL keypad );
#define HideKeypad HideCommon
#define ShowKeypad RevealCommon

//KEYPAD_PROC(PSI_CONTROL, GetKeypadCommon )( PSI_CONTROL keypad );
#define GetKeypadCommon(keypad) (keypad)
//KEYPAD_PROC(PSI_CONTROL, GetKeypad )( PSI_CONTROL pc );
#define GetKeypad(pc) (pc)

KEYPAD_PROC( void, SetKeypadEnterEvent )( PSI_CONTROL pc, void (CPROC *event)(uintptr_t,PSI_CONTROL), uintptr_t psv );
KEYPAD_PROC( void, SetKeypadCancelEvent )( PSI_CONTROL pc, void (CPROC *event)(uintptr_t,PSI_CONTROL), uintptr_t psv );

KEYPAD_PROC( PSI_CONTROL, MakeKeypadHotkey )( PSI_CONTROL frame
														  , int32_t x
														  , int32_t y
														  , uint32_t w
														  , uint32_t h
														  , TEXTCHAR *keypad
														  );
KEYPAD_PROC( void, KeyIntoKeypad )( PSI_CONTROL keypad, int64_t value );
KEYPAD_PROC( void, KeyIntoKeypadNoEnter )( PSI_CONTROL pc, uint64_t value );
KEYPAD_PROC( void, KeypadInvertValue )( PSI_CONTROL keypad );

KEYPAD_PROC( CDATA, KeypadGetDisplayBackground )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetDisplayTextColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetBackgroundColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetNumberKeyColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetEnterKeyColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetC_KeyColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetNumberKeyTextColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetEnterKeyTextColor )( PSI_CONTROL keypad );
KEYPAD_PROC( CDATA, KeypadGetC_KeyTextColor )( PSI_CONTROL keypad );
KEYPAD_PROC( void, KeypadSetDisplayBackground )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetDisplayTextColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetNumberKeyColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetEnterKeyColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetC_KeyColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetBackgroundColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetNumberKeyTextColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetEnterKeyTextColor )( PSI_CONTROL keypad, CDATA color );
KEYPAD_PROC( void, KeypadSetC_KeyTextColor )( PSI_CONTROL keypad, CDATA color );

KEYPAD_PROC( void, KeypadSetupConfig )( PCONFIG_HANDLER pch, uintptr_t *ppsv );
KEYPAD_PROC( void, KeypadWriteConfig )( FILE *file, CTEXTSTR indent, PSI_CONTROL pc_keypad );
KEYPAD_PROC( void, KeypadAddMagicKeySequence )( PSI_CONTROL keypad, CTEXTSTR sequence, void (CPROC*event_proc)( uintptr_t ), uintptr_t psv_sequence );
KEYPAD_PROC( void, KeypadSetAccumulator )( PSI_CONTROL pc, CTEXTSTR name );


enum keypad_styles
{
	KEYPAD_INVERT = 1,   // number keys are 1,2,3 instead of 7,8,9 on first row
	KEYPAD_STYLE_YES_NO = 2,        // 0b000010
	KEYPAD_STYLE_ENTER_CANCEL = 4,  // 0b000100 
	KEYPAD_STYLE_ENTER_CORRECT = 6, // 0b000110
	KEYPAD_STYLE_ENTER_CLEAR = 8,   // 0b001000
 	KEYPAD_STYLE_DOUBLE0_CLEAR = 10,// 0b001010
	KEYPAD_MODE_MASK = 0x1E, // use 3 bits to mask styles...
	KEYPAD_STYLE_FULL_KEYBOARD = 0x20,  // not just a keypad, uses different rules/styles...
	KEYPAD_STYLE_PASSWORD = 0x40, // show *** for text in accumulator
	KEYPAD_DISPLAY_ALIGN_LEFT = 0x80, // align text in the display to the left (else right justified)
	KEYPAD_DISPLAY_ALIGN_CENTER = 0x100 // align text in the display centered (else right justified)
};
KEYPAD_PROC( void, SetKeypadStyle )( PSI_CONTROL keypad, enum keypad_styles style );
KEYPAD_PROC( enum keypad_styles, GetKeypadStyle )( PSI_CONTROL pc_keypad );
//
// format string is '#' to display a character of the accumulator
//
KEYPAD_PROC( void, KeypadSetDisplayFormat)(  PSI_CONTROL pc_keypad, CTEXTSTR format );
KEYPAD_PROC( void, KeypadGetDisplayFormat )( PSI_CONTROL pc_keypad, TEXTSTR format, int buflen );

KEYPAD_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::widgets::buttons;
#endif

#endif
