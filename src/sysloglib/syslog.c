/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2017++
 *
 *   created to provide standard logging features
 *   lprintf( format, ... ); simple, basic
 *   if DEBUG, then logs to a file of the same name as the program
 *   if RELEASE most of this logging goes away at compile time.
 *
 *  standardized to never use int.
 *
 * see also - include/logging.h
 *
 */

//#define SUPPORT_LOG_ALLOCATE
//#define DEFAULT_OUTPUT_STDERR
//#define DEFAULT_OUTPUT_STDOUT

#define COMPUTE_CPU_FREQUENCY
#define NO_UNICODE_C
//#undef UNICODE

#ifdef __LCC__
#include <intrinsics.h>
#endif

#include <time.h>

#ifdef __LINUX__
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/un.h> // struct sockaddr_un
#endif

#include <stdhdrs.h>
#include <timers.h>
#include <loadsock.h>
#include <time.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef WIN32
#ifndef _ARM_
#include <io.h> // unlink
#endif
#endif
#include <stdio.h>
#include <deadstart.h>

#include <idle.h>
#include <logging.h>
#include <filesys.h>
// okay this brings TIGHT integration.... but standardization for core levels.
#include <filesys.h>
#include <procreg.h>
#ifndef __NO_OPTIONS__
#include <sqlgetoption.h>
#endif
#ifdef __cplusplus
#include <cstdio>
LOGGING_NAMESPACE
#endif


#ifndef _SH_DENYWR
#  define _SH_COMPAT 0x00
#  define _SH_DENYRW 0x10
#  define _SH_DENYWR 0x20
#  define _SH_DENYRD 0x30
#  define _SH_DENYNO 0x40
#  define _SH_SECURE 0x80
#endif

struct syslog_local_data {
int cannot_log;
#define cannot_log (*syslog_local).cannot_log
uint64_t cpu_tick_freq;
#define cpu_tick_freq (*syslog_local).cpu_tick_freq
// (*syslog_local).flags that control the operation of system logging....
struct state_flags{
	BIT_FIELD bInitialized : 1;
	BIT_FIELD bUseDay : 1;
	BIT_FIELD bUseDeltaTime : 1;
	BIT_FIELD bLogTime : 1;
	BIT_FIELD bLogHighTime : 1;
	BIT_FIELD bLogCPUTime : 1;
	BIT_FIELD bProtectLoggedFilenames : 1;
	BIT_FIELD bLogProgram : 1;
	BIT_FIELD bLogThreadID : 1;
	BIT_FIELD bLogOpenAppend : 1;
	BIT_FIELD bLogOpenBackup : 1;
	BIT_FIELD bLogSourceFile : 1;
	BIT_FIELD bOptionsLoaded : 1;
	BIT_FIELD group_ok : 1;
} flags;

// a conserviative minimalistic configuration...
//} (*syslog_local).flags = { 0,0,1,0,1,0,1,1,0};

 TEXTCHAR *pProgramName;
#define pProgramName (*syslog_local).pProgramName
 UserLoggingCallback UserCallback;
//#define User1Callback (*syslog_local).UserCallback

 enum syslog_types logtype;
#define logtype (*syslog_local).logtype

 uint32_t nLogLevel; // default log EVERYTHING
#define nLogLevel (*syslog_local).nLogLevel
 uint32_t nLogCustom; // bits enabled and disabled for custom mesasges...
#define nLogCustom (*syslog_local).nLogCustom
 CTEXTSTR gFilename;// = "LOG";
#define gFilename (*syslog_local).gFilename
 FILE *file;
 SOCKET   hSock;
#define hSock (*syslog_local).hSock
 SOCKET   hSyslogdSock;
 int bCPUTickWorks; // assume this works, until it fails
#define bCPUTickWorks (*syslog_local).bCPUTickWorks
 uint64_t tick_bias;
 uint64_t lasttick;
 uint64_t lasttick2;

 LOGICAL bStarted;
 LOGICAL bLogging;
 LOGICAL bSyslogdLogging;

	PLINKQUEUE buffers;
#if defined( WIN32 )
	DWORD next_lprintf_tls;
#elif defined( __LINUX__ )
	pthread_key_t next_lprintf_tls;
#endif
};

#ifdef __ANDROID__
//#  if !USE_CUSTOM_ALLOCER
//#    define __STATIC_GLOBALS__
//#  endif //!USE_CUSTOM_ALLOCER
#endif

#ifndef __STATIC_GLOBALS__
struct syslog_local_data *syslog_local;
#else
struct syslog_local_data _syslog_local;
struct syslog_local_data *syslog_local = &_syslog_local;
#endif

static void free_next_info( void );
static void DoSystemLog( const TEXTCHAR *buffer );
//----------------------------------------------------------------------------

// we should really wait until the very end to cleanup?
PRIORITY_ATEXIT( CleanSyslog, ATEXIT_PRIORITY_SYSLOG )
{
	enum syslog_types _logtype;
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
		return;
#endif
	_logtype = logtype;
	if( ( _logtype == SYSLOG_AUTO_FILE && (*syslog_local).file ) || ( _logtype == SYSLOG_NONE ) )
		lprintf( "Final log - syslog clos(ing)ed." );


	pProgramName = NULL; // this was dynamic allocated memory, and it is now gone.
	logtype = SYSLOG_NONE;
	switch( _logtype )
	{
	case SYSLOG_FILE:  // usually this is stderr ... don't do anything
		break;
	case SYSLOG_FILENAME:
		sack_fclose( (*syslog_local).file );
		break;
#ifdef __LINUX__
#  ifndef __DISABLE_SYSLOGD_SYSLOG__
	case SYSLOG_SOCKET_SYSLOGD:
		closesocket( (*syslog_local).hSyslogdSock );
		(*syslog_local).hSyslogdSock = INVALID_SOCKET;
		break;
#  endif
#endif
#ifndef __DISABLE_UDP_SYSLOG__
	case SYSLOG_UDP:
	case SYSLOG_UDPBROADCAST:
		closesocket( hSock );
		hSock = INVALID_SOCKET;
		break;
#endif
	default:
		// else... no resources to cleanup
		break;
	}
}


#if 0
        /*
         * this code would ideally check to see if
         * the cpu rdtsc instruction worked....
         * someday we should consider using the rdtscp instruction
         * but that will require fetching CPU characteristics
         * - SEE mmx.asm in src/imglib/
         */
void TestCPUTick( void )
{
	uint64_t tick, _tick;
	int n;
	bCPUTickWorks = 1;
	_tick = tick = GetCPUTick();
	for( n = 0; n < 10000000; n++ )
	{
#ifdef GCC
		//asm( "cpuid\n" );
#endif
		tick = GetCPUTick();
		if( tick > _tick )
		{
			//lprintf( "%020Ld %020Ld", _tick, tick );
			_tick = tick;
		}
		else
		{
			lprintf( "CPU TICK FAILED!" );
			bCPUTickWorks = 0;
			break;
		}
		Relinquish();
	}
}
#endif

#ifdef __WATCOMC__
unsigned __int64 rdtsc( void);
#pragma aux rdtsc = 0x0F 0x31 value [edx eax] parm nomemory modify exact [edx eax] nomemory;
//#pragma aux GetCPUTicks3 = "rdtsc"   "mov dword ptr tick, eax"   	"mov dword ptr tick+4, edx "
#endif

