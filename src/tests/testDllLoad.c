#ifdef WIN32
#include <windows.h>
#include <stdio.h>
int main( int argc, char**argv ) {
	if( argc > 1 ) {
		int i;
		for( i = 1; i < (argc-1); i+=2 ) {
			HMODULE status = LoadLibrary( argv[ i ] );
			printf( "%s Status is:%p %d", argv[i], status, GetLastError() );
			FARPROC x = GetProcAddress( status, argv[i+1] );
			printf( "entry found: %p", x );
		}
	}else {
      printf( "Specify library name and function to load... %s <library> <function>", argv[0] );
	}
   return 0;
}



#else
int main( void ) {
	return 0;
}
#endif