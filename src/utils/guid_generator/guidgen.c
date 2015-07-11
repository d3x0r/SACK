#include <stdhdrs.h>
#include <pssql.h>

int main( int argc, char **argv )
{
	int n;
   CTEXTSTR guid = GetGUID();
	for( n = 1; n < argc; n++ )
	{
		if( argv[n][0] == '-' && ( ( argv[n][1] == 's' ) || ( argv[n][1] == 'S' ) ) )
         guid = GetSeqGUID();
	}
   if( argc == 1 )
		printf( "-s parameter allows getting a sequential guid instead of random\n" );
	printf( "%s\n", guid );

   return 0;
}
