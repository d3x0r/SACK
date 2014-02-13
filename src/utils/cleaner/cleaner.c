#include <stdhdrs.h>
#include <filesys.h>

int level;

static void CPROC ProcessFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
   level++;
   printf( WIDE("%3d - %s\n"), level, name );
	if( flags & SFF_DIRECTORY )
	{
		TEXTCHAR tmp[4];
      TEXTSTR use_name;
      int n;
		POINTER info = NULL;
		tmp[0] = '.';
      tmp[1] = '/';
		tmp[3] = 0;
		for( n = 0; n < 26; n++ )
		{
			tmp[2] = 'a' + n;
         lprintf( WIDE("rename %s to %s?"), name, tmp + 2 );
			if( sack_rename( name, tmp + 2 ) )
			{
            use_name = tmp;
            lprintf( WIDE("go to %s"), tmp );
				if( !SetCurrentPath( tmp ) )
               printf( WIDE("Failed?") );
			}
			else
			{
            use_name = name;
				printf( WIDE("set path ...") );
            lprintf( WIDE("go to %s"), name );
				if( !SetCurrentPath( name ) )
               printf( WIDE("Failed?") );
            break;

			}
		}
		while( ScanFiles( WIDE("."), NULL, &info, ProcessFile, SFF_DIRECTORIES|SFF_NAMEONLY|SFF_SUBPATHONLY
							 , 0 ) );

      SetCurrentPath( WIDE("..") );
		sack_rmdir( 0, use_name );

	}
	else
	{
      sack_unlink( 0, name );
	}
   level--;

}

int main( int argc, char **argv )
{
	POINTER info = NULL;

	while( ScanFiles( WIDE("."), NULL, &info, ProcessFile, SFF_DIRECTORIES|SFF_NAMEONLY|SFF_SUBPATHONLY
                    , 0 ) );

   return 0;
}
