
#include <controls.h>
#include <timers.h>

CRITICALSECTION cs;
PSI_CONTROL pc;


uintptr_t CPROC Thread1( PTHREAD thread )
{
   while( 1 )
	if( !pc )
	{
      EnterCriticalSec( &cs );
		pc = CreateFrame( "test frame", 0, 0, 256, 256, 0, NULL );
      DisplayFrame( pc );
      LeaveCriticalSec( &cs );
	}
	else
      Relinquish();
   return 0;
}

uintptr_t CPROC Thread2( PTHREAD thread )
{
   while( 1 )
	if( pc )
	{
      EnterCriticalSec( &cs );
		DestroyFrame( &pc );
      LeaveCriticalSec( &cs );

	}
	else
      Relinquish();
   return 0;
}

int main( void )
{
	ThreadTo( Thread1, 0 );
	ThreadTo( Thread2, 0 );
	WakeableSleep( 120000 );
   return 0;
}

