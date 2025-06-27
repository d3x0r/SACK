#include <stdhdrs.h>

int main( int argc, char**argv ) {
	if( argc > 1 ) {
		int status = LoadLibrary( argv[1] );
		printf( "Status is:%d", status );
	}else {
      printf( "Specify library name to load... %s <library>", argv[0] );
	}
   return 0;
}