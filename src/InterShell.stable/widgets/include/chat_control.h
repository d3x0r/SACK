#ifndef CHAT_CONTROL_DEFINED
#define CHAT_CONTROL_DEFINED

#include <psi.h>

#ifdef CHAT_CONTROL_SOURCE
#define CHAT_CONTROL_PROC(a,b) EXPORT_METHOD a b
#else
#define CHAT_CONTROL_PROC(a,b) IMPORT_METHOD a b
#endif

typedef struct chat_time_tag
{
	_16 ms;
	_8 sc,mn,hr,dy,mo;
	_16 yr;
	S_8 zhr, zmn;
} CHAT_TIME;
typedef struct chat_time_tag *PCHAT_TIME;

CHAT_CONTROL_PROC( void, Chat_SetMessageInputHandler )( PSI_CONTROL pc, void (CPROC *Handler)( PTRSZVAL psv, PTEXT input ), PTRSZVAL psv );
CHAT_CONTROL_PROC( void, Chat_SetPasteInputHandler )( PSI_CONTROL pc, void (CPROC *Handler)( PTRSZVAL psv ), PTRSZVAL psv ); // app read clipboard
CHAT_CONTROL_PROC( void, Chat_SetDropInputHandler )( PSI_CONTROL pc, void (CPROC *Handler)( PTRSZVAL psv, CTEXTSTR input, S_32 x, S_32 y ), PTRSZVAL psv );  // get filepath... called multiple times for multiselect
CHAT_CONTROL_PROC( void, Chat_SetSeenCallback )( PSI_CONTROL pc, void (CPROC *Handler)( PTRSZVAL psv ) ); // psv comes from enque message/image
CHAT_CONTROL_PROC( void, Chat_SetExpire )( PSI_CONTROL pc, int delta_seconds );

CHAT_CONTROL_PROC( void, Chat_EnqueMessage )( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , CTEXTSTR text
							 , PTRSZVAL psvSeen );
CHAT_CONTROL_PROC( void, Chat_EnqueImage )( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , Image image
							 , PTRSZVAL psvSeen );
CHAT_CONTROL_PROC( void, Chat_ClearMessages )( PSI_CONTROL pc );

CHAT_CONTROL_PROC( PSI_CONTROL, ImageViewer_ShowImage )( Image image );

CHAT_CONTROL_PROC( void, Chat_GetCurrentTime )( PCHAT_TIME timebuf );
CHAT_CONTROL_PROC( void, Chat_ClearOldMessages )( PSI_CONTROL pc, int delete_time );


#endif
