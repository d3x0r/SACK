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
		if( DoSQLQuery( WIDE("Select 1+1"), &result ) )
		{
			lprintf( WIDE("%d : Result of query = %s\n"), n, result );
			printf( WIDE("%d : Result of query = %s\n"), n, result );
		}
		else
		{
         GetSQLError( &result );
			printf( WIDE("%s\n"), result );
		}
	}
   return 0;
}
