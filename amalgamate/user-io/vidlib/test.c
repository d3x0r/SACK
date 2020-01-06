#define __STATIC__
#define DEFINE_DEFAULT_RENDER_INTERFACE
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
