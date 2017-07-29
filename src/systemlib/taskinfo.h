
typedef struct handle_info_tag
{
	//struct mydatapath_tag *pdp;
   PTEXT pLine; // partial inputs...
   char *name;
	int       bNextNew;
   PTHREAD   hThread;
#ifdef WIN32
   HANDLE    handle;   // read/write handle
#else
   int       pair[2];
   int       handle;   // read/write handle
#endif
} HANDLEINFO, *PHANDLEINFO;


//typedef void (CPROC*TaskEnd)(uintptr_t, struct task_info_tag *task_ended);
struct task_info_tag {
	struct {
		BIT_FIELD closed : 1;
		BIT_FIELD process_ended : 1;
		BIT_FIELD bSentIoTerminator : 1;
		BIT_FIELD log_input : 1;
	} flags;
	TaskEnd EndNotice;
	TaskOutput OutputEvent;
	uintptr_t psvEnd;
	HANDLEINFO hStdIn;
	HANDLEINFO hStdOut;
	volatile PTHREAD pOutputThread;
	//HANDLEINFO hStdErr;
#if defined(WIN32)

	HANDLE hReadOut, hWriteOut;
	//HANDLE hReadErr, hWriteErr;
	HANDLE hReadIn, hWriteIn;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
   DWORD exitcode;
#elif defined( __LINUX__ )
   int hReadOut, hWriteOut;
   //HANDLE hReadErr, hWriteErr;
	int hReadIn, hWriteIn;
   pid_t pid;
   uint32_t exitcode;
#endif
};

typedef struct loaded_function_tag
{
	uint32_t references;
	void (CPROC*function)(void );
	struct loaded_library_tag *library;
	DeclareLink( struct loaded_function_tag );
	CTEXTSTR name;  // can be an integer value... instead of a string...
	TEXTCHAR _name[1]; // this is more than 1; allocation pads extra bytes for the name.
} FUNCTION, *PFUNCTION;

#ifdef WIN32
typedef HMODULE HLIBRARY;
#else
typedef void* HLIBRARY;
#endif
typedef struct loaded_library_tag
{
	uintptr_t nLibrary; // when unloading...
	HLIBRARY library;
	LOGICAL mapped;
	PFUNCTION functions; 
	DeclareLink( struct loaded_library_tag );
	TEXTCHAR *name; // points into full_name after last slash - just library name
	int loading;
	TEXTCHAR *alt_full_name;// this is appended after full_name and is l.library_path
	TEXTCHAR full_name[1];// this is more than 1; allocation pads extra bytes for the name. prefixed iwth l.load_path
} LIBRARY, *PLIBRARY;

#ifndef SYSTEM_CORE_SOURCE
extern
#endif
  struct local_systemlib_data {
	CTEXTSTR load_path;
	CTEXTSTR library_path;
	CTEXTSTR common_data_path;
	struct system_local_flags{
		BIT_FIELD bLog : 1;
		BIT_FIELD bInitialized : 1;
	} flags;
	CTEXTSTR filename;  // pointer to just filename part...
	TEXTCHAR *work_path;
	PLIST system_tasks;
	PLIBRARY libraries;
	PTREEROOT pFunctionTree;
	int nLibrary;
	LOGICAL (CPROC*ExternalLoadLibrary)( const char *filename );

	char * (CPROC*ExternalFindProgram)( const char *filename ); // please Release or Deallocate the reutrn value

	// on XP this is in PSAPI.DLL later it's in Kernel32.DLL 
#ifdef WIN32
	BOOL (WINAPI* EnumProcessModules)( HANDLE hProcess, HMODULE *lphModule
	                                 , DWORD cb, LPDWORD lpcbNeeded );
#endif
} *local_systemlib;

#ifdef l
#   undef l
#endif
#define l (*local_systemlib)

int TryShellExecute( PTASK_INFO task, CTEXTSTR path, CTEXTSTR program, PTEXT cmdline );

