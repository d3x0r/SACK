
// the higher the number the earlier it is run

#define ATEXIT_PRIORITY_SYSLOG   SYSLOG_PRELOAD_PRIORITY
#define ATEXIT_PRIORITY_SHAREMEM 1
#define ATEXIT_PRIORITY_TIMERS   (SYSLOG_PRELOAD_PRIORITY-1)

#define ATEXIT_PRIORITY_MSGCLIENT 85
#define ATEXIT_PRIORITY_DEFAULT  100

// this is the first exit to be run.
// under linux it is __attribute__((destructor))
// under all it is registered during preload as atexit()
// only the runexits in deadstart should use ROOT_ATEXIT
#ifdef __WATCOMC__
#define ATEXIT_PRIORITY_ROOT 255
#else
#define ATEXIT_PRIORITY_ROOT 101
#endif
