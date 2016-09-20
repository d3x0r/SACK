#include <stdhdrs.h>
#include <sharemem.h>
#include <filesys.h>
#include <pssql.h>
#include <sqlgetoption.h>

int main( int argc, TEXTCHAR **argv )
{
	int saveas = 0;
   TEXTCHAR outpath[256];
	TEXTCHAR *app = argc>3?argv[3]:NULL;
	if( !app || argc < 2 )
	{
		printf( WIDE("Usage: %s <config filename> [optional - set as config filename]\n" ), argv[0] );
      return 1;

	}
	if( argc > 2 )
		saveas = 1;

	{
      TEXTSTR tmp = (TEXTSTR)StrDup( argv[1] );
		TEXTSTR filename = (TEXTSTR)pathrchr( tmp );
      lprintf( "blah" );
		if( !filename )
         filename = tmp;


		{
			uint32_t size = 0;
			POINTER mem = OpenSpace( NULL, argv[1], &size );
			if( mem && size )
			{

#ifndef __NO_SQL__
				SACK_WritePrivateProfileBlob( "intershell/configuration", saveas?argv[2]:filename, (TEXTCHAR*)mem, size, app );
#endif
				CloseSpace( mem );
			}
			else
				printf( "Failed to open %s", argv[1] );

		}
	}
    return 1;
}
