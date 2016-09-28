#include <stdio.h>

int main( void )
{
	char buf[256];
	if( fgets( buf, 255, stdin ) )
		printf( "%s", buf );
	return 0;
}

