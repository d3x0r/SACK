#include <windows.h>

int main( void )
{
	__declspec(dllimport) void Shutdown( void );
	__declspec(dllimport) void Library_main( void );
   atexit( Shutdown );
   Library_main();
	return 0;
}

