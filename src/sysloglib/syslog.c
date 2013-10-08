/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
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
#define COMPUTE_CPU_FREQUENCY

//#undef UNICODE

#ifdef __LCC__
#include <intrinsics.h>
#endif

#ifdef __LINUX__
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <sys/un.h> // struct sockaddr_un
#endif

#include <stdhdrs.h>
#include <loadsock.h>
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
LOGGING_NAMESPACE
#endif

// Add thread local storage for next lprintf file/line...
#if defined( _MSC_VER )
#define HAS_TLS 1
#define ThreadLocal static __declspec(thread)
#endif
#if defined( GNUC )
#define HAS_TLS 1
#define ThreadLocal static __thread
#endif

#ifndef HAS_TLS
#define ThreadLocal
#endif

struct syslog_local_data {
int cannot_log;
#define cannot_log l.cannot_log
_64 cpu_tick_freq;
#define cpu_tick_freq l.cpu_tick_freq
// flags that control the operation of system logging....
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
#define flags l.flags

// a conserviative minimalistic configuration...
//} flags = { 0,0,1,0,1,0,1,1,0};

 TEXTCHAR *pProgramName;
#define pProgramName l.pProgramName
 UserLoggingCallback UserCallback;
#define UserCallback l.UserCallback

 enum syslog_types logtype;
#define logtype l.logtype

 _32 nLogLevel; // default log EVERYTHING
#define nLogLevel l.nLogLevel
 _32 nLogCustom; // bits enabled and disabled for custom mesasges...
#define nLogCustom l.nLogCustom
 CTEXTSTR gFilename;// = "LOG";
#define gFilename l.gFilename
 FILE *file;
 SOCKET   hSock;
#define hSock l.hSock
 SOCKET   hSyslogdSock;
 int bCPUTickWorks; // assume this works, until it fails
#define bCPUTickWorks l.bCPUTickWorks
 _64 tick_bias;
#define tick_bias l.tick_bias
 _64 lasttick;
 _64 lasttick2;
 
 LOGICAL bStarted;
 LOGICAL bLogging;
 LOGICAL bSyslogdLogging;

};

#ifndef __STATIC_GLOBALS__
struct syslog_local_data *syslog_local;
#define l (*syslog_local)
#else
struct syslog_local_data syslog_local;
#define l (syslog_local)
#endif


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
	if( ( _logtype == SYSLOG_AUTO_FILE && l.file ) || ( _logtype == SYSLOG_NONE ) )
		lprintf( WIDE( "Final log - syslog clos(ing)ed." ) );


	pProgramName = NULL; // this was dynamic allocated memory, and it is now gone.
	logtype = SYSLOG_NONE;
	switch( _logtype )
	{
	case SYSLOG_FILE:
	case SYSLOG_FILENAME:
		fclose( l.file );
		break;
#ifdef __LINUX__
#  ifndef __DISABLE_SYSLOGD_SYSLOG__
	case SYSLOG_SOCKET_SYSLOGD:
		closesocket( l.hSyslogdSock );
		l.hSyslogdSock = INVALID_SOCKET;
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

#ifndef __STATIC_GLOBALS__
	if( syslog_local )
	{
		Deallocate( struct syslog_local_data *, syslog_local );
		syslog_local = NULL;
	}
#endif
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
	_64 tick, _tick;
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

_64 GetCPUTick(void )
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
		_64 tick = rdtsc();
#ifndef __WATCOMC__
		// haha a nasty compiler trick to get the variable used
		// but it's also a 'meaningless expression' so watcom pukes.
		(1)?(0):(tick = 0);
#endif
		if( !l.lasttick )
			l.lasttick = tick;
		else if( tick < l.lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			tick_bias = l.lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick = l.lasttick + 1; // more than prior, but no longer valid.
		}
		l.lasttick = tick;
		return tick;
#elif defined( _MSC_VER )
#ifdef _M_CEE_PURE
		//return System::DateTime::now;
		return 0;
#else
#   if defined( _WIN64 )
		_64 tick = __rdtsc();
#   else
		static _64 tick;
#if _ARM_ 
		tick = tick+1;
#else
		_asm rdtsc;
		_asm mov dword ptr [tick], eax;
		_asm mov dword ptr [tick + 4], edx;
#endif
#   endif
		if( !l.lasttick )
			l.lasttick = tick;
		else if( tick < l.lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			tick_bias = l.lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick = l.lasttick + 1; // more than prior, but no longer valid.
		}
		l.lasttick = tick;
		return tick;
		if( !l.lasttick )
			l.lasttick = tick;
		else if( tick < l.lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			tick_bias = l.lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick = l.lasttick + 1; // more than prior, but no longer valid.
		}
		l.lasttick = tick;
		return tick;
#endif
#elif defined( __GNUC__ ) && !defined( __arm__ )
		union {
			_64 tick;
			PREFIX_PACKED struct { _32 low, high; } PACKED parts;
		}tick;
		asm( "rdtsc\n" : "=a"(tick.parts.low), "=d"(tick.parts.high) );
		if( !l.lasttick )
			l.lasttick = tick.tick;
		else if( tick.tick < l.lasttick )
		{
			bCPUTickWorks = 0;
			cpu_tick_freq = 1;
			tick_bias = l.lasttick - ( timeGetTime()/*GetTickCount()*/ * 1000 );
			tick.tick = l.lasttick + 1; // more than prior, but no longer valid.
		}
		l.lasttick = tick.tick;
		return tick.tick;
#else
		DebugBreak();
#endif
	}
	return tick_bias + (timeGetTime()/*GetTickCount()*/ * 1000);
}

