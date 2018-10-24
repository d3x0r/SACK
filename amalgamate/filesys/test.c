#include "sack_filelib.h"

//-------------------------------- BEGIN TEST MAIN ----------------

int main( void ) {
	PLIST list = NULL;
	INDEX idx;
	char *name;
	AddLink( &list, "asdf" );
	LIST_FORALL( list, idx, char *, name ) {
		printf( "list has: %d = %s\n", idx, name );
	}


	return 0;
}

