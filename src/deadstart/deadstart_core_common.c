
#include <sack_types.h>
#include <deadstart.h>
#include "deadstart_core.h"

struct deadstart_local_data_ deadstart_local_data;

#ifndef __STATIC_GLOBALS__

EXPORT_METHOD struct deadstart_local_data_* GetDeadstartSharedGlobal( void ){
	return &deadstart_local_data;
}

#endif