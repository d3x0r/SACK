

#include <../bard.h>

void CPROC GetAnyEvent( PTRSZVAL psv, char *extra )
{
	printf( WIDE("Received event with extra : %s\n"), extra );
}

char *name;
void CPROC GetAnEvent( PTRSZVAL psv, char *extra )
{
	printf( WIDE("Received %s event with extra : %s\n"), name, extra );
}



int main( int argc, char **argv )
{
	if( argc > 2 )
		BARD_RegisterForSimpleEvent( argv[2], GetAnyEvent, 0 );
   if( argc > 1 )
		BARD_IssueSimpleEvent( argv[1] );
   exit(0);
}

