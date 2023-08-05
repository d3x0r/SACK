
#include <Windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

void create( int newGroup ) {
    memset( &si, 0, sizeof( STARTUPINFO ) );
    si.cb = sizeof( STARTUPINFO );
    memset( &pi, 0, sizeof( PROCESS_INFORMATION ) );
	BOOL a;
	switch( newGroup & 0xFF ) {
	case 0: 
	case 1: 
     {
			/*
    a = CreateProcess( NULL, "node.exe"
                           , NULL,NULL,TRUE
                           , 0
//                            | CREATE_NO_WINDOW
//                            | DETACHED_PROCESS
                            | (newGroup?CREATE_NEW_PROCESS_GROUP:0)
                           , NULL
                           , NULL
                           , &si
                           , &pi );
			*/
    }
		break;
	case 2:
	{
		char name[] = "wait.exe";
    a = CreateProcess( NULL, name
                           , NULL,NULL,TRUE
                           , 0
//                            | CREATE_NO_WINDOW
//                            | DETACHED_PROCESS
                            | (CREATE_NEW_PROCESS_GROUP)
                           , NULL
                           , NULL
                           , &si
                           , &pi );
	}
		break;
	case 3:
	 {
		 char name[] = "wait.bat";
		a = CreateProcess( NULL, name
                           , NULL,NULL,TRUE
                           , 0
//                            | CREATE_NO_WINDOW
//                            | DETACHED_PROCESS
                            | (CREATE_NEW_PROCESS_GROUP)
                           , NULL
                           , NULL
                           , &si
                           , &pi );
	}
		break;

	case 4:{
	 {
			char name[] = "wait1.bat";
			a = CreateProcess( NULL, name
                           , NULL,NULL,TRUE
                           , 0
//                            | CREATE_NO_WINDOW
//                            | DETACHED_PROCESS
                            | (CREATE_NEW_PROCESS_GROUP)
                           , NULL
                           , NULL
                           , &si
                           , &pi );
	}
		} 
		break;
	}
	printf( "Status:%d  %d\n", a, GetLastError() );
}

BOOL WINAPI HandlerRoutine( DWORD dwCtrlType ){
	// never called.
	printf( "Got Ctrl event: %d\n", dwCtrlType );
	return TRUE; // let everyone else handle it.
}


void killProcess( void ) {
	BOOL a;
	DWORD dwLast = pi.dwProcessId;
	printf( "kill process( with break )\n" );
	a = FreeConsole();
	if( !a ) {
		printf( "Attach Free Failed %d", GetLastError() );
	}
	a = AttachConsole( dwLast );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Attach %d %d\n", a, dwError );
	}else printf( "Attach OK.\n" );
	a = SetConsoleCtrlHandler( HandlerRoutine, TRUE );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Setup handler failed?\n" );
	} else 		printf( "RESetup handler success.\n" );
	//a = GenerateConsoleCtrlEvent( CTRL_C_EVENT, pi.dwProcessId );
	a = GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT , dwLast );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Generate %d %d\n", a, dwError );
	}else printf( "Generate ctrl-c worked.\n" );
	a = FreeConsole();
	if( !a ) {
		printf( "Attach Free Failed %d", GetLastError() );
	}
	a = AttachConsole( ATTACH_PARENT_PROCESS );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "De-Attach %d %d\n", a, dwError );
	}else printf( "De-Attach OK.\n" );
	a = SetConsoleCtrlHandler( HandlerRoutine, TRUE );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Setup handler failed?\n" );
	} else 		printf( "RESetup handler success.\n" );

}

int main( int argc, char **argv ) {
    uint32_t now = GetTickCount();
	 DWORD dwVal;
	BOOL a = SetConsoleCtrlHandler(HandlerRoutine, TRUE );

	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Setup handler failed?\n" );
	}else 		printf( "Setup handler success.\n" );

    create( 2 );

	//system( "tasklist /?" );
    while( ( GetTickCount() - now ) < 50 ) Sleep( 10 );
	killProcess( );

	Sleep(1);
	if( WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 )
		printf( "Process died.\n" );
   else printf( "Process is still hanging out\n" );

    create( 4 );

	//system( "tasklist /?" );
    while( ( GetTickCount() - now ) < 50 ) Sleep( 10 );
	killProcess( );

	printf( "I got killed myself?\n" ); // this won't log if CREATE_NEW_PROCESS_GROUP is not set
    while( ( GetTickCount() - now ) < 1500 ) Sleep( 10 );
	if( WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 )
		printf( "Process died.\n" );
   else printf( "Process is still hanging out\n" );


//	TerminateProcess( pi.hProcess, 0 );
    return 0;
}
