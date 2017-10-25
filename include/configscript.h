/*
 *  Creator: Jim Buckeyne
 *  Header for configscript.lib(bag.lib)
 *  Provides definitions for handling configuration files
 *  or any particular file which has machine generated
 *  characteristics, it can handle translators to decrypt
 *  encrypt.  Method of operation is to create a configuration
 *  evaluator, then AddConfiguratMethod()s to it.
 *  configuration methods are format descriptors for the lines
 *  and a routine which is called when such a line is matched.
 *  One might think of it as a trigger library for MUDs ( a
 *  way to trigger an event based on certain text input,
 *  variations in the text input may be assigned as variables
 *  to be used within the event.
 *
 *  More about configuration string parsing is available in
 *  $(SACK_BASE)/src/configlib/config.rules text file.
 *
 *  A vague attempt at providing a class to derrive a config-
 *  uration reader class, which may contain private data
 *  within such a class, or otherwise provide an object with
 *  simple namespace usage. ( add(), go() )
 *
 *  This library also imlements several PTEXT based methods
 *  which can evaluate text segments into valid binary types
 *  such as text to integer, float, color, etc.  Some of the type
 *  validators applied for the format argument matching of added
 *  methods are available for external reference.
 *
 */


#ifndef CONFIGURATION_SCRIPT_HANDLER
#define CONFIGURATION_SCRIPT_HANDLER
#include <sack_types.h>
#include <stdarg.h>
#include <colordef.h>
#include "fractions.h"

#ifdef CONFIGURATION_LIBRARY_SOURCE
#define CONFIGSCR_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define CONFIGSCR_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
SACK_NAMESPACE namespace config {
#endif

typedef char *__arg_list[1];
typedef __arg_list arg_list;
// declare 'va_list args = NULL;' to use successfully...
// the resulting thing is of type va_list.
typedef struct va_args_tag va_args;
enum configArgType {
	CONFIG_ARG_STRING,
	CONFIG_ARG_INT64,
	CONFIG_ARG_FLOAT,
	CONFIG_ARG_DATA,
	CONFIG_ARG_DATA_SIZE,
	CONFIG_ARG_LOGICAL,
	CONFIG_ARG_FRACTION,
	CONFIG_ARG_COLOR,
};
struct va_args_tag {
	int argsize; arg_list *args; arg_list *tmp_args; int argCount;
};
//#define va_args struct { int argsize; arg_list *args; arg_list *tmp_args; }
#define init_args(name) name.argCount = 0; name.argsize = 0; name.args = NULL;
#define ARG_STACK_SIZE 4  // 32 bits.
#define PushArgument( argset, argType, type, arg ) \
	((argset.args = (arg_list*)Preallocate( argset.args  \
		, argset.argsize += ((sizeof( enum configArgType ) \
				+ sizeof( type )  \
				+ (ARG_STACK_SIZE-1) )&-ARG_STACK_SIZE) ) ) \
	?(argset.argCount++),((*(enum configArgType*)(argset.args))=(argType)),(*(type*)(((uintptr_t)argset.args)+sizeof(enum ConfigArgType)) = (arg)),0   \
	:0)
#define PopArguments( argset ) { Release( argset.args ); argset.args=NULL; }
#define pass_args(argset) (( (argset).tmp_args = (argset).args ),(*(arg_list*)(&argset.tmp_args)))

/*
 * Config methods are passed an arg_list
 * parameters from arg_list are retrieved using
 * PARAM( arg_list_param_name, arg_type, arg_name );
 * ex.
 *
 *   PARAM( args, char *, name );
 *    // results in a variable called name
 *    // initialized from the first argument in arg_list args;
 */
#define my_va_arg(ap,type)     ((ap)[0]+=\
        ((sizeof(enum configArgType)+sizeof(type)+ARG_STACK_SIZE-1)&~(ARG_STACK_SIZE-1)),\
        (*(type *)((ap)[0]-((sizeof(type)+ARG_STACK_SIZE-1)&~(ARG_STACK_SIZE-1)))))
