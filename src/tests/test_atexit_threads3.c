#include <windows.h>

int main( void )
{
	__declspec(dllimport) void Library_main( void );
   Library_main();
	return 0;
}

