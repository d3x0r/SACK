#include <stdio.h>
#include <logging.h>
#include <pssql.h>


int main( void )
{
   int n;
	CTEXTSTR result = NULL;
   n = 0;
//	while( 1 )
	for( n=0;  n < 1000; n++ )
	{
		if( DoSQLQuery( "Select 1+1", &result ) )
		{
			lprintf( "%d : Result of query = %s\n", n, result );
			printf( "%d : Result of query = %s\n", n, result );
		}
		else
		{
         GetSQLError( &result );
			printf( "%s\n", result );
		}
	}
   return 0;
}