uint64_t GetCPUTick(void )
{
/*
 * being the core of CPU tick layer type stuff
 * this should result in ticks, and fail ticks
 * to return reasonable defaults...
 * I guess there should be a tick_base to result
 * the same type of number when it does go backwards
 */
	if( bCPUTickWorks )
	{
#if defined( __LCC__ )
		return _rdtsc();
#elif defined( __WATCOMC__ )
		uint64_t tick = rdtsc();
		if( !(*syslog_local).lasttick )
			(*syslog_local).lasttick = tick;
		else if( tick < (*syslog_local).lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			(*syslog_local).tick_bias = (*syslog_local).lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick = (*syslog_local).lasttick + 1; // more than prior, but no longer valid.
		}
		(*syslog_local).lasttick = tick;
		return tick;
#elif defined( _MSC_VER )
#  ifdef _M_CEE_PURE
		//return System::DateTime::now;
		return 0;
#  else
#   if defined( _WIN64 )
		uint64_t tick = __rdtsc();
#   else
		static uint64_t tick;
#     if _ARM_
		tick = tick+1;
#     else
		_asm rdtsc;
		_asm mov dword ptr [tick], eax;
		_asm mov dword ptr [tick + 4], edx;
#     endif
#   endif
		if( !(*syslog_local).lasttick )
			(*syslog_local).lasttick = tick;
		else if( tick < (*syslog_local).lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			(*syslog_local).tick_bias = (*syslog_local).lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick = (*syslog_local).lasttick + 1; // more than prior, but no longer valid.
		}
		(*syslog_local).lasttick = tick;
		return tick;
#  endif
#elif defined( __GNUC__ ) && !defined( __arm__ ) && !defined( __aarch64__ ) && !defined( __asmjs__ )
		union {
			uint64_t tick;
			PREFIX_PACKED struct { uint32_t low, high; } PACKED parts;
		}tick;
#ifndef PEDANTIC_TEST
		asm( "rdtsc\n" : "=a"(tick.parts.low), "=d"(tick.parts.high) );
#endif
		if( !(*syslog_local).lasttick )
			(*syslog_local).lasttick = tick.tick;
		else if( tick.tick < (*syslog_local).lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			(*syslog_local).tick_bias = (*syslog_local).lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick.tick = (*syslog_local).lasttick + 1; // more than prior, but no longer valid.
		}
		(*syslog_local).lasttick = tick.tick;
		return tick.tick;
#else
		DebugBreak();
#endif
	}
	return (*syslog_local).tick_bias + (timeGetTime()/*GetTickCount()*/ * 1000);
}

uint64_t GetCPUFrequency( void )
{
#ifdef COMPUTE_CPU_FREQUENCY
	{
		uint64_t cpu_tick, _cpu_tick;
		uint32_t tick, _tick;
		cpu_tick = _cpu_tick = GetCPUTick();
		tick = _tick = timeGetTime()/*GetTickCount()*/;
		cpu_tick_freq = 0;
		while( bCPUTickWorks && ( ( tick = timeGetTime()/*GetTickCount()*/ ) - _tick ) < 25 );
		cpu_tick = GetCPUTick();
		if( bCPUTickWorks )
			cpu_tick_freq = ( ( cpu_tick - _cpu_tick ) / ( tick - _tick ) )  / 1000; // microseconds;
	}
#else
	cpu_tick_freq = 1;
#endif
	return cpu_tick_freq;
}

void SetDefaultName( CTEXTSTR path, CTEXTSTR name, CTEXTSTR extra )
{
	TEXTCHAR *newpath;
	size_t len;
	static CTEXTSTR filepath;// = GetProgramPath();
	static CTEXTSTR filename;// = GetProgramName();
	if( path )
	{
		if( filepath )
			Release( (POINTER)filepath );
		filepath = StrDup( path );
	}
	if( name )
	{
		if( filename )
			Release( (POINTER)filename );
		filename = StrDup( name );
	}
	if( !filepath ) {
#ifdef _WIN32		
		filepath = ExpandPath( "*/" );
#else
		filepath = ExpandPath( ";/" );
#endif
	}
	if( !filename )
		filename = StrDup( GetProgramName() );
	if( !filename )
		filename = "org.d3x0r.sack";
	// this has to come from C heap.. my init isn't done yet probably and
	// sharemem will just fai(*syslog_local).  (it's probably trying to log... )
	newpath = (TEXTCHAR*)malloc( len = sizeof(TEXTCHAR)*(9 + StrLen( filepath ) + StrLen( filename ) + (extra?StrLen(extra):0) + 5) );
#ifdef __cplusplus_cli
	tnprintf( newpath, len, "%s%s%s.cli.log", filepath, filename, extra?extra:"" );
#else
	tnprintf( newpath, len, "%s%s%s.log", filepath, filename, extra?extra:"" );
#endif
	gFilename = newpath;//( newpath ); // use the C heap.
	//free( newpath ); // get rid of this ...
}

