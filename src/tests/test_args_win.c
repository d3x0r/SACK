
#include <sack_system.h>

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow )
{
	LPSTR other;
	int argc;
   char **argv;
	printf( "Input is [%s]\n", lpCmd );
	printf( "System Function is [%s]\n", GetCommandLine() );
	ParseIntoArgs( GetCommandLine(), &argc, &argv );
	{
		int n;
		for( n = 0; n < argc; n++ )
		{
			printf( "arg[%d] = %s\n", n, argv[n] );
		}
	}
	return 0;
}
