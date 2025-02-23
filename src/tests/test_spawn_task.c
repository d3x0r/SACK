#include <stdhdrs.h>

LOGICAL ended;

void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task_ended )
{
   printf( "Ended.\n" );
	fflush( stdout );
   ended = TRUE;
}

void CPROC HandleTaskOutput( uintptr_t psvTaskInfo, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	printf( "%s", buffer );
   fflush( stdout );
}



int main( void )
{
	PTASK_INFO task = LaunchPeerProgram( "ping.exe"
												  , NULL
												  , NULL
												  , HandleTaskOutput
												  , TaskEnded
												  , 0 );
	while( !ended )
		WakeableSleep( 250 );
   return 0;
}
