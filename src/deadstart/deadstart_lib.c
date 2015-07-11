
#define DISABLE_DEBUG_REGISTER_AND_DISPATCH

#if defined( __GNUC__ )
#ifndef __cplusplus
#include <stdhdrs.h>
#include <deadstart.h>

#pragma GCC visibility push(hidden)

#define paste(a,b) a##b
#define paste2(a,b) paste(a,b)

//#ifndef (
//static
//#endif
	void paste2( TARGET_LABEL,_RegisterStartups)( void ) __attribute__((constructor)) __attribute__((used));
//PRIORITY_PRELOAD( RunStartups, 25 )
// This becomes the only true contstructor...
// this is loaded in the main program, and not in a library
// this ensures that the libraries registration (if any)
// is definatly done to the main application
//(the one place for doing the work)


static int Registered;
// this one is used when the library loads.  (there is only one of these.)
// and constructors are run every time a library is loaded....
// I wonder whose fault that is....
void paste2( TARGET_LABEL,_RegisterStartups)( void )
{
#define DeclareList(n) paste2(n,TARGET_LABEL)
	extern struct rt_init DeclareList( begin_deadstart_ );
	extern struct rt_init DeclareList( end_deadstart_ );
	struct rt_init *begin = &DeclareList( begin_deadstart_ );
	struct rt_init *end = &DeclareList( end_deadstart_ );
	struct rt_init *current;
#ifdef __NO_BAG__
   printf( "Not doing deadstarts\n" );
	return;
#endif
	Registered=1;
	//cygwin_dll_init();
	if( begin[0].scheduled )
      return;
	if( (begin+1) < end )
	{
		for( current = begin + 1; current < end; current++ )
		{
			if( !current[0].scheduled )
			{
#ifdef _DEBUG
				RegisterPriorityStartupProc( current->routine, current->funcname, current->priority, NULL, current->file, current->line );
#else
				RegisterPriorityStartupProc( current->routine, current->funcname, current->priority, NULL );
#endif
				current[0].scheduled = 1;
			}
			else
			{
#ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
				lprintf( WIDE("Not Register(already did this once) %d %s@%s(%d)"), current->priority, current->funcname, current->file, current->line );
#endif
			}
		}
	}
	// should be setup in such a way that this ignores all external invokations until the core app runs.
	//InvokeDeadstart();
}
#pragma GCC visibility pop

#endif
#endif
