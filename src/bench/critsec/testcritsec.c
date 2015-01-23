
#include <stdhdrs.h>

	CRITICAL_SECTION cs1;
	CRITICALSECTION cs2;
int main( void )
{
	_64 ticks = 0;
   _32 start = GetTickCount();
	_32 now = start;
   InitializeCriticalSection( &cs1 );

	while( ( now - start )<  5000 )
	{
		EnterCriticalSection( &cs1 );
		{
			ticks++;
			if( ( ticks % 10000 ) == 0 )
				now = GetTickCount();
		}
		LeaveCriticalSection( &cs1 );
	}

   printf( "%lld per-ms %lld\n", ticks, ticks/(now-start));

	start = now;
   ticks = 0;

	while( ( now - start )<  5000 )
	{
		TryEnterCriticalSection( &cs1 );
		{
			ticks++;
			if( ( ticks % 10000 ) == 0 )
				now = GetTickCount();
		}
		LeaveCriticalSection( &cs1 );
	}

   printf( "%lld per-ms %lld\n", ticks, ticks/(now-start));

	start = now;
   ticks = 0;

   while( ( now - start )<  5000 )
	{
		EnterCriticalSec( &cs2 );
		{
			ticks++;
			if( ( ticks % 10000 ) == 0 )
				now = GetTickCount();
		}
		LeaveCriticalSec( &cs2 );
	}

   printf( "%lld per-ms %lld\n", ticks, ticks/(now-start));

	start = now;
   ticks = 0;

   while( ( now - start )<  5000 )
	{
      THREAD_ID prior;
		EnterCriticalSecNoWait( &cs2, &prior );
		{
			ticks++;
			if( ( ticks % 10000 ) == 0 )
				now = GetTickCount();
		}
		LeaveCriticalSec( &cs2 );
	}

   printf( "%lld per-ms %lld\n", ticks, ticks/(now-start));

   return 0;
}

