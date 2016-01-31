#include <stdhdrs.h>

#include <salty_generator.h>
#include <sqlgetoption.h>


#define nIsDebuggerPresent 22
static const char sIsDebuggerPresent[] =          { '\x16','\x35','\x45','\x49','\x47','\x1C','\xF6','\xE0','\x78','\x34','\x9D','\x26','\x25','\x07','\x15','\x73','\x27','\x19','\x0F','\x06','\x04','\x80','\x02'// };
#define sGetCurrentProcess (sIsDebuggerPresent + nIsDebuggerPresent + 1)
#define nGetCurrentProcess 22
/*static const TEXTCHAR sGetCurrentProcess[] =     {*/, '\x16','\x6D','\x49','\x43','\x6B','\xF6','\x9F','\x1F','\x80','\x4F','\x2D','\x3B','\xBC','\x80','\xFD','\x07','\x4A','\x01','\x55','\x15','\x96','\xE6','\xDF'// };
#define sGetCurrentThread (sGetCurrentProcess + nGetCurrentProcess + 1)
#define nGetCurrentThread 21
/*static const TEXTCHAR sGetCurrentThread[] =      {*/, '\x15','\x5C','\x38','\x39','\x46','\xFE','\x71','\xC2','\xFE','\x37','\x1D','\xDA','\x89','\xBF','\x43','\x60','\x9C','\xAD','\x91','\x62','\x3D','\xFB' //};
#define sCheckRemoteDebuggerPresent (sGetCurrentThread + nGetCurrentThread + 1)
#define nCheckRemoteDebuggerPresent 31
/*static const TEXTCHAR sCheckRemoteDebuggerPresent[] = {*/, '\x1F','\x5F','\x66','\x53','\x68','\x15','\x8D','\x13','\x5F','\xBF','\x40','\xD4','\x50','\xB2','\xBB','\x99','\xF5','\xFA','\x22','\xA5','\x41','\x6D','\xC3','\x4E','\x05','\x62','\xA2','\x2D','\x73','\x54','\x4B','\xFD'// };
#define sGetCurrentProcessId (sCheckRemoteDebuggerPresent + nCheckRemoteDebuggerPresent + 1)
#define nGetCurrentProcessId 24
/*static const TEXTCHAR sGetCurrentProcessId[] =   {*/, '\x18','\x6F','\x6B','\x6B','\x63','\x9D','\x2D','\xAE','\xC7','\xF3','\xB4','\x97','\xDA','\xB8','\x75','\x0C','\xEC','\x3F','\x4D','\x97','\x2C','\xD6','\xA8','\xFD','\x2E' //};
#define sOpenProcess (sGetCurrentProcessId + nGetCurrentProcessId + 1)
#define nOpenProcess 16
/*static const TEXTCHAR sOpenProcess[] =           {*/, '\x10','\x69','\x44','\x38','\x3E','\xB9','\x16','\xD3','\x36','\xAB','\x9C','\x73','\x8D','\x63','\x04','\x49','\xB6' //};
#define sCloseHandle (sOpenProcess + nOpenProcess + 1)
#define nCloseHandle 16
/*static const TEXTCHAR sCloseHandle[] =           {*/, '\x10','\x61','\x42','\x5F','\x66','\xBA','\xCE','\xF7','\xBD','\xBE','\x3B','\xAA','\x90','\xF5','\xBC','\x9E','\x08' //};
#define sKernel32 (sCloseHandle + nCloseHandle + 1)
#define nKernel32 15
/*static const TEXTCHAR sKernel32[] =              {*/, '\x11','\x45','\x4B','\x62','\x50','\x5C','\x61','\x04','\x63','\x68','\x2D','\x65','\x6A','\xF0','\x00','\x97','\xB8','\x05' };

//static TEXTCHAR sFindWindow[] =                     { '\x0F','\x3A','\x51','\x41','\x46','\xB6','\xB4','\x04','\xF4','\xB9','\x41','\x3E','\xC4','\xFE','\x1B','\xF3'};
#ifndef _UNICODE
static const char sFindWindowA[] =                { '\x10','\x64','\x3C','\x39','\x40','\xC5','\xC3','\x54','\x0D','\x7A','\xC7','\xFE','\xD5','\x88','\xEF','\xDD','\x49'};
#else
static const char sFindWindowW[] =                { '\x10','\x6D','\x46','\x54','\x39','\x8F','\x6F','\x6E','\xB2','\x19','\xA1','\x16','\x85','\x0C','\x2C','\xCF','\xB0' };
#endif
static const char sUser32[] =                     { '\x0F','\x36','\x52','\x5F','\x64','\xBE','\x6E','\x02','\x22','\x98','\xB8','\x92','\x6F','\x8F','\xB7','\x1C'};

