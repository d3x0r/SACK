#include <stdio.h>
#include <sack_system.h>
#include <sqlgetoption.h>

int main( void )
{
	TEXTCHAR buffer[256];
   TEXTCHAR buf2[64];

   printf( "Waiting for user input\n" );
	while( fgets( buf2, sizeof( buf2 ), stdin ) )
	{
		SACK_GetProfileString( GetProgramName(), "Test Option", "default", buffer, sizeof( buffer ) );
		printf( "Waiting for user input\n" );
	}

   return 0;
}
