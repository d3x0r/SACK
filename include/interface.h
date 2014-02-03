// find and replace INTERFACE with desired header name...
#ifndef INTERFACE_DEFINED
#define INTERFACE_DEFINED
#include <sack_types.h>

#if defined( INTERFACE_SOURCE )
#define INTERFACE_PROC(type,name) EXPORT_METHOD type CPROC name
#define INTERFACE_PROC_PTR(type,name) EXPORT_METHOD type (*CPROC name)
#else
#define INTERFACE_PROC(type,name) __declspec(dllimport) type CPROC name
#define INTERFACE_PROC_PTR(type,name) __declspec(dllimport) type (*CPROC name)
#endif

#define METHOD_PTR(type,name) type (CPROC *_##name)
#define DMETHOD_PTR(type,name) type (CPROC **_##name)

#define METHOD_ALIAS(i,name) ((i)->_##name)
#define PDMETHOD_ALIAS(i,name) (*(i)->_##name)
//#define AliasedMethod METHOD_ALIAS(default,AliasedMethod)


//INTERFACE_PROC( POINTER, GetInterface )( char *service );
//INTERFACE_PROC( void, DropInterface )( char *service, POINTER );



#endif
