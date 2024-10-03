#include <stdhdrs.h>
#include <network.h>
#include <filesys.h>

#ifdef __LINUX__

#ifdef __cplusplus 
namespace sack {
	namespace network {
#endif

struct listener_pid_info_list {
	struct listener_pid_info_list **me;
	struct listener_pid_info_list *next;
	struct listener_pid_info info;
	uint64_t inode;
};

static struct netstat_linux_local {
	struct listener_pid_info_list *listeners;
	struct listener_pid_info_list *solved;
	PDATALIST pdlNodes;
} netstat_linux_local;



static void ProcessProcFD( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags ) {
	char buf[256];
	int len = readlink( name, buf, 256 );
	if( !len ) return;
	buf[len] = 0;
	if( strstr( buf, "socket:" ) ) {
		size_t inode;
		sscanf( buf + 8, "%zd", &inode );
		//lprintf( "node:%s %s %zd %zd", name, buf, inode, psv );
		struct listener_pid_info_list *list = netstat_linux_local.listeners;
		while( list ) {
			struct listener_pid_info_list *next = list->next;
			//lprintf( "Looking at item %p %lld %lld", list, list->inode, inode );
			if( list->info.pid == -1 ) {
				if( list->inode == inode ) {
					list->info.pid = psv;
					if( list->next ) 
						list->next->me = list->me;
					list->me[0] = list->next;

					if( list->next = netstat_linux_local.solved )
						list->next->me = &list->next;
					netstat_linux_local.solved = list;
				}
			}
			list = next;
		}
	}
}

static void ProcessProc( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags ) {
	POINTER info = NULL;
	char path[256];
	int pid;
	int n = sscanf( name, "/proc/%d", &pid );
	if( !n ) return;
	snprintf( path, 256, "%s/cmdline", name );
	FILE *f = fopen( path, "r" );
	if( f ) 
		fclose(f);
	else {
		return;
	}
	snprintf( path, 256, "%s/fd", name );
	while( ScanFiles( path, "*", &info, ProcessProcFD, (enum ScanFileFlags)0, pid));
}



//  /proc/net/tcp /proc/net/tcp6
//    1: 00000000000000000000000001000000:18D2 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075532 1 0000000000e3a644 99 0 0 10 0

void SackNetstat_GetListeners( PDATALIST *ppList ){
	PDATALIST pdlNodes = CreateDataList( sizeof( struct listener_pid_info_list )) ;
	ppList[0] = CreateDataList( sizeof( struct listener_pid_info ) );
	char buf[256];
	FILE *f;
	int l = 0;
	struct listener_pid_info info;
	struct listener_pid_info_list link;
	f = fopen( "/proc/net/tcp", "r" );
	if( f ) {
		while( fgets( buf, 256, f ) ) {
			if( !l ) {
				l++;
				continue;
			}else {
				//2: 0100007F:18D2 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075533 1 00000000584f51e4 99 0 0 10 0
				uint64_t inode;
				int port;
				int addr;
				int rport;
				int raddr;
				int state;
				int line;
				int junk;
				uintptr_t junk2;
				//            0   1  2  3  4   5  6  7  8  9 10 11 12 13           14  15 16 17 18
				sscanf( buf, "%d: %x:%x %x:%x %x %x:%x %x:%x %x %x %x %" PRIu64 "d %x %zx %x %x %x",
					&line, // 0
					&addr, // 1  local addr
					&port, // 2 local port
					&raddr, // 3 remote addr
					&rport, // 4  remote port 
					&state, // 5  state
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
					&junk // 18 ppList[0]->data[0].proto
				);
				if( state != 10 ) continue;
				//lprintf( "Add port4: %d %zd", port, inode );
				link.info.port = port;
				link.info.pid = -1;
				link.inode = inode;
				AddDataItem( &pdlNodes, &link );
			}
		}
		fclose( f );
	}


	f = fopen( "/proc/net/tcp6", "r" );
	l = 0;
	if( f ) {
		while( fgets( buf, 256, f ) ) {
			//lprintf( "Why isn't this what I think it is?%s", buf );
			if( !l ) {
				l++;
				continue;
			}else {
				// 0                                 1    2                                3    4  5        6        7  8        9      10     11       12      13 14              15 16  17  18  19  20
				// 1: 00000000000000000000000001000000:18D2 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 2075532 1 0000000000e3a644 99  0   0   10   0
				uint64_t inode;
				int port;
				char addr[39];
				int rport;
				char raddr[39];
				int line;
				int junk;
				int state;
				uintptr_t junk2;
				int n;
				//            0  1:2  3:4   5  6  7  8  9 10 11 12 13 14  15 16 17 18
				n = sscanf( buf, "%d: %s %s %x %x:%x %x:%x %x %x %x %" PRIu64 "d %x %zx %x %x %x",
					&line, // 0
					addr, // 1:2
					raddr, // 3:4  remote addr
					&state, // 5  
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
					&junk // 18 ppList[0]->data[0].proto
				);
				//lprintf( "Parsed enough arguments? %d  %s %s", n, addr, raddr );
				if( state != 10 ) continue;
				port = strtol( addr+33, NULL, 16 );
				//lprintf( "Add port6: %s %d %zd", addr+33, port, inode );
				link.info.port = port;
				link.info.pid = -1;
				link.inode = inode;
				AddDataItem( &pdlNodes, &link );
			}
		}
		fclose( f );
	}

	{
		struct listener_pid_info_list *link;
		INDEX idx;
		DATA_FORALL( pdlNodes, idx, struct listener_pid_info_list *, link ) {
			//lprintf( "Link %p %p %p %lld", link, link->me, link->next, link->inode );
			if( ( link->next = netstat_linux_local.listeners ) != NULL )
				link->next->me = &link->next;
			link->me = &netstat_linux_local.listeners;
			netstat_linux_local.listeners = link;
		}

	}

	POINTER scaninfo = NULL;
	while( ScanFiles( "/proc", "*", &scaninfo, ProcessProc, (enum ScanFileFlags)0, 0));
	{
		struct listener_pid_info_list *link = netstat_linux_local.solved;
		while( link ) {
			AddDataItem( ppList, &link->info );
			link = link->next;
		}
	}
	DeleteDataList( &pdlNodes );
	netstat_linux_local.listeners = NULL;
	netstat_linux_local.solved = NULL;
	netstat_linux_local.pdlNodes = NULL;
}


#ifdef __cplusplus 
   }
}
#endif

#endif