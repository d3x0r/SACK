#ifndef BUTTON_METHODS_DEFINED
#define BUTTON_METHODS_DEFINED

#define BUTTON_CLICK( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY WIDE("/methods/Button/Click") \
	                                        , name, WIDE("void"), _WIDE(#name), WIDE("(uintptr_t,PCOMMON)") ); } \
	void CPROC name args

#define BUTTON_DRAW( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY WIDE("/methods/Button/Draw") \
	                                        , name, WIDE("void"), _WIDE(#name), WIDE("(uintptr_t,PCOMMON)") ); } \
	void CPROC name args

#define BUTTON_CHECK( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY WIDE("/methods/Button/Check") \
	                                        , name, WIDE("void"), _WIDE(#name), WIDE("(uintptr_t,PCOMMON)") ); } \
	void CPROC name args




#endif

