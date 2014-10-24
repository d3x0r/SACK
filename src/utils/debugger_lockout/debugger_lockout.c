#include <stdhdrs.h>
#include <sqlgetoption.h>

static POINTER CPROC GetAntiDebugInterface( void )
{
	return (POINTER)0x12345678;
}

static void CheckDebugger( void )
{
#ifdef WIN32
	// would have done most of these... but sequence does resemeble this all in 1 I found while gathering intel.
	// implemented based on http://index-of.es/exploit/Anti-Debugging%208211%20A%20Developers%20View.pdf
	// 
	if( IsDebuggerPresent() )
	{
		(*(int*)0x13) = 0;
		BAG_Exit( 0xD1E );
	}
	{
		BOOL pbIsPresent;
		CheckRemoteDebuggerPresent( GetCurrentProcess(), &pbIsPresent );
		if( pbIsPresent )
		{
			(*(int*)0x17) = 0;
			BAG_Exit( 0xD1E );
		}
	}
	/*
	SetLastError( 0xbab1f1ea );
	OutputDebugString( "Are you a debugger?" );
	if( GetLastError() != 0xbab1f1ea )
	{
		(*(int*)0x15) = 0;
		BAG_Exit( 0xD1E );
	}
	*/
	{
		// test for various window class names
		TEXTCHAR classname[64];
		int n = 1;
		TEXTCHAR key[12];
		do
		{
			snprintf( key, 12, "%d", n );
			SACK_GetProfileString( "Debugger/Lockout/class", key, "", classname, 64 );
			if( classname[0] )
				if( FindWindow( classname, NULL ) )
				{
					(*(int*)0x1b) = 0;
					BAG_Exit( 0xD1E );
				}
		} while( classname[0] );
		n = 1;
		do
		{
			snprintf( key, 12, "%d", n );
			SACK_GetProfileString( "Debugger/Lockout/Window Title", key, "", classname, 64 );
			{
				// this needs to be an enumeration of windows and a test with... FileMask (GLOB expressions)?

			}
		} while( classname[0] );
	}
	{
		// check if debuggers are attached
		int (APIENTRY *_NtQueryInformationProcess)();
		int status;
		int retVal;
		int debugFlag;
		_NtQueryInformationProcess = (int(APIENTRY*)())LoadFunction( "ntdll.dll", "NtQueryInformationProcess" );
		if( _NtQueryInformationProcess )
		{
			status = _NtQueryInformationProcess( -1, 0x07, &retVal, 4, NULL );
			if( retVal != 0 ) {
				(*(int*)0x19) = 0;
				BAG_Exit( 0xD1E );
			}
			status = _NtQueryInformationProcess( -1, 0x1F/*DebugProcessFlags*/, &debugFlag, 4, NULL ); 
			if( debugFlag == 0 ) {
				(*(int*)0x1d) = 0;
				BAG_Exit( 0xD1E );
			}
		}
	}
	if( 1 )
	{
		// detach a debugger.
		int (APIENTRY *_NtSetInformationThread)();
		int status;
		_NtSetInformationThread = (int(APIENTRY*)())LoadFunction( "ntdll.dll", 
					"NtSetInformationThread"); 
		if( _NtSetInformationThread )
		{
			_NtSetInformationThread( GetCurrentThread(), 0x11, 0, 0 ); 
		}	}
	if( 0 )
	{
		// this actually requires to be another process
		// a process cannot attach to itself as a 
		DWORD pid = GetCurrentProcessId(); 
		int success;
		/*
		_itow_s((int)pid, (wchar_t*)&pid_str, 8, 10); 
		wcsncat_s((wchar_t*)&szCmdline, 64, (wchar_t*)pid_str, 4); 
		success = CreateProcess(path, szCmdline, NULL, NULL, 
								FALSE, 0, NULL, NULL, &si, &pi); 
		*/
		//success = DebugActiveProcess(pid); 
		if (!success) { 
			(*(int*)0x21) = 0;
			BAG_Exit( 0xD1E );
		}
	}
#endif
}

static void CPROC TickDebugger( PTRSZVAL psv )
{
	CheckDebugger();
}

PRELOAD( SetupDebuggerLockout )
{
	CheckDebugger();
	AddTimer( 5000, TickDebugger, 0 );
}