#ifndef __NO_OPTIONS__
static void LoadOptions( void )
{
	if( !(*syslog_local).flags.bOptionsLoaded )
	{
		(*syslog_local).flags.bLogSourceFile = SACK_GetProfileIntEx( GetProgramName()
																 , "SACK/Logging/Log Source File"
																 , (*syslog_local).flags.bLogSourceFile, TRUE );

#ifndef __ANDROID__
		// android has a system log that does just fine/ default startup sets that.

		if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Enable System Log"
										, 0
										, TRUE ) )
		{
			logtype = SYSLOG_SYSTEM;
			(*syslog_local).flags.bLogProgram = 1;
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Enable File Log"
										, ( logtype == SYSLOG_AUTO_FILE )
										, TRUE ) )
		{
			//logtype = SYSLOG_AUTO_FILE;
			(*syslog_local).flags.bLogOpenAppend = 0;
			(*syslog_local).flags.bLogOpenBackup = 1;
			(*syslog_local).flags.bLogProgram = 1;
		}
		// set all default parts of the name.
		// this overrides options with options available from SQL database.
		if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Default Log Location is current directory", 0, TRUE ) )
		{
			// override filepath, if log exception.
			TEXTCHAR buffer[256];
			GetCurrentPath( buffer, sizeof( buffer ) );
			SetDefaultName( buffer, NULL, NULL );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Default Log Location is exectuable directory", 0, TRUE ) )
		{
			SetDefaultName( GetProgramPath(), NULL, NULL );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Default Log Location is common data directory", 1, TRUE ) )
		{
			SetDefaultName( NULL, NULL, NULL );
		}
		else
		{
			TEXTCHAR buffer[256];
			// if this is blank, then length result from getprofilestring is 0, and default is with the program.
			// so I'll lave functionality as expected for a default.
			SACK_GetProfileStringEx( GetProgramName(), "SACK/Logging/Default Log Location", "", buffer, sizeof( buffer ), TRUE );
			if( buffer[0] )
			{
				SetDefaultName( buffer, NULL, NULL );
			}
		}
#endif

		if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Send Log to UDP", 0, TRUE ) )
		{
			if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Broadcast UDP", 0, TRUE ) )
				logtype = SYSLOG_UDPBROADCAST;
			else
				logtype = SYSLOG_UDP;
		}
		nLogLevel = SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Default Log Level (1001:all, 100:least)", nLogLevel, TRUE );

		// use the defaults; they may be overriden by reading the options.
		(*syslog_local).flags.bLogThreadID = SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Thread ID", (*syslog_local).flags.bLogThreadID, TRUE );
		(*syslog_local).flags.bLogProgram = SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Program", (*syslog_local).flags.bLogProgram, TRUE );
		(*syslog_local).flags.bLogSourceFile = SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Source File", (*syslog_local).flags.bLogSourceFile, TRUE );

		if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log CPU Tick Time and Delta", (*syslog_local).flags.bLogCPUTime, TRUE ) )
		{
			SystemLogTime( SYSLOG_TIME_CPU|SYSLOG_TIME_DELTA );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Time as Delta", (*syslog_local).flags.bUseDeltaTime, TRUE ) )
		{
			SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Time", (*syslog_local).flags.bLogTime, TRUE ) )
		{
			if( SACK_GetProfileIntEx( GetProgramName(), "SACK/Logging/Log Date", (*syslog_local).flags.bUseDay, TRUE ) )
			{
				SystemLogTime( SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
			}
			else
				SystemLogTime( SYSLOG_TIME_HIGH );
		}
		else
			SystemLogTime( 0 );
		(*syslog_local).flags.bOptionsLoaded = 1;
	}
}
#endif


//static int init_complete;
void InitSyslog( int ignore_options )
{
#ifndef __STATIC_GLOBALS__
	if( syslog_local )
	{
#ifndef __NO_OPTIONS__
		if( !ignore_options )
			LoadOptions();
#endif
		return;
	}
	SimpleRegisterAndCreateGlobal( syslog_local );

#endif
	if( !(*syslog_local).flags.bInitialized )
	{
		//logtype = SYSLOG_FILE;
		//(*syslog_local).file = stderr;
#if defined( WIN32 )
		(*syslog_local).next_lprintf_tls = TlsAlloc();
#elif defined( __LINUX__ )
		pthread_key_create( &((*syslog_local).next_lprintf_tls), NULL );
#endif
		(*syslog_local).flags.bLogThreadID = 1;
		hSock = INVALID_SOCKET;
		(*syslog_local).hSyslogdSock = INVALID_SOCKET;
		bCPUTickWorks = 1;
		nLogLevel = LOG_NOISE-1; // default log EVERYTHING
#ifdef __ANDROID__
		{
			logtype = SYSLOG_SYSTEM;
			nLogLevel = LOG_NOISE + 1000; // default log EVERYTHING
			(*syslog_local).flags.bLogSourceFile = 1;
			(*syslog_local).flags.bUseDeltaTime = 1;
			(*syslog_local).flags.bLogCPUTime = 1;
			(*syslog_local).flags.bLogThreadID = 1;
			(*syslog_local).flags.bLogProgram = 0;
			SystemLogTime( SYSLOG_TIME_HIGH );
		}
#else
#  if defined( _DEBUG ) || defined( DEFAULT_OUTPUT_STDERR ) || defined( DEFAULT_OUTPUT_STDOUT )
		{
#    if defined( __LINUX__ ) && 0
			logtype = SYSLOG_SOCKET_SYSLOGD;
			(*syslog_local).flags.bLogProgram = 1;

#    else
			/* using SYSLOG_AUTO_FILE option does not require this to be open.
			* it is opened on demand.
			*/
#      if !defined( DEFAULT_OUTPUT_STDERR ) &&  !defined( DEFAULT_OUTPUT_STDOUT )
			logtype = SYSLOG_AUTO_FILE;
			(*syslog_local).flags.bLogOpenBackup = 1;
#      else
			logtype = SYSLOG_FILE;
#        if defined( DEFAULT_OUTPUT_STDERR )
			(*syslog_local).file = stderr;
#        else
			(*syslog_local).file = stdout;
#        endif

			(*syslog_local).flags.bLogOpenBackup = 0;
#      endif
			(*syslog_local).flags.bUseDeltaTime = 1;
			(*syslog_local).flags.bLogCPUTime = 1;
			(*syslog_local).flags.bUseDeltaTime = 1;
			(*syslog_local).flags.bLogCPUTime = 1;
			(*syslog_local).flags.bLogProgram = 0;
			SystemLogTime( SYSLOG_TIME_HIGH );
#    endif
			nLogLevel = LOG_NOISE + 1000; // default log EVERYTHING
			(*syslog_local).flags.bLogOpenAppend = 0;
			(*syslog_local).flags.bLogSourceFile = 1;
			(*syslog_local).flags.bLogThreadID = 1;
			//SetDefaultName( NULL, NULL, NULL );
			//lprintf( "Syslog Initializing, debug mode, startup programname.log\n" );
		}
#  else
		// stderr?
		logtype = SYSLOG_NONE;
		(*syslog_local).file = NULL;
#  endif
#endif

		(*syslog_local).flags.bInitialized = 1;
	}
#ifndef __NO_OPTIONS__
	if( !ignore_options )
		LoadOptions();
#else
	(*syslog_local).flags.bOptionsLoaded = 1;
#  ifndef __ANDROID__
	SetDefaultName( NULL, NULL, NULL );
#  endif
#endif
}

PRIORITY_PRELOAD( InitSyslogPreload, SYSLOG_PRELOAD_PRIORITY )
{
	InitSyslog( 1 );
}

// delay reading options (unless we had to because of a logging requirement) but all core
// logging should be disabled (usually) until after init.
// that will allow these to be set with interface.conf defaults.
// but still fairly early...
PRIORITY_PRELOAD( InitSyslogPreloadWithOptions, NAMESPACE_PRELOAD_PRIORITY + 1 )
{
	InitSyslog( 0 );
	OnThreadExit( free_next_info );
}

PRIORITY_PRELOAD( InitSyslogPreloadAllowGroups, DEFAULT_PRELOAD_PRIORITY + 1 )
{
	(*syslog_local).flags.group_ok = 1;
}

//----------------------------------------------------------------------------
CTEXTSTR GetTimeEx( int bUseDay )
{
	/* used by sqlite extension to support now() */
#ifdef _WIN32
#  ifndef WIN32
#    define WIN32 _WIN32
#  endif
#endif

#if defined( WIN32 ) && !defined( __ANDROID__ )
	static TEXTCHAR timebuffer[256];
	SYSTEMTIME st;
	GetLocalTime( &st );

	if( bUseDay )
		tnprintf( timebuffer, sizeof(timebuffer), "%02d/%02d/%d %02d:%02d:%02d"
		       , st.wMonth, st.wDay, st.wYear
		       , st.wHour, st.wMinute, st.wSecond );
	else
		tnprintf( timebuffer, sizeof(timebuffer), "%02d:%02d:%02d"
		       , st.wHour, st.wMinute, st.wSecond );
	return timebuffer;

#else
	static char timebuffer[256];
	struct tm *timething;
	time_t timevalnow;
	time(&timevalnow);
	timething = localtime( &timevalnow );
	strftime( timebuffer
				, sizeof( timebuffer )
				, (bUseDay)?"%m/%d/%Y %H:%M:%S":"%H:%M:%S"
			  , timething );
#endif
	return timebuffer;
}

CTEXTSTR GetTime( void )
{
	return GetTimeEx( (*syslog_local).flags.bUseDay );
}



CTEXTSTR GetPackedTime( void )
{
	/* used by sqlite extension to support now() */
#ifdef _WIN32
#  ifndef WIN32
#    define WIN32 _WIN32
#  endif
#endif

#if defined( WIN32 ) && !defined( __ANDROID__ )
	static TEXTCHAR timebuffer[256];
	SYSTEMTIME st;
	GetLocalTime( &st );

	tnprintf( timebuffer, sizeof(timebuffer), "%04d%02d%02d%02d%02d%02d"
	        , st.wYear
	        , st.wMonth, st.wDay
	        , st.wHour, st.wMinute, st.wSecond );

#else
	static char timebuffer[256];
	struct tm *timething;
	time_t timevalnow;
	time(&timevalnow);
	timething = localtime( &timevalnow );
	strftime( timebuffer
	        , sizeof( timebuffer )
	        , "%Y%m%d%H%M%S"
	        , timething );
#endif
	return timebuffer;
}

int GetTimeZone( void ){
    time_t gmt, rawtime = time(NULL);
    struct tm *ptm;

#if !defined(WIN32)
    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);
#else
    ptm = gmtime(&rawtime);
#endif
    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);
	{
		int seconds = (int)difftime( rawtime, gmt );
		int sign = 1;
		if( seconds < 0 ) {
			sign = -1;
			seconds = -seconds;
		}
		return sign * (((seconds / 60 / 60) * 100) + ((seconds / 60) % 60));
	}
}

#if 0
#ifdef _WIN32
	{
		static int isSet;
		static int tz;
		if( isSet ) return tz;
		// Get the local system time.

		{
			DWORD dwType;
			DWORD dwValue;
			DWORD dwSize = sizeof( dwValue );
			HKEY hTemp;
			DWORD dwStatus;

			dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE
			                       , "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", 0
			                       , KEY_READ, &hTemp );

			if( (dwStatus == ERROR_SUCCESS) && hTemp )
			{
				dwSize = sizeof( dwValue );
				dwStatus = RegQueryValueEx(hTemp, "ActiveTimeBias", 0
				                          , &dwType
				                          , (PBYTE)&dwValue
				                          , &dwSize );

				RegCloseKey( hTemp );
			}
			else
				dwValue = 0;
			//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation
			// Get the timezone info.
			//TIME_ZONE_INFORMATION TimeZoneInfo;
			//GetTimeZoneInformation( &TimeZoneInfo );

			// Convert local time to UTC.
			//TzSpecificLocalTimeToSystemTime( &TimeZoneInfo,
			//								 &LocalTime,
			//								 &GmtTime );
			// Local time expressed in terms of GMT bias.
			{
				timebuf->zhr = (int8_t)( -( (int)dwValue/60 ) ) ;
				timebuf->zmn = (dwValue>0)?( dwValue % 60 ):((-dwValue)%60);
			}
			tz = (int)dwValue;
			isSet = TRUE;
			return tz;
		}
	}
#endif
}
#endif

