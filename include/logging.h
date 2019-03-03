/*
 *  Crafted by James Buckeyne
 *  Part of SACK github.com/d3x0r/SACK
 *
 *   (c) Freedom Collective 2000-2006++, 2016++
 *
 *   created to provide standard logging features
 *   lprintf( format, ... ); simple, basic
 *   if DEBUG, then logs to a file of the same name as the program
 *   if RELEASE most of this logging goes away at compile time.
 *
 *  standardized to never use int.
 */


#ifndef LOGGING_MACROS_DEFINED
#define LOGGING_MACROS_DEFINED
#include <stdarg.h>
#include <sack_types.h>

#define SYSLOG_API CPROC
#ifdef SYSLOG_SOURCE
#define SYSLOG_PROC EXPORT_METHOD
#else
#define SYSLOG_PROC IMPORT_METHOD
#endif


#ifdef __cplusplus
#define LOGGING_NAMESPACE namespace sack { namespace logging {
#define LOGGING_NAMESPACE_END } }
#else
#define LOGGING_NAMESPACE 
#define LOGGING_NAMESPACE_END
#endif
#ifdef __cplusplus
	namespace sack {
/* Handles log output. Logs can be directed to UDP directed, or
   broadcast, or localhost, or to a file location, and under
   windows the debugging console log.
   
   
   
   lprintf
   
   SetSystemLog
   
   SystemLogTime
   
   
   
   there are options, when options code is enabled, which
   control logging output and format. Log file location can be
   specified generically for instance.... see Options.


	This namespace contains the logging functions. The most basic
   thing you can do to start logging is use 'lprintf'.
   
   <code lang="c++">
   lprintf( "My printf like format %s %d times", "string", 15 );
   </code>
   
   This function takes a format string and arguments compatible
   with vsnprintf. Internally strings are truncated to 4k
   length. (that is no single logging message can be more than
   4k in length).
   
   
   
   There are functions to control logging behavior.
   
   
   See Also
   SetSystemLog
   
   SystemLogTime
   
   SystemLogOptions
   
   lprintf
   
   _lprintf
   
   xlprintf
   
   _xlprintf
                                                                 */
		namespace logging {
#endif


/* \Parameters for SetSystemLog() to specify where the logging
   should go.                                                  */
enum syslog_types {
SYSLOG_NONE     =   -1 // disable any log output.
, /* Set logging to off. */
SYSLOG_UDP      =    0
, /* Send messages UDP localhost port 514. 127.0.0.1:514. */
SYSLOG_FILE     =    1
, /* Set logging to output to a file. The file passed is a FILE*. This
   may be a FILE* like stdout, stderr, or some file the
   application opens.                                                */
SYSLOG_FILENAME =    2
, /* Set logging to go to a file, pass the string text name of the
   \file to open as the second parameter of SetSystemLog.        */
SYSLOG_SYSTEM   =    3
, /* Specify that logging should go to system (this actually means
   Windows system debugging channel. OutputDebugString() ).      */
SYSLOG_UDPBROADCAST= 4
// Allow user to specify a void UserCallback( char * )
// which recieves the formatted output.
, /* Send logging messages on UDP full broadcast address port 514. */
SYSLOG_CALLBACK    = 5
, /* Send Logging to a specified user callback to handle. This
   lets logging go anywhere else that's not already thought of. */
SYSLOG_AUTO_FILE = SYSLOG_FILE + 100 /* Send logging to a file. If the file is not open, open the
   \file. If no logging happens, no log file is created.     */
, /* Send logging to /dev/log (unix socket), default LINUX option */
SYSLOG_SOCKET_SYSLOGD
};

#if !defined( NO_LOGGING )
#define DO_LOGGING
#endif
// this was forced, force no_logging off...
#if defined( DO_LOGGING )
#undef NO_LOGGING
#endif


#ifdef __LINUX__
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

SYSLOG_PROC  LOGICAL SYSLOG_API  IsBadReadPtr ( CPOINTER pointer, uintptr_t len );
#endif

SYSLOG_PROC  CTEXTSTR SYSLOG_API  GetPackedTime ( void );


//  returns the millisecond of the day (since UNIX Epoch) * 256 ( << 8 )
// the lowest 8 bits are the timezone / 15.  
// The effect of the low [7/]8 bits being the time zone is that within the same millisecond
// UTC +0 sorts first, followed by +1, +2, ... etc until -14, -13, -12,... -1
// the low [7/]8 bits are the signed timezone
// (timezone could have been either be hr*60 + min (ISO TZ format)
// or in minutes (hr*60+mn) this would only take 7 bits
// one would think 8 bit shifts would be slightly more efficient than 7 bits.
// and sign extension for 8 bits already exists.
// - REVISION - timezone with hr*100 does not divide by 15 cleanly.
//     The timezone is ( hour*60 + min ) / 15 which is a range from -56 to 48
//     minimal representation is 7 bits (0 - 127 or -64 - 63)
//     still keeping 8 bits for shifting, so the effective range is only -56 to 48 of -128 to 127
// struct time_of_day {
//    uint64_t epoch_milliseconds : 56;
//    int64_t timezone : 8; divided by 15... hours * 60 / 15 
// }
SYSLOG_PROC  int64_t SYSLOG_API GetTimeOfDay( void );

// binary little endian order; somewhat
typedef struct sack_expanded_time_tag
{
	uint16_t ms;
	uint8_t sc,mn,hr,dy,mo;
	uint16_t yr;
	int8_t zhr, zmn;
} SACK_TIME;
typedef struct sack_expanded_time_tag *PSACK_TIME;

// convert a integer time value to an expanded structure.
SYSLOG_PROC void     SYSLOG_API ConvertTickToTime( int64_t, PSACK_TIME st );
// convert a expanded time structure to a integer value.
SYSLOG_PROC int64_t SYSLOG_API ConvertTimeToTick( PSACK_TIME st );

// returns timezone as hours*100 + minutes.
// result is often negated?
SYSLOG_PROC  int SYSLOG_API GetTimeZone(void);

// 
typedef void (CPROC*UserLoggingCallback)( CTEXTSTR log_string );
SYSLOG_PROC  void SYSLOG_API  SetSystemLog ( enum syslog_types type, const void *data );
SYSLOG_PROC  void SYSLOG_API  ProtectLoggedFilenames ( LOGICAL bEnable );

SYSLOG_PROC  void SYSLOG_API  SystemLogFL ( CTEXTSTR FILELINE_PASS );
SYSLOG_PROC  void SYSLOG_API  SystemLogEx ( CTEXTSTR DBG_PASS );
SYSLOG_PROC  void SYSLOG_API  SystemLog ( CTEXTSTR );
SYSLOG_PROC  void SYSLOG_API  LogBinaryFL ( const uint8_t* buffer, size_t size FILELINE_PASS );
SYSLOG_PROC  void SYSLOG_API  LogBinaryEx ( const uint8_t* buffer, size_t size DBG_PASS );
SYSLOG_PROC  void SYSLOG_API  LogBinary ( const uint8_t* buffer, size_t size );
// logging level defaults to 1000 which is log everything
SYSLOG_PROC  void SYSLOG_API  SetSystemLoggingLevel ( uint32_t nLevel );
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
/* Log a binary buffer. Logs lines representing 16 bytes of data
   at a time. The hex of each byte in a buffer followed by the
   text is logged.
   
   
   
   
   Example
   <code lang="c#">
   char sample[] = "sample string";
   
   LogBinary( sample, sizeof( sample ) );
   </code>
   
   Results with the following output in the log...
   <code>
   
    73 61 6D 70 6C 65 20 73 74 72 69 6E 67 00 sample string.
   </code>
   
   The '.' at the end of 'sample string' is a non printable
   character. characters 0-31 and 127+ are printed as '.'.       */
#define LogBinary(buf,sz) LogBinaryFL((uint8_t*)(buf),sz DBG_SRC )
#define SystemLog(buf)    SystemLogFL(buf DBG_SRC )
#else
// need to include the typecast... binary logging doesn't really care what sort of pointer it gets.
#define LogBinary(buf,sz) LogBinary((uint8_t*)(buf),sz )
//#define LogBinaryEx(buf,sz,...) LogBinaryFL(buf,sz FILELINE_NULL)
//#define SystemLogEx(buf,...) SystemLogFL(buf FILELINE_NULL )
#endif

// int result is useless... but allows this to be
// within expressions, which with this method should be easy.
typedef INDEX (CPROC*RealVLogFunction)(CTEXTSTR format, va_list args )
//#if defined( __GNUC__ ) && !defined( _UNICODE )
//	__attribute__ ((__format__ (__vprintf__, 1, 2)))
//#endif
	;
typedef INDEX (CPROC*RealLogFunction)(CTEXTSTR format,...)
#if defined( __GNUC__ ) && !defined( _UNICODE )
	__attribute__ ((__format__ (__printf__, 1, 2)))
#endif
	;
SYSLOG_PROC  RealVLogFunction SYSLOG_API  _vxlprintf ( uint32_t level DBG_PASS );
SYSLOG_PROC  RealLogFunction SYSLOG_API  _xlprintf ( uint32_t level DBG_PASS );

// utility function to format a cpu delta into a buffer...
// end-start is always printed... therefore tick_end-0 is
// print absolute time... formats as millisecond.NNN
SYSLOG_PROC  void SYSLOG_API  PrintCPUDelta ( TEXTCHAR *buffer, size_t buflen, uint64_t tick_start, uint64_t tick_end );
// return the current CPU tick
SYSLOG_PROC  uint64_t SYSLOG_API  GetCPUTick ( void );
// result in nano seconds - thousanths of a millisecond...
SYSLOG_PROC  uint32_t SYSLOG_API  ConvertTickToMicrosecond ( uint64_t tick );
SYSLOG_PROC  uint64_t SYSLOG_API  GetCPUFrequency ( void );
SYSLOG_PROC  CTEXTSTR SYSLOG_API  GetTimeEx ( int bUseDay );

SYSLOG_PROC  void SYSLOG_API  SetSyslogOptions ( FLAGSETTYPE *options );
/* When setting options using SetSyslogOptions() these are the
   defines for the bits passed.
   
   SYSLOG_OPT_OPENAPPEND - the file, when opened, will be opened
   for append.
   
   SYSLOG_OPT_OPEN_BACKUP - the file, if it exists, will be
   renamed automatically.
   
   SYSLOG_OPT_LOG_PROGRAM_NAME - enable logging the program
   executable (probably the same for all messages, unless they
   are network)
   
   SYSLOG_OPT_LOG_THREAD_ID - enables logging the unique process
   and thread ID.
   
   SYSLOG_OPT_LOG_SOURCE_FILE - enable logging source file
   information. See <link DBG_PASS>
   
   SYSLOG_OPT_MAX - used for declaring a flagset to pass to
   setoptions.                                                   */
enum system_logging_option_list {
		/* the file, when opened, will be opened for append.
		 */
		SYSLOG_OPT_OPENAPPEND
										  ,  /* the file, if it exists, will be renamed automatically.
										  */
										  SYSLOG_OPT_OPEN_BACKUP
                                
                                , /* enable logging the program executable (probably the same for
                                   all messages, unless they are network)
                                                                                                */
                                 SYSLOG_OPT_LOG_PROGRAM_NAME
										  , /* enables logging the unique process and thread ID.
										                                                       */
                                 SYSLOG_OPT_LOG_THREAD_ID
                                , /* enable logging source file information. See <link DBG_PASS>
                                                                                               */
										   SYSLOG_OPT_LOG_SOURCE_FILE
										  , /* used for declaring a flagset to pass to setoptions. */
										  SYSLOG_OPT_MAX
                                
};

// this solution was developed to provide the same
// functionality for compilers that refuse to implement __VA_ARGS__
// this therefore means that the leader of the function is replace
// and that extra parenthesis exist after this... therefore the remaining
// expression must be ignored... thereofre when defining a NULL function
// this will result in other warnings, about ignored, or meaningless expressions
# if defined( DO_LOGGING )
#  define vlprintf      _vxlprintf(LOG_NOISE DBG_SRC)
#  define lprintf       _xlprintf(LOG_NOISE DBG_SRC)
#  define _lprintf(file_line,...)       _xlprintf(LOG_NOISE file_line,##__VA_ARGS__)
#  define xlprintf(level)       _xlprintf(level DBG_SRC)
#  define vxlprintf(level)       _vxlprintf(level DBG_SRC)
# else
#  ifdef _MSC_VER
#   define vlprintf      (1)?(0):
#   define lprintf       (1)?(0):
#   define _lprintf(DBG_VOIDRELAY)       (1)?(0):
#   define xlprintf(level)       (1)?(0):
#   define vxlprintf(level)      (1)?(0):
#  else
#   define vlprintf(f,...)
/* use printf formating to output to the log. (log printf).
   
   
   Parameters
   Format :  Just like printf, the format string to print.
   ... :     extra arguments passed as required for the format.
   
   Example
   <code lang="c++">
      lprintf( "Test Logging %d %d", 13, __LINE__ );
   </code>                                                      */
#   define lprintf(f,...)
#   define  _lprintf(DBG_VOIDRELAY)       lprintf
#   define xlprintf(level) lprintf
#   define vxlprintf(level) lprintf
#  endif
# endif
#undef LOG_WARNING
#undef LOG_ADVISORIES
#undef LOG_INFO
// Defined Logging Levels
enum {
	  // and you are free to use any numerical value,
	  // this is a rough guideline for wide range
	  // to provide a good scaling for levels of logging
	LOG_ALWAYS = 1 // unless logging is disabled, this will be logged
	, LOG_ERRORS = 50 // logging level set to 50 or more will cause this to log
	, /* Specify a logging level which only ERROR level logging is
	   logged.                                                   */
	 LOG_ERROR = LOG_ERRORS // logging level set to 50 or more will cause this to log
	, /* Specify that this message is of ERROR importance. */
	 LOG_WARNINGS = 500 // .......
	, /* Set logging level to log warnings and errors. */
	 LOG_WARNING = LOG_WARNINGS // .......
   , /* Use to specify that the log message is a warning level
      message.                                               */
    LOG_ADVISORY = 625
   , /* Specify that the this message is an advisory level. */
    LOG_ADVISORIES = LOG_ADVISORY
	, /* A symbol to specify to log Adviseries, Warnings and Error
	   level messages only.                                      */
	 LOG_INFO = 750
	  , /* A moderate logging level, which is near maximum verbosity of
	     logging.                                                     */
	   LOG_NOISE = 1000
     , /* Define that the message is just noisy - though verbosly
	  informative, it's level is less critical than even INFO.
	  default iS LOG_NOISE which is 1000, an ddefault for disabling most messages
	  is to set log level to 999.  Have to increase to 2000 to see debug, and this name
     has beviously
	  */
      LOG_LEVEL_DEBUG = 2000
	, /* Specify the message is of DEBUG importance, which is far
	   above even NOISY. If debug logging is enabled, all logging,
	   ERROR, WARNING, ADVISORY, INFO, NOISY and DEBUG will be
	   logged.                                                     */
	 LOG_CUSTOM = 0x40000000 // not quite a negative number, but really big
	, /* A bit with LOG_CUSTOM might be enabled, and the lower bits
	   under 0x40000000 (all bits 0x3FFFFFFF ) can be used to
	   indicate a logging type. Then SetLoggingLevel can be passed a
	   mask of bits to filter types of messages.                     */
	 LOG_CUSTOM_DISABLE = 0x20000000 // not quite a negative number, but really big
	// bits may be user specified or'ed with this value
	// such that ...
	// Example 1:SetSystemLoggingLevel( LOG_CUSTOM | 1 ) will
	// enable custom logging messages which have the '1' bit on... a logical
	// and is used to test the low bits of this value.
	// example 2:SetSystemLogging( LOG_CUSTOM_DISABLE | 1 ) will disable logging
	// of messages with the 1 bit set.
#define LOG_CUSTOM_BITS 0xFFFFFF  // mask of bits which may be used to enable and disable custom logging
};

 // this is a flag set consisting of 0 or more or'ed symbols
enum SyslogTimeSpecifications {
 SYSLOG_TIME_DISABLE = 0, // disable time logging
 SYSLOG_TIME_ENABLE  = 1, // enable is anything not zero.
 SYSLOG_TIME_HIGH    = 2, // specify to log milliseconds
 SYSLOG_TIME_LOG_DAY = 4, // log the year/month/day also
 SYSLOG_TIME_DELTA   = 8, // log the difference in time instead of the absolute time
 SYSLOG_TIME_CPU     =16 // logs cpu ticks... implied delta
};

/* Specify how time is logged. */
SYSLOG_PROC void SYSLOG_API SystemLogTime( uint32_t enable );

#ifndef NO_LOGGING

#define OutputLogString(s) SystemLog(s)
/* Depricated. Logs a format string that takes 0 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log(s)                                   SystemLog( s )
#else
#define OutputLogString(s) 
/* Depricated. Logs a format string that takes 0 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log(s)
#endif
/* Depricated. Logs a format string that takes 1 parameter.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log1(s,p1)                               lprintf( s, p1 )
/* Depricated. Logs a format string that takes 2 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log2(s,p1,p2)                            lprintf( s, p1, p2 )
/* Depricated. Logs a format string that takes 3 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log3(s,p1,p2,p3)                         lprintf( s, p1, p2, p3 )
/* Depricated. Logs a format string that takes 4 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log4(s,p1,p2,p3,p4)                      lprintf( s, p1, p2, p3,p4)
/* Depricated. Logs a format string that takes 5 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log5(s,p1,p2,p3,p4,p5)                   lprintf( s, p1, p2, p3,p4,p5)
/* Depricated. Logs a format string that takes 6 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log6(s,p1,p2,p3,p4,p5,p6)                lprintf( s, p1, p2, p3,p4,p5,p6)
/* Depricated. Logs a format string that takes 7 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log7(s,p1,p2,p3,p4,p5,p6,p7)             lprintf( s, p1, p2, p3,p4,p5,p6,p7 )
/* Depricated. Logs a format string that takes 8 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log8(s,p1,p2,p3,p4,p5,p6,p7,p8)          lprintf( s, p1, p2, p3,p4,p5,p6,p7,p8 )
/* Depricated. Logs a format string that takes 9 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log9(s,p1,p2,p3,p4,p5,p6,p7,p8,p9)       lprintf( s, p1, p2, p3,p4,p5,p6,p7,p8,p9 )
/* Depricated. Logs a format string that takes 10 parameters.
   See Also
   <link sack::logging::lprintf, lprintf>                    */
#define Log10(s,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)  lprintf( s, p1, p2, p3,p4,p5,p6,p7,p8,p9,p10 )

LOGGING_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::logging;
#endif

#endif