static const char sNtQueryInformationProcess[] =  { '\x1E','\x5B','\x60','\x32','\x5F','\x42','\xBA','\x22','\xF4','\x11','\x53','\x5D','\x01','\xE4','\x5D','\xBF','\x76','\x81','\xC6','\x08','\xB4','\xFF','\xAB','\x98','\xC4','\xD1','\x83','\x1B','\x0B','\x92','\x9B'};
//static TEXTCHAR sNtSetInformationThread[] =         { '\x1B','\x5F','\x6A','\x5F','\x6F','\x5C','\x65','\xA4','\x1D','\x87','\xC2','\x59','\x58','\xF4','\xE1','\xA5','\x19','\x9F','\x02','\x35','\x9B','\x3E','\x86','\x06','\xF5','\x2C','\x1B','\x00'};
static const char sNtSetInformationThread[] =     { '\x1B','\x59','\x40','\x35','\x42','\x3D','\x62','\xC9','\xC8','\x6A','\x1E','\x5D','\xA6','\x37','\xE7','\x19','\x23','\xA6','\x77','\x30','\xB5','\x55','\xF2','\x53','\xDA','\x88','\xA5','\x35'};
static const char sNTDll[] =                      { '\x0E','\x5E','\x54','\x3C','\x30','\x06','\x9B','\xD2','\xA3','\x38','\x6A','\xED','\x6E','\xF8','\x5E' };

static const char sDebugWndClassLocks[] =         { '\x1B','\x64','\x3B','\x6A','\x54','\x3F','\xFF','\xB2','\x87','\x28','\x7B','\x43','\x0D','\xB0','\x75','\x14','\x24','\x71','\x8D','\x2F','\x1B','\xB3','\xD9','\x7C','\x4E','\x0A','\x5F','\x82' };
static const char sDebugWndNameLocks[] =          { '\x22','\x38','\x3D','\x43','\x65','\xAA','\x14','\x1F','\xDE','\x4D','\xA2','\xC0','\x3C','\xC1','\xE7','\x0E','\x6F','\xCD','\x72','\xC9','\x40','\x3C','\x8F','\x0B','\x03','\x7E','\x4B','\xC9','\xE4','\xB2','\x32','\xA9','\x16','\x67','\x6C' };
static const char sAddTimerExx[] =                { '\x10','\x5E','\x50','\x37','\x69','\xEE','\xB1','\x5A','\x42','\xBD','\x8A','\xCF','\x44','\xA2','\xDA','\xB7','\x05' };

//static const TEXTCHAR sACK_GetProfileString[] = { 0 };

#if 0
// allow application to hook into anti-debugging?
static POINTER CPROC GetAntiDebugInterface( void )
{
	return (POINTER)0x12345678;
}
#endif

//#define OBFUSCATE

#ifdef OBFUSCATE

TEXTSTR CryptStringEx( CTEXTSTR input, size_t len )
#define CryptString(s) CryptStringEx(s,sizeof(s))
{
	P_8 output;
	size_t outsize;
	size_t n;
	size_t ofs;
	static TEXTCHAR stringbuf[256];
	CTEXTSTR chars = "0123456789ABCDEF";
	SRG_EncryptRawData( input, len, &output, &outsize );

	stringbuf[0] = '\'';
	stringbuf[1] = '\\';
	stringbuf[2] = 'x';
	stringbuf[3] = chars[outsize >> 4];
	stringbuf[4] = chars[outsize & 0xF];
	stringbuf[5] = '\'';
	ofs = 6;
	for( n = 0; n < outsize; n++ )
	{
		ofs += snprintf( stringbuf + ofs, 256 - ofs, ",\'\\x%02X\'", output[n] );
	}
	lprintf( "%s=%s", input, stringbuf );
	return stringbuf;
}

#endif

typedef _32 (CPROC *PaddTimerExx)( _32 start, _32 frequency
					, TimerCallbackProc callback
					, PTRSZVAL user DBG_PASS);

