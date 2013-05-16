#include <stdio.h>
#include <system.h>
#include <sqlgetoption.h>

int main( void )
{
	TEXTCHAR buffer[256];
   TEXTCHAR buf2[64];

   printf( WIDE("Waiting for user input\n") );
	while( fgets( buf2, sizeof( buf2 ), stdin ) )
	{
		SACK_GetProfileString( GetProgramName(), WIDE("Test Option"), WIDE("default"), buffer, sizeof( buffer ) );
		printf( WIDE("Waiting for user input\n") );
	}

   return 0;
}
