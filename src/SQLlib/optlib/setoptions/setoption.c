#include <stdhdrs.h>
#include <stdio.h>
#include <sqlgetoption.h>

SaneWinMain( argc, argv )
{
	int argn = 1;
	if( argc < 5 )
	{
		printf( "Usage: %s [-d] [root_name] [path] [option] [value]\n", argv[0] );
		printf( "   -d reads the value isntead of setting the value." );
		printf( "   root_name = \"flashdrive.ini\" (if \"\" then option will go under DEFAULT \n" );
		printf( "   path = \"some/path/long quoted path name\"\n" );
		printf( "   option = \"option name to set\"\n" );
		printf( "   value = \"value of the option\"\n" );
		return 0;
	}
	if( argn < argc && argv[argn][0] == '-' && argv[argn][1] == 'd'  ) {
		argn++;
		TEXTCHAR buf[256];
		size_t buflen = 256;
		if( argv[argn+1][0] ) {
			SACK_GetPrivateProfileStringExxx(NULL, argv[argn + 2], argv[argn + 3], "", buf, buflen, argv[argn + 1], FALSE DBG_SRC);
			printf("%.*s", buf, buflen);
		} else {
			TEXTCHAR *buf;
			SACK_GetProfileBlob( argv[argn+2], argv[argn+3], &buf, &buflen );
			printf("%.*s", buf, buflen);
		}
	} else {
		if( argv[argn+1][0] )
			SACK_WritePrivateProfileString( argv[argn+2], argv[argn+3], argv[argn+4], argv[argn+1] );
		else
			SACK_WriteProfileString( argv[argn+2], argv[argn+3], argv[argn+4] );
	}
	return 0;
}
EndSaneWinMain()
