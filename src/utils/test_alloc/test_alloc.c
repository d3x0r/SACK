#include <stdhdrs.h>
#include <sharemem.h>
#include <timers.h>

#define SIZE 128
uint32_t threads;

uintptr_t CPROC tester( PTHREAD thread )
{
	uintptr_t psv = GetThreadParam( thread );
	POINTER p;
	int n;
	int m;
	for( n = 0; n < 10000000; n++ )
	{
		p = Allocate( SIZE );
		memset( p, psv, SIZE );
		//Relinquish();
		for( m = 0; m < SIZE; m++ )
			if( ((uint8_t*)p)[m] != psv )
				printf( "FAIL %d %d", m, psv );
		Release( p );
	}
	threads--;
}




int main( void )
{
	int n;
	for( n = 0; n < 50; n++ )
	{
      threads++;
      ThreadTo( tester, n );
	}
	while( threads )
{
printf( "thread: %d\n", threads );
WakeableSleep( 1000 );
}
}

