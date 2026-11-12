#include <stdhdrs.h>
#include <network.h>

int main( void ) {
	PDATALIST list = NULL;
	struct listener_pid_info* info;
	INDEX idx;
	SackNetstat_GetListeners( &list );
	DATA_FORALL( list, idx, struct listener_pid_info*, info ) {
		INDEX p;
		// pdlPids holds every pid listening on the port; the element size is
		// platform specific (see netstat_win32.c / netstat_linux.c).
#ifdef _WIN32
		uint32_t* pid;
		DATA_FORALL( info->pdlPids, p, uint32_t*, pid )
#else
		uint64_t* pid;
		DATA_FORALL( info->pdlPids, p, uint64_t*, pid )
#endif
			printf( "Found port: %d pid: %llu\n", info->port, (unsigned long long)pid[0] );
	}
	return 0;
}