static PaddTimerExx CheckDebugger( void )
{
#ifdef WIN32
#  ifdef __GNUC__
#    define LIBNAME "libbag.dll"
#  else
#    define LIBNAME "bag.dll"
#  endif
	// would have done most of these... but sequence does resemeble this all in 1 I found while gathering intel.
	// implemented based on http://index-of.es/exploit/Anti-Debugging%208211%20A%20Developers%20View.pdf
	// 
	static BOOL (__stdcall*isDebuggerPresent)(void);
	static BOOL (__stdcall*checkRemoteDebuggerPresent)( HANDLE hProcess, PBOOL pbDebuggerPresent );
	static HANDLE (__stdcall*getCurrentProcess)(void);
	static DWORD (__stdcall*getCurrentProcessId)(void);
	static HANDLE (__stdcall*openProcess)( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId );
	static BOOL (__stdcall*closeHandle)( HANDLE hObject );
	static HANDLE (__stdcall*getCurrentThread)(void);
	static HWND (__stdcall*findWindow)( CTEXTSTR classname, CTEXTSTR wndname );

	static void (*_DecryptRawData)( CPOINTER binary, size_t length, P_8 *buffer, size_t *chars );
	static PaddTimerExx addTimer;
	//TEXTCHAR *tmp;
	P_8 output;
	size_t outsize;
	P_8 output2 = NULL;
	size_t outsize2;
	P_8 output3 = NULL;
	size_t outsize3;
	P_8 output4 = NULL;
	size_t outsize4;
	TEXTCHAR stringbuf[256];
	
	if( !_DecryptRawData )
	{
		int n;
		for( n = 0; n < 18; n++ )
		{
			stringbuf[n] = "@ATLWvpajcgArdWrgr"[n] ^ 0x13;
		}
		stringbuf[n] = 0;
		_DecryptRawData = (void (*)( CPOINTER , size_t , P_8 *, size_t * ))LoadFunction( WIDE(LIBNAME), stringbuf );

		_DecryptRawData( sAddTimerExx + 1, sAddTimerExx[0], &output, &outsize );
		addTimer = (PaddTimerExx)LoadFunction( WIDE(LIBNAME), (CTEXTSTR)output );
		Release( output );
	}

#ifdef OBFUSCATE
	{
		CTEXTSTR tmp;
		tmp = CryptString( "IsDebuggerPresent" );
		tmp = CryptString( "GetCurrentProcess" );
		tmp = CryptString( "GetCurrentThread" );
		tmp = CryptString( "CheckRemoteDebuggerPresent" );
		tmp = CryptString( "GetCurrentProcessId" );
		tmp = CryptString( "OpenProcess" );
		tmp = CryptString( "CloseHandle" );
		tmp = CryptString( "KERNEL32.DLL" );
		tmp = CryptString( "FindWindowA" );
		tmp = CryptString( "FindWindowW" );
		tmp = CryptString( "USER32.DLL" );
		tmp = CryptString( "NtQueryInformationProcess" );
		tmp = CryptString( "NtSetInformationThread" );
		tmp = CryptString( "NTDLL.DLL" );
		tmp = CryptString( "Debugger/Lockout/class" );
		tmp = CryptString( "Debugger/Lockout/Window Title" );
		tmp = CryptString( "AddTimerExx" );
		tmp = CryptString( "SACK_GetProfileString" );
	}
#endif

	if( !isDebuggerPresent )
	{
		if( !output2 )
			_DecryptRawData( sKernel32+1, sKernel32[0], &output2, &outsize2 );
		_DecryptRawData( sIsDebuggerPresent + 1, sIsDebuggerPresent[0], &output, &outsize );
		isDebuggerPresent = (BOOL (__stdcall*)(void))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
		Release( output );
	}
	if( isDebuggerPresent() )
	{
		BAG_Exit( 0xD1E );
		(*(int*)0x13) = 0;
	}

	{
		BOOL pbIsPresent;
		HANDLE hProcess;
		if( !checkRemoteDebuggerPresent )
		{
			if( !output2 )
				_DecryptRawData( sKernel32+1, sKernel32[0], &output2, &outsize2 );
			if( !getCurrentProcess )
			{
				_DecryptRawData( sGetCurrentProcess + 1, sGetCurrentProcess[0], &output, &outsize );
				getCurrentProcess = (HANDLE (__stdcall*)(void))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
				Release( output );
			}
			_DecryptRawData( sCheckRemoteDebuggerPresent + 1, sCheckRemoteDebuggerPresent[0], &output, &outsize );
			checkRemoteDebuggerPresent = (BOOL (__stdcall*)( HANDLE , PBOOL ))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
			Release( output );

			_DecryptRawData( sGetCurrentProcessId + 1, sGetCurrentProcessId[0], &output, &outsize );
			getCurrentProcessId = (DWORD (__stdcall*)( void ))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
			Release( output );

			_DecryptRawData( sOpenProcess + 1, sOpenProcess[0], &output, &outsize );
			openProcess = (HANDLE (__stdcall*)( DWORD , BOOL , DWORD ))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
			Release( output );

			_DecryptRawData( sCloseHandle + 1, sCloseHandle[0], &output, &outsize );
			closeHandle = (BOOL (__stdcall*)( HANDLE ))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
			Release( output );
		}
		hProcess = openProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, getCurrentProcessId());
		if( checkRemoteDebuggerPresent( hProcess, &pbIsPresent ) )
		{
			if( pbIsPresent )
			{
				BAG_Exit( 0xD1E );
				(*(int*)0x17) = 0;
			}
		}
		else
		{
			//int err = GetLastError();
		}
		closeHandle( hProcess );
	}
	/*
	SetLastError( 0xbab1f1ea );
	OutputDebugString( "Are you a debugger?" );
	if( GetLastError() != 0xbab1f1ea )
	{
		BAG_Exit( 0xD1E );
		(*(int*)0x15) = 0;
	}
	*/
	{
		// test for various window class names
		TEXTCHAR classname[64];
		int n = 1;
		TEXTCHAR key[12];
		if( !findWindow )
		{
			if( !output4 )
				_DecryptRawData( sUser32 + 1, sUser32[0], &output4, &outsize4 );
#ifdef _UNICODE
			_DecryptRawData( sFindWindowW + 1, sFindWindowW[0], &output, &outsize );
#else
			_DecryptRawData( sFindWindowA + 1, sFindWindowA[0], &output, &outsize );
#endif
			findWindow = (HWND(__stdcall*)(CTEXTSTR,CTEXTSTR))LoadFunction( (CTEXTSTR)output4, (CTEXTSTR)output );
			Release( output );
		}
		_DecryptRawData( sDebugWndClassLocks + 1, sDebugWndClassLocks[0], &output, &outsize );
		do
		{
			tnprintf( key, 12, WIDE("%d"), n );
			SACK_GetProfileString( (CTEXTSTR)output, key, WIDE(""), classname, 64 );
			if( classname[0] )
				if( findWindow( classname, NULL ) )
				{
					BAG_Exit( 0xD1E );
					(*(int*)0x1b) = 0;
				}
		} while( classname[0] );
		Release( output );
		n = 1;
		_DecryptRawData( sDebugWndNameLocks + 1, sDebugWndNameLocks[0], &output, &outsize );
		do
		{
			tnprintf( key, 12, WIDE("%d"), n );
			SACK_GetProfileString( (CTEXTSTR)output, key, WIDE(""), classname, 64 );
			if( classname[0] )
			{
				if( findWindow( NULL, classname ) )
				{
					BAG_Exit( 0xD1E );
					(*(int*)0x1c) = 0;
				}
			}
		} while( classname[0] );
		Release( output );
	}
	{
		// check if debuggers are attached
		static int (APIENTRY *_NtQueryInformationProcess)();
		int status;
		int retVal;
		int debugFlag;
		if( !_NtQueryInformationProcess )
		{
			_DecryptRawData( sNtQueryInformationProcess + 1, sNtQueryInformationProcess[0], &output, &outsize );
			_DecryptRawData( sNTDll + 1, sNTDll[0], &output3, &outsize3 );
			
			_NtQueryInformationProcess = (int(APIENTRY*)())LoadFunction( (CTEXTSTR)output3, (CTEXTSTR)output );
		}
		if( _NtQueryInformationProcess )
		{
			status = _NtQueryInformationProcess( -1, 0x07, &retVal, 4, NULL );
			if( retVal != 0 ) {
				BAG_Exit( 0xD1E );
				(*(int*)0x19) = 0;
			}
			status = _NtQueryInformationProcess( -1, 0x1F/*DebugProcessFlags*/, &debugFlag, 4, NULL ); 
			if( debugFlag == 0 ) {
				BAG_Exit( 0xD1E );
				(*(int*)0x1d) = 0;
			}
		}
	}
	//if( 1 )
	{
		// hide from debugger....
		// detach a debugger.
		enum tic { zero };
		static int (APIENTRY *_NtSetInformationThread)(HANDLE,enum tic,PVOID,ULONG);

		if( !_NtSetInformationThread )
		{
			_DecryptRawData( sNtSetInformationThread + 1, sNtSetInformationThread[0], &output, &outsize );
			if( !output3 )
				_DecryptRawData( sNTDll + 1, sNTDll[0], &output3, &outsize3 );
			
			_NtSetInformationThread = (int(APIENTRY*)(HANDLE,enum tic,PVOID,ULONG))LoadFunction( (CTEXTSTR)output3, (CTEXTSTR)output );
			Release( output );
		}
		if( !getCurrentThread )
		{
			_DecryptRawData( sGetCurrentThread + 1, sGetCurrentThread[0], &output, &outsize );
			if( !output2 )
				_DecryptRawData( sKernel32 + 1, sKernel32[0], &output2, &outsize2 );
			
			getCurrentThread = (HANDLE(APIENTRY*)(void))LoadFunction( (CTEXTSTR)output2, (CTEXTSTR)output );
			Release( output );
		}
		if( _NtSetInformationThread )
		{
			_NtSetInformationThread( getCurrentThread(), (enum tic)0x11, 0, 0 ); 
		}	}

