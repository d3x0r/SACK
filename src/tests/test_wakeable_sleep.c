
#include <stdhdrs.h>

static struct test_data
{
   PTHREAD _100_sleep;
   PTHREAD _10_sleep;
}l;


static PTRSZVAL CPROC TestWake1( PTHREAD thread )
{
	while( 1 )
	{
		lprintf( "sleeping 100" );
		WakeableSleep( 100 );
		lprintf( "woke 100" );
	}
}

static PTRSZVAL CPROC TestWake2( PTHREAD thread )
{
   int n = 0;
	while( 1 )
	{
		lprintf( "sleeping 10" );
		WakeableSleep( 10 );
		lprintf( "woke 10" );
		if( n++ > 2 )
		{
         WakeThread( l._100_sleep );
         n = 0;
		}
	}
}


SaneWinMain( argc, argv )
{
   lprintf( "Begin main..." );
	l._100_sleep = ThreadTo( TestWake1, 0 );
	l._10_sleep = ThreadTo( TestWake2, 0 );
	while( 1 )
	{
      WakeableSleep( 10000 );
	}

   return 0;
}
EndSaneWinMain()
