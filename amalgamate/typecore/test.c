#include "sack_ucb_typelib.h"

int main( void ) {
	PLIST list = NULL;
	INDEX idx;
	char *name;
	AddLink( &list, "asdf" );
	AddLink( &list, "ghij" );
	LIST_FORALL( list, idx, char *, name ) {
		printf( "list has: %d = %s\n", idx, name );
	}
	return 0;
}
