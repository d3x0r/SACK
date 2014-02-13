#include <stdhdrs.h>
#include <stdio.h>
#include <sqlgetoption.h>

SaneWinMain( argc, argv )
{
	if( argc < 5 )
	{
		printf( WIDE("Usage: %s [root_name] [path] [option] [value]\n"), argv[0] );
		printf( WIDE("   root_name = \"flashdrive.ini\" (if \"\" then option will go under DEFAULT \n") );
		printf( WIDE("   path = \"eltanin receiver\"\n") );
		printf( WIDE("   option = \"Bonanza Enable\"\n") );
		printf( WIDE("   value = 0/1\n") );
      printf( WIDE("  %s \"/flashdrive.ini/eltanin receiver\" \"Bonanza Enable\" 1\n"), argv[0] );
      return 0;
	}
	if( argv[1][0] )
		SACK_WritePrivateProfileString( argv[2], argv[3], argv[4], argv[1] );
	else
		SACK_WriteProfileString( argv[2], argv[3], argv[4] );
    return 0;
}
EndSaneWinMain()
