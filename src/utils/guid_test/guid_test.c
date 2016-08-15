#include <stdhdrs.h>
#include <pssql.h>

int main( void )
{
	CTEXTSTR key = GetGUID();
	CTEXTSTR zero = GuidZero();
	uint32_t tick = timeGetTime();
   uint32_t othertick;
	int n;
	for( n = 0; n < 10000; n++ )
		GetSeqGUID();
   othertick = timeGetTime();
	printf( "%d in %d %g\n", n, othertick - tick, (othertick-tick)/(float)n );
   tick = timeGetTime();
	for( n = 0; n < 100000; n++ )
		GetGUID();
   othertick = timeGetTime();
	printf( "%d in %d %g\n", n, othertick - tick, (othertick-tick)/(float)n );

	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
	printf( "Guid = %s\n", GetSeqGUID() );
   printf( "Zero = %s\n", zero );
   return 0;
}
