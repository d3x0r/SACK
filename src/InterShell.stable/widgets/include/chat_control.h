
#ifdef CHAT_CONTROL_SOURCE
#define CHAT_CONTROL_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define CHAT_CONTROL_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

CHAT_CONTROL_PROC(void, ScrollableChatControl_AddConfigurationMethods )( PCONFIG_HANDLER pch );
