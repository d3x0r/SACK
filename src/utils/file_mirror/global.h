

struct filemirror_global {
	struct {
		BIT_FIELD exit_when_done : 1;
		BIT_FIELD log_network_read : 1;
		BIT_FIELD log_file_ops : 1;
		BIT_FIELD bServeClean : 1;
		BIT_FIELD bServeWindows : 1;
	} flags;
	PACCOUNT AccountList;
	PNETBUFFER NetworkBuffers;
	PTHREAD main_thread;
	CTEXTSTR configname;
};
#define g global_filemirror_data

#ifndef RELAY_SOURCE
extern
#endif
struct filemirror_global global_filemirror_data;


void CPROC LogOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size );
