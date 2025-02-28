#include "../intershell_registry.h"

#define OnQueryPageCycle(name) \
	DefineRegistryMethod(TASK_PREFIX,QueryPageCycle,"page_cycle","query_page_cycle",name,LOGICAL,(void),__LINE__)

#define InvokePageCycle(pc_canvas)  {     \
	   void (CPROC*f)(PSI_CONTROL);       \
		   f = GetRegisteredProcedure2( TASK_PREFIX "/page_cycle/commands", void, "cycle_now", (PSI_CONTROL) ); \
		   if( f ) f(pc_canvas);          \
   }
