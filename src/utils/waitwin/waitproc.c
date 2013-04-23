
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <typelib.h>
#include <sharemem.h>

BOOL GetProcessList( const TEXTCHAR *file )
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		lprintf( "CreateToolhelp32Snapshot Failed: %d", GetLastError() );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		lprintf( "Failed Process32First: %d", GetLastError() );
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
      //lprintf( "Does %s contain %s", pe32.szExeFile, file );
		if( StrCaseStr( pe32.szExeFile, file ) )
			return 1;
	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
	return( FALSE );
}


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
				while( !GetProcessList( argv[1] ) )
					Sleep( 250 );
			}
      if( wait_exit )
			while( GetProcessList( argv[1] ) )
				Sleep( 250 );
	}
	else
	{
		printf( "%s <process partial name> <started>\n"
				 " - while a process containing the partial name exists, this waits.\n"
				 " - if 'started' is specified as a second argument, "
				 "   then this waits for the process to start instead of waiting for it to exit\n"
				, argv[0] );
	}
   return 0;
}
