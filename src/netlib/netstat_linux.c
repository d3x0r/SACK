#include <stdhdrs.h>


struct listener_pid_info_list {
	struct listener_pid_info_list **me;
	struct listener_pid_info_list *next;
	struct listener_pid_info *info;
};

static struct netstat_linux_local {
	struct listener_pid_info_list *listeners;
	PDATALIST pdlNodes;
} netstat_linux_local;



static ProcessProcFD( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags ) {
	char buf[256];
	readlink( name, buf, 256 );
	if( strstr( buf, "socket:" ) ) {
		int inode;
		sscanf( buf + 9, "%d", &inode );
		struct listener_pid_info_list *list = netstat_linux_local.listeners;
		while( list ) {
			struct listener_pid_info *info = list->info;
			if( info->pid == -1 ) {
				if( info->inode == inode ) {
					info->pid = psv;
					list->me[0] = list->next;
				}
			}
			list = list->next;
		}
	}
}

static void ProcessProc( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags ) {
	POINTER info;
	char path[256];
	int pid;
	sscanf( name, "/proc/%d", &pid );
	snprintf( path, 256, "%s/fd", name );
	while( ScanFiles( path, "*", &info, ProcessProcFD, 0, pid)){

	}
}



//  /proc/net/tcp /proc/net/tcp6
//    1: 00000000000000000000000001000000:18D2 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075532 1 0000000000e3a644 99 0 0 10 0

void SackNetstat_GetListeners( PDATALIST *ppList ){
	DWORD dwErr;
	PDATALIST pdlNodes = CreateDataList( sizeof( struct listener_pid_info_list )) ;
	ppList[0] = CreateDataList( sizeof( struct listener_pid_info ) );
	char buf[256];
	FILE *f;
	int l = 0;
	struct listener_pid_info info;
	struct listener_pid_info_list link;
	f = sack_fopen( "/proc/net/tcp", "r" );
	if( f ) {
		while( fgets( buf, 256, f ) ) {
			if( !l ) {
				l++;
				continue;
			}else {
				//2: 0100007F:18D2 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075533 1 00000000584f51e4 99 0 0 10 0
				int inode;
				int port;
				int addr;
				int rport;
				int raddr;
				int line;
				int junk;
				uintptr_t junk2;
				//            0   1  2  3  4   5  6  7  8  9 10 11 12 13 14  15 16 17 18
				sscanf( buf, "%d: %x:%x %x:%x %x %x:%x %x:%x %x %x %x %x %x %zx %x %x %x",
					&line, // 0
					&addr, // 1
					&port, // 2
					&raddr, // 3
					&rport, // 4  remote addr
					&junk, // 5  remote port
					&junk,// 6 ppList[0]->data[0].tx_queue,
					&junk,// 7 ppList[0]->data[0].rx_queue,
					&junk,// 8 ppList[0]->data[0].tr,
					&junk,// 9 ppList[0]->data[0].tm_when,
					&junk,// 10 ppList[0]->data[0].retrnsmt,
					&junk,// 11 ppList[0]->data[0].uid,
					&junk,// 12 ppList[0]->data[0].timeout,
					&inode, // 13
					&junk,// 14 ppList[0]->data[0].ref,
					&junk2,// 15 ppList[0]->data[0].pointer,
					&junk,// 16 ppList[0]->data[0].drops,
					&junk,// 17 ppList[0]->data[0].lock,
					&junk,// 18 ppList[0]->data[0].proto
				);
				port = ntohs( port );
				info.port = port;
				info.pid = -1;
				AddDataItem( ppList, &info );
				POINTER p = GetDataItem( ppList, ppList->Cnt-1 );
				link.info = (struct listener_pid_info *)p;
				link.me = &netstat_linux_local.listeners;
				link.next = netstat_linux_local.listeners;
				AddDataItem( pdlNodes, &link );
				struct listener_pid_info_list *listlink = GetDataItem( pdlNodes, pdlNodes->Cnt-1 );
				netstat_linux_local.listeners = listlink;
			}
		}
		fclose( f );
	}


	f = sack_fopen( "/proc/net/tcp6", "r" );
	l = 0;
	if( f ) {
		while( fgets( buf, 256, f ) ) {
			if( !l ) {
				l++;
				continue;
			}else {
				// 0                                 1    2                                3    4  5        6        7  8        9      10     11       12      13 14              15 16  17  18  19  20
				// 1: 00000000000000000000000001000000:18D2 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075532 1 0000000000e3a644 99  0   0   10   0
				int inode;
				int port;
				char addr[33];
				int rport;
				char raddr[33];
				int line;
				int junk;
				uintptr_t junk2;
				//            0   1  2  3  4   5  6  7  8  9 10 11 12 13 14  15 16 17 18
				sscanf( buf, "%d: %s:%x %s:%x %x %x:%x %x:%x %x %x %x %x %x %zx %x %x %x",
					&line, // 0
					addr, // 1
					&port, // 2
					raddr, // 3
					&rport, // 4  remote addr
					&junk, // 5  remote port
					&junk,// 6 ppList[0]->data[0].tx_queue,
					&junk,// 7 ppList[0]->data[0].rx_queue,
					&junk,// 8 ppList[0]->data[0].tr,
					&junk,// 9 ppList[0]->data[0].tm_when,
					&junk,// 10 ppList[0]->data[0].retrnsmt,
					&junk,// 11 ppList[0]->data[0].uid,
					&junk,// 12 ppList[0]->data[0].timeout,
					&inode, // 13
					&junk,// 14 ppList[0]->data[0].ref,
					&junk2,// 15 ppList[0]->data[0].pointer,
					&junk,// 16 ppList[0]->data[0].drops,
					&junk,// 17 ppList[0]->data[0].lock,
					&junk,// 18 ppList[0]->data[0].proto
				);
				port = ntohs( port );
				info.port = port;
				info.pid = -1;
				AddDataItem( ppList, &info );
				POINTER p = GetDataItem( ppList, ppList->Cnt-1 );
				link.info = (struct listener_pid_info *)p;
				link.me = &netstat_linux_local.listeners;
				link.next = netstat_linux_local.listeners;
				AddDataItem( pdlNodes, &link );
				struct listener_pid_info_list *listlink = GetDataItem( pdlNodes, pdlNodes->Cnt-1 );
				netstat_linux_local.listeners = listlink;
			}
		}
		fclose( f );
	}

	POINTER info = NULL;
	while( ScanFiles( "/proc", "*", &info, ProcessProc, 0, 0)){

	}
}
