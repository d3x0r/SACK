#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int Version;
char buf[256];

int DoVersionFile( void )
{
	FILE *file = fopen( "autovers.h", "rb+" );
	if( !file )
		file = fopen( "autovers.h", "wb" );
	if( !file )
	{
		printf( "AutoVersion program Failed to open version.h!" );
		return -1;
	}
	if( fgets( buf, 255, file ) )
	{
		buf[255] = 0;
		if( sscanf( buf, "//%d", &Version ) != 1 )
		{
			printf( "AutoVersion's version.h was corrupt! (//###) first line." );
			return -1;
		}
	}
	else
		Version = 0;
   rewind( file );
	Version++;
   fprintf( file, "//%d\n", Version );
	fprintf( file, "#define LOCALVERSION \"%d\"\n", Version );
   fclose( file );
   return 0;
}

int LaunchCmd( char *command, LPDWORD lpdwExit )
{
#ifdef _WIN32
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
	 	memset( &si, 0, sizeof( STARTUPINFO ) );
		si.cb = sizeof( STARTUPINFO );

		if( !CreateProcess( NULL, command, NULL, NULL, 0, 0
									, NULL, NULL
									, &si, &pi ) )
		{
      	printf( "CreateProcess failed:%d\n", GetLastError() );
			return -1;
		}
		if( WaitForSingleObject( pi.hProcess, INFINITE ) != WAIT_OBJECT_0 )
		{
			printf( "WaitForObject failed: %d\n", GetLastError() ) ;
			return -1;
		}
      if( !GetExitCodeProcess( pi.hProcess, lpdwExit ) )
      {
      	printf( "GetExitCode failed:%d\n", GetLastError() );
      	return -1;
      }
      CloseHandle( pi.hProcess );
      CloseHandle( pi.hThread );
      return 0;
	}
#endif
}

int main( char argc, char **argv )
{
	DWORD dwCode;
	if( argc < 2 )
	{
		return DoVersionFile();
	}
	else
	{
		if( strcmp( argv[1], "get" ) == 0 )
		{
			int r;
        	if( LaunchCmd( "cvs update autovers.h", &dwCode )  )
        		return -1;

        	if( dwCode )
        	{
        		if( dwCode == 1 )
       			printf( "autover: Login error... any others??\n" );
       		else
        			printf( "autover: dwCode: %d", dwCode );
           	return dwCode;
			}
			else
				printf( "Success?" );
			if( DoVersionFile() )
				return -1;
		}
		else if( strcmp( argv[1], "put" ) == 0 )
		{
			struct stat statbuf;
			if( stat( "autovers.h", &statbuf ) < 0 )
			{
				printf( "Cannout put autovers.h, it does not exist!\n" );
				return -1;
			}
			if( LaunchCmd( "cvs commit -m\"Auto increment\" autovers.h", &dwCode ) || dwCode )
			{
				printf( "autover: Attempting to add..." );
				if( LaunchCmd( "cvs add autovers.h", &dwCode ) || dwCode )
				{
					printf( "autover: Failed to add, failed to commit, some other error?\n" );
					return -1;
				}
				if( LaunchCmd( "cvs commit -m\"Auto increment\" autovers.h", &dwCode ) || dwCode )
				{
					printf( "autover: Added okay, failed to commit?\n" );
					return -1;
				}
			}
			remove( "autovers.h" );
		}
	}
	return 0;
}

//$Log: autover.c,v $
//Revision 1.5  2002/04/15 17:56:06  panther
//Okay...
//
//Revision 1.4  2002/04/15 17:53:04  panther
//*** empty log message ***
//
