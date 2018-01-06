#include <stdhdrs.h>

#include <timers.h>

CRITICALSECTION cs;
int xx[32];

static uintptr_t  ThreadWrapper( PTHREAD pThread ){
	int n = GetThreadParam( pThread );
	while( 1 ) {
		EnterCriticalSecEx( &cs );
		xx[n]++;
		LeaveCriticalSecEx( &cs );
	}
	return 0;
}

int main( void ) {

	int n;
	for( n = 0; n < 24; n++ )
	{
		ThreadTo( ThreadWrapper, (void*)n );
	}
	Sleep( 10000 );
	for( n = 0; n < 24; n++ )
		printf( "%d = %d\n", n, xx[n] );
}