_64 GetCPUFrequency( void )
{
#ifdef COMPUTE_CPU_FREQUENCY
	{
		_64 cpu_tick, _cpu_tick;
		_32 tick, _tick;
		cpu_tick = _cpu_tick = GetCPUTick();
		tick = _tick = timeGetTime()/*GetTickCount()*/;
		cpu_tick_freq = 0;
		while( bCPUTickWorks && ( ( tick = timeGetTime()/*GetTickCount()*/ ) - _tick ) < 25 );
		{
			cpu_tick = GetCPUTick();
		}
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
	if( !filepath )
		filepath = StrDup( GetProgramPath() );
	if( !filename )
      filename = StrDup( GetProgramName() );
	// this has to come from C heap.. my init isn't done yet probably and
   // sharemem will just fail.  (it's probably trying to log... )
	newpath = (TEXTCHAR*)malloc( len = sizeof(TEXTCHAR)*(9 + strlen( filepath ) + strlen( filename ) + (extra?strlen(extra):0) + 5) );
#ifdef __cplusplus_cli
	snprintf( newpath, len, WIDE("%s/%s%s.cli.log"), filepath, filename, extra?extra:WIDE("") );
#else
	snprintf( newpath, len, WIDE("%s/%s%s.log"), filepath, filename, extra?extra:WIDE("") );
#endif
	gFilename = newpath;//( newpath ); // use the C heap.
	//free( newpath ); // get rid of this ...
}

#ifndef __NO_OPTIONS__
static void LoadOptions( void )
{
	if( !flags.bOptionsLoaded )
	{
		flags.bLogSourceFile = SACK_GetProfileIntEx( GetProgramName()
																 , WIDE( "SACK/Logging/Log Source File")
																 , flags.bLogSourceFile, TRUE );

#ifndef __ANDROID__
      // android has a system log that does just fine/ default startup sets that.
#  ifndef __LINUX__ 
		if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Enable File Log" )
#    ifdef _DEBUG
										, 1
#    else
										, 0
#    endif
										, TRUE ) )
		{
			logtype = SYSLOG_AUTO_FILE;
			flags.bLogOpenAppend = 0;
			flags.bLogOpenBackup = 1;
			flags.bLogProgram = 1;
		}
#  endif
#endif
		// set all default parts of the name.
		// this overrides options with options available from SQL database.
		if( SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Logging/Default Log Location is current directory"), 0, TRUE ) )
		{
			// override filepath, if log exception.
			TEXTCHAR buffer[256];
			GetCurrentPath( buffer, sizeof( buffer ) );
			SetDefaultName( buffer, NULL, NULL );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Logging/Default Log Location is exectuable directory"), 1, TRUE ) )
		{
			SetDefaultName( GetProgramPath(), NULL, NULL );
		}
		else
		{
			TEXTCHAR buffer[256];
			// if this is blank, then length result from getprofilestring is 0, and default is with the program.
			// so I'll lave functionality as expected for a default.
			SACK_GetProfileStringEx( GetProgramName(), WIDE( "SACK/Logging/Default Log Location" ), WIDE( "" ), buffer, sizeof( buffer ), TRUE );
			if( buffer[0] )
			{
				SetDefaultName( buffer, NULL, NULL );
			}
		}

		if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Send Log to UDP" ), 0, TRUE ) )
		{
			if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Broadcast UDP" ), 0, TRUE ) )
				logtype = SYSLOG_UDPBROADCAST;
			else
				logtype = SYSLOG_UDP;
		}
		nLogLevel = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Default Log Level (1001:all, 100:least)" ), nLogLevel, TRUE );

		// use the defaults; they may be overriden by reading the options.
		flags.bLogThreadID = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Thread ID" ), flags.bLogThreadID, TRUE );
		flags.bLogProgram = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Program" ), flags.bLogProgram, TRUE );
		flags.bLogSourceFile = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Source File" ), flags.bLogSourceFile, TRUE );

		if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log CPU Tick Time and Delta" ), 0, TRUE ) )
		{
			SystemLogTime( SYSLOG_TIME_CPU|SYSLOG_TIME_DELTA );
		}
		else if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Time as Delta" ), 0, TRUE ) )
		{
			SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
		}
#  ifdef __ANDROID__
		else if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Time" ), 0, TRUE ) )
#  else
		else if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Time" ), 1, TRUE ) )
#  endif
		{
#  ifndef __ANDROID__
			if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Date" ), 0, TRUE ) )
#  else
			if( SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/Logging/Log Date" ), 1, TRUE ) )
#  endif
			{
				SystemLogTime( SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
			}
			else
				SystemLogTime( SYSLOG_TIME_HIGH );
		}
		else
			SystemLogTime( 0 );
		flags.bOptionsLoaded = 1;
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

	if( !flags.bInitialized )
#endif
	{
		logtype = SYSLOG_NONE;
		hSock = INVALID_SOCKET;
		l.hSyslogdSock = INVALID_SOCKET;
		bCPUTickWorks = 1;
		nLogLevel = LOG_NOISE-1; // default log EVERYTHING

#ifdef __ANDROID__
		{
			logtype = SYSLOG_SYSTEM;
			nLogLevel = LOG_NOISE + 1000; // default log EVERYTHING
			flags.bLogSourceFile = 1;
			flags.bUseDeltaTime = 1;
			flags.bLogCPUTime = 1;
			flags.bLogThreadID = 1;
			flags.bLogProgram = 0;
			SystemLogTime( SYSLOG_TIME_HIGH );
		}
#else
#  ifdef _DEBUG
		{
#    ifdef __LINUX__
			logtype = SYSLOG_SOCKET_SYSLOGD;
#    else
			/* using SYSLOG_AUTO_FILE option does not require this to be open.
			* it is opened on demand.
			*/
			logtype = SYSLOG_AUTO_FILE;
#    endif
			nLogLevel = LOG_NOISE + 1000; // default log EVERYTHING
			flags.bLogOpenAppend = 0;
			flags.bLogOpenBackup = 1;
			flags.bLogSourceFile = 1;
			flags.bUseDeltaTime = 1;
			flags.bLogCPUTime = 1;
			flags.bLogThreadID = 1;
			flags.bLogProgram = 0;
			SetDefaultName( NULL, NULL, NULL );
			SystemLogTime( SYSLOG_TIME_HIGH );
			//lprintf( WIDE("Syslog Initializing, debug mode, startup programname.log\n") );
		}
#  else
		// stderr?
		logtype = SYSLOG_NONE;
		l.file = NULL;
#  endif
#endif

		flags.bInitialized = 1;
	}
#ifndef __NO_OPTIONS__
	if( !ignore_options )
		LoadOptions();
#else
	flags.bOptionsLoaded = 1;
	SetDefaultName( NULL, NULL, NULL );
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
}

PRIORITY_PRELOAD( InitSyslogPreloadAllowGroups, DEFAULT_PRELOAD_PRIORITY )
{
   flags.group_ok = 1;
}

//----------------------------------------------------------------------------
CTEXTSTR GetTimeEx( int bUseDay )
{
	static TEXTCHAR timebuffer[256];
   /* used by sqlite extension to support now() */
#ifdef _WIN32
#ifndef WIN32
#define WIN32 _WIN32
#endif
#endif

#if defined( WIN32 ) && !defined( __ANDROID__ )
   SYSTEMTIME st;
	GetLocalTime( &st );

	if( bUseDay )
	   snprintf( timebuffer, sizeof(timebuffer), WIDE("%02d/%02d/%d %02d:%02d:%02d")
   	                  , st.wMonth, st.wDay, st.wYear
      	               , st.wHour, st.wMinute, st.wSecond );
	else
	   snprintf( timebuffer, sizeof(timebuffer), WIDE("%02d:%02d:%02d")
      	               , st.wHour, st.wMinute, st.wSecond );

#else
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
   return GetTimeEx( flags.bUseDay );
}



CTEXTSTR GetPackedTime( void )
{
	static TEXTCHAR timebuffer[256];
   /* used by sqlite extension to support now() */
#ifdef _WIN32
#ifndef WIN32
#define WIN32 _WIN32
#endif
#endif

#if defined( WIN32 ) && !defined( __ANDROID__ )
   SYSTEMTIME st;
	GetLocalTime( &st );

	snprintf( timebuffer, sizeof(timebuffer), WIDE("%04d%02d%02d%02d%02d%02d")
			  , st.wYear
			  , st.wMonth, st.wDay
			  , st.wHour, st.wMinute, st.wSecond );

#else
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

//----------------------------------------------------------------------------
#ifndef BCC16 // no gettime of day - no milliseconds
static TEXTCHAR *GetTimeHigh( void )
{
    static TEXTCHAR timebuffer[256];
#if defined WIN32 && !defined( __ANDROID__ )
	static SYSTEMTIME _st;
	SYSTEMTIME st, st_save;

	if( flags.bUseDeltaTime )
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

	if( flags.bUseDay )
		snprintf( timebuffer, sizeof(timebuffer), WIDE("%02d/%02d/%d %02d:%02d:%02d.%03d")
		        , st.wMonth, st.wDay, st.wYear
		        , st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
	else
		snprintf( timebuffer, sizeof(timebuffer), WIDE("%02d:%02d:%02d.%03d")
		        , st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
#else
	static struct timeval _tv;
	static struct tm _tm;
	struct timeval tv, tv_save;
	struct tm *timething, tm, tm_save;
	int len;
	gettimeofday( &tv, NULL );
	if( flags.bUseDeltaTime )
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
                  , (flags.bUseDay)?"%m/%d/%Y %H:%M:%S":"%H:%M:%S"
                  , &tm );
	sprintf( timebuffer + len, WIDE(".%03ld"), tv.tv_usec / 1000 );
   /*
    // this code is kept in case borland's compiler don't like it.
    {
    time_t timevalnow;
    time(&timevalnow);
    timething = localtime( &timevalnow );
    strftime( timebuffer
    , sizeof( timebuffer )
    , WIDE("%m/%d/%Y %H:%M:%S.000")
    , timething );
    }
    */
#endif
	return timebuffer;
}
#else
#define GetTimeHigh GetTime
#endif

_32 ConvertTickToMicrosecond( _64 tick )
{
	if( bCPUTickWorks )
	{
		if( !cpu_tick_freq )
			GetCPUFrequency();
		if( !cpu_tick_freq )
			return 0;
		return (_32)(tick / cpu_tick_freq);
	}
	else
      return (_32)tick;
}




void PrintCPUDelta( TEXTCHAR *buffer, size_t buflen, _64 tick_start, _64 tick_end )
{
#ifdef COMPUTE_CPU_FREQUENCY
	if( !cpu_tick_freq )
		GetCPUFrequency();
	if( cpu_tick_freq )
		snprintf( buffer, buflen, WIDE("%")_64fs WIDE(".%03") _64f
				 , ((tick_end-tick_start) / cpu_tick_freq ) / 1000
				 , ((tick_end-tick_start) / cpu_tick_freq ) % 1000
				 );
   else
#endif
		snprintf( buffer, buflen, WIDE("%")_64fs, tick_end - tick_start
		   	 );
}

static TEXTCHAR *GetTimeHighest( void )
{
	_64 tick;
	static TEXTCHAR timebuffer[64];
	tick = GetCPUTick();
	if( !l.lasttick2 )
		l.lasttick2 = tick;
	if( flags.bUseDeltaTime )
	{
#ifdef UNICODE
		size_t ofs = 0;
		snprintf( timebuffer, sizeof( timebuffer ), WIDE("%20lld") WIDE(" "), tick );
		ofs += StrLen( timebuffer );
#else
		int ofs = snprintf( timebuffer, sizeof( timebuffer ), WIDE("%20lld") WIDE(" "), tick );
#endif
		PrintCPUDelta( timebuffer + ofs, sizeof( timebuffer ) - ofs, l.lasttick2, tick );
		l.lasttick2 = tick;
	}
	else
		snprintf( timebuffer, sizeof( timebuffer ), WIDE("%20") _64fs, tick );
	// have to find a generic way to get this from _asm( rdtsc );
	return timebuffer;
}

static CTEXTSTR GetLogTime( void )
{
	if( flags.bLogTime )
	{
		if( flags.bLogHighTime )
		{
			return GetTimeHigh();
		}
		else if( flags.bLogCPUTime )
		{
			return GetTimeHighest();
		}
		else
		{
			return GetTime();
		}
	}
	return WIDE("");
}

//----------------------------------------------------------------------------
#ifndef __DISABLE_UDP_SYSLOG__

#if !defined( FBSD ) && !defined(__QNX__)
static SOCKADDR saLogBroadcast  = { 2, { 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff } };
static SOCKADDR saLog  = { 2, { 0x02, 0x02, 0x7f, 0x00, 0x00, 0x01 } };
static SOCKADDR saBind = { 2, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
#else
static SOCKADDR saLogBroadcast  = { 2, 0x02, 0x02, (char)0xff, (char)0xff, (char)0xff, (char)0xff };
static SOCKADDR saLog  = { 2, 0x02, 0x02, 0x7f, 0x00, 0x00, 0x01  };
static SOCKADDR saBind = { 2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  };
#endif

static void UDPSystemLog( const TEXTCHAR *message )
{
#ifdef HAVE_IDLE
	while( l.bLogging )
		Idle();
#endif
	l.bLogging = 1;
	if( !l.bStarted )
	{
#ifdef _WIN32
#ifndef MAKEWORD
#define MAKEWORD(a,b) (((a)<<8)|(b))
#endif
		WSADATA ws;  // used to start up the socket services...
		if( WSAStartup( MAKEWORD(1,1), &ws ) )
		{
			l.bLogging = 0;
			return;
		}
#endif
		l.bStarted = TRUE;
	}
	if( hSock == INVALID_SOCKET )
	{
		LOGICAL bEnable = TRUE;
		hSock = socket(PF_INET,SOCK_DGRAM,0);
		if( hSock == INVALID_SOCKET )
		{
			l.bLogging = 0;
			return;
		}
		if( bind(hSock,&saBind,sizeof(SOCKADDR)) )
		{
			closesocket( hSock );
			hSock = INVALID_SOCKET;
			l. bLogging = 0;
			return;
		}
#ifndef BCC16
		if( setsockopt( hSock, SOL_SOCKET
						  , SO_BROADCAST, (char*)&bEnable, sizeof( bEnable ) ) )
		{
			Log( WIDE("Failed to set sock opt - BROADCAST") );
		}
#endif
	}
	{
		INDEX nSent;
		int nSend;
		static TEXTCHAR realmsg[1024];
		nSend = snprintf( realmsg, sizeof( realmsg ), /*"[%s]"*/ WIDE("%s")
				  //, pProgramName
				  , message );
		message = realmsg;
#ifdef __cplusplus_cli
		char *tmp = CStrDup( realmsg );
#define SENDBUF tmp
#else
#define SENDBUF message
#endif
		nSent = sendto( hSock, (const char *)SENDBUF, nSend, 0
						  ,(logtype == SYSLOG_UDPBROADCAST)?&saLogBroadcast:&saLog, sizeof( SOCKADDR ) );
#ifdef __cplusplus_cli
		Release( tmp );
#endif
		if( logtype != SYSLOG_UDPBROADCAST )
			Relinquish(); // allow logging agents time to pick this up...
	}
	l.bLogging = 0;
}
#endif

//----------------------------------------------------------------------------
#ifdef __LINUX__
#  ifndef __DISABLE_SYSLOGD_SYSLOG__

#    if !defined( FBSD ) && !defined(__QNX__)
static struct sockaddr_un saSyslogdAddr  = { AF_UNIX, "/dev/log" };
#    else
static struct sockaddr_un saSyslogdAddr  = { AF_UNIX, {"/dev/log"} };
#    endif

static void SyslogdSystemLog( const TEXTCHAR *message )
{
	while( l.bSyslogdLogging )
#    ifdef HAVE_IDLE
		Idle();
#    else
		Relinquish();
#    endif
	//fprintf( stderr, "present." );
	l.bSyslogdLogging = 1;
	if( l.hSyslogdSock == INVALID_SOCKET )
	{
		LOGICAL bEnable = TRUE;
		l.hSyslogdSock = socket(AF_UNIX,SOCK_DGRAM,0);
		if( l.hSyslogdSock == INVALID_SOCKET )
		{
			//fprintf( stderr, "failed..." );
			l.bSyslogdLogging = 0;
			return;
		}
		if(connect(l.hSyslogdSock,(struct sockaddr *)&saSyslogdAddr,sizeof(saSyslogdAddr)) )
		{
			//fprintf( stderr, "failed..." );
			closesocket( l.hSyslogdSock );
			l.hSyslogdSock = INVALID_SOCKET;
			l.bSyslogdLogging = 0;
			return;
		}
	}
	if( l.hSyslogdSock != INVALID_SOCKET )
	{
		if( send( l.hSyslogdSock, message, strlen( message ), 0 ) == 0 )
		{
			//fprintf( stderr, "failed..." );
			closesocket( l.hSyslogdSock );
			l.hSyslogdSock = INVALID_SOCKET;
		}
	}
	l.bSyslogdLogging = 0;
}
#  endif
#endif

#ifdef __LINUX__
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

LOGICAL IsBadReadPtr( CPOINTER pointer, PTRSZVAL len )
{
	static FILE *maps;
	//return FALSE;
   //DebugBreak();
	if( !maps )
		maps = fopen( WIDE("/proc/self/maps"), WIDE("rt") );
	else
      fseek( maps, 0, SEEK_SET );
   //fprintf( stderr, WIDE("Testing a pointer..\n") );
	if( maps )
	{
      PTRSZVAL ptr = (PTRSZVAL)pointer;
		char line[256];
		while( fgets( line, sizeof(line)-1, maps ) )
		{
			PTRSZVAL low, high;
			sscanf( line, WIDE("%")PTRSZVALfx WIDE("-%") PTRSZVALfx, &low, &high );
			//fprintf( stderr, WIDE("%s") WIDE("Find: %08") PTRSZVALfx WIDE(" Low: %08") PTRSZVALfx WIDE(" High: %08") PTRSZVALfx WIDE("\n")
			//		 , line, pointer, low, high );
			if( ptr >= low && ptr <= high )
			{
				//fclose( maps );
				return FALSE;
			}
		}
		//fclose( maps );
	}
   //fprintf( stderr, WIDE("%p is not valid. %d"), pointer, errno );
   return TRUE;
}

//---------------------------------------------------------------------------
#endif

static void FileSystemLog( CTEXTSTR message )
{
	if( l.file )
	{
		fputs( message, l.file );
		fputs( WIDE("\n"), l.file );
		fflush( l.file );
	}
}


static void BackupFile( const TEXTCHAR *source, int source_name_len, int n )
{
	FILE *testfile;
	INDEX group;
#if  _MSC_VER >=1600 
	testfile = sack_fsopen( group = GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ), source, WIDE("rt"), _SH_DENYWR );
#else
	testfile = sack_fopen( group = GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ), source, WIDE("rt") );
#endif
	if( testfile )
	{
		TEXTCHAR backup[256];
		sack_fclose( testfile );
		// move file to backup..
		snprintf( backup, sizeof( backup ), WIDE("%*.*s.%d")
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
		//lprintf( WIDE( "%s->%s" ), source, backup );
		sack_rename( source, backup );
	}
}


void DoSystemLog( const TEXTCHAR *buffer )
{
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		InitSyslog( 1 );
		if( logtype == SYSLOG_AUTO_FILE && !l.file )
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
			if( !l.file && gFilename )
			{
				int n_retry = 0;
			retry_again:
				logtype = SYSLOG_NONE; // disable logging - internal functions might inadvertantly log something...
				if( !flags.bOptionsLoaded )
				{
#if  _MSC_VER >=1600 
					l.file = sack_fsopen( 0, gFilename, WIDE("wt")
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
						, _SH_DENYWR );
#else
					l.file = sack_fopen( 0, gFilename, WIDE("wt") 
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
						);
#endif
				}
				else
				{
				if( flags.bLogOpenBackup )
				{
					BackupFile( gFilename, (int)strlen( gFilename ), 1 );
				}
				else if( flags.bLogOpenAppend )
#if  _MSC_VER >=1600 
					l.file = sack_fsopen( flags.group_ok?GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ):(INDEX)0, gFilename, WIDE("at+")
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
						, _SH_DENYWR );
#else
					l.file = sack_fopen( flags.group_ok?GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ):(INDEX)0, gFilename, WIDE("at+") 
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
						);
#endif
				if( l.file )
					fseek( l.file, 0, SEEK_END );
				else
#if  _MSC_VER >=1600 
					l.file = sack_fsopen( flags.group_ok?GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ):(INDEX)0, gFilename, WIDE("wt")
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
							, _SH_DENYWR );
#else
					l.file = sack_fopen( flags.group_ok?GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ):(INDEX)0, gFilename, WIDE("wt") 
#ifdef _UNICODE 
							WIDE(", ccs=UNICODE")
#endif
							);
#endif
				}
				logtype = SYSLOG_AUTO_FILE;

				if( !l.file )
				{
					if( n_retry < 500 )
					{
						TEXTCHAR tmp[10];
						snprintf( tmp, sizeof( tmp ), WIDE("%d"), n_retry++ );
						SetDefaultName( NULL, NULL, tmp );
						goto retry_again;
					}
					else
						// can't open the logging file, stop trying now, will save us trouble in the future
						logtype = SYSLOG_NONE;
				}
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
		__android_log_print( ANDROID_LOG_INFO, GetProgramName(), buffer );
#    else
		OutputDebugString( buffer );
		OutputDebugString( WIDE("\n") );
#    endif
#  endif
	} 
	else 
#endif
	if( logtype == SYSLOG_CALLBACK )
		UserCallback( buffer );
}

void SystemLogFL( const TEXTCHAR *message FILELINE_PASS )
{
	static TEXTCHAR buffer[4096];
	static TEXTCHAR threadid[32];
	static TEXTCHAR sourcefile[256];
	CTEXTSTR logtime;
	static _32 lock;
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		InitSyslog( 1 );
	}
#endif
	if( cannot_log )
		return;
#ifdef WIN32
	while( InterlockedExchange( (long volatile*)&lock, 1 ) ) Relinquish();
#else
	while( LockedExchange( &lock, 1 ) ) Relinquish();
#endif
	logtime = GetLogTime();
	if( flags.bLogSourceFile && pFile )
	{
#ifndef _LOG_FULL_FILE_NAMES
		CTEXTSTR p;
		for( p = pFile + strlen(pFile) -1;p > pFile;p-- )
			if( p[0] == '/' || p[0] == '\\' )
			{
				pFile = p+1;break;
			}
#endif
		snprintf( sourcefile, sizeof( sourcefile ), WIDE("") FILELINE_FILELINEFMT  FILELINE_RELAY );
	}
	else
		sourcefile[0] = 0;
	if( flags.bLogThreadID )
		snprintf( threadid, sizeof( threadid ), WIDE("%012") _64fX WIDE("~"), GetMyThreadID() );
	if( pFile )
		snprintf( buffer, sizeof( buffer )
				  , WIDE("%s%s%s%s%s%s%s")
				  , logtime, logtime[0]?WIDE("|"):WIDE("")
				  , flags.bLogThreadID?threadid:WIDE("")
				  , flags.bLogProgram?(GetProgramName()):WIDE("")
				  , flags.bLogProgram?WIDE("@"):WIDE("")
				  , flags.bLogSourceFile?sourcefile:WIDE("")
				  , message );
	else
		snprintf( buffer, sizeof( buffer )
				  , WIDE("%s%s%s%s%s%s")
				  , logtime, logtime[0]?WIDE("|"):WIDE("")
				  , flags.bLogThreadID?threadid:WIDE("")
				  , flags.bLogProgram?(GetProgramName()):WIDE("")
				  , flags.bLogProgram?WIDE("@"):WIDE("")
				  , message );
	DoSystemLog( buffer );
	lock = 0;
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

 void  LogBinaryFL ( P_8 buffer, size_t size FILELINE_PASS )
{
	size_t nOut = size;
	P_8 data = buffer;
#ifndef _LOG_FULL_FILE_NAMES
	if( pFile )
	{
		CTEXTSTR p;
		for( p = pFile + (pFile?strlen(pFile) -1:0);p > pFile;p-- )
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
			ofs += snprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, WIDE("%02X "), (unsigned char)data[x] );
		// space fill last partial buffer
		for( ; x < 16; x++ )
			ofs += snprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, WIDE("   ") );

		for ( x=0; x<nOut && x<16; x++ )
		{
			if( data[x] >= 32 && data[x] < 127 )
				ofs += snprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, WIDE("%c"), (unsigned char)data[x] );
			else
				ofs += snprintf( cOut+ofs, sizeof(cOut)/sizeof(TEXTCHAR)-ofs, WIDE(".") );
		}
		SystemLogFL( cOut FILELINE_RELAY );
		nOut -= x;
		data += x;
	}
}
#undef LogBinaryEx
 void  LogBinaryEx ( P_8 buffer, size_t size DBG_PASS )
{
#ifdef _DEBUG
	LogBinaryFL( buffer,size DBG_RELAY );
#else
	LogBinaryFL( buffer,size FILELINE_NULL );
#endif
}
#undef LogBinary
 void  LogBinary ( P_8 buffer, size_t size )
{
	LogBinaryFL( buffer,size, NULL, 0 );
}

