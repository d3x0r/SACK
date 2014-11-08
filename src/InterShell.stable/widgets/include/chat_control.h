#ifndef CHAT_CONTROL_DEFINED
#define CHAT_CONTROL_DEFINED

#include <psi.h>

#ifndef CHAT_CONTROL_SOURCE
#define CHAT_CONTROL_PROC(a,b) EXPORT_METHOD a b
#else
#define CHAT_CONTROL_PROC(a,b) IMPORT_METHOD a b
#endif

typedef struct chat_time_tag
{
	_16 ms;
	_8 sc,mn,hr,dy,mo;
	_16 yr;
} CHAT_TIME;
typedef struct chat_time_tag *PCHAT_TIME;

CHAT_CONTROL_PROC( void, Chat_SetMessageInputHandler )( PSI_CONTROL pc, void (CPROC *Handler)( PTRSZVAL psv, PTEXT input ), PTRSZVAL psv );
CHAT_CONTROL_PROC( void, Chat_EnqueMessage )( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , CTEXTSTR text );
CHAT_CONTROL_PROC( void, Chat_ClearMessages )( PSI_CONTROL pc );
#endif
