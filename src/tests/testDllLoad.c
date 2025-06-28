#include <stdhdrs.h>

int main( int argc, char**argv ) {
	if( argc > 1 ) {
		int i;
		for( i = 1; i < argc; i++ ) {
			POINTER status = LoadFunction( argv[ i ], NULL );
			printf( "%s Status is:%p", argv[i], status );
		}
	}else {
      printf( "Specify library name to load... %s <library>", argv[0] );
	}
   return 0;
}