#if SELF_ATTACH
	if( 0 )
	{
		// this actually requires to be another process
		// a process cannot attach to itself as a 
		WORD pid = getCurrentProcessId(); 
		int success;
		/*
		_itow_s((int)pid, (wchar_t*)&pid_str, 8, 10); 
		wcsncat_s((wchar_t*)&szCmdline, 64, (wchar_t*)pid_str, 4); 
		success = CreateProcess(path, szCmdline, NULL, NULL, 
								FALSE, 0, NULL, NULL, &si, &pi); 
		*/
		//success = DebugActiveProcess(pid); 
		if (!success) { 
			BAG_Exit( 0xD1E );
			(*(int*)0x21) = 0;
		}
	}
#endif
	if( output2 )
		Release( output2 );
	if( output3 )
		Release( output3 );
	if( output4 )
		Release( output4 );
	return addTimer;
#endif
}

static void CPROC TickDebugger( PTRSZVAL psv )
{
	CheckDebugger();
}

PUBLIC( PTRSZVAL, dlopen )( CTEXTSTR name )
{
	LoadFunction( name, NULL );
	return (PTRSZVAL)strdup( name );
}
PUBLIC( void, dlclose )( PTRSZVAL lib )
{
	UnloadFunction( (generic_function*)lib );
}
PUBLIC( PTRSZVAL, dlsym )( PTRSZVAL psvLib, PTRSZVAL psvName )
{
	return (PTRSZVAL)LoadFunction( (CTEXTSTR)psvLib, (CTEXTSTR)psvName );
}
PUBLIC( const char *, dlerror )( void )
{
	return "No error information available.";
}

