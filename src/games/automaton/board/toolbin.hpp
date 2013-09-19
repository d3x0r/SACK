

#include "board.hpp"

#ifdef TOOLBIN_SOURCE 
#define TOOLBIN_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define TOOLBIN_PROC(type,name) IMPORT_METHOD type CPROC name
#endif


extern "C"
{
	TOOLBIN_PROC( void, CreateToolbin )( PIBOARD board );
};


