#include <stdhdrs.h>
#if _WIN32
#undef Sleep
#endif
int main( int argc, char ** argv )
{
	int n;
	if( argc < 2 )
	{
		printf( WIDE("Usage: rundelay <delay in seconds>\n") );
      return 0;
	}
	n = atoi( argv[1] );
	if( n )
	{
      printf( WIDE("Delaying for %d seconds...\n"), n );
      Sleep( n * 1000 );
	}
   return 0;
}

