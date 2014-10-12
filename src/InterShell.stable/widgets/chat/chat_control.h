

#ifndef CHAT_CONTROL_SOURCE
#define CHAT_CONTROL_PROC(a,b) EXPORT_METHOD a b
#else
#define CHAT_CONTROL_PROC(a,b) IMPORT_METHOD a b
#endif

typedef struct chat_time_tag
{
	_8 hr,mn,sc;
	_8 mo,dy;
	_16 year;
} CHAT_TIME;
typedef struct chat_time_tag *PCHAT_TIME;

CHAT_CONTROL_PROC( void, Chat_SetMessageInputHandler )( PSI_CONTROL pc, void (*Handler)( PTRSZVAL psv, PTEXT input ), PTRSZVAL psv );
CHAT_CONTROL_PROC( void, Chat_EnqueMessage )( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , CTEXTSTR text );