void ConvertTickToTime( int64_t tick, PSACK_TIME st ) {
	int8_t tz = (int8_t)tick;
	int sign = (tz < 0) ? -1 : 1;
	if( tz < 0 ) tz = -tz;
#ifdef _WIN32
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
	tick >>= 8;
	tick *= 10000LL;
	tick += EPOCH;
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	SYSTEMTIME  system_time;
	FILETIME    file_time;

	file_time.dwLowDateTime = tick & 0xFFFFFFFF;
	file_time.dwHighDateTime = ( tick >> 32 ) & 0xFFFFFFFF;
	FileTimeToSystemTime( &file_time, &system_time );

	st->yr = system_time.wYear;
	st->mo = (uint8_t)system_time.wMonth;
	st->dy = (uint8_t)system_time.wDay;
	st->hr = (uint8_t)system_time.wHour;
	st->mn = (uint8_t)system_time.wMinute;
	st->sc = (uint8_t)system_time.wSecond;
	st->ms = system_time.wMilliseconds;
	st->zhr = sign* (( tz * 15 ) / 60);
	st->zmn = (tz*15) % 60;
#else
	struct timeval tv;
	struct tm tm;
	tv.tv_sec = ( tick >> 8 ) / 1000;
	tv.tv_usec =  ( ( tick >> 8 ) % 1000 ) * 1000;
	localtime_r( &tv.tv_sec, &tm );

	st->yr = tm.tm_year + 1900;
	st->mo = tm.tm_mon+1;
	st->dy = tm.tm_mday;
	st->hr = tm.tm_hour;
	st->mn = tm.tm_min;
	st->sc = tm.tm_sec;
	st->ms = tv.tv_usec / 1000;
	st->zhr = sign* (( tz * 15 ) / 60);
	st->zmn = (tz*15) % 60;
#endif
}


int64_t GetTimeOfDay( uint64_t* tick, int8_t* ptz )
{
	//struct timezone tzp;
	int tz = GetTimeZone();
	if( tz < 0 )
		tz = -(((-tz / 100) * 60) + (-tz % 100)) / 15; // -840/15 = -56
	else
		tz = (((tz / 100) * 60) + (tz % 100)) / 15; // -840/15 = -56  720/15 = 48

	tick[0] = timeGetTime64ns();
	tick[0] += (int64_t)tz * 900 * (int64_t)1000000000;
	ptz[0] = tz;
	return ( tick[0] /1000000 )<<8 | (tz&0xFF);

#ifdef _WIN32
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime( &system_time );
	SystemTimeToFileTime( &system_time, &file_time );
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;
	return (((uint64_t)((time - EPOCH) / 10000L)) << 8) | (tz & 0xFF);
#else
	{
		struct timeval tp;
		gettimeofday( &tp, NULL );
		return  (((uint64_t)(tp.tv_sec * 1000L) + (uint64_t)(tp.tv_usec)) << 8) | (tz & 0xFF);
	}
#endif
}



int64_t ConvertTimeToTick( PSACK_TIME st ) {
	int tz;
	int sign = st->zhr < 0 ? -1 : 1;
	tz = sign * (((sign*st->zhr * 60) + st->zmn) / 15);
#ifdef _WIN32
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;
	system_time.wYear = st->yr;
	system_time.wMonth = st->mo;
	system_time.wDay = st->dy;
	system_time.wHour = st->hr;
	system_time.wMinute = st->mn;
	system_time.wSecond = st->sc;
	system_time.wMilliseconds = st->ms;
	SystemTimeToFileTime( &system_time, &file_time );

	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	return (((uint64_t)((time - EPOCH) / 10000L)) << 8) | (tz & 0xFF);
#else
	struct tm t;
	time_t t_of_day;

	t.tm_year = st->yr - 1900;
	t.tm_mon = st->mo-1;           // Month, 0 - jan
	t.tm_mday = st->dy;          // Day of the month
	t.tm_hour = st->hr;
	t.tm_min = st->mn;
	t.tm_sec = st->sc;
	t.tm_isdst = 0;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
	t_of_day = timegm( &t );

	return ((((int64_t)t_of_day) * 1000ULL + st->ms) << 8) | (tz&0xFF);
#endif
}



