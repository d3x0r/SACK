#define __STATIC__
#include "sack_vidlib.h"

int main( void ) {
	PLIST list = NULL;
	INDEX idx;
	char *name;
	PRENDERER r = OpenDisplay( 0 );
	LIST_FORALL( list, idx, char *, name ) {
		printf( "list has: %d = %s\n", idx, name );
	}
	return 0;
}
