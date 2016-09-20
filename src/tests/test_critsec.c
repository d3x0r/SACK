#include <stdhdrs.h>

#include <timers.h>
#ifdef WIN32
#include <conio.h>
#endif

CRITICALSECTION *cs1;
CRITICALSECTION *cs2;

int cycles[10], locked[10];

uintptr_t CPROC TestThread( PTHREAD thread )
{
   int t = GetThreadParam( thread );
	int n;
	for( n = 0; n < 1000000; n ++ )
	{
      cycles[t]++;
      EnterCriticalSec( cs1 );
		EnterCriticalSec( cs2 );
		LeaveCriticalSec( cs1 );
      WakeableSleep( 5 );
		LeaveCriticalSec( cs2 );
      WakeableSleep( 25 );
      locked[t]++;
	}
   return 0;
}

void gotoxy( int x, int y )
{
#ifdef WIN32
	COORD p; p.X = x; p.Y=y;
	SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), p );
#endif
}

void CPROC Status( uintptr_t psv )
{
   int n;
	gotoxy( 0, 0 );
   for( n = 0; n < 10; n++ )
		printf( "%d  %d\n", cycles[n], locked[n] );
   fflush( stdout );
}

int main( void )
{
	PTHREAD threads[10];
	int n;
   uint32_t size = sizeof( CRITICALSECTION ) * 5;
	CRITICALSECTION *secs = (CRITICALSECTION*)OpenSpace( "shared_test_crit_sec", NULL, &size );
	cs1 = secs;
	cs2 = secs + 1;
	InitializeCriticalSec( cs1 );
   InitializeCriticalSec( cs2 );
   AddTimer( 100, Status, 0 );
	for( n = 0; n < 10; n++ )
	{
      threads[n] = ThreadTo( TestThread, n );
	}
   // wait 2 minutes to test...
   WakeableSleep( 120000 );
   return 0;
}

