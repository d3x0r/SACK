#ifndef KEYPAD_ISP_PUBLIC_DEFINED
#define KEYPAD_ISP_PUBLIC_DEFINED 

#ifdef KEYPAD_ISP_SOURCE
#define KEYPAD_ISP_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define KEYPAD_ISP_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef INTERSHELL_CORE_BUILD
#include "../../include/keypad.h"
#else
#include "InterShell/widgets/keypad.h"
#endif

PUBLIC( PSI_CONTROL, GetKeypadOfType )( CTEXTSTR type );
PUBLIC( void, GetKeypadsOfType )( PLIST *ppResultList, CTEXTSTR type );
PUBLIC( void, CreateKeypadType )( CTEXTSTR name );

// the uintptr_t here is retreived using InterShell_GetButtonUserData( button )
PUBLIC( void, SetKeypadType )( PSI_CONTROL keypad, CTEXTSTR type );
// define special key procedure.
// Special Characters are led by @... @E = enter, @C= cancel @B=backspace....
// more to be defined.  @@ is an @ (full keyboard only)
PUBLIC( void, Keypad_AddMagicKeySequence )( PSI_CONTROL keypad, CTEXTSTR sequence, void (CPROC*event_proc)( uintptr_t ), uintptr_t psv_sequence );



#define OnKeypadEnter(name) \
	  DefineRegistryMethod(TASK_PREFIX,KeypadEnter,"common","keypad enter",name "_on_keypad_enter",void,(PSI_CONTROL),__LINE__)
#define OnKeypadCancel(name) \
	  DefineRegistryMethod(TASK_PREFIX,KeypadCancel,"common","keypad cancel",name "_on_keypad_cancel",void,(PSI_CONTROL),__LINE__)

#define OnKeypadEnterType(name, typename) \
	DefineRegistryMethod(TASK_PREFIX,KeypadTypeEnter,"common/"typename,"keypad enter",name "_on_keypad_enter",void,(PSI_CONTROL),__LINE__)

#define OnKeypadCancelType(name, typename) \
	  DefineRegistryMethod(TASK_PREFIX,KeypadTypeEnter,"common/"typename,"keypad cancel",name "_on_keypad_enter",void,(PSI_CONTROL),__LINE__)


#endif