//----------------------------------------------------------------------------
#ifndef BCC16 // no gettime of day - no milliseconds
static TEXTCHAR *GetTimeHigh( void )
{
#  if defined WIN32 && !defined( __ANDROID__ )
	 static TEXTCHAR timebuffer[256];
	static SYSTEMTIME _st;
	SYSTEMTIME st, st_save;

	if( (*syslog_local).flags.bUseDeltaTime )
	{
		GetLocalTime( &st );
		st_save = st;
		if( !_st.wYear )
			_st = st;
		st.wMilliseconds -= _st.wMilliseconds;
		if( st.wMilliseconds & 0x8000 )
		{
			st.wMilliseconds = (st.wMilliseconds+1000) & 0xFFFF;
			st.wSecond--;
		}
		st.wSecond -= _st.wSecond;
		if( st.wSecond & 0x8000 )
		{
			st.wSecond += 60;
			st.wMinute--;
		}
		st.wMinute -= _st.wMinute;
		if( st.wMinute & 0x8000 )
		{
			st.wMinute += 60;
			st.wHour--;
		}
		st.wHour -= _st.wHour;
		if( st.wHour & 0x8000 )
			st.wHour += 24;
		_st = st_save;
	}
	else
		GetLocalTime( &st );

	if( (*syslog_local).flags.bUseDay )
		tnprintf( timebuffer, sizeof(timebuffer), "%02d/%02d/%d %02d:%02d:%02d.%03d"
		        , st.wMonth, st.wDay, st.wYear
		        , st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
	else
		tnprintf( timebuffer, sizeof(timebuffer), "%02d:%02d:%02d.%03d"
		        , st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
#  else
	static char timebuffer[256];
	static struct timeval _tv;
	static struct tm _tm;
	struct timeval tv, tv_save;
	struct tm *timething, tm, tm_save;
	int len;
	gettimeofday( &tv, NULL );
	if( (*syslog_local).flags.bUseDeltaTime )
	{
		tv_save = tv;
		timething = localtime( &tv.tv_sec );
		tm = tm_save = *timething;
		if( !_tm.tm_year )
		{
			_tm = *timething;
			_tv = tv;
		}
		tv.tv_usec -= _tv.tv_usec;
		if( tv.tv_usec < 0 )
		{
			tv.tv_usec += 1000000;
			tm.tm_sec--;
		}
		tm.tm_sec -= _tm.tm_sec;
		if( tm.tm_sec < 0 )
		{
			tm.tm_sec += 60;
			tm.tm_min--;
		}
		tm.tm_min -= _tm.tm_min;
		if( tm.tm_min < 0 )
		{
			tm.tm_min += 60;
			tm.tm_hour--;
		}
		tm.tm_hour -= _tm.tm_hour;
		if( tm.tm_hour < 0 )
			tm.tm_hour += 24;

		_tm = tm_save;
		_tv = tv_save;
	}
	else
	{
		timething = localtime( &tv.tv_sec );
		tm = *timething;
	}

	len = strftime( timebuffer
	               , sizeof( timebuffer )
	               , ((*syslog_local).flags.bUseDay)?"%m/%d/%Y %H:%M:%S":"%H:%M:%S"
					  , &tm );
#  undef snprintf
	snprintf( timebuffer + len, 5, ".%03ld", tv.tv_usec / 1000 );
	/*
	// this code is kept in case borland's compiler don't like it.
	{
		time_t timevalnow;
		time(&timevalnow);
		timething = localtime( &timevalnow );
		strftime( timebuffer
		        , sizeof( timebuffer )
		        , "%m/%d/%Y %H:%M:%S.000"
		        , timething );
	}
	*/
#  endif
	return timebuffer;
}
#else
#  define GetTimeHigh GetTime
#endif

uint32_t ConvertTickToMicrosecond( uint64_t tick )
{
	if( bCPUTickWorks )
	{
		if( !cpu_tick_freq )
			GetCPUFrequency();
		if( !cpu_tick_freq )
			return 0;
		return (uint32_t)(tick / cpu_tick_freq);
	}
	else
		return (uint32_t)tick;
}


void PrintCPUDelta( TEXTCHAR *buffer, size_t buflen, uint64_t tick_start, uint64_t tick_end )
{
#ifdef COMPUTE_CPU_FREQUENCY
	if( !cpu_tick_freq )
		GetCPUFrequency();
	if( cpu_tick_freq )
		tnprintf( buffer, buflen, "%" _64f ".%03" _64f
				 , ((tick_end-tick_start) / cpu_tick_freq ) / 1000
				 , ((tick_end-tick_start) / cpu_tick_freq ) % 1000
				 );
	else
#endif
		tnprintf( buffer, buflen, "%" _64fs, tick_end - tick_start
		   	 );
}

static TEXTCHAR *GetTimeHighest( void )
{
	uint64_t tick;
	static TEXTCHAR timebuffer[64];
	tick = GetCPUTick();
	if( !(*syslog_local).lasttick2 )
		(*syslog_local).lasttick2 = tick;
	if( (*syslog_local).flags.bUseDeltaTime )
	{
#ifdef UNICODE
		size_t ofs = 0;
		tnprintf( timebuffer, sizeof( timebuffer ), "%20lld" " ", tick );
		ofs += StrLen( timebuffer );
#else
		int ofs = tnprintf( timebuffer, sizeof( timebuffer ), "%20" _64fs " ", tick );
#endif
		PrintCPUDelta( timebuffer + ofs, sizeof( timebuffer ) - ofs, (*syslog_local).lasttick2, tick );
		(*syslog_local).lasttick2 = tick;
	}
	else
		tnprintf( timebuffer, sizeof( timebuffer ), "%20" _64fs, tick );
	// have to find a generic way to get this from _asm( rdtsc );
	return timebuffer;
}

static CTEXTSTR GetLogTime( void )
{
	if( (*syslog_local).flags.bLogTime )
	{
		if( (*syslog_local).flags.bLogHighTime )
		{
			return GetTimeHigh();
		}
		else if( (*syslog_local).flags.bLogCPUTime )
		{
			return GetTimeHighest();
		}
		else
		{
			return GetTime();
		}
	}
	return "";
}

//----------------------------------------------------------------------------
#ifndef __DISABLE_UDP_SYSLOG__

#  if !defined( FBSD ) && !defined(__QNX__)
#    if defined( __MAC__ )
static SOCKADDR saLogBroadcast  = { 8, 2, { 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff } };
static SOCKADDR saLog  = { 8, 2, { 0x02, 0x02, 0x7f, 0x00, 0x00, 0x01 } };
static SOCKADDR saBind = { 8, 2, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
#    else
static SOCKADDR saLogBroadcast  = { 2, { 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff } };
static SOCKADDR saLog  = { 2, { 0x02, 0x02, 0x7f, 0x00, 0x00, 0x01 } };
static SOCKADDR saBind = { 2, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
#    endif
#  else
static SOCKADDR saLogBroadcast  = { 2, 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff };
static SOCKADDR saLog  = { 2, 0x02, 0x02, 0x7f, 0x00, 0x00, 0x01  };
static SOCKADDR saBind = { 2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  };
#  endif

static void UDPSystemLog( const TEXTCHAR *message )
{
#ifdef HAVE_IDLE
	while( (*syslog_local).bLogging )
		Idle();
#endif
	(*syslog_local).bLogging = 1;
	if( !(*syslog_local).bStarted )
	{
#ifdef _WIN32
#ifndef MAKEWORD
#define MAKEWORD(a,b) (((a)<<8)|(b))
#endif
		WSADATA ws;  // used to start up the socket services...
		if( WSAStartup( MAKEWORD(1,1), &ws ) )
		{
			(*syslog_local).bLogging = 0;
			return;
		}
#endif
		(*syslog_local).bStarted = TRUE;
	}
	if( hSock == INVALID_SOCKET )
	{
		LOGICAL bEnable = TRUE;
		hSock = socket(PF_INET,SOCK_DGRAM,0);
		if( hSock == INVALID_SOCKET )
		{
			(*syslog_local).bLogging = 0;
			return;
		}
		if( bind(hSock,&saBind,sizeof(SOCKADDR)) )
		{
			closesocket( hSock );
			hSock = INVALID_SOCKET;
			(*syslog_local). bLogging = 0;
			return;
		}
#ifndef BCC16
		if( setsockopt( hSock, SOL_SOCKET
		              , SO_BROADCAST, (char*)&bEnable, sizeof( bEnable ) ) )
		{
			Log( "Failed to set sock opt - BROADCAST" );
		}
#endif
	}
	{
		//INDEX nSent;
		int nSend;
		static TEXTCHAR realmsg[1024];
		nSend = tnprintf( realmsg, sizeof( realmsg ), /*"[%s]"*/ "%s"
				  //, pProgramName
		                , message );
		message = realmsg;
#ifdef __cplusplus_cli
		char *tmp = CStrDup( realmsg );
#  define SENDBUF tmp
#else
#  define SENDBUF message
#endif
		sendto( hSock, (const char *)SENDBUF, nSend, 0
		      , (logtype == SYSLOG_UDPBROADCAST)
		        ? &saLogBroadcast
		        : &saLog
		      , sizeof( SOCKADDR ) );
#ifdef __cplusplus_cli
		Release( tmp );
#endif
		if( logtype != SYSLOG_UDPBROADCAST )
			Relinquish(); // allow logging agents time to pick this up...
	}
	(*syslog_local).bLogging = 0;
}
#endif

//----------------------------------------------------------------------------
#ifdef __LINUX__
#  ifndef __DISABLE_SYSLOGD_SYSLOG__

#    if !defined( FBSD ) && !defined(__QNX__)
#       if defined( __MAC__ )
static struct sockaddr_un saSyslogdAddr  = { 11, AF_UNIX, "/dev/log" };
#       else
static struct sockaddr_un saSyslogdAddr  = { AF_UNIX, "/dev/log" };
#       endif
#    else
static struct sockaddr_un saSyslogdAddr  = { AF_UNIX, {"/dev/log"} };
#    endif

static void SyslogdSystemLog( const TEXTCHAR *message )
{
	while( (*syslog_local).bSyslogdLogging )
#    ifdef HAVE_IDLE
		Idle();
#    else
		Relinquish();
#    endif
	//fprintf( stderr, "present." );
	(*syslog_local).bSyslogdLogging = 1;
	if( (*syslog_local).hSyslogdSock == INVALID_SOCKET )
	{
		(*syslog_local).hSyslogdSock = socket(AF_UNIX,SOCK_DGRAM,0);
		if( (*syslog_local).hSyslogdSock == INVALID_SOCKET )
		{
			//fprintf( stderr, "failed..." );
			(*syslog_local).bSyslogdLogging = 0;
			return;
		}
		if(connect((*syslog_local).hSyslogdSock,(struct sockaddr *)&saSyslogdAddr,sizeof(saSyslogdAddr)) )
		{
			//fprintf( stderr, "failed..." );
			closesocket( (*syslog_local).hSyslogdSock );
			(*syslog_local).hSyslogdSock = INVALID_SOCKET;
			(*syslog_local).bSyslogdLogging = 0;
			return;
		}
	}
	if( (*syslog_local).hSyslogdSock != INVALID_SOCKET )
	{
		if( send( (*syslog_local).hSyslogdSock, message, StrLen( message ), 0 ) == 0 )
		{
			//fprintf( stderr, "failed..." );
			closesocket( (*syslog_local).hSyslogdSock );
			(*syslog_local).hSyslogdSock = INVALID_SOCKET;
		}
	}
	(*syslog_local).bSyslogdLogging = 0;
}
#  endif
#endif

#ifdef __LINUX__
//---------------------------------------------------------------------------
#  ifdef __EMSCRIPTEN_
LOGICAL IsBadReadPtr( CPOINTER pointer, uintptr_t len )
{
   return FALSE;
}
#  else
//---------------------------------------------------------------------------

LOGICAL IsBadReadPtr( CPOINTER pointer, uintptr_t len )
{
   (void)len; // reference unused.
	static FILE *maps;
	//return FALSE;
	//DebugBreak();
	if( !maps )
		maps = fopen( "/proc/self/maps", "rt" );
	else
		fseek( maps, 0, SEEK_SET );
	//fprintf( stderr, "Testing a pointer..\n" );
	if( maps )
	{
		uintptr_t ptr = (uintptr_t)pointer;
		char line[256];
		while( fgets( line, sizeof(line)-1, maps ) )
		{
			size_t low, high;
			sscanf( line, "%zd-%zd" cPTRSZVALfx, &low, &high );
			//fprintf( stderr, "%s" "Find: %08" PTRSZVALfx " Low: %08" PTRSZVALfx " High: %08" PTRSZVALfx "\n"
			//		 , line, pointer, low, high );
			if( ptr >= low && ptr <= high )
			{
				return FALSE;
			}
		}
	}
	//fprintf( stderr, "%p is not valid. %d", pointer, errno );
	return TRUE;
}

//---------------------------------------------------------------------------
#endif // emscripten
#endif

static void FileSystemLog( CTEXTSTR message )
{
	if( (*syslog_local).file )
	{
#ifdef SUPPORT_LOG_ALLOCATE
		fputs( message, (*syslog_local).file );
		fputs( "\n", (*syslog_local).file );
		fflush( (*syslog_local).file );
#else
#  ifdef UNICODE
		fputws( message, (*syslog_local).file );
		fputws( "\n", (*syslog_local).file );
#  else
		sack_fputs( message, (*syslog_local).file );
		sack_fputs( "\n", (*syslog_local).file );
#  endif
		sack_fflush( (*syslog_local).file );
#endif
	}
}


static void BackupFile( const TEXTCHAR *source, int source_name_len, int n )
{
	FILE *testfile;
	INDEX group;
	testfile = sack_fsopenEx( group = GetFileGroup( "system.logs", GetProgramPath() ), source, "rt", _SH_DENYWR, NULL );
	if( testfile )
	{
		TEXTCHAR backup[256];
		sack_fclose( testfile );
		// move file to backup..
		tnprintf( backup, sizeof( backup ), "%*.*s.%d"
				  , source_name_len
				  , source_name_len
				  , source, n );
		if( n < 10 )
		{
			BackupFile( backup
							, source_name_len
							, n+1 );
		}
		else
			sack_unlink( group, source );
		//lprintf( "%s->%s", source, backup );
		sack_rename( source, backup );
	}
}


void DoSystemLog( const TEXTCHAR *buffer )
{
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		InitSyslog( 1 );
		if( logtype == SYSLOG_AUTO_FILE && !(*syslog_local).file )
		{
			// cannot log until system log is complete
			return;
		}
		if( ( logtype == SYSLOG_UDPBROADCAST ) || ( logtype == SYSLOG_UDP ) )
			return;
	}
#endif

#ifdef __LINUX__
#  ifndef __DISABLE_SYSLOGD_SYSLOG__
	if( logtype == SYSLOG_SOCKET_SYSLOGD )
		SyslogdSystemLog( buffer );
#  endif
#endif

#ifndef __DISABLE_UDP_SYSLOG__
	if( logtype == SYSLOG_UDP
		|| logtype == SYSLOG_UDPBROADCAST )
		UDPSystemLog( buffer );
#else
	if( 0 )
		;
#endif
	else if( ( logtype == SYSLOG_FILE ) || ( logtype == SYSLOG_AUTO_FILE ) )
	{
		if( logtype == SYSLOG_AUTO_FILE )
		{
			if( !(*syslog_local).file && gFilename )
			{
				int n_retry = 0;
			retry_again:
				logtype = SYSLOG_NONE; // disable logging - internal functions might inadvertantly log something...
				if(
#ifdef SUPPORT_LOG_ALLOCATE
					0 &&
#endif
					!(*syslog_local).flags.bOptionsLoaded )
				{
#ifdef SUPPORT_LOG_ALLOCATE
					(*syslog_local).file = fopen( gFilename, "wt" );
#else
					(*syslog_local).file = sack_fsopen( 0, gFilename, "wt"
#  ifdef _UNICODE
						", ccs=UNICODE"
#  endif
						, _SH_DENYWR );
#endif
				}
				else
				{
					if(
#ifdef SUPPORT_LOG_ALLOCATE
						0 &&
#endif
						(*syslog_local).flags.bLogOpenBackup )
					{
						BackupFile( gFilename, (int)StrLen( gFilename ), 1 );
					}
					else if( (*syslog_local).flags.bLogOpenAppend )
#ifdef SUPPORT_LOG_ALLOCATE
					(*syslog_local).file = fopen( gFilename, "at+" );
#else
					(*syslog_local).file = sack_fsopen( (*syslog_local).flags.group_ok?GetFileGroup( "system.logs", GetProgramPath() ):(INDEX)0, gFilename, "at+"
#  ifdef _UNICODE
						", ccs=UNICODE"
#  endif
						, _SH_DENYWR );
#endif
					if( (*syslog_local).file )
						fseek( (*syslog_local).file, 0, SEEK_END );
					else
#ifdef SUPPORT_LOG_ALLOCATE
					(*syslog_local).file = fopen( gFilename, "wt" );
#else
					(*syslog_local).file = sack_fsopenEx( (*syslog_local).flags.group_ok?GetFileGroup( "system.logs", GetProgramPath() ):(INDEX)0, gFilename, "wt"
#  ifdef _UNICODE
							", ccs=UNICODE"
#  endif
							, _SH_DENYWR, NULL );
#endif
				}
				//logtype = SYSLOG_AUTO_FILE;

				if( !(*syslog_local).file )
				{
					if( n_retry < 500 )
					{
						TEXTCHAR tmp[10];
						tnprintf( tmp, sizeof( tmp ), "%d", n_retry++ );
						SetDefaultName( NULL, NULL, tmp );
						goto retry_again;
					}
					else
						// can't open the logging file, stop trying now, will save us trouble in the future
						logtype = SYSLOG_NONE;
				}
				logtype = SYSLOG_AUTO_FILE;
			}
		}
		FileSystemLog( buffer );
	}
#if defined( _WIN32 ) || defined( __ANDROID__ )
	else if( logtype == SYSLOG_SYSTEM )
	{
#  ifdef __cplusplus_cli
		// requires referenced xperdex.classes... if this doesn't compile, please add the reference
		//xperdex::classes::Log::log( gcnew System::String( buffer ) );
		//System::Console::WriteLine( gcnew System::String( buffer ) );
		//System::Diagnostics::Debug
#  else
#    ifdef __ANDROID__
		{
			static char *program_string;
			char *string = CStrDup( buffer );
			if( !program_string )
				program_string = CStrDup( GetProgramName() );
			if( !program_string )
				program_string = "com.unknown.app";
			__android_log_print( ANDROID_LOG_INFO, program_string, string );
			Release( string );
		}
#    else
		OutputDebugString( buffer );
		OutputDebugString( "\n" );
#    endif
#  endif
	}
	else
#endif
	if( logtype == SYSLOG_CALLBACK )
		(*syslog_local).UserCallback( buffer );
}

	static volatile uint32_t openLock;
	static volatile uint32_t lowLevelLock;
void SystemLogFL( const TEXTCHAR *message FILELINE_PASS )
{
	static TEXTCHAR buffer[4096];
	static TEXTCHAR threadid[32];
	static TEXTCHAR sourcefile[256];
	CTEXTSTR logtime;
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		if( !openLock ) {
			openLock = 1;
			InitSyslog( 1 );
		} else
			return;
		openLock = 0;
	}
#endif
	if( cannot_log )
		return;
	if( !(*syslog_local).flags.group_ok && openLock )
		return;
#ifdef WIN32
	while( InterlockedExchange( (long volatile*)&lowLevelLock, 1 ) ) Relinquish();
#else
	while( LockedExchange( &lowLevelLock, 1 ) ) Relinquish();
#endif
	logtime = GetLogTime();
	if( (*syslog_local).flags.bLogSourceFile && pFile )
	{
#ifndef _LOG_FULL_FILE_NAMES
		CTEXTSTR p;
		for( p = pFile + StrLen(pFile) -1;p > pFile;p-- )
			if( p[0] == '/' || p[0] == '\\' )
			{
				pFile = p+1;break;
			}
#endif
		tnprintf( sourcefile, sizeof( sourcefile ), "" FILELINE_FILELINEFMT  FILELINE_RELAY );
	}
	else
		sourcefile[0] = 0;
	if( (*syslog_local).flags.bLogThreadID )
		tnprintf( threadid, sizeof( threadid ), "%012" _64fX "~", GetThisThreadID() );
	if( pFile )
		tnprintf( buffer, sizeof( buffer )
				  , "%s%s%s%s%s%s%s"
				  , logtime, logtime[0]?"|":""
				  , (*syslog_local).flags.bLogThreadID?threadid:""
				  , (*syslog_local).flags.bLogProgram?(GetProgramName()):""
				  , (*syslog_local).flags.bLogProgram?"@":""
				  , (*syslog_local).flags.bLogSourceFile?sourcefile:""
				  , message );
	else
		tnprintf( buffer, sizeof( buffer )
				  , "%s%s%s%s%s%s"
				  , logtime, logtime[0]?"|":""
				  , (*syslog_local).flags.bLogThreadID?threadid:""
				  , (*syslog_local).flags.bLogProgram?(GetProgramName()):""
				  , (*syslog_local).flags.bLogProgram?"@":""
				  , message );
	DoSystemLog( buffer );
	lowLevelLock = 0;
}

#undef SystemLogEx
void SystemLogEx ( const TEXTCHAR *message DBG_PASS )
{
#ifdef _DEBUG
	SystemLogFL( message DBG_RELAY );
#else
	SystemLogFL( message FILELINE_NULL );
#endif
}

#undef SystemLog
 void  SystemLog ( const TEXTCHAR *message )
{
	SystemLogFL( message, NULL, 0 );
}

 void  LogBinaryFL ( const uint8_t* buffer, size_t size FILELINE_PASS )
{
	size_t nOut = size;
	const uint8_t* data = buffer;
#ifndef _LOG_FULL_FILE_NAMES
	if( pFile )
	{
		CTEXTSTR p;
		for( p = pFile + (pFile?StrLen(pFile) -1:0);p > pFile;p-- )
			if( p[0] == '/' || p[0] == '\\' )
			{
				pFile = p+1;break;
			}
	}
#endif
	// should make this expression something in signed_usigned_comparison...
	while( nOut && !( nOut & ( ((size_t)1) << ( ( sizeof( nOut ) * CHAR_BIT ) - 1 ) ) ) )
	{
		TEXTCHAR cOut[96];
		size_t ofs = 0;
		size_t x;
		ofs = 0;
		for ( x=0; x<nOut && x<16; x++ )
			ofs += tnprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, "%02X ", (unsigned char)data[x] );
		// space fill last partial buffer
		for( ; x < 16; x++ )
			ofs += tnprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, "   " );

		for ( x=0; x<nOut && x<16; x++ )
		{
			if( data[x] >= 32 && data[x] < 127 )
				ofs += tnprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, "%c", (unsigned char)data[x] );
			else
				ofs += tnprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, "." );
		}
		SystemLogFL( cOut FILELINE_RELAY );
		nOut -= x;
		data += x;
	}
}
#undef LogBinaryEx
 void  LogBinaryEx ( const uint8_t* buffer, size_t size DBG_PASS )
{
#ifdef _DEBUG
	LogBinaryFL( buffer,size DBG_RELAY );
#else
	LogBinaryFL( buffer,size FILELINE_NULL );
#endif
}
#undef LogBinary
 void  LogBinary ( const uint8_t* buffer, size_t size )
{
	LogBinaryFL( buffer,size, NULL, 0 );
}

