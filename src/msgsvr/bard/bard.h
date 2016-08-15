
#ifndef BARD_SERVICES_DEFINED
#define BARD_SERVICES_DEFINED

#include <stdhdrs.h>

#ifdef BARD_SOURCE
#define BARD_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define BARD_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

BARD_PROC( int, BARD_RegisterForSimpleEvent )( char *eventname, void (CPROC*eventproc)(uintptr_t,char *extra),uintptr_t );
BARD_PROC( int, BARD_IssueSimpleEvent )( char *eventname );



#endif
