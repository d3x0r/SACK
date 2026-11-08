#include <stdhdrs.h>
#include <network.h>
#include <iphlpapi.h>

#ifdef _WIN32

#ifdef __cplusplus 
namespace sack {
	namespace network {
#endif

// Record that `pid` is listening on `port`, as one entry per port.
//
// The IPv4 pass used to append an entry per table row, so a port listened on from
// several processes (SO_REUSEPORT) or from several local addresses came back as
// repeated entries carrying one pid each; only the IPv6 pass looked for an existing
// port, and it stopped at the first match, so its pid joined just one of those
// duplicates.  Both passes come through here now, which is what makes pdlPids a list
// of every pid owning the port rather than a list of one.
//
// A process listening on both 0.0.0.0:p and [::]:p is two rows naming the same pid,
// so the pid is only added if it is not already recorded for the port.
static void AddListenerPid( PDATALIST *ppList, uint16_t port, uint32_t pid ) {
	INDEX idx;
	struct listener_pid_info* info;
	DATA_FORALL( ppList[0], idx, struct listener_pid_info*, info ) {
		if( info->port == port ) {
			INDEX p;
			uint32_t* known;
			DATA_FORALL( info->pdlPids, p, uint32_t*, known ) {
				if( known[0] == pid ) return;
			}
			AddDataItem( &info->pdlPids, &pid );
			return;
		}
	}
	{
		struct listener_pid_info l;
		l.port = port;
		l.pdlPids = CreateDataList( sizeof( uint32_t ) );
		AddDataItem( &l.pdlPids, &pid );
		AddDataItem( ppList, &l );
	}
}

void SackNetstat_GetListeners( PDATALIST *ppList ){
	DWORD dwErr;
	ppList[0] = CreateDataList( sizeof( struct listener_pid_info ) );
	MIB_TCPTABLE_OWNER_PID* table = NULL;
	DWORD dwSize;
	dwErr = GetExtendedTcpTable( table, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
	if( dwErr != ERROR_INSUFFICIENT_BUFFER ) {
		lprintf( "unexpected error getting listening sockets: %lu", dwErr );
		return;
	}

	table = (MIB_TCPTABLE_OWNER_PID*)Allocate( dwSize );
	dwErr = GetExtendedTcpTable( table, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
	if( dwErr == ERROR_SUCCESS ) {
		for( int i = 0; i < table->dwNumEntries; i++ ) {
			if( table->table[i].dwState == MIB_TCP_STATE_LISTEN ) {
				AddListenerPid( ppList, ntohs( (uint16_t)table->table[i].dwLocalPort )
				              , table->table[i].dwOwningPid );
			}
		}
		Release( table );
	} else {
		lprintf( "unexpected error getting listening sockets: %lu", dwErr );
		return;
	}


	MIB_TCP6TABLE_OWNER_PID*table6 = NULL;
	dwSize = 0;
	dwErr = GetExtendedTcpTable( table6, &dwSize, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0 );
	if( dwErr != ERROR_INSUFFICIENT_BUFFER ) {
		lprintf( "unexpected error getting listening sockets: %lu", dwErr );
		return;
	}

	table6 = (MIB_TCP6TABLE_OWNER_PID*)Allocate( dwSize );
	dwErr = GetExtendedTcpTable( table6, &dwSize, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0 );
	if( dwErr == ERROR_SUCCESS ) {
		for( int i = 0; i < table6->dwNumEntries; i++ ) {
			if( table6->table[i].dwState == MIB_TCP_STATE_LISTEN ) {
				AddListenerPid( ppList, ntohs( (uint16_t)table6->table[i].dwLocalPort )
				              , table6->table[i].dwOwningPid );
			}
		}
		Release( table6 );
	} else {
		lprintf( "unexpected error getting listening sockets: %lu", dwErr );
		return;
	}


#if 0
		[out]     PVOID           pTcpTable,
		[in, out] PDWORD          pdwSize,
		[in]      BOOL            bOrder,
		[in]      ULONG           ulAf,
		[in]      TCP_TABLE_CLASS TableClass,
		[in]      ULONG           Reserved
	);
#endif

#if 0
	ULONG size = 0;
	MIB_TCPTABLE2 *table = NULL;
	// this is only IPV4
	DWORD dwErr = GetTcpTable2( NULL, &size, FALSE );
	if( dwErr == ERROR_INSUFFICIENT_BUFFER ) {
		table = (MIB_TCPTABLE2*)Allocate( size );
		dwErr = GetTcpTable2( table, &size, FALSE );
		if( dwErr ) {
			lprintf( "Still had an error: %d", dwErr );
			Deallocate( MIB_TCPTABLE2*, table );
			return;
		}
	} else {
		lprintf( "Unhandled initial error - expected size error: %lu", dwErr );
		return;
	}
	int count = size / sizeof( MIB_TCPTABLE2 );
	for( int i = 0; i < table->dwNumEntries; i++ ) {
		if( table->table[i].dwState == MIB_TCP_STATE_LISTEN ) {
			struct listener l;
			l.pid = table->table[i].dwOwningPid;
			l.pid = ntohs( table->table[i].dwLocalPort );
			AddDataItem( ppList, &l );
		}
	  switch (table->table[i].dwState) {
			case MIB_TCP_STATE_CLOSED:
				printf("CLOSED\n");
				break;
			case MIB_TCP_STATE_LISTEN:
				printf("LISTEN\n");
				break;
			case MIB_TCP_STATE_SYN_SENT:
				printf("SYN-SENT\n");
				break;
			case MIB_TCP_STATE_SYN_RCVD:
				printf("SYN-RECEIVED\n");
				break;
			case MIB_TCP_STATE_ESTAB:
				printf("ESTABLISHED\n");
				break;
			case MIB_TCP_STATE_FIN_WAIT1:
				printf("FIN-WAIT-1\n");
				break;
			case MIB_TCP_STATE_FIN_WAIT2:
				printf("FIN-WAIT-2 \n");
				break;
			case MIB_TCP_STATE_CLOSE_WAIT:
				printf("CLOSE-WAIT\n");
				break;
			case MIB_TCP_STATE_CLOSING:
				printf("CLOSING\n");
				break;
			case MIB_TCP_STATE_LAST_ACK:
				printf("LAST-ACK\n");
				break;
			case MIB_TCP_STATE_TIME_WAIT:
				printf("TIME-WAIT\n");
				break;
			case MIB_TCP_STATE_DELETE_TCB:
				printf("DELETE-TCB\n");
				break;
			default:
				printf("UNKNOWN dwState value\n");
				break;
			}
		//lprintf( "Handle: %d", table[i].
	}
#endif

}

#ifdef __cplusplus 
   }
}
#endif

#endif

