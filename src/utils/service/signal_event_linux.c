
#include <unistd.h>
#include <stdio.h>

int main( int argc, char **argv ) {
    if( argc < 2 ) {
        printf( "Usage: %s <Event name>\n", argv[0] );
        return 0;
    }
	int dir = opendir( "/tmp" );
	int hPipe = openat( dir, argv[1], 0666 );
	if( hPipe >= 0 ) {
		write( hPipe, "X", 1 );
		printf( "Event signaled." );
	} else
		printf( "Failed to set event? %d\n", errno );
	return 0;
}
