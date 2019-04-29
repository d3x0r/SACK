#ifndef BUTTON_METHODS_DEFINED
#define BUTTON_METHODS_DEFINED

#define BUTTON_CLICK( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY "/methods/Button/Click" \
	                                        , name, "void", _WIDE(#name), "(uintptr_t,PCOMMON)" ); } \
	void CPROC name args

#define BUTTON_DRAW( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY "/methods/Button/Draw" \
	                                        , name, "void", _WIDE(#name), "(uintptr_t,PCOMMON)" ); } \
	void CPROC name args

#define BUTTON_CHECK( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY "/methods/Button/Check" \
	                                        , name, "void", _WIDE(#name), "(uintptr_t,PCOMMON)" ); } \
	void CPROC name args




#endif

