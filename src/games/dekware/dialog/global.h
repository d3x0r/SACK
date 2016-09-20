#include <timers.h>

typedef struct {
	union {
		struct {
			// invoke this after the script is invoked...
         // well okay/cancel buttons have this problem...
			ButtonPushMethod clickproc;
         uintptr_t clickdata;
		} button;
	} data;
} MY_CONTROL_DATA, *PMY_CONTROL_DATA;


typedef struct common_tracker COMMON_TRACKER, *PCOMMON_TRACKER;
struct common_tracker {
	struct {
		uint32_t created_internally : 1;
		uint32_t menu : 1;
	} flags;
	union {
		PCOMMON pc;
		PMENU menu;
	}control;


};


#ifndef MAIN_SOURCE
extern
#endif

	struct {
		INDEX iCommon;
		PENTITY peCreating;
		CRITICALSECTION csCreating;
      PLIST pMyFrames;
	} g;


void DestroyAControl( PENTITY pe );
PENTITY GetOneOfMyFrames( PCOMMON pc );
int IsOneOfMyFrames( PENTITY pe );
int CPROC SaveCommonMacroData( PCOMMON pc, PVARTEXT pvt );
PENTITY CommonInitControl( PCOMMON pc );
