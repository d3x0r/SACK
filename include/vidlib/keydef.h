#ifndef KEY_STRUCTURE_DEFINED

RENDER_NAMESPACE

#ifndef RENDER_INTERFACE_INCLUDED
/* type for callback signature to handle a bound keyboard event. */
typedef LOGICAL (CPROC *KeyTriggerHandler)(PTRSZVAL,_32 keycode);


/* <combine sack::image::render::KeyDefine>
   
	\ \                                      */
typedef struct KeyDefine *PKEYDEFINE;
#endif

struct key_function
{
		struct key_triggers{
			KeyTriggerHandler trigger;
			KeyTriggerHandler extended_key_trigger;
		}data;
	PTRSZVAL psv;
   PTRSZVAL extended_key_psv;
};

//#if !defined( DISPLAY_SOURCE ) && !defined( DISPLAY_SERVICE ) && !defined( DISPLAY_CLIENT )
#define KEY_STRUCTURE_DEFINED
typedef struct keybind_tag { // overrides to default definitions
//DOM-IGNORE-BEGIN
   struct {
		int bFunction:1;
		int bRelease:1;
		int bAll:1; // application wants presses and releases..
	} flags;
	PLIST key_procs;
#if 0
		struct key_triggers{
			KeyTriggerHandler trigger;
			KeyTriggerHandler extended_key_trigger;
		}data;
	PTRSZVAL psv;
	PTRSZVAL extended_key_psv;
#endif
//DOM-IGNORE-END
} KEYBIND, *PKEYBIND;

typedef struct KeyDefine {
//DOM-IGNORE-BEGIN
   // names which keys may be also called if the key table is dumped
   char *name1;
   // names which keys may be also called if the key table is dumped
   char *name2;
   int flags;
	KEYBIND mod[8];
} KEYDEFINE;
//#endif


RENDER_NAMESPACE_END


#endif
