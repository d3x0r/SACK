
#include <stdio.h>
#include <windows.h>

int main( int argc, char **argv )
{
	if( argv[1] )
	{
      int wait_exit = 1;
		printf( "waiting for [%s]\n", argv[1] );
		if( argc > 2 && argv[2] )
			if( stricmp( argv[2], "started" ) == 0 )
			{
				wait_exit = 0;
				while( !FindWindow( NULL, argv[1] ) )
					Sleep( 250 );
			}
      if( wait_exit )
			while( FindWindow( NULL, argv[1] ) )
				Sleep( 250 );
	}
	else
	{
		printf( "%s <window title> <started>\n"
				 " - while a window with the title exists, this waits.\n"
				 " - if 'started' is specified as a second argument, "
				 "   then this waits for the window to exist instead of waiting for it to close\n"
				, argv[0] );
	}
   return 0;
}
