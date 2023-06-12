
#include <windows.h>
#include <stdio.h>

int main( int argc, char **argv ) {
    if( argc < 2 ) {
        printf( "Usage: %s <Event name>\n", argv[0] );
        return 0;
    }
    HANDLE hEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, argv[1] );
	 if( hEvent != INVALID_HANDLE_VALUE ) if( !SetEvent( hEvent ) ) printf( "Failed to set event? %d\n", GetLastError() );
	 else printf( "Event signaled." );

}
