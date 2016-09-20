
#include <../bard.h>

void CPROC GetAnyEvent( uintptr_t psv, char *extra )
{
	printf( WIDE("Received event with extra : %s\n"), extra );
}

int main( int argc, char **argv )
{

	if( BARD_RegisterForSimpleEvent( argv[1], GetAnyEvent, 0 ) )
		printf( WIDE("Listening for event: %s\n"), argv[1]?argv[1]:"ANY" );
	else
	{
		printf( WIDE("Failed to register with BARD service") );
		return 1;
	}

	while(1)
      Sleep( SLEEP_FOREVER );
}


