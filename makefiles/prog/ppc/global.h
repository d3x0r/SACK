
#define stddbg stderr

#include "./types.h"
#include "fileio.h"

typedef struct include_reference_tag {
	struct {
      _32 bMacros;
	} flags;
   char *name;
} INCLUDE_REF, *PINCLUDE_REF;


typedef struct global_tag
{
	struct {
		_32 do_trigraph : 1;
		_32 bWriteLine : 1;
		_32 bLineUsesLineKeyword : 1;
		_32 bNoOutput : 1; // when -imacro is used...
		_32 bAllWarnings : 1; // enable normally harmless warnings.
		_32 bEmitUnknownPragma : 1;
		_32 bForceBackslash : 1;
		_32 bForceForeslash : 1;
		_32 bStdout : 1;
		_32 keep_comments : 1;
		_32 keep_includes : 1;
		_32 bWriteLineInfo : 1;
	} flags;
	int bDebugLog;
	char pExecPath[256];
	char pExecName[256];
   char pWorkPath[256];
   DECLTEXTSZ( pCurrentPath, 256 );
   _32 ErrorCount;
	/******************************/

	PLIST pSysIncludePath; // list of paths to search includes for...
	// the include path should have appended to it the default
	// system include file... this probably comes from an enviroment
	// environment will be an important thing to mimic in my operation
	// system...

	PLIST pUserIncludePath;
	int AllDependancies;     // include 'system' dependancies
	int bAutoDepend; // include 'system' dependancies
	FILE *AutoDependFile;
	VARTEXT vt; // safe junk buffer to print into...
	unsigned char CurrentOutName[256];
	int nIfLevels;
	unsigned long nAllocates;
	unsigned long nReleases;
	unsigned long nAllocSize;
	unsigned char AutoTargetName[256]; // target name to reference when
	                                   //building auto depend...
	PLINKSTACK pIncludeList;
} GLOBAL;

// debug Log options....
#define DEBUG_SUBST 0x02
#define DEBUG_DEFINES 0x04
#define DEBUG_READING 0x08


#ifndef CPP_MAIN_SOURCE
extern
#endif
GLOBAL g;