#define my_va_arg_type(ap,type)     ( \
        (*(type *)((ap)[0]-(sizeof(enum configArgType)+(sizeof(type)+ARG_STACK_SIZE-1)&~(ARG_STACK_SIZE-1)))))
#define my_va_next_arg_type(ap,type)     ( ( *(enum configArgType *)((ap)[0]) ) )

#define PARAM_COUNT( args ) (((int*)(args+1))[0])
#define PARAM( args, type, name ) type name = my_va_arg( args, type )
#define PARAMEX( args, type, name, argTypeName ) type name = my_va_arg( args, type ); enum configArgType argTypeName = my_va_arg_type(args)
#define FP_PARAM( args, type, name, fa ) type (CPROC*name)fa = (type (CPROC*)fa)(my_va_arg( args, void *))

typedef struct config_file_tag* PCONFIG_HANDLER;


CONFIGSCR_PROC( PCONFIG_HANDLER, CreateConfigurationEvaluator )( void );
#define CreateConfigurationHandler CreateConfigurationEvaluator
CONFIGSCR_PROC( void, DestroyConfigurationEvaluator )( PCONFIG_HANDLER pch );
#define DestroyConfigurationHandler DestroyConfigurationEvaluator

// this pushes all prior state information about configuration file
// processing, and allows a new set of rules to be made...
CONFIGSCR_PROC( void, BeginConfiguration )( PCONFIG_HANDLER pch );
// begins a sub configuration, and marks to save it for future use
// so we don't have to always recreate the configuration states...
CONFIGSCR_PROC( LOGICAL, BeginNamedConfiguration )( PCONFIG_HANDLER pch, CTEXTSTR name );

// then, when you're done with the new set of rules (end of config section)
// use this to restore the prior configuration state.
CONFIGSCR_PROC( void, EndConfiguration )( PCONFIG_HANDLER pch );

typedef uintptr_t (CPROC*USER_CONFIG_HANDLER)( uintptr_t, arg_list args );
CONFIGSCR_PROC( void, AddConfigurationEx )( PCONFIG_HANDLER pch
														, CTEXTSTR format
														, USER_CONFIG_HANDLER Process DBG_PASS );
//CONFIGSCR_PROC( void, AddConfiguration )( PCONFIG_HANDLER pch
//					, char *format
//													 , USER_CONFIG_HANDLER Process );
// make a nice wrapper - otherwise we get billions of complaints.
//#define AddConfiguration(pch,format,process) AddConfiguration( (pch), (format), process )
#define AddConfiguration(pch,f,pr) AddConfigurationEx(pch,f,pr DBG_SRC )
#define AddConfigurationMethod AddConfiguration

// FILTER receives a uintptr_t that was given at configuration (addition to handler)
// it receives a PTEXT block of (binary) data... and must result with
// PTEXT segments which are lines which may or may not have \r\n\\ all
// of which are removed before being resulted to the application.
//   POINTER* is a pointer to a pointer, this pointer may be used
//      for private state data.  The last line of the configuration will
//      call the filter chain with NULL to flush data...
typedef PTEXT (CPROC*USER_FILTER)( POINTER *, PTEXT );
CONFIGSCR_PROC( void, AddConfigurationFilter )( PCONFIG_HANDLER pch, USER_FILTER filter );
CONFIGSCR_PROC( void, ClearDefaultFilters )( PCONFIG_HANDLER pch );
CONFIGSCR_PROC( void, SetConfigurationEndProc )( PCONFIG_HANDLER pch, uintptr_t (CPROC *Process)( uintptr_t ) );
CONFIGSCR_PROC( void, SetConfigurationUnhandled )( PCONFIG_HANDLER pch
																, uintptr_t (CPROC *Process)( uintptr_t, CTEXTSTR ) );

CONFIGSCR_PROC( int, ProcessConfigurationFile )( PCONFIG_HANDLER pch
															  , CTEXTSTR name
															  , uintptr_t psv
															  );
CONFIGSCR_PROC( uintptr_t, ProcessConfigurationInput )( PCONFIG_HANDLER pch, CTEXTSTR block, size_t size, uintptr_t psv );

