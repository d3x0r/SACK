#include <stdhdrs.h>
#include <sharemem.h>
#include <timers.h>

CRITICALSECTION cs;
int done;
int xx[32];
int done;
static uintptr_t  ThreadWrapper( PTHREAD pThread ){
	int n = GetThreadParam( pThread );
	while( !done ) {
		EnterCriticalSec( &cs );
		xx[n]++;
		LeaveCriticalSec( &cs );
	}
	return 0;
}

int main( void ) {

	int n;
	int total = 0;
	InitializeCriticalSec( &cs );
	for( n = 0; n < 12; n++ )
	{
		ThreadTo( ThreadWrapper, (uintptr_t)n );
	}
	Sleep( 10000 );
	for( n = 0; n < 12; n++ ) {
		printf( "%d = %d\n", n, xx[n] );
              total += xx[n];
        }
	printf( "Total = %d\n", total );
	done = 1;
}
