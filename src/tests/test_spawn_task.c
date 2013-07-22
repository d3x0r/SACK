#include <stdhdrs.h>

LOGICAL ended;

void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task_ended )
{
   printf( WIDE("Ended.\n") );
	fflush( stdout );
   ended = TRUE;
}

void CPROC HandleTaskOutput( PTRSZVAL psvTaskInfo, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	printf( WIDE("%s"), buffer );
   fflush( stdout );
}



int main( void )
{
	PTASK_INFO task = LaunchPeerProgram( WIDE("ping.exe")
												  , NULL
												  , NULL
												  , HandleTaskOutput
												  , TaskEnded
												  , 0 );
	while( !ended )
		WakeableSleep( 250 );
   return 0;
}
