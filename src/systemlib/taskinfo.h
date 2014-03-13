
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


//typedef void (CPROC*TaskEnd)(PTRSZVAL, struct task_info_tag *task_ended);
struct task_info_tag {
	struct {
		BIT_FIELD closed : 1;
		BIT_FIELD process_ended : 1;
		BIT_FIELD bSentIoTerminator : 1;
		BIT_FIELD log_input : 1;
	} flags;
	TaskEnd EndNotice;
	TaskOutput OutputEvent;
	PTRSZVAL psvEnd;
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
   _32 exitcode;
#endif
};


typedef struct loaded_function_tag
{
	_32 references;
	void (CPROC*function)(void );
	struct loaded_library_tag *library;
	DeclareLink( struct loaded_function_tag );
	TEXTCHAR name[];
} FUNCTION, *PFUNCTION;

#ifdef WIN32
typedef HMODULE HLIBRARY;
#else
typedef void* HLIBRARY;
#endif
typedef struct loaded_library_tag
{
	PTRSZVAL nLibrary; // when unloading...
	HLIBRARY library;
	PFUNCTION functions;
	DeclareLink( struct loaded_library_tag );
	TEXTCHAR *name;
	TEXTCHAR full_name[];
} LIBRARY, *PLIBRARY;

#ifndef SYSTEM_CORE_SOURCE
extern
#endif
  struct local_systemlib_data {
	CTEXTSTR load_path;
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
} *local_systemlib;

#define l (*local_systemlib)

int TryShellExecute( PTASK_INFO task, CTEXTSTR path, CTEXTSTR program, PTEXT cmdline );

