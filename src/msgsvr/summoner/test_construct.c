
#include <stdhdrs.h>
#include <timers.h>

int main( int argc, char **argv )
{
	int n;
	int bFail = 0;
   int nSleep;
   //extern void CPROC LoadComplete();
	printf( "Okay... sending ready\n" );
	for( n = 1; n < argc; n++ )
	{
		if( strcmp( argv[n], "fail" ) == 0 )
			bFail = 1;
		else
         nSleep = atoi( argv[n] );
	}
   if( argc == 1 )
		LoadComplete();
	Sleep( nSleep );
   return 0;
}
