#include <stdhdrs.h>
#include <network.h>

int main( void ) {
	PDATALIST list = NULL;
	struct listener_pid_info* info;
	INDEX idx;
	SackNetstat_GetListeners( &list );
	DATA_FORALL( list, idx, struct listener_pid_info*, info ) {
		printf( "Found port: %d pid: %d\n", info->port, info->pid );
	}
	return 0;
}

