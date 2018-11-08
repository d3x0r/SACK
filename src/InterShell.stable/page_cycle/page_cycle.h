#include "../intershell_registry.h"

#define OnQueryPageCycle(name) \
	DefineRegistryMethod(TASK_PREFIX,QueryPageCycle,WIDE( "page_cycle" ),WIDE( "query_page_cycle" ),name,LOGICAL,(void),__LINE__)

#define InvokePageCycle(pc_canvas)  {     \
	   void (CPROC*f)(PSI_CONTROL);       \
		   f = GetRegisteredProcedure2( TASK_PREFIX WIDE( "/page_cycle/commands" ), void, WIDE("cycle_now"), (PSI_CONTROL) ); \
		   if( f ) f(pc_canvas);          \
   }
