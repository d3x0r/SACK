#include <stdhdrs.h>
#include <stdio.h>
#include <sqlgetoption.h>

SaneWinMain( argc, argv )
{
	if( argc < 5 )
	{
		printf( "Usage: %s [root_name] [path] [option] [value]\n", argv[0] );
		printf( "   root_name = \"flashdrive.ini\" (if \"\" then option will go under DEFAULT \n" );
		printf( "   path = \"some/path/long quoted path name\"\n" );
		printf( "   option = \"option name to set\"\n" );
		printf( "   value = \"value of the option\"\n" );
		return 0;
	}
	if( argv[1][0] )
		SACK_WritePrivateProfileString( argv[2], argv[3], argv[4], argv[1] );
	else
		SACK_WriteProfileString( argv[2], argv[3], argv[4] );
    return 0;
}
EndSaneWinMain()