void  SetSystemLog ( enum syslog_types type, const void *data )
{
	if( l.file )
	{
		fclose( l.file );
		l.file = NULL;
	}
	if( type == SYSLOG_FILE )
	{
		if( data )
		{
			logtype = type;
			l.file = (FILE*)data;
		}
	}
	else if( type == SYSLOG_FILENAME )
	{
		FILE *log;
#if  _MSC_VER >=1600 
		log = sack_fsopen( GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ), (CTEXTSTR)data, WIDE("wt")
#ifdef _UNICODE 
				WIDE(", ccs=UNICODE")
#endif
				, _SH_DENYWR );
#else
		log = sack_fopen( GetFileGroup( WIDE( "system.logs" ), GetProgramPath() ), (CTEXTSTR)data, WIDE("wt")
#ifdef _UNICODE 
				WIDE(", ccs=UNICODE")
#endif
				);
#endif
		l.file = log;

		logtype = SYSLOG_FILE;
	}
	else if( type == SYSLOG_CALLBACK )
	{
		UserCallback = (UserLoggingCallback)data;
	}
	else
	{
		logtype = type;
	}
	//SystemLog( WIDE("thing is: ") STRSYM( (SYSLOG_EXTERN) ) );
}

 void  SystemLogTime ( LOGICAL enable )
{
	flags.bLogTime = FALSE;
	flags.bUseDay = FALSE;
	flags.bUseDeltaTime = FALSE;
	flags.bLogHighTime = FALSE;
	flags.bLogCPUTime = FALSE;
	if( enable )
	{
		flags.bLogTime = TRUE;
		if( enable & SYSLOG_TIME_HIGH )
			flags.bLogHighTime = TRUE;
		if( enable & SYSLOG_TIME_LOG_DAY )
			flags.bUseDay = TRUE;
		if( enable & SYSLOG_TIME_DELTA )
			flags.bUseDeltaTime = TRUE;
		if( enable & SYSLOG_TIME_CPU )
			flags.bLogCPUTime = TRUE;
	}
}

