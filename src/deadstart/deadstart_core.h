
typedef struct startup_proc_tag {
	DeclareLink( struct startup_proc_tag );
	int bUsed;
	int priority;
	void (CPROC*proc)(void);
	CTEXTSTR func;
#ifdef _DEBUG
	CTEXTSTR file;
	int line;
#endif
} STARTUP_PROC, *PSTARTUP_PROC;

typedef struct shutdown_proc_tag {
	DeclareLink( struct shutdown_proc_tag );
	int bUsed;
	int priority;
	void (CPROC*proc)(void);
	CTEXTSTR func;
#ifdef _DEBUG
	CTEXTSTR file;
	int line;
#endif
} SHUTDOWN_PROC, *PSHUTDOWN_PROC;

struct deadstart_local_data_
{
	// this is a lot of procs...
	int nShutdownProcs;
#ifdef __LINUX__
	LOGICAL registerdSigint ;
	struct sigaction prior_sigint;
#endif
#define nShutdownProcs l.nShutdownProcs
	SHUTDOWN_PROC shutdown_procs[512];
#define shutdown_procs l.shutdown_procs
	int bInitialDone;
#define bInitialDone l.bInitialDone
	LOGICAL bInitialStarted;
#define bInitialStarted l.bInitialStarted
	int bSuspend;
#define bSuspend l.bSuspend
	int bDispatched;
//#define bDispatched l.bDispatched

	PSHUTDOWN_PROC shutdown_proc_schedule;
#define shutdown_proc_schedule l.shutdown_proc_schedule

	int nProcs; // count of used procs...
#define nProcs l.nProcs
	STARTUP_PROC procs[1024];
#define procs l.procs
	PSTARTUP_PROC proc_schedule;
#define proc_schedule l.proc_schedule
	struct
	{
		BIT_FIELD bInitialized : 1;
		BIT_FIELD bLog : 1;
	} flags;
};