void  SetSystemLog ( enum syslog_types type, const void *data )
{
	if( (*syslog_local).file && ( logtype != SYSLOG_FILE ) )
	{
		FILE *close_file = (*syslog_local).file;
		(*syslog_local).file = NULL;  // reset this first, in case logging closing.
      if( !( close_file == stderr || close_file == stdout ) )
			sack_fclose( close_file );
	}
	if( type == SYSLOG_FILE )
	{
		if( data )
		{
			logtype = type;
			(*syslog_local).file = (FILE*)data;
		}
	}
	else if( type == SYSLOG_FILENAME )
	{
		FILE *log;
		log = sack_fsopen( GetFileGroup( "system.logs", GetProgramPath() ), (CTEXTSTR)data, "wt"
#ifdef _UNICODE
				", ccs=UNICODE"
#endif
				, _SH_DENYWR );
		(*syslog_local).file = log;

		logtype = SYSLOG_FILE;
	}
	else if( type == SYSLOG_CALLBACK )
	{
		(*syslog_local).UserCallback = (UserLoggingCallback)(uintptr_t)data;
	}
	else
	{
		logtype = type;
	}
	//SystemLog( "thing is: " STRSYM( (SYSLOG_EXTERN) ) );
}

 void  SystemLogTime ( LOGICAL enable )
{
	(*syslog_local).flags.bLogTime = FALSE;
	(*syslog_local).flags.bUseDay = FALSE;
	(*syslog_local).flags.bUseDeltaTime = FALSE;
	(*syslog_local).flags.bLogHighTime = FALSE;
	(*syslog_local).flags.bLogCPUTime = FALSE;
	if( enable )
	{
		(*syslog_local).flags.bLogTime = TRUE;
		if( enable & SYSLOG_TIME_HIGH )
			(*syslog_local).flags.bLogHighTime = TRUE;
		if( enable & SYSLOG_TIME_LOG_DAY )
			(*syslog_local).flags.bUseDay = TRUE;
		if( enable & SYSLOG_TIME_DELTA )
			(*syslog_local).flags.bUseDeltaTime = TRUE;
		if( enable & SYSLOG_TIME_CPU )
			(*syslog_local).flags.bLogCPUTime = TRUE;
	}
}

