
#include <Windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct process_id_pair {
	DWORD parent;
	DWORD child;
};
struct process_id_pair pairs[32];
int usedPairs = 0;


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
                            | ((newGroup & 256)?0:CREATE_NEW_PROCESS_GROUP)
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
                            | ((newGroup & 256)?0:CREATE_NEW_PROCESS_GROUP)
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
                            | ((newGroup & 256)?0:CREATE_NEW_PROCESS_GROUP)
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
	printf( "Got Ctrl event: %d\n", dwCtrlType );
	return TRUE; // let everyone else handle it.
}


void ProcIdFromParentProcId( DWORD dwProcessId ) {
	struct process_id_pair pair = { 0, dwProcessId };
	int i = 0;
	int maxId = 1;
	int minId = 0;
	HANDLE hp = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof( PROCESSENTRY32 );
	pairs[usedPairs++] = pair;
	while( 1 ) {
		int found;
		int idx;
		struct process_id_pair*procId;
		found = 0;
		if( Process32First( hp, &pe ) ) {
			do {
				idx = minId;
				for( idx = minId; idx < maxId; idx++ ) {
					procId = pairs + idx;
					if( idx > maxId ) break;
					if( pe.th32ParentProcessID == procId->child ) {
						found = 1;
						pair.parent = procId->child;
						pair.child = pe.th32ProcessID;
						pairs[usedPairs++] = pair;
					}
				}
			} while( Process32Next( hp, &pe ) );
			if( !found ) break;
			minId = maxId;
			maxId = usedPairs;
		}
	}
	CloseHandle( hp );
}


void killProcess( int killChildMost ) {
	BOOL a;
	DWORD dwLast = pi.dwProcessId;
	printf( "kill (%s) process( with break )\n", killChildMost?"Youngest":"" );
	if( killChildMost ) {
		struct process_id_pair* pair;
		int idx;
		ProcIdFromParentProcId( pi.dwProcessId );
		for( idx = 0; idx < usedPairs; idx++ ) {
			pair = pairs + idx;
			printf( "Process Chain has: %6d %6d\n", pair->parent, pair->child );
			//dwLast = pair->child;

		}
		printf( "Killing %d instead of %d\n", dwLast, pi.dwProcessId );
	}
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
	} else 		printf( "Setup handler success.\n" );
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
	a = AttachConsole( -1 );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "De-Attach %d %d\n", a, dwError );
	}else printf( "De-Attach OK.\n" );
	a = SetConsoleCtrlHandler( HandlerRoutine, TRUE );
	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Setup handler failed?\n" );
	} else 		printf( "Setup handler success.\n" );

}

int main( int argc, char **argv ) {
    uint32_t now = GetTickCount();
	 DWORD dwVal;
	BOOL a = SetConsoleCtrlHandler(HandlerRoutine, TRUE );
	FreeConsole();
	AllocConsole();
	GetHandleInformation( GetStdHandle( STD_INPUT_HANDLE  ), &dwVal );
	printf( "In %d\n", dwVal );
	GetHandleInformation( GetStdHandle( STD_OUTPUT_HANDLE  ), &dwVal );
	printf( "Out %d\n", dwVal );
	GetHandleInformation( GetStdHandle( STD_ERROR_HANDLE  ), &dwVal );
	printf( "Err %d\n", dwVal );

	if( !a ) {
		DWORD dwError = GetLastError();
		printf( "Setup handler failed?\n" );
	}else 		printf( "Setup handler success.\n" );

    create( (argc-1)?atoi( argv[1] ):0 );

	//system( "tasklist /?" );
    while( ( GetTickCount() - now ) < 500 ) Sleep( 10 );
	killProcess( (argc>2)?atoi(argv[2]):0 );

	Sleep(1);
	printf( "I got killed myself?\n" ); // this won't log if CREATE_NEW_PROCESS_GROUP is not set
    while( ( GetTickCount() - now ) < 1500 ) Sleep( 10 );
	if( WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 )
		printf( "Process died.\n" );
   else printf( "Process is still hanging out\n" );


//	TerminateProcess( pi.hProcess, 0 );
    return 0;
}