static void CPROC name(void); 
#ifdef _MSC_VER
	static int CPROC schedule_name(void);   
	static __declspec(allocate(_STARTSEG_)) int (CPROC*x_name)(void) = schedule_name; 
	int CPROC schedule_name(void) {
		RegisterPriorityStartupProc( name,TOSTR(name),100,x_name,"dl.c",__LINE__ );
		return 0;
	}
#endif
#ifdef __WATCOMC__
   static void schedule_name(void);
	static struct rt_init __based(__segname("XI")) name_ctor_label={0,(DEADSTART_PRELOAD_PRIORITY-32),schedule_name};
	void schedule_name(void) {
		RegisterPriorityStartupProc( name,TOSTR(name),100,&name_ctor_label,"dl.c",__LINE__ );
	}
#endif
#ifdef __GNUC__
	static void schedule_name(void) __attribute__((constructor)) __attribute__((used));
#  ifdef _DEBUG
#     define EXTRA ,"dl.c",__LINE__
#  else
#     define EXTRA
#  endif
	void schedule_name(void) { RegisterPriorityStartupProc( name,TOSTR(name),100,schedule_name EXTRA ); }
#endif
	/*static __declspec(allocate(_STARTSEG_)) void (CPROC*pointer_##name)(void) = pastejunk(schedule_,name);*/ \
static void CPROC name(void)
{
	_32 (CPROC *addTimerExx)( _32 start, _32 frequency
					, TimerCallbackProc callback
					, PTRSZVAL user DBG_PASS);
	addTimerExx = CheckDebugger();
	if( addTimerExx )
		addTimerExx( 0, 5000, TickDebugger, 0 DBG_SRC );
}

#ifdef __WATCOMC__
PUBLIC( void, ExportThis )( void )
{
}
#endif