// information for the call to _real_lprintf file and line information...
struct next_lprint_info{
	// please use this enter when resulting a function, and leave from said function.
	// oh - but then we couldn't exist before crit sec code...
	//CRITICALSECTION cs;
	uint32_t nLevel;
	CTEXTSTR pFile;
	int nLine;
};
#define next_lprintf (*_next_lprintf)
static struct next_lprint_info *GetNextInfo( void )
{
	struct next_lprint_info *next;
#ifdef USE_CUSTOM_ALLOCER
#  if defined( WIN32 )
	if( !( next = (struct next_lprint_info*)TlsGetValue( (*syslog_local).next_lprintf_tls ) ) )
		TlsSetValue( (*syslog_local).next_lprintf_tls, next = (struct next_lprint_info*)malloc( sizeof( struct next_lprint_info ) ) );
#  elif defined( __LINUX__ )
	if( !( next = (struct next_lprint_info*)pthread_getspecific( (*syslog_local).next_lprintf_tls ) ) )
		pthread_setspecific( (*syslog_local).next_lprintf_tls, next = (struct next_lprint_info*)malloc( sizeof( struct next_lprint_info ) ) );
#  endif
#else
#  if defined( WIN32 )
	if( !(next = (struct next_lprint_info*)TlsGetValue( (*syslog_local).next_lprintf_tls )) )
		TlsSetValue( (*syslog_local).next_lprintf_tls, next = New( struct next_lprint_info ) );
#  elif defined( __LINUX__ )
	if( !(next = (struct next_lprint_info*)pthread_getspecific( (*syslog_local).next_lprintf_tls )) )
		pthread_setspecific( (*syslog_local).next_lprintf_tls, next = New( struct next_lprint_info ) );
#  endif
#endif
	return next;
}

void free_next_info( void ) {
	struct next_lprint_info *next;

#ifdef USE_CUSTOM_ALLOCER
#  if defined( WIN32 )
	if( ( next = (struct next_lprint_info*)TlsGetValue( (*syslog_local).next_lprintf_tls ) ) ){
		TlsSetValue( (*syslog_local).next_lprintf_tls, NULL );
		free( next );
	}
#  elif defined( __LINUX__ )
	if( ( next = (struct next_lprint_info*)pthread_getspecific( (*syslog_local).next_lprintf_tls ) ) ) {
		pthread_setspecific( (*syslog_local).next_lprintf_tls, NULL );
		free( next );
	}
#  endif
#else
#  if defined( WIN32 )
	if( (next = (struct next_lprint_info*)TlsGetValue( (*syslog_local).next_lprintf_tls )) ) {
		TlsSetValue( (*syslog_local).next_lprintf_tls, NULL );
		Release( next );
	}
#  elif defined( __LINUX__ )
	if( (next = (struct next_lprint_info*)pthread_getspecific( (*syslog_local).next_lprintf_tls )) ) {
		pthread_setspecific( (*syslog_local).next_lprintf_tls, NULL );
		Release( next );
	}
#  endif
#endif
	
}

static INDEX CPROC _null_vlprintf ( CTEXTSTR format, va_list args )
{
   (void)format; // fix unused
   (void)args; // fix unused
	return 0;
}



