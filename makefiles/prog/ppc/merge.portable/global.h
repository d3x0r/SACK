
#define stddbg g.dbgout
//stderr

#  if _MSC_VER > 1500
#    define mkdir _mkdir
#    define fileno _fileno
#    define stricmp _stricmp
#    define strdup _strdup
#  endif

#ifdef __GNUC_PREREQ
#  if __GNUC_PREREQ(14,0)
#    define stricmp strcasecmp
#  endif
#endif

//#include "./types.h"
#include "fileio.h"
#include "panthers_c_preprocessor.h"

#ifdef __cplusplus
namespace d3x0r {
namespace ppc {
#endif

typedef struct include_reference_tag {
	struct {
		uint32_t bMacros;
	} flags;
	char *name;
} INCLUDE_REF, *PINCLUDE_REF;

typedef struct global_tag {
	struct {
		uint32_t do_trigraph : 1;
		uint32_t bWriteLine : 1;
		uint32_t bLineUsesLineKeyword : 1;
		uint32_t bNoOutput : 1;    // when -imacro is used...
		uint32_t bAllWarnings : 1; // enable normally harmless warnings.
		uint32_t bEmitUnknownPragma : 1;
		uint32_t bForceBackslash : 1;
		uint32_t bForceForeslash : 1;
		uint32_t bStdout : 1;
		uint32_t keep_comments : 1;
		uint32_t keep_includes : 1;
		uint32_t bWriteLineInfo : 1;
		uint32_t load_once : 1;
		uint32_t bSkipSystemIncludeOut : 1; // don't output system include headers
		uint32_t bIncludedLastFile : 1;     // a status of the last processinclude
		uint32_t doing_system_file : 1;
		uint32_t skip_define_processing : 1;
		uint32_t skip_logic_processing : 1;
		uint32_t config_loaded : 1;
	} flags;
	FILE *output;

	int bDebugLog;
	// char pExecPath[256];
	// char pExecName[256];
	char pWorkPath[ 256 ];
	DECLTEXTSZTYPE( pCurrentPath, 256 );
	uint32_t ErrorCount;
	/******************************/

	PLIST pSysIncludePath; // list of paths to search includes for...
	// the include path should have appended to it the default
	// system include file... this probably comes from an enviroment
	// environment will be an important thing to mimic in my operation
	// system...

	PLIST pUserIncludePath;
	int AllDependancies; // include 'system' dependancies
	int bAutoDepend;     // include 'system' dependancies
	FILE *AutoDependFile;
	PVARTEXT pvt; // safe junk buffer to print into...
	char CurrentOutName[ 256 ];
	int nIfLevels;
	unsigned long nAllocates;
	unsigned long nReleases;
	size_t nAllocSize;
	char AutoTargetName[ 256 ]; // target name to reference when
	                            // building auto depend...
	PLINKSTACK pIncludeList;
	FILE *dbgout;
	PFILETRACK pAllFileStack;

	PFILETRACK pFileStack;

	PVARTEXT pvtOut; // builds output into this buffer instead of writing output
	                 // to a file.
	uintptr_t current_process_info;
	char const *current_file_name;


	// all files in Root->pAlso are top level dependancies.
	PFILEDEP FileDependancyRoot;

	union {
		struct {
			uint32_t bLesser : 1;
			uint32_t bGreater : 1;
			uint32_t bColon : 1;
			uint32_t bPercent : 1;
			uint32_t bQuestion1 : 1;
			uint32_t bQuestion2 : 1;
		};
		uint32_t dw;
	} input_flags;


} GLOBAL;

// debug Log options....
#define DEBUG_SUBST 0x02
#define DEBUG_DEFINES 0x04
#define DEBUG_READING 0x08
#define DEBUG_MEMORY 0x10

// extended text flag for internal use
#define TF_NOEXPAND 0x00200000 // don't expand - define substitution handling...

#ifdef __cplusplus
thread_local
#else
_Thread_local
#endif

#ifndef CPP_MAIN_SOURCE
extern
#endif
     GLOBAL g;

void DumpSegs( PTEXT pOp );

#ifdef __cplusplus
} // namespace ppc
} // namespace d3x0r
#endif
