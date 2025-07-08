#define _POSIX_C_SOURCE  200809L
#define _POSIX_SOURCE
#include <unistd.h>
#include <errno.h>

#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

int main( int argc, char **argv ) {
    if( argc < 2 ) {
        printf( "Usage: %s <Event name>\n", argv[0] );
        return 0;
    }
	int dir = open("/tmp", O_RDONLY | O_DIRECTORY);

	int hPipe = openat( dir, argv[1], 0666 );
	if( hPipe >= 0 ) {
		write( hPipe, "X", 1 );
		printf( "Event signaled." );
	} else
		printf( "Failed to set event? %d\n", errno );
	return 0;
}