static INDEX CPROC _real_vlprintf ( CTEXTSTR format, va_list args )
{
#ifdef _DEBUG
	// this can be used to force logging early to stdout
	struct next_lprint_info *_next_lprintf = GetNextInfo();
#endif
	if( cannot_log )
		return 0;
	if( logtype != SYSLOG_NONE )
	{
		CTEXTSTR logtime = GetLogTime();
		size_t ofs;
		// because of threading concerns... either I dynamically allocate this...
		// or lock it.... or ...
		TEXTCHAR *buffer;
		TEXTCHAR threadid[32];

		cannot_log = 1;
		if( !(*syslog_local).buffers )
		{
			int n;
			(*syslog_local).buffers = CreateLinkQueue();
			for( n = 0; n < 6; n++ )
				EnqueLink( &(*syslog_local).buffers, (POINTER)1 );
			for( n = 0; n < 6; n++ )
				DequeLink( &(*syslog_local).buffers );
		}

		buffer = (TEXTCHAR*)DequeLink( &(*syslog_local).buffers );
		if( !buffer )
		{
			//DoSystemLog( "Adding Logging Buffer" );
			buffer = NewArray( TEXTCHAR, 4096 );
		}
		// at this point we're not doing internal allocations...
		cannot_log = 0;

		if( logtime[0] )
#ifdef UNDER_CE
		{
			StringCbPrintf( buffer, 4096, "%s|"
							  , logtime );
			ofs = StrLen( buffer );
		}
#else
			ofs = tnprintf( buffer, 4095, "%s|"
							  , logtime );
#endif
		else
			ofs = 0;
		// argsize - the program's giving us file and line
		// debug for here or not, this must be used.
		if( (*syslog_local).flags.bLogThreadID )
			tnprintf( threadid, sizeof( threadid ), "%012" _64fX "~", GetThisThreadID() );
#ifdef UNDER_CE
		tnprintf( buffer + ofs, 4095 - ofs, "%s%s%s"
				  , (*syslog_local).flags.bLogThreadID?threadid:""
				  , (*syslog_local).flags.bLogProgram?GetProgramName():""
				  , (*syslog_local).flags.bLogProgram?"@":""
				  );
		ofs += StrLen( buffer + ofs );
#else
		tnprintf( buffer + ofs, 4095 - ofs, "%s%s%s"
				  , (*syslog_local).flags.bLogThreadID?threadid:""
				  , (*syslog_local).flags.bLogProgram?GetProgramName():""
				  , (*syslog_local).flags.bLogProgram?"@":""
				  );
		ofs += StrLen( buffer + ofs );
#endif
		{
#ifdef _DEBUG
			CTEXTSTR pFile;
#ifndef _LOG_FULL_FILE_NAMES
			CTEXTSTR p;
#endif
			uint32_t nLine;
			if( (*syslog_local).flags.bLogSourceFile && ( pFile = next_lprintf.pFile ) )
			{
				if( (*syslog_local).flags.bProtectLoggedFilenames )
					if( IsBadReadPtr( pFile, 2 ) )
						pFile = "(Unloaded file?)";
#   ifndef _LOG_FULL_FILE_NAMES
				for( p = pFile + StrLen(pFile) -1;p > pFile;p-- )
					if( p[0] == '/' || p[0] == '\\' )
					{
						pFile = p+1;break;
					}
#   endif
				nLine = next_lprintf.nLine;
#   ifdef UNDER_CE
				tnprintf( buffer + ofs, 4095 - ofs, "%s(%" _32f "):"
									, pFile, nLine );
				ofs += StrLen( buffer + ofs );
#   else
				tnprintf( buffer + ofs, 4095 - ofs, "%s(%" _32f "):"
									, pFile, nLine );
				ofs += StrLen( buffer + ofs );
#   endif
			}
#endif
#ifdef UNICODE
			vswprintf( buffer + ofs, 4095 - ofs, format, args );
#else
			vsnprintf( buffer + ofs, 4095 - ofs, format, args );
#endif
			// okay, so even the above is unsafe, because Micro$oft has
			// decreed to be stupid.
			buffer[4095] = 0;
		}
		DoSystemLog( buffer );

		{
			cannot_log = 1;
			EnqueLink( &(*syslog_local).buffers, buffer );
			cannot_log = 0;
		}
	}
	//LeaveCriticalSec( &next_lprintf.cs );
	return 0;
}

static INDEX CPROC _real_lprintf( CTEXTSTR f, ... )
{
	va_list args;
	va_start( args, f );
	return _real_vlprintf( f, args );
}

static INDEX CPROC _null_lprintf( CTEXTSTR f, ... )
{
   (void)f; // fix unused
	return 0;
}


RealVLogFunction  _vxlprintf ( uint32_t level DBG_PASS )
{
	struct next_lprint_info *_next_lprintf;
	//EnterCriticalSec( &next_lprintf.cs );
	_next_lprintf = GetNextInfo();
	next_lprintf.nLevel = level;
#if _DEBUG
	next_lprintf.pFile = pFile;
	next_lprintf.nLine = nLine;
#endif
	if( level & LOG_CUSTOM )
	{
		if( !(level & nLogCustom) )
			return _null_vlprintf;
		return _real_vlprintf;
	}
	else if( level <= nLogLevel )
	{
		return _real_vlprintf;
	}
	return _null_vlprintf;
}

RealLogFunction _xlprintf( uint32_t level DBG_PASS )
{
	struct next_lprint_info *_next_lprintf;
	//EnterCriticalSec( &next_lprintf.cs );
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		if( !openLock ) {
			openLock = 1;
			InitSyslog( 1 );
		} else
			return _null_lprintf;
		//return _null_lprintf;
		openLock = 0;
	}
#else
	if( !(*syslog_local).flags.bInitialized )
		InitSyslog(1);
#endif
	_next_lprintf = GetNextInfo();
#if _DEBUG
	next_lprintf.pFile = pFile;
	next_lprintf.nLine = nLine;
#endif
	if( level & LOG_CUSTOM )
	{
		if( !(level & nLogCustom) )
			return _null_lprintf;
		next_lprintf.nLevel = level;
		return _real_lprintf;
	}
	else if( level <= nLogLevel )
	{
		next_lprintf.nLevel = level;
		return _real_lprintf;
	}
	return _null_lprintf;
}

#ifdef __WATCOMC__
// # again - WATCOM Compiler warning here, function
// defined, but never referenced.  This is true,
// but when the linker comes around to it, it cares for the
// presense of this function in order to force floating support
// for printf and scanf, since this is module is passed ANY
// format and ANY paramter, it may require floating point.
// no special handling required for GCC, lcc, MSVC
static int f(void )
{
	extern int fltused_;
	return fltused_ + (int)f;
	//int n = fltused_; // force inclusion of math libs...
}
#endif

void ProtectLoggedFilenames( LOGICAL bEnable )
{
	(*syslog_local).flags.bProtectLoggedFilenames = bEnable;
}

void SetSystemLoggingLevel( uint32_t nLevel )
{
	if( nLevel & LOG_CUSTOM )
	{
		nLogCustom |= nLevel & ( LOG_CUSTOM_BITS );
	}
	else if( nLevel & LOG_CUSTOM_DISABLE )
	{
		nLogCustom &= ~( nLevel & ( LOG_CUSTOM_BITS ) );
	}
	else
		nLogLevel = nLevel;
}

void SetSyslogOptions( FLAGSETTYPE *options )
{
	// the mat operations don't turn into valid bitfield operators. (watcom)
	(*syslog_local).flags.bLogOpenAppend = TESTFLAG( options, SYSLOG_OPT_OPENAPPEND )?1:0; // open for append, else open for write //-V616
	(*syslog_local).flags.bLogOpenBackup = TESTFLAG( options, SYSLOG_OPT_OPEN_BACKUP )?1:0; // open for append, else open for write
	(*syslog_local).flags.bLogProgram = TESTFLAG( options, SYSLOG_OPT_LOG_PROGRAM_NAME )?1:0; // open for append, else open for write
	(*syslog_local).flags.bLogThreadID = TESTFLAG( options, SYSLOG_OPT_LOG_THREAD_ID )?1:0; // open for append, else open for write
	(*syslog_local).flags.bLogSourceFile = TESTFLAG( options, SYSLOG_OPT_LOG_SOURCE_FILE )?1:0; // open for append, else open for write
}

#ifdef __cplusplus_cli

static public ref class Log
{
public:
	static void log( System::String^ ouptut )
	{
				pin_ptr<const WCHAR> _output = PtrToStringChars(ouptut);
				TEXTSTR __ouptut = DupWideToText( _output );
		lprintf( "%s", __ouptut );
		Release( __ouptut );
	}
};
#endif

#ifdef __cplusplus
LOGGING_NAMESPACE_END
#endif


//---------------------------------------------------------------------------
// $Log: syslog.c,v $
// Revision 1.74  2005/05/30 11:56:36  d3x0r
// various fixes... working on psilib update optimization... various stabilitizations... also extending msgsvr functionality.
//
// < --- CUT ALL LOGGING --- >
//
// Revision 1.56  2003/12/04 10:41:30  panther
// remove watcom compile only option code
//
//
