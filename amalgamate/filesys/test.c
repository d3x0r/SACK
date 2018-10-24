#include "sack_ucb_filelib.h"

//-------------------------------- BEGIN TEST MAIN ----------------

int main( void ) {
	FILE *file;
	const char *line;
	char buf[256];
	file = sack_fopen( 0, "test", "rt" );
	while( line = sack_fgets( buf, 256, file ) )
		printf( "%s", line );
	sack_fclose( file );


	return 0;
}