// information for the call to _real_lprintf file and line information...
ThreadLocal struct next_lprint_info{
	// please use this enter when resulting a function, and leave from said function.
	// oh - but then we couldn't exist before crit sec code...
	//CRITICALSECTION cs;
	_32 nLevel;
	CTEXTSTR pFile;
	int nLine;
} next_lprintf;

static INDEX CPROC _null_vlprintf ( CTEXTSTR format, va_list args )
{
   return 0;
}

static PLINKQUEUE buffers;


static INDEX CPROC _real_vlprintf ( CTEXTSTR format, va_list args )
{
   // this can be used to force logging early to stdout
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
		if( !buffers )
		{
			int n;
			buffers = CreateLinkQueue();
			for( n = 0; n < 6; n++ )
				EnqueLink( &buffers, (POINTER)1 );
			for( n = 0; n < 6; n++ )
				DequeLink( &buffers );
		}

		buffer = (TEXTCHAR*)DequeLink( &buffers );
		if( !buffer )
		{
			//DoSystemLog( WIDE( "Adding Logging Buffer" ) );
			buffer = NewArray( TEXTCHAR, 4096 );
		}
		// at this point we're not doing internal allocations...
		cannot_log = 0;

		if( logtime[0] )
#ifdef UNDER_CE
		{
			StringCbPrintf( buffer, 4096, WIDE("%s|")
							  , logtime );
		   ofs = StrLen( buffer );
		}
#else
			ofs = snprintf( buffer, 4095, WIDE("%s|")
							  , logtime );
#endif
		else
			ofs = 0;
		// argsize - the program's giving us file and line
		// debug for here or not, this must be used.
		if( flags.bLogThreadID )
			snprintf( threadid, sizeof( threadid ), WIDE("%012") _64fX WIDE("~"), GetMyThreadID() );
#ifdef UNDER_CE
		snprintf( buffer + ofs, 4095 - ofs, WIDE("%s%s%s")
				  , flags.bLogThreadID?threadid:WIDE("")
				  , flags.bLogProgram?GetProgramName():WIDE("")
				  , flags.bLogProgram?WIDE("@"):WIDE("")
				  );
		ofs += StrLen( buffer + ofs );
#else
		snprintf( buffer + ofs, 4095 - ofs, WIDE("%s%s%s")
				  , flags.bLogThreadID?threadid:WIDE("")
				  , flags.bLogProgram?GetProgramName():WIDE("")
				  , flags.bLogProgram?WIDE("@"):WIDE("")
				  );
		ofs += StrLen( buffer + ofs );
#endif
		{
			CTEXTSTR pFile;
#ifndef _LOG_FULL_FILE_NAMES
			CTEXTSTR p;
#endif
			_32 nLine;
			if( flags.bLogSourceFile && ( pFile = next_lprintf.pFile ) )
			{
				if( flags.bProtectLoggedFilenames )
					if( IsBadReadPtr( pFile, 2 ) )
                  pFile = WIDE("(Unloaded file?)");
#ifndef _LOG_FULL_FILE_NAMES
				for( p = pFile + strlen(pFile) -1;p > pFile;p-- )
					if( p[0] == '/' || p[0] == '\\' )
					{
						pFile = p+1;break;
					}
#endif
				nLine = next_lprintf.nLine;
#ifdef UNDER_CE
				snprintf( buffer + ofs, 4095 - ofs, WIDE("%s(%") _32f WIDE("):")
									, pFile, nLine );
				ofs += StrLen( buffer + ofs );
#else
				snprintf( buffer + ofs, 4095 - ofs, WIDE("%s(%") _32f WIDE("):")
									, pFile, nLine );
				ofs += StrLen( buffer + ofs );
#endif
			}
			vsnprintf( buffer + ofs, 4095 - ofs, format, args );
			// okay, so even the above is unsafe, because Micro$oft has
			// decreed to be stupid.
			buffer[4095] = 0;
		}
		DoSystemLog( buffer );

		{
			cannot_log = 1;
			EnqueLink( &buffers, buffer );
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
	return 0;
}


RealVLogFunction  _vxlprintf ( _32 level DBG_PASS )
{
	//EnterCriticalSec( &next_lprintf.cs );
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		InitSyslog( 1 );
	}
#endif
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

RealLogFunction _xlprintf( _32 level DBG_PASS )
{
	//EnterCriticalSec( &next_lprintf.cs );
#ifndef __STATIC_GLOBALS__
	if( !syslog_local )
	{
		static int opening;
		if( !opening )
			opening = 1;
		else
			return _null_lprintf;
		//return _null_lprintf;
		InitSyslog( 1 );
		opening = 0;
	}
#endif
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
	flags.bProtectLoggedFilenames = bEnable;
}

void SetSystemLoggingLevel( _32 nLevel )
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
	flags.bLogOpenAppend = TESTFLAG( options, SYSLOG_OPT_OPENAPPEND )?1:0; // open for append, else open for write
	flags.bLogOpenBackup = TESTFLAG( options, SYSLOG_OPT_OPEN_BACKUP )?1:0; // open for append, else open for write
	flags.bLogProgram = TESTFLAG( options, SYSLOG_OPT_LOG_PROGRAM_NAME )?1:0; // open for append, else open for write
	flags.bLogThreadID = TESTFLAG( options, SYSLOG_OPT_LOG_THREAD_ID )?1:0; // open for append, else open for write
	flags.bLogSourceFile = TESTFLAG( options, SYSLOG_OPT_LOG_SOURCE_FILE )?1:0; // open for append, else open for write
}

#ifdef __cplusplus_cli

static public ref class Log
{
public:
	static void log( System::String^ ouptut )
	{
				pin_ptr<const WCHAR> _output = PtrToStringChars(ouptut);
				TEXTSTR __ouptut = DupWideToText( _output );
		lprintf( WIDE("%s"), __ouptut );
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