/*
 * TO BE IMPLEMENTED
 *
CONFIGSCR_PROC( int, vcsprintf )( PCONFIG_HANDLER pch, CTEXTSTR format, va_list args );
CONFIGSCR_PROC( int, csprintf )( PCONFIG_HANDLER pch, CTEXTSTR format, ... );
*/
CONFIGSCR_PROC( int, GetBooleanVar )( PTEXT *start, LOGICAL *data );
CONFIGSCR_PROC( int, GetColorVar )( PTEXT *start, CDATA *data );
//CONFIGSCR_PROC( int, IsBooleanVar )( PCONFIG_ELEMENT pce, PTEXT *start );
//CONFIGSCR_PROC( int, IsColorVar )( PCONFIG_ELEMENT pce, PTEXT *start );

// takes a binary block of data and creates a base64-like string which may be stored.
CONFIGSCR_PROC( void, EncodeBinaryConfig )( TEXTSTR *encode, POINTER data, size_t length );
// this isn't REALLY the same function that's used, but serves the same purpose...
CONFIGSCR_PROC( int, DecodeBinaryConfig )( CTEXTSTR String, POINTER *binary_buffer, size_t *buflen );


CONFIGSCR_PROC( CTEXTSTR, FormatColor )( CDATA color );
CONFIGSCR_PROC( void, StripConfigString )( TEXTSTR out, CTEXTSTR in );
CONFIGSCR_PROC( void, ExpandConfigString )( TEXTSTR out, CTEXTSTR in );

#ifdef __cplusplus

//typedef uintptr_t CPROC ::(*USER_CONFIG_METHOD)( ... );

typedef class config_reader {
   PCONFIG_HANDLER pch;
public:
	config_reader() {
      pch = CreateConfigurationEvaluator();
	}
	~config_reader() {
		if( pch ) DestroyConfigurationEvaluator( pch );
      pch = (PCONFIG_HANDLER)NULL;
	}
	inline void add( CTEXTSTR format, USER_CONFIG_HANDLER Process )
	{
      AddConfiguration( pch, format, Process );
	}
   /*
	inline void add( char *format, USER_CONFIG_METHOD Process )
	{
		union {
			struct {
				uint32_t junk;
            USER_CONFIG_HANDLER Process
			} c;
         USER_CONFIG_METHOD Process;
		} x;
      x.Process = Process;
      AddConfiguration( pch, format, x.c.Process );
		}
      */
	inline int go( CTEXTSTR file, POINTER p )
	{
		return ProcessConfigurationFile( pch, file, (uintptr_t)p );
	}
} CONFIG_READER;

#endif

#ifdef __cplusplus
} //namespace sack { namespace config {
SACK_NAMESPACE_END
using namespace sack::config;
#endif


#endif
// $Log: configscript.h,v $
// Revision 1.17  2004/12/05 15:32:06  panther
// Some minor cleanups fixed a couple memory leaks
//
// Revision 1.16  2004/08/13 16:48:19  d3x0r
// added ability to put filters on config script data read.
//
// Revision 1.15  2004/02/18 20:46:37  d3x0r
// Add some aliases for badly named routines
//
// Revision 1.14  2004/02/08 23:33:15  d3x0r
// Add a iList class for c++, public access to building parameter va_lists
//
// Revision 1.13  2003/12/09 16:15:56  panther
// Define unhnalded callback set
//
// Revision 1.12  2003/11/09 22:31:58  panther
// Fix CPROC indication on endconfig method
//
// Revision 1.11  2003/10/13 04:25:14  panther
// Fix configscript library... make sure types are consistant (watcom)
//
// Revision 1.10  2003/10/12 02:47:05  panther
// Cleaned up most var-arg stack abuse ARM seems to work.
//
// Revision 1.9  2003/09/24 02:53:58  panther
// Define c++ wrapper for config script library
//
// Revision 1.8  2003/07/24 22:49:01  panther
// Modify addconfig method macro to auto typecast - dangerous by simpler
//
// Revision 1.7  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.6  2003/04/17 09:32:51  panther
// Added true/false result from processconfigfile.  Added default load from /etc to msgsvr and display
//
// Revision 1.5  2003/03/25 08:38:11  panther
// Add logging
//
