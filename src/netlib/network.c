///////////////////////////////////////////////////////////////////////////
//
// Filename    -  Network.C
//
// Description -  Network services for Communications Client
//
// Author      -  James Buckeyne
//
// Create Date -  Before now.
// Conversion update for Linux GLIBC 2.1 9/26/2000
//
///////////////////////////////////////////////////////////////////////////

//
//  DEBUG FLAGS IN netstruc.h
//
#define FIX_RELEASE_COM_COLLISION
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stddef.h>
#include <ctype.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#define MAIN_PROGRAM
#include "netstruc.h"
#include <network.h>

//#define DO_LOGGING // override no _DEBUG def to do loggings...
//#define NO_LOGGING // force neverlog....

#include <logging.h>
#include <procreg.h>
#ifdef __LINUX__
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#endif

#include <sharemem.h>
#include <timers.h>
#include <idle.h>

//for GetMacAddress
#ifdef __LINUX__
#include <net/if.h>
//#include <sys/timeb.h>

//*******************8
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#ifdef __MAC__
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
//*******************8

#endif
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#ifdef __CYGWIN__
#include <mingw/tchar.h>
#else
#include <tchar.h>
#endif
#include <wincrypt.h>
#include <iphlpapi.h>
#endif

SACK_NETWORK_NAMESPACE

PRELOAD( InitNetworkGlobalOptions )
{
#ifndef __NO_OPTIONS__
	globalNetworkData.flags.bLogProtocols = SACK_GetProfileIntEx( WIDE("SACK"), WIDE( "Network/Log Protocols" ), 0, TRUE );
	globalNetworkData.flags.bShortLogReceivedData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Received Data(64 byte max)" ), 0, TRUE );
	globalNetworkData.flags.bLogReceivedData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Received Data" ), 0, TRUE );
	globalNetworkData.flags.bLogSentData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Sent Data" ), globalNetworkData.flags.bLogReceivedData, TRUE );
#  ifdef LOG_NOTICES
	globalNetworkData.flags.bLogNotices = 1 || SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Notifications" ), 0, TRUE );
#  endif
	globalNetworkData.dwReadTimeout = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Read wait timeout" ), 5000, TRUE );
	globalNetworkData.dwConnectTimeout = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Connect timeout" ), 10000, TRUE );
#else
	globalNetworkData.flags.bLogNotices = 1;
	globalNetworkData.dwReadTimeout = 5000;
	globalNetworkData.dwConnectTimeout = 10000;
#endif
}

static void LowLevelNetworkInit( void )
{
	if( !global_network_data ) {
		SimpleRegisterAndCreateGlobal( global_network_data );
		InitializeCriticalSec( &globalNetworkData.csNetwork );
	}
}

PRIORITY_PRELOAD( InitNetworkGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY - 1 )
{
	LowLevelNetworkInit();
	if( !globalNetworkData.system_name )
	{
		globalNetworkData.system_name = WIDE("no.network");
	}
}

//----------------------------------------------------------------------------
// forward declaration for the window proc...
_TCP_NAMESPACE
void AcceptClient(PCLIENT pc);
int TCPWriteEx(PCLIENT pc DBG_PASS);
#define TCPWrite(pc) TCPWriteEx(pc DBG_SRC)

size_t FinishPendingRead(PCLIENT lpClient DBG_PASS );
LOGICAL TCPDrainRead( PCLIENT pClient );

_TCP_NAMESPACE_END

//----------------------------------------------------------------------------

#ifndef __MAC__
#  define INCLUDE_MAC_SUPPORT
#endif

NETWORK_PROC( int, GetMacAddress)(PCLIENT pc, uint8_t* buf, size_t *buflen )//int get_mac_addr (char *device, unsigned char *buffer)
{
#ifdef INCLUDE_MAC_SUPPORT
#  ifdef __LINUX__
#    ifdef __THIS_CODE_GETS_MY_MAC_ADDRESS___
	int fd;
	struct ifreq ifr;

	fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		lprintf(WIDE ("Unable to create socket for pclient: %p"), pc);
		return -1;
	}

	strcpy (ifr.ifr_name, GetNetworkLong(pc,GNL_IP));

	if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	close (fd);

	memcpy (pc->hwClient, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
#    endif
	   /* this code queries the arp table to figure out who the other side is */
	//int fd;
	struct arpreq arpr;
	struct ifconf ifc;
	MemSet( &arpr, 0, sizeof( arpr ) );
	lprintf( WIDE( "this is broken." ) );
	MemCpy( &arpr.arp_pa, pc->saClient, sizeof( SOCKADDR ) );
	arpr.arp_ha.sa_family = AF_INET;
	{
		char buf[256];
		ifc.ifc_len = sizeof( buf );
		ifc.ifc_buf = buf;
		ioctl( pc->Socket, SIOCGIFCONF, &ifc );
		{
			int i;
			struct ifreq *IFR;
			IFR = ifc.ifc_req;
			for( i = ifc.ifc_len / sizeof( struct ifreq); --i >=0; IFR++ )
			{
				printf( WIDE( "IF: %s\n" ), IFR->ifr_name );
				strcpy( arpr.arp_dev, WIDE( "eth0" ) );
			}
		}
	}
	DebugBreak();
	if( ioctl( pc->Socket, SIOCGARP, &arpr ) < 0 )
	{
		lprintf( WIDE( "Error of some sort ... %s" ), strerror( errno ) );
		DebugBreak();
	}

	return 0;
#  endif
#  ifdef WIN32
	HRESULT hr;
	ULONG   ulLen;
	// I don't understand this useless cast - from size_t to ULONG?
	// isn't that the same thing?
	ulLen = (ULONG)(*buflen);

	//needs ws2_32.lib and iphlpapi.lib in the linker.
	hr = SendARP ( (IPAddr)GetNetworkLong(pc,GNL_MYIP), (IPAddr)GetNetworkLong(pc,GNL_MYIP), (PULONG)buf, &ulLen);
	(*buflen) = ulLen;
//  The second parameter of SendARP is a PULONG, which is typedef'ed to a pointer to
//  an unsigned long.  The pc->hwClient is a pointer to an array of uint8_t (unsigned chars),
//  actually defined in netstruc.h as uint8_t hwClient[6]; Well, in the end, they are all
//  just addresses, whether they be address to information of eight bits in length, or
//  of (sizeof(unsigned)) in length.  Although this may, in the future, throw a warning.
	//hr = SendARP (GetNetworkLong(pc,GNL_IP), 0, (PULONG)pc->hwClient, &ulLen);
	//lprintf (WIDE("Return %08x, length %8d\n"), hr, ulLen);

	return hr == S_OK;
#  endif
#else
	return 0;
#endif
}

NETWORK_PROC( PLIST, GetMacAddresses)( void )//int get_mac_addr (char *device, unsigned char *buffer)
{
#ifdef INCLUDE_MAC_SUPPORT
#ifdef __LINUX__
#ifdef __THIS_CODE_GETS_MY_MAC_ADDRESS___
	int fd;
	struct ifreq ifr;

	fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		lprintf(WIDE ("Unable to create socket for pclient: %p"), pc);
		return -1;
	}

	strcpy (ifr.ifr_name, GetNetworkLong(pc,GNL_IP));

	if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	close (fd);

	memcpy (pc->hwClient, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
#endif
	/* this code queries the arp table to figure out who the other side is */
	//int fd;
	struct arpreq arpr;
	MemSet( &arpr, 0, sizeof( arpr ) );
#if 0
	lprintf( WIDE( "this is broken." ) );
	MemCpy( &arpr.arp_pa, pc->saClient, sizeof( SOCKADDR ) );
	arpr.arp_ha.sa_family = AF_INET;
	{
		char buf[256];
		ifc.ifc_len = sizeof( buf );
		ifc.ifc_buf = buf;
		ioctl( pc->Socket, SIOCGIFCONF, &ifc );
		{
			int i;
			struct ifreq *IFR;
			IFR = ifc.ifc_req;
			for( i = ifc.ifc_len / sizeof( struct ifreq); --i >=0; IFR++ )
			{
				printf( WIDE( "IF: %s\n" ), IFR->ifr_name );
				strcpy( arpr.arp_dev, WIDE( "eth0" ) );
			}
		}
	}
	DebugBreak();
	if( ioctl( pc->Socket, SIOCGARP, &arpr ) < 0 )
	{
		lprintf( WIDE( "Error of some sort ... %s" ), strerror( errno ) );
		DebugBreak();
	}
#endif

	return 0;
#endif
#ifdef WIN32

	HRESULT hr;
	ULONG   ulLen;
	uint8_t hwClient[6];
	ulLen = 6;

	//needs ws2_32.lib and iphlpapi.lib in the linker.
	hr = SendARP ((IPAddr)GetNetworkLong(NULL,GNL_IP), 0x100007f, (PULONG)&hwClient, &ulLen);
//  The second parameter of SendARP is a PULONG, which is typedef'ed to a pointer to
//  an unsigned long.  The pc->hwClient is a pointer to an array of uint8_t (unsigned chars),
//  actually defined in netstruc.h as uint8_t hwClient[6]; Well, in the end, they are all
//  just addresses, whether they be address to information of eight bits in length, or
//  of (sizeof(unsigned)) in length.  Although this may, in the future, throw a warning.
	//hr = SendARP (GetNetworkLong(pc,GNL_IP), 0, (PULONG)pc->hwClient, &ulLen);
	lprintf (WIDE("Return %08x, length %8d\n"), hr, ulLen);

	return 0;
#endif
#else
	return 0;

#endif
}

//----------------------------------------------------------------------------

void DumpLists( void )
{
	int c = 0;
	PCLIENT pc;
	for( pc = globalNetworkData.AvailableClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( WIDE("Available %p"), pc );
		if( (*pc->me) != pc )
			DebugBreak();	
		c++;
	}
	if( c > 50 )
	{
		lprintf( WIDE( "Overflow available clients." ) );
		//DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ActiveClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( WIDE( WIDE( "Active %p(%d)" ) ), pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( WIDE( "Overflow active clients." ) );
		DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ClosedClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( WIDE( "Closed %p(%d)" ), pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( WIDE( "Overflow closed clients." ) );
		DebugBreak();
	}
}


//----------------------------------------------------------------------------

LOGICAL IsAddressV6( SOCKADDR *addr )
{
	if( addr->sa_family == AF_INET6 && SOCKADDR_LENGTH( addr ) == 28 )
		return TRUE;
	return FALSE;
}

const char * GetAddrString( SOCKADDR *addr )
{
	static char buf[256];
	//lprintf( "addr family is: %d", addr->sa_family );
	if( addr->sa_family == AF_INET )
		snprintf( buf, 256, "%d.%d.%d.%d"
			, *(((unsigned char *)addr) + 4),
			*(((unsigned char *)addr) + 5),
			*(((unsigned char *)addr) + 6),
			*(((unsigned char *)addr) + 7) );
	else if( addr->sa_family == AF_INET6 )
	{
		int first0 = 8;
		int last0 = 0;
		int after0 = 0;
		int n;
		int ofs = 0;
		uint32_t peice;
		for( n = 0; n < 8; n++ ) {
			peice = (*(((unsigned short *)((unsigned char*)addr + 8 + (n * 2)))));
			if( peice ) {
				if( first0 < 8 )
					after0 = 1;
				if( !ofs ) {
					ofs += snprintf( buf + ofs, 256 - ofs, "%x", ntohs( peice ) );
				}
				else {
					//console.log( last0, n );
					if( last0 == 4 && first0 == 0 )
						if( peice == 0xFFFF ) {
							snprintf( buf, 256, "::ffff:%d.%d.%d.%d",
								(*((unsigned char*)addr + 20)),
								(*((unsigned char*)addr + 21)),
								(*((unsigned char*)addr + 22)),
								(*((unsigned char*)addr + 23)) );
							break;
						}
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( peice ) );
				}
			}
			else {
				if( !after0 ) {
					if( first0 > n ) {
						first0 = n;
						ofs += snprintf( buf + ofs, 256 - ofs, ":" );
					}
					if( last0 < n )
						last0 = n;
				}
				if( last0 < n )
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( peice ) );
			}
		}
		if( !after0 )
			ofs += snprintf( buf + ofs, 256 - ofs, ":" );
	}
	else
		snprintf( buf, 256, "unknown protocol" );

	return buf;
}

const char * GetAddrName( SOCKADDR *addr )
{
	char * tmp = ((char**)addr)[-1];
	if( !tmp )
	{
		const char *buf = GetAddrString( addr );
		((char**)addr)[-1] = strdup( buf );
	}
	return ((char**)addr)[-1];
}

void SetAddrName( SOCKADDR *addr, const char *name )
{
	((uintptr_t*)addr)[-1] = (uintptr_t)strdup( name );
}

//---------------------------------------------------------------------------

SOCKADDR *AllocAddrEx( DBG_VOIDPASS )
{
	SOCKADDR *lpsaAddr=(SOCKADDR*)AllocateEx( MAGIC_SOCKADDR_LENGTH + 2 * sizeof( uintptr_t ) DBG_RELAY );
	MemSet( lpsaAddr, 0, MAGIC_SOCKADDR_LENGTH );
	//initialize socket length to something identifiable?
	((uintptr_t*)lpsaAddr)[0] = 3;
	((uintptr_t*)lpsaAddr)[1] = 0; // string representation of address

	lpsaAddr = (SOCKADDR*)( ( (uintptr_t)lpsaAddr ) + sizeof(uintptr_t) * 2 );
	return lpsaAddr;
}
//----------------------------------------------------------------------------

PCLIENT GrabClientEx( PCLIENT pClient DBG_PASS )
#define GrabClient(pc) GrabClientEx( pc DBG_SRC )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		_lprintf(DBG_RELAY)( WIDE( "grabbed client %p(%d)" ), pClient, pClient->Socket );
		lprintf( WIDE( "grabbed client %p Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))" )
				 , pClient->me
					 , &globalNetworkData.ActiveClients, globalNetworkData.ActiveClients, globalNetworkData.ActiveClients?globalNetworkData.ActiveClients->Socket:0
					 , &globalNetworkData.AvailableClients, globalNetworkData.AvailableClients, globalNetworkData.AvailableClients?globalNetworkData.AvailableClients->Socket:0
					 , &globalNetworkData.ClosedClients, globalNetworkData.ClosedClients, globalNetworkData.ClosedClients?globalNetworkData.ClosedClients->Socket:0
				 );
		DumpLists();
#endif
		pClient->dwFlags &= ~CF_STATEFLAGS;
		if( pClient->dwFlags & CF_AVAILABLE )
			lprintf( "Grabbed. %p  %08x", pClient, pClient->dwFlags );
		pClient->LastEvent = GetTickCount();
		if( ( (*pClient->me) = pClient->next ) )
			pClient->next->me = pClient->me;
	}
	return pClient;
}

//----------------------------------------------------------------------------

PCLIENT AddAvailable( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( WIDE( "Add Avail client %p(%d)" ), pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_AVAILABLE;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.AvailableClients;
		if( ( pClient->next = globalNetworkData.AvailableClients ) )
			globalNetworkData.AvailableClients->me = &pClient->next;
		globalNetworkData.AvailableClients = pClient;
	}
	return pClient;
}

//----------------------------------------------------------------------------

PCLIENT AddActive( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( WIDE( "Add Active client %p(%d)" ), pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_ACTIVE;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.ActiveClients;
		if( ( pClient->next = globalNetworkData.ActiveClients ) )
			globalNetworkData.ActiveClients->me = &pClient->next;
		globalNetworkData.ActiveClients = pClient;
	}
	return pClient;
}

//----------------------------------------------------------------------------

PCLIENT AddClosed( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( WIDE( "Add Closed client %p(%d)" ), pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_CLOSED;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.ClosedClients;
		if( ( pClient->next = globalNetworkData.ClosedClients ) )
			globalNetworkData.ClosedClients->me = &pClient->next;
		globalNetworkData.ClosedClients = pClient;
	}
	return pClient;
}

//----------------------------------------------------------------------------

void ClearClient( PCLIENT pc DBG_PASS )
{
	uintptr_t* pbtemp;
	PCLIENT next;
	PCLIENT *me;
	CRITICALSECTION cs;
	// keep the closing flag until it's really been closed. (getfreeclient will try to nab it)
	enum NetworkConnectionFlags  dwFlags = pc->dwFlags & (CF_STATEFLAGS | CF_CLOSING | CF_CONNECT_WAITING | CF_CONNECT_CLOSED);
#ifdef VERBOSE_DEBUG
	lprintf( WIDE("CLEAR CLIENT!") );
#endif
	me = pc->me;
	next = pc->next;
	pbtemp = pc->lpUserData;
	cs = pc->csLock;

	ReleaseAddress( pc->saClient );
	ReleaseAddress( pc->saSource );
#if _WIN32
	if( pc->event ) {
		if( globalNetworkData.flags.bLogNotices )
			_lprintf(DBG_RELAY)( "Closing network event:%p  %p", pc, pc->event );
		WSACloseEvent( pc->event );
	}
#endif
	// sets socket to 0 - so it's not quite == INVALID_SOCKET
#ifdef LOG_NETWORK_EVENT_THREAD
	if( globalNetworkData.flags.bLogNotices )
		_lprintf(DBG_RELAY)( WIDE( "Clear Client %p  %08x   %08x" ), pc, pc->dwFlags, dwFlags );
#endif
	MemSet( pc, 0, sizeof( CLIENT ) ); // clear all information...
	pc->csLock = cs;
	pc->lpUserData = pbtemp;
	if( pc->lpUserData )
		MemSet( pc->lpUserData, 0, globalNetworkData.nUserData * sizeof( uintptr_t ) );
	pc->next = next;
	pc->me = me;
	pc->dwFlags = dwFlags;
}

//----------------------------------------------------------------------------

void NetworkGloalLock( DBG_VOIDPASS ) {
	LOGICAL locked = FALSE;
	do {
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( TryEnterCriticalSection( &globalNetworkData.csNetwork ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
#ifdef LOG_NETWORK_LOCKING
			_lprintf( DBG_RELAY )(WIDE( "Failed enter global?" ));
#endif
			Relinquish();
		}
		else
			locked = TRUE;
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )(WIDE( "Got global lock" ));
#endif
	} while( !locked );
}


void NetworkGloalUnlock( DBG_VOIDPASS ) {
#ifdef USE_NATIVE_CRITICAL_SECTION
	LeaveCriticalSection( &globalNetworkData.csNetwork );
#else
	LeaveCriticalSecEx( &globalNetworkData.csNetwork DBG_RELAY );
#endif
}

//----------------------------------------------------------------------------

void TerminateClosedClientEx( PCLIENT pc DBG_PASS )
{
#ifdef VERBOSE_DEBUG
	_lprintf(DBG_RELAY)( WIDE( "terminate client %p " ), pc );
#endif
	if( !pc )
		return;
	if( pc->dwFlags & CF_TOCLOSE ) {
		lprintf( "WAIT FOR CLOSE LATER..." );
		return;
	}
	if( pc->dwFlags & CF_CLOSED )
	{
		PendingBuffer * lpNext;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		RemoveThreadEvent( pc );

		//lprintf( WIDE( "Terminating closed client..." ) );
		if( IsValid( pc->Socket ) )
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE( "close socket." ) );
#endif
			closesocket( pc->Socket );
			while( pc->lpFirstPending )
			{
				lpNext = pc->lpFirstPending -> lpNext;

				if( pc->lpFirstPending->s.bDynBuffer )
					Deallocate( POINTER, pc->lpFirstPending->buffer.p );

				if( pc->lpFirstPending != &pc->FirstWritePending )
				{
#ifdef LOG_PENDING
					lprintf( WIDE(WIDE( "Data queued...Deleting in remove." )) );
#endif
					Deallocate( PendingBuffer*, pc->lpFirstPending);
				}
				else
				{
#ifdef LOG_PENDING
					lprintf( WIDE("Normal send queued...Deleting in remove.") );
#endif
				}
				if (!lpNext)
					pc->lpLastPending = NULL;
				pc->lpFirstPending = lpNext;
			}
		}
		ClearClient( pc DBG_RELAY );
		// this should move from globalNetworkData.close to globalNetworkData.available.
		AddAvailable( GrabClient( pc ) );
		pc->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		//NetworkUnlock( pc );
	}
	else
		lprintf( WIDE("Client's state was not CLOSED...") );
}

//----------------------------------------------------------------------------

static void CPROC PendingTimer( uintptr_t unused )
{
	PCLIENT pc, next;
#ifdef VERBOSE_DEBUG
	lprintf( WIDE("Enter timer network lock...") );
#endif
	EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef VERBOSE_DEBUG
	lprintf( WIDE("Have network lock.") );
#endif
#ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE("pending timer in global") );
#endif
restart:
	for( pc = globalNetworkData.ActiveClients; pc; pc = next )
	{
		// pc can go away during check, so just grab his next now.
		next = pc->next;
#if defined( LOG_CLIENT_LISTS ) && defined( VERBOSE_DEBUG )
		lprintf( WIDE("Checking active client %p(%d)"), pc, pc->Socket );
#endif
		if( IsValid(pc->Socket) &&
			!(pc->dwFlags & CF_UDP ) )
		{
			//lprintf( WIDE("Entering non UDP client lock...") );
			if( EnterCriticalSecNoWait( &pc->csLock, NULL ) == 1 )
			{
				LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
				lprintf( WIDE("pending timer left global") );
#endif

				//lprintf( WIDE("Have non UDP client lock.") );
				if( !(pc->dwFlags & CF_ACTIVE ) )
				{
					// change to inactive status by the time we got here...
					LeaveCriticalSec( &pc->csLock );
					goto restart;
				}
				if( pc->dwFlags & CF_ACTIVE )
				{
					if( pc->lpFirstPending )
					{
						lprintf( WIDE("Pending Timer:Write") );
						TCPWrite( pc );
					}
					if( (pc->dwFlags & CF_CONNECTED) && (pc->dwFlags & CF_ACTIVE))
					{
						//lprintf( WIDE("Pending Timer:Read"));
						FinishPendingRead( pc DBG_SRC );
						if( pc->dwFlags & CF_TOCLOSE )
						{
							lprintf( WIDE( "Pending read failed - reset connection. (posting to application)" ) );
							InternalRemoveClientEx( pc, FALSE, FALSE );
						}
					}
				}
				LeaveCriticalSec( &pc->csLock );
				EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
				lprintf( WIDE("pending timer in global") );
#endif
			}
			//else
			//   lprintf( WIDE("Failed network lock on non UDP client.") );
		}
	}
	// fortunatly closed clients are owned by this timer...
	for( pc = globalNetworkData.ClosedClients; pc; pc = next )
	{
		lprintf( "This should trigger global event (windows)" );
		next = pc->next; // will dissappear when closed so save it.
		if( GetTickCount() > (pc->LastEvent + 2000) )
		{
			lprintf( WIDE("Closing delay terminated client.") );
			TerminateClosedClient( pc );
		}
	}
	//lprintf( WIDE("Leaving network lock.") );
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE("pending timer left global") );
#endif
}

#ifdef _WIN32

//----------------------------------------------------------------------------

static int NetworkStartup( void )
{
	static int attempt = 0;
	static int nStep = 0,
	          nError;
	static SOCKET sockMaster;
	static SOCKADDR remSin;
	switch( nStep )
	{
	case 0 :
		SystemCheck();
		nStep++;
		attempt = 0;
	case 1 :
		// Sit around, waiting for the network to start...
		//--------------------
		// sorry this is really really ugly to read!
		sockMaster = OpenSocket( TRUE, FALSE, FALSE, 0 );
		if( sockMaster == INVALID_SOCKET )
		{
			lprintf( WIDE( "Clever OpenSocket failed... fallback... and survey sez..." ) );
			//--------------------
			sockMaster = socket( AF_INET, SOCK_DGRAM, 0);
			//--------------------
		}
		//--------------------

		if( sockMaster == INVALID_SOCKET )
		{
			nError = WSAGetLastError();

			lprintf( WIDE( "Failed to create a socket - error is %ld" ), WSAGetLastError() );
			if( nError == 10106 ) // provvider init fail )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				return -2;
			}
			if( nError == WSAENETDOWN )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				//else return NetworkPause(WIDE( "Socket is delaying..." ));
			}
			 nStep = 0; // reset...
			if( ++attempt >= 30 ) return NetworkQuit();
			return 1;//NetworkQuit();
		}

		// Retrieve my IP address and UDP Port number
		remSin.sa_family=AF_INET;
		remSin.sa_data[0]=0;
		remSin.sa_data[1]=0;
		((SOCKADDR_IN*)&remSin)->sin_addr.s_addr=INADDR_ANY;

		nStep++;
		attempt = 0;
		// Fall into next state..............
	case 2 :
		// Associate an address with a socket. (bind)
		if( bind( sockMaster, (PSOCKADDR)&remSin, sizeof(remSin))
			 == SOCKET_ERROR )
		{
			if( WSAGetLastError() == WSAENETDOWN )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				//else return NetworkPause(WIDE( "Bind is Delaying" ));
			}
			nStep = 0; // reset...
			return NetworkQuit();
		}
		nStep++;
		attempt = 0;
		// Fall into next state..............
	case 3 :
		closesocket(sockMaster);
		sockMaster = INVALID_SOCKET;
		nStep = 0; // reset...
		break;
	}
	return 0;
}

//----------------------------------------------------------------------------

static void CPROC NetworkPauseTimer( uintptr_t psv )
{
	int nResult;
	nResult = NetworkStartup();
	if( nResult == 0 )
	{
		while( !globalNetworkData.uNetworkPauseTimer )
			Relinquish();
		RemoveTimer( globalNetworkData.uNetworkPauseTimer );
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = TRUE;
	}
	else if( nResult == -1 )
	{
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = FALSE;
		// exiting ... bad stuff happens
	}
	else if( nResult == -2 )
	{
		// delaying... okay....
	}
}

//----------------------------------------------------------------------------

static void HandleEvent( PCLIENT pClient )
{
	WSANETWORKEVENTS networkEvents;
	if( globalNetworkData.bQuit )
	{
		lprintf( WIDE( "Task is shutting down network... don't handle event." ) );
		return;
	}
	if( !pClient )
	{
		lprintf( WIDE( "How did a NULL client get here?!" ) );
		return;
	}

	pClient->dwFlags |= CF_PROCESSING;
#ifdef LOG_NETWORK_EVENT_THREAD
	//if( globalNetworkData.flags.bLogNotices )
	//	lprintf( WIDE( "Client event on %p" ), pClient );
#endif
	if( WSAEnumNetworkEvents( pClient->Socket, pClient->event, &networkEvents ) == ERROR_SUCCESS )
	{
		if( networkEvents.lNetworkEvents == 0 ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "zero events...%p %p %p" ), pClient, pClient->Socket, pClient->event );
#endif
			return;
		}
		{
			if( pClient->dwFlags & CF_UDP )
			{
				if( networkEvents.lNetworkEvents & FD_READ )
				{
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "Got UDP FD_READ" ) );
#endif
					FinishUDPRead( pClient, 0 );
				}
			}
			else
			{
				THREAD_ID prior = 0;
#ifdef LOG_CLIENT_LISTS
				lprintf( WIDE( "client lists Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))" )
						 , &globalNetworkData.ActiveClients, globalNetworkData.ActiveClients, globalNetworkData.ActiveClients?globalNetworkData.ActiveClients->Socket:0
						 , &globalNetworkData.AvailableClients, globalNetworkData.AvailableClients, globalNetworkData.AvailableClients?globalNetworkData.AvailableClients->Socket:0
						 , &globalNetworkData.ClosedClients, globalNetworkData.ClosedClients, globalNetworkData.ClosedClients?globalNetworkData.ClosedClients->Socket:0
						 );
#endif

#ifdef LOG_NETWORK_LOCKING
				lprintf( WIDE( "Handle Event left global" ) );
#endif
				// if an unknown socket issued a
				// notification - close it - unknown handling of unknown socket.
#ifdef LOG_NETWORK_EVENT_THREAD
				if( globalNetworkData.flags.bLogNotices )
					lprintf( WIDE( "events : %08x on %p" ), networkEvents.lNetworkEvents, pClient );
#endif
				if( networkEvents.lNetworkEvents & FD_CONNECT )
				{
					{
						uint16_t wError = networkEvents.iErrorCode[FD_CONNECT_BIT];
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("FD_CONNECT on %p"), pClient );
#endif
						if( !wError )
							pClient->dwFlags |= CF_CONNECTED;
						else
						{
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( WIDE("Connect error: %d"), wError );
#endif
							pClient->dwFlags |= CF_CONNECTERROR;
						}
						if( !( pClient->dwFlags & CF_CONNECTERROR ) )
						{
							// since this is done before connecting is clear, tcpwrite
							// may make notice of previously queued data to
							// connection opening...
							//lprintf( WIDE("Sending any previously queued data.") );

							// with events, we get a FD_WRITE also... which calls tcpwrite.
							//TCPWrite( pClient );
						}
						pClient->dwFlags &= ~CF_CONNECTING;
						if( pClient->connect.ThisConnected )
						{
							if( !wError && !pClient->saSource ) {
#ifdef __LINUX__
								socklen_t
#else
								int
#endif
									nLen = MAGIC_SOCKADDR_LENGTH;
								if( !pClient->saSource )
									pClient->saSource = AllocAddr();
								if( getsockname( pClient->Socket, pClient->saSource, &nLen ) )
								{
									lprintf( WIDE( "getsockname errno = %d" ), errno );
								}
							}
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( WIDE( "Post connect to application %p  error:%d" ), pClient, wError );
#endif
							if( pClient->dwFlags & CF_CPPCONNECT )
								pClient->connect.CPPThisConnected( pClient->psvConnect, wError );
							else
								pClient->connect.ThisConnected( pClient, wError );
						}
						//lprintf( WIDE("Returned from application connected callback") );
						// check to see if the read was queued before the connect
						// actually completed...
						if( (pClient->dwFlags & ( CF_ACTIVE | CF_CONNECTED )) ==
							( CF_ACTIVE | CF_CONNECTED ) )
						{
							if( pClient->read.ReadComplete )
								if( pClient->dwFlags & CF_CPPREAD )
									pClient->read.CPPReadComplete( pClient->psvRead, NULL, 0 );
								else
									pClient->read.ReadComplete( pClient, NULL, 0 );
						}
						if( pClient->pWaiting )
							WakeThread( pClient->pWaiting );
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "FD_CONNECT Completed" ) );
#endif
						//lprintf( WIDE("Returned from application inital read complete.") );
					}
				}
				if( networkEvents.lNetworkEvents & FD_READ )
				{
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( WIDE( "FD_READ" ) );
#endif
					if( pClient->bDraining )
					{
						TCPDrainRead( pClient );
					}
					else
					{
						// got a network event, and won't get another until recv is called.
						// mark that the socket has data, then the pend_read code will trigger the finishpendingread.
						if( FinishPendingRead( pClient DBG_SRC ) == 0 )
						{
							pClient->dwFlags |= CF_READREADY;
						}
						if( pClient->dwFlags & CF_TOCLOSE )
						{
							lprintf( WIDE( "Pending read failed - and wants to close." ) );
							//InternalRemoveClientEx( pc, TRUE, FALSE );
						}
					}
				}
				if( networkEvents.lNetworkEvents & FD_WRITE )
				{
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( WIDE("FD_Write") );
#endif
					TCPWrite(pClient);
				}

				if( networkEvents.lNetworkEvents & FD_CLOSE )
				{
					if( !pClient->bDraining )
					{
						size_t bytes_read;
						// act of reading can result in a close...
						// there are things like IE which close and send
						// adn we might get the close notice at application level indicating there might still be data...
						while( ( bytes_read = FinishPendingRead( pClient DBG_SRC) ) > 0
							&& bytes_read != (size_t)-1 ); // try and read...
						//if( pClient->dwFlags & CF_TOCLOSE )
						{
							//lprintf( "Pending read failed - reset connection. (well this is FD_CLOSE so yeah...???)" );
							//InternalRemoveClientEx( pc, TRUE, FALSE );
						}
					}
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf(WIDE( "FD_CLOSE... %p  %08x" ), pClient, pClient->dwFlags );
#endif
					if( pClient->dwFlags & CF_ACTIVE )
					{
						// might already be cleared and gone..
						InternalRemoveClientEx( pClient, FALSE, TRUE );
						TerminateClosedClient( pClient );
					}
					// section will be blank after termination...(correction, we keep the section state now)
					pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
				}
				if( networkEvents.lNetworkEvents & FD_ACCEPT )
				{
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( WIDE("FD_ACCEPT on %p"), pClient );
#endif
					AcceptClient(pClient);
				}
				//lprintf( WIDE("leaveing event handler...") );
				//lprintf( WIDE("Left event handler CS.") );
			}
		}
	}
	else
	{
		DWORD dwError = WSAGetLastError();
		if( dwError == 10038 )
		{
			// no longer a socket, probably in a closed or closing state.
		}
		else
			lprintf( WIDE( "Event enum failed... do what? close socket? %p %" _32f ), pClient, dwError );
	}
	pClient->dwFlags &= ~CF_PROCESSING;
}

#endif // if defined _WIN32

//----------------------------------------------------------------------------

void SetNetworkWriteComplete( PCLIENT pClient
                            , cWriteComplete WriteComplete )
{
	if( pClient && IsValid( pClient->Socket ) )
	{
		pClient->write.WriteComplete = WriteComplete;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkWriteComplete( PCLIENT pClient
                               , cppWriteComplete WriteComplete
                               , uintptr_t psv)
{                                        
	if( pClient && IsValid( pClient->Socket ) )
	{
		pClient->write.CPPWriteComplete = WriteComplete;
		pClient->psvWrite = psv;
		pClient->dwFlags |= CF_CPPWRITE;
	}
}

//----------------------------------------------------------------------------

void SetNetworkCloseCallback( PCLIENT pClient
                            , cCloseCallback CloseCallback )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CloseCallback = CloseCallback;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkCloseCallback( PCLIENT pClient
                               , cppCloseCallback CloseCallback
                               , uintptr_t psv)
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CPPCloseCallback = CloseCallback;
		pClient->psvClose = psv;
		pClient->dwFlags |= CF_CPPCLOSE;
	}
}

//----------------------------------------------------------------------------

void SetNetworkReadComplete( PCLIENT pClient
                           , cReadComplete pReadComplete )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.ReadComplete = pReadComplete;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkReadComplete( PCLIENT pClient
                              , cppReadComplete pReadComplete
                              , uintptr_t psv)
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.CPPReadComplete = pReadComplete;
		pClient->psvRead = psv;
		pClient->dwFlags |= CF_CPPREAD;
	}
}

//----------------------------------------------------------------------------


#if defined( _WIN32 )
//----------------------------------------------------------------------------

static uintptr_t CPROC NetworkThreadProc( PTHREAD thread );

void RemoveThreadEvent( PCLIENT pc ) {
	struct peer_thread_info *thread = pc->this_thread;
	if( !thread ) return; // could be closed (accept, initial read, protocol causes close before ever completing getting scheduled)

	// reduce peer wait count to 1.
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "Remove client %p from %p thread events...  proc:%d  ev:%d  wait:%d", pc, thread, thread->flags.bProcessing, thread->nEvents, thread->nWaitEvents );
#endif
	thread->nEvents = 1;
	if( thread->thread != MakeThread() && !thread->flags.bProcessing )
		while( thread->nEvents != thread->nWaitEvents ) {
			if( !thread->flags.bProcessing )
			{
				// have to make sure threads reset to the new list.
				//lprintf( "have to wait for thread to be in wait state..." );
				WSASetEvent( thread->hThread );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				while( (thread->nWaitEvents > 1) || thread->flags.bProcessing )
					Relinquish();
				EnterCriticalSec( &globalNetworkData.csNetwork );
			}
			else
				break;
			Relinquish();
		}
	{
		INDEX idx;
		INDEX c = 0;
		PCLIENT previous;
		PLIST newList = CreateList();
		SetLink( &newList, 65, 0 );
		//EmptyDataList( &thread->event_list );
		LIST_FORALL( thread->monitor_list, idx, PCLIENT, previous ) {
			if( previous != pc )
			{
				AddLink( &newList, previous );
				SetDataItem( &thread->event_list, c++, GetDataItem( &thread->event_list, idx ) );
			}
		}
		thread->event_list->Cnt = c;
		DeleteListEx( &thread->monitor_list DBG_SRC );
		thread->monitor_list = newList;
		pc->this_thread = NULL;
		thread->nEvents = (int)c;
#ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "peer %p now has %d events", thread, thread->nEvents );
#endif
	}
	if( thread->parent_peer )  // don't bubble sort root thread
		while( ( thread->nEvents < thread->parent_peer->nEvents ) && thread->parent_peer->parent_peer ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			//lprintf( "swapping this with parent peer ... this events %d  parent %d", thread->nEvents, thread->parent_peer->nEvents );
#endif
			if( thread->child_peer )
				thread->child_peer->parent_peer = thread->parent_peer;
			thread->parent_peer->child_peer = thread->child_peer;
			struct peer_thread_info *tmp = thread->parent_peer;
			tmp->parent_peer->child_peer = thread;
			thread->parent_peer = tmp->parent_peer;
			tmp->parent_peer = thread;
			thread->child_peer = tmp;
		}
}

// unused parameter broadcsat on windows; not needed.
void AddThreadEvent( PCLIENT pc, int broadcsat )
{
	struct peer_thread_info *peer = globalNetworkData.root_thread;
	LOGICAL addPeer = FALSE;
#ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "Add thread event %p %p %08x", pc, pc->event, pc->dwFlags );
#endif
	for( ; peer; peer = peer->child_peer ) {
		if( !peer->child_peer ) {
#ifdef LOG_NOTICES
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "On last peer..." );
#endif
			if( peer->nEvents > globalNetworkData.nPeers ) {
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "global peers is %d, this has %d", globalNetworkData.nPeers, peer->nEvents );
#endif
				addPeer = TRUE;
				break;
			}
			if( peer->nEvents >= 60 ) {
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "this has max events already....", globalNetworkData.nPeers, peer->nEvents );
#endif
				addPeer = TRUE;
				break;
			}
		}
		if( peer->nEvents < 60 ) {
			if( !peer->child_peer )// last thread.
				break;
			if( peer->nEvents < peer->child_peer->nEvents ) {
#ifdef LOG_NOTICES
				//if( globalNetworkData.flags.bLogNotices )
				//	lprintf( "this event has fewer than the next thread's events %d  %d", peer->nEvents, peer->child_peer->nEvents );
#endif
				break;
			}
		}
	}
	if( addPeer ) {
#ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( "Creating a new thread...." );
#endif
		AddLink( &globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)peer ) );
		globalNetworkData.nPeers++;
		while( !peer->child_peer )
			Relinquish();
		if( globalNetworkData.root_thread != peer ) {
			// relink to be higher in list of peers so it's found earlier.
#ifdef LOG_NOTICES
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "Relinking thread to be after root peer (no events, so it must be first)" );
#endif
			peer->child_peer->child_peer = globalNetworkData.root_thread->child_peer;
			globalNetworkData.root_thread->child_peer->parent_peer = peer->child_peer;
			globalNetworkData.root_thread->child_peer = peer->child_peer;
			peer->child_peer->parent_peer = globalNetworkData.root_thread;
			peer->child_peer = NULL;
			peer = globalNetworkData.root_thread->child_peer;
		}
		else
			peer = peer->child_peer;
	}

	// make sure to only add this handle when the first peer will also be added.
	// this means the list can be 61 and at this time no more.
	AddLink( &peer->monitor_list, pc );
	AddDataItem( &peer->event_list, &pc->event );
	pc->this_thread = peer;
	pc->flags.bAddedToEvents = 1;
	peer->nEvents++;
#ifdef LOG_NETWORK_EVENT_THREAD
	//lprintf( "peer %p now has %d events", peer, peer->nEvents );
#endif
	if( !peer->flags.bProcessing && peer->parent_peer ) // scheduler thread already awake do not wake him.
		WSASetEvent( peer->hThread );
}

// this is passed '0' when it is called internally
// this is passed '1' when it is called by idleproc
int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check )
{
	//lprintf( WIDE("Check messages.") );
	if( globalNetworkData.bQuit )
		return -1;
	if( thread->flags.bProcessing )
		return 0;
	// disallow changing of the lists.
	if( !thread->parent_peer )
	{
		EnterCriticalSec( &globalNetworkData.csNetwork );
		{
			PCLIENT pc;
			PCLIENT next;
			for( pc = globalNetworkData.ClosedClients; pc; pc = next )
			{
#    ifdef LOG_NOTICES
				lprintf( "Have a closed client to check..." );
#    endif
				next = pc->next;
				if( +GetTickCount() > (pc->LastEvent + 1000) )
				{
					lprintf( "Remove thread event on closed thread (should be terminate here..)" );
					TerminateClosedClient( pc );  // also does the remove.
					//RemoveThreadEvent( pc ); // also does the close.
				}
			}
		}
		{
			PCLIENT pc;
			while( pc = (PCLIENT)DequeLink( &globalNetworkData.client_schedule ) )
			{
				// use this for "added to schedule".  Closing removes from schedule.
				if( !pc->flags.bAddedToEvents )
				{
					if( pc->dwFlags & CF_CLOSED || (!(pc->dwFlags & CF_ACTIVE)) )
					{
						lprintf( WIDE( " Found closed? %p" ), pc );
						continue;
					}
#ifdef LOG_NETWORK_EVENT_THREAD
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "Added to schedule : %p %08x" ), pc, pc->dwFlags );
#endif
					AddThreadEvent( pc, 0 );
				}
				else
					lprintf( "Client in schedule queue, but it is already schedule?! %p", pc );
			}
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( WIDE( "Process Network left global" ) );
#endif
	}
	else
	{
		// wait for master thread to set up the proper wait
		while( thread->nEvents == 0 )
			Relinquish();
	}
	while( 1 )
	{
		int32_t result;
		// want to wait here anyhow...
#ifdef LOG_NETWORK_EVENT_THREAD
		//if( globalNetworkData.flags.bLogNotices )
		//	lprintf( WIDE( "%p Waiting on %d events" ), thread, thread->nEvents );
#endif
		thread->nWaitEvents = thread->nEvents;
		thread->flags.bProcessing = 0;
		result = WSAWaitForMultipleEvents( thread->nEvents
													, (const HANDLE *)thread->event_list->data
													, FALSE
													, (quick_check)?0:WSA_INFINITE
													, FALSE
													);
		if( globalNetworkData.bQuit )
			return -1;
#ifdef LOG_NETWORK_EVENT_THREAD
		//if( globalNetworkData.flags.bLogNotices )
		//	lprintf( WIDE( "Event Wait Result was %d" ), result );
#endif
		// this should never be 0, but we're awake, not sleeping, and should say we're in a place
		// where we probably do want to be woken on a 0 event.
		if( result != WSA_WAIT_EVENT_0 )
		{
#ifdef LOG_NETWORK_EVENT_THREAD
			//if( globalNetworkData.flags.bLogNotices )
			//	lprintf( WIDE("Begin - thread processing %d"), result );
#endif
			thread->flags.bProcessing = 1;
		}
		thread->nWaitEvents = 0;
		if( result == WSA_WAIT_FAILED )
		{
			DWORD dwError = WSAGetLastError();
			if( dwError == WSA_INVALID_HANDLE )
			{
				lprintf( WIDE( "Rebuild list, have a bad event handle somehow." ) );
				break;
			}
			lprintf( WIDE( "error of wait is %d   %p" ), dwError, thread );
			LogBinary( thread->event_list->data, 64 );
			break;
		}
#ifndef UNDER_CE
		else if( result == WSA_WAIT_IO_COMPLETION )
		{
			// reselect... not sure where io completion fits for network...
			continue;
		}
#endif
		else if( result == WSA_WAIT_TIMEOUT )
		{
			if( quick_check )
				return 1;
		}
		else if( result >= WSA_WAIT_EVENT_0 )
		{
			// if result is _0, then it's the global event, and we just return.
			if( result > WSA_WAIT_EVENT_0 )
			{
				PCLIENT pc = (PCLIENT)GetLink( &thread->monitor_list, result - (WSA_WAIT_EVENT_0) );
				PCLIENT pcLock;
				//lprintf( "index is %d", result-(WSA_WAIT_EVENT_0) );
				while( !(pcLock = NetworkLockEx( pc DBG_SRC ) ) )
				{
					// done with events; inactive sockets can't have events
					if( !(pc->dwFlags & CF_ACTIVE) )
					{
						pcLock = NULL;
						break;
					}
					Relinquish();
				}
				if( pcLock ) {
					if( pc->dwFlags & CF_AVAILABLE ) {
						lprintf( "thread event happened on a now available client." );
					}
					else
						HandleEvent( pc );
					NetworkUnlock( pc );
				}
				/*
				if( thread->parent_peer )
				{
					// if this is a child worker, wait for main to rebuild events.
					// if this was the main thread, it would wake us anyway...
					WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
					while( thread->nEvents != 1 )
						Relinquish();
				}
				*/
				if( !quick_check )
					continue;
			}
			//else
			{
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( thread->parent_peer?WIDE("RESET THREAD EVENT"):WIDE( "RESET GLOBAL EVENT" ) );
#endif
				WSAResetEvent( thread->hThread );
			}
			return 1;
		}
	}
	// result 0... we had nothing to do
	// but we are this thread.
	return 0;
}

//----------------------------------------------------------------------------
static int CPROC IdleProcessNetworkMessages( uintptr_t quick_check )
{
	struct peer_thread_info *this_thread = IsNetworkThread();
	if( this_thread )
		return ProcessNetworkMessages( this_thread, quick_check );
	return -1;
}


#else // if !__LINUX__

static uintptr_t CPROC NetworkThreadProc( PTHREAD thread );

void RemoveThreadEvent( PCLIENT pc ) {
	struct peer_thread_info *thread = pc->this_thread;
	// could be closed (accept, initial read, protocol causes close before ever completing getting scheduled)
	if( !thread ) return;
	{
#  ifdef __MAC__
#    ifdef __64__
		kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		} else {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET64( &ev, pc->SocketBroadcast, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#    else
		kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		} else {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET( &ev, pc->SocketBroadcsat, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		}
#    endif
#  else
		int r;
		r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->Socket, NULL );
		if( r < 0 ) lprintf( "Error removing:%d", errno );
		if( pc->SocketBroadcast ) {
			r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->SocketBroadcast, NULL );
			if( r < 0 ) lprintf( "Error removing:%d", errno );
		}
		pc->flags.bAddedToEvents = 0;
		pc->this_thread = NULL;
#  endif
	}
	LockedDecrement( &thread->nEvents );
	// don't bubble sort root thread
	if( thread->parent_peer )
		while( (thread->nEvents < thread->parent_peer->nEvents) && thread->parent_peer->parent_peer ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			//lprintf( "swapping this with parent peer ... this events %d  parent %d", thread->nEvents, thread->parent_peer->nEvents );
#endif
			if( thread->child_peer )
				thread->child_peer->parent_peer = thread->parent_peer;
			thread->parent_peer->child_peer = thread->child_peer;
			struct peer_thread_info *tmp = thread->parent_peer;
			tmp->parent_peer->child_peer = thread;
			thread->parent_peer = tmp->parent_peer;
			tmp->parent_peer = thread;
			thread->child_peer = tmp;
		}
}

struct event_data {
	PCLIENT pc;
	int broadcast;
};

void AddThreadEvent( PCLIENT pc, int broadcast )
{
	struct peer_thread_info *peer = globalNetworkData.root_thread;
	LOGICAL addPeer = FALSE;
#ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "Add thread event %p %d %08x  %s", pc, broadcast?pc->SocketBroadcast:pc->Socket, pc->dwFlags, broadcast?"broadcast":"direct" );
#endif
	if( !broadcast ) {
		for( ; peer; peer = peer->child_peer ) {
			if( !peer->child_peer ) {
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "On last peer..." );
#endif
				if( peer->nEvents > globalNetworkData.nPeers ) {
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "global peers is %d, this has %d", globalNetworkData.nPeers, peer->nEvents );
#endif
					addPeer = TRUE;
					break;
				}
				if( peer->nEvents >= 2560 ) {
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "this has max events already.... %d %d", globalNetworkData.nPeers, peer->nEvents );
#endif
					addPeer = TRUE;
					break;
				}
			}
			if( peer->nEvents < 2560 ) {
												// last thread.
				if( !peer->child_peer )
					break;
				if( peer->nEvents < peer->child_peer->nEvents ) {
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "this event has fewer than the next thread's events %d  %d", peer->nEvents, peer->child_peer->nEvents );
#endif
					break;
				}
			}
		}
		if( addPeer ) {
#ifdef LOG_NOTICES
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "Creating a new thread...." );
#endif
			AddLink( &globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)peer ) );
			globalNetworkData.nPeers++;
			while( !peer->child_peer )
				Relinquish();
			if( globalNetworkData.root_thread != peer ) {
				// relink to be higher in list of peers so it's found earlier.
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "Relinking thread to be after root peer (no events, so it must be first)" );
#endif
				peer->child_peer->child_peer = globalNetworkData.root_thread->child_peer;
				globalNetworkData.root_thread->child_peer->parent_peer = peer->child_peer;
				globalNetworkData.root_thread->child_peer = peer->child_peer;
				peer->child_peer->parent_peer = globalNetworkData.root_thread;
				peer->child_peer = NULL;
				peer = globalNetworkData.root_thread->child_peer;
			}
			else
				peer = peer->child_peer;
		}
	} else {
		peer = pc->this_thread; // add broadcast to the same event as the original.
	}
	// make sure to only add this handle when the first peer will also be added.
	// this means the list can be 61 and at this time no more.
	{
#  ifdef __MAC__
#    ifdef __64__
		kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		else {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#    else
		kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)pc );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
		else {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (uintptr_t)pc );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD|EV_ENABLE|EV_CLEAR, 0, 0, (uintptr_t)pc );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
#    endif
#  else
		int r;
		struct epoll_event ev;
		ev.data.ptr = New( struct event_data );
		((struct event_data*)ev.data.ptr)->pc = pc;
		((struct event_data*)ev.data.ptr)->broadcast = broadcast;
		if( pc->dwFlags & CF_LISTEN )
			ev.events = EPOLLIN;
		else {

			ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
		}
#ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "peer add socket to %d now has 0x%x events", peer->epoll_fd, ev.events );
#endif
		r = epoll_ctl( peer->epoll_fd, EPOLL_CTL_ADD, broadcast?pc->SocketBroadcast:pc->Socket, &ev );
		if( r < 0 ) lprintf( "Error adding:%d", errno );
#  endif
	}
	if( !pc->this_thread ) {
		LockedIncrement( &peer->nEvents );
		pc->this_thread = peer;
		pc->flags.bAddedToEvents = 1;
	}
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "peer %p now has %d events", peer, peer->nEvents );
#endif
	// scheduler thread already awake do not wake him.
}

int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t unused )
{
	int cnt;
	struct timeval time;
	if( globalNetworkData.bQuit )
		return -1;

	{
#  ifdef __MAC__
#    ifdef __64__
		kevent64_s events[10];
		cnt = kevent64( thread->kqueue, NULL, 0, &events, 10, 0, NULL );
#    else
		kevent events[10];
		cnt = kevent( thread->kqueue, NULL, 0, &events, 10, NULL );
#    endif
#  else
		struct epoll_event events[10];
#    ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "Wait on %d", thread->epoll_fd );
#    endif
		cnt = epoll_wait( thread->epoll_fd, events, 10, -1 );
#  endif
		if( cnt < 0 )
		{
			int err = errno;
			if( err == EINTR )
				return 1;
			Log1( WIDE( "Sorry epoll_pwait/kevent call failed... %d" ), err );
			return 1;
		}
		if( cnt > 0 )
		{
			int n;
			struct event_data *event_data;
			THREAD_ID prior = 0;
			PCLIENT next;
			for( n = 0; n < cnt; n++ ) {
#  ifdef __MAC__
				pc = (PCLIENT)events[n].udata;
#  else
				event_data = (struct event_data*)events[n].data.ptr;
#  endif
#  ifdef LOG_NOTICES
				lprintf( "Process %d %x", event_data->broadcast?event_data->pc->SocketBroadcast:event_data->pc->Socket
						 , events[n].events );
#  endif
				if( event_data == (struct event_data*)1 ) {
					//char buf;
					//stat = read( GetThreadSleeper( thread->pThread ), &buf, 1 );
					//call wakeable sleep to just clear the sleep; because this is an event on the sleep pipe.
					WakeableSleep( SLEEP_FOREVER );
					return 1;
				}
				while( !NetworkLock( event_data->pc ) ) {
					if( event_data->pc->dwFlags & CF_AVAILABLE )
                  break;
					Relinquish();
				}
				if( event_data->pc->dwFlags & CF_AVAILABLE )
					continue;

				if( !IsValid( event_data->pc->Socket ) ) {
					NetworkUnlock( event_data->pc );
					continue;
				}

#  ifdef __MAC__
				if( events[n].filter == EVFILT_READ )
#  else
				if( events[n].events & EPOLLIN )
#  endif
				{
					//lprintf( "EPOLLIN/EVFILT_READ" );
					if( !(event_data->pc->dwFlags & CF_ACTIVE) )
					{
						// change to inactive status by the time we got here...
					}

					else if( event_data->pc->dwFlags & CF_LISTEN )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "accepting..." ) );
#endif
						AcceptClient( event_data->pc );
					}
					else if( event_data->pc->dwFlags & CF_UDP )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "UDP Read Event..." ) );
#endif
						//lprintf( "UDP READ" );
						FinishUDPRead( event_data->pc, event_data->broadcast );
					}
					else if( event_data->pc->bDraining )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "TCP Drain Event..." ) );
#endif
						TCPDrainRead( event_data->pc );
					}
					else if( event_data->pc->dwFlags & CF_READPENDING )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "TCP Read Event..." ) );
#endif
						// packet oriented things may probably be reading only
						// partial messages at a time...
						FinishPendingRead( event_data->pc DBG_SRC );
						if( event_data->pc->dwFlags & CF_TOCLOSE )
						{
//#ifdef LOG_NOTICES
//							if( globalNetworkData.flags.bLogNotices )
								lprintf( WIDE( "Pending read failed - reset connection." ) );
//#endif
							InternalRemoveClientEx( event_data->pc, FALSE, FALSE );
							TerminateClosedClient( event_data->pc );
						}
						else if( !event_data->pc->RecvPending.s.bStream )
							event_data->pc->dwFlags |= CF_READREADY;
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "TCP Set read ready..." ) );
#endif
						event_data->pc->dwFlags |= CF_READREADY;
					}
				}

#  ifdef __MAC__
				if( events[n].filter == EVFILT_WRITE )
#  else
				if( events[n].events & EPOLLOUT )
#  endif
				{
					//lprintf( "EPOLLOUT" );
					if( !(event_data->pc->dwFlags & CF_ACTIVE) )
					{
						// change to inactive status by the time we got here...
					}
					else if( event_data->pc->dwFlags & CF_CONNECTING )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "Connected!" ) );
#endif
						event_data->pc->dwFlags |= CF_CONNECTED;
						event_data->pc->dwFlags &= ~CF_CONNECTING;

						{
                     PCLIENT pc = event_data->pc;
#ifdef __LINUX__
							socklen_t
#else
								int
#endif
								nLen = MAGIC_SOCKADDR_LENGTH;
							if( !pc->saSource )
								pc->saSource = AllocAddr();
							if( getsockname( pc->Socket, pc->saSource, &nLen ) )
							{
								lprintf( WIDE("getsockname errno = %d"), errno );
							}
                     if( pc->saSource->sa_family == AF_INET )
								SET_SOCKADDR_LENGTH( pc->saSource, IN_SOCKADDR_LENGTH );
                     else if( pc->saSource->sa_family == AF_INET6 )
								SET_SOCKADDR_LENGTH( pc->saSource, IN6_SOCKADDR_LENGTH );
							else
								SET_SOCKADDR_LENGTH( pc->saSource, nLen );
						}

						{
							int error;
							socklen_t errlen = sizeof( error );
							getsockopt( event_data->pc->Socket, SOL_SOCKET
								, SO_ERROR
										 , &error, &errlen );
							//lprintf( WIDE( "Error checking for connect is: %s on %d" ), strerror( error ), event_data->pc->Socket );
							if( event_data->pc->pWaiting )
							{
#ifdef LOG_NOTICES
								if( globalNetworkData.flags.bLogNotices )
									lprintf( WIDE( "Got connect event, waking waiter.." ) );
#endif
								WakeThread( event_data->pc->pWaiting );
							}
							if( event_data->pc->connect.ThisConnected )
								event_data->pc->connect.ThisConnected( event_data->pc, error );
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connect error was: %d", error );
#endif
							// if connected okay - issue first read...
							if( !error )
							{
#ifdef LOG_NOTICES
								lprintf( "Read Complete" );
#endif
								if( event_data->pc->read.ReadComplete )
								{
#ifdef LOG_NOTICES
									lprintf( "Initial Read Complete" );
#endif
									event_data->pc->read.ReadComplete( event_data->pc, NULL, 0 );
								}
								if( event_data->pc->lpFirstPending )
								{
									lprintf( WIDE( "Data was pending on a connecting socket, try sending it now" ) );
									TCPWrite( event_data->pc );
								}
							}
							else
							{
								event_data->pc->dwFlags |= CF_CONNECTERROR;
							}
						}
					}
					else if( event_data->pc->dwFlags & CF_UDP )
					{
						//lprintf( "UDP WRITE IS NEVER QUEUED." );
						// udp write event complete....
						// do we ever care? probably sometime...
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "TCP Write Event..." ) );
#endif
						event_data->pc->dwFlags &= ~CF_WRITEISPENDED;
						TCPWrite( event_data->pc );
					}
				}
				LeaveCriticalSec( &event_data->pc->csLock );
			}
			// had some event  - return 1 to continue working...
		}
		return 1;
	}
	//lprintf( WIDE("Attempting to wake thread (send sighup!)") );
	//WakeThread( globalNetworkData.pThread );
	//lprintf( WIDE("Only reason should get here is if it's not this thread...") );
	return -1;
}

//----------------------------------------------------------------------------
static int CPROC IdleProcessNetworkMessages( uintptr_t quick_check )
{
	struct peer_thread_info *this_thread = IsNetworkThread();
	if( this_thread )
		return ProcessNetworkMessages( this_thread, quick_check );
	return -1;
}

#endif


uintptr_t CPROC NetworkThreadProc( PTHREAD thread )
{
	struct peer_thread_info *peer_thread = (struct peer_thread_info*)GetThreadParam( thread );
	struct peer_thread_info this_thread;
	// and when unloading should remove these timers.
	if( !peer_thread )
	{
		//globalNetworkData.uPendingTimer = AddTimer( 5000, PendingTimer, 0 );
#ifdef _WIN32
		globalNetworkData.uNetworkPauseTimer = AddTimerEx( 1, 1000, NetworkPauseTimer, 0 );
		if( !globalNetworkData.client_schedule )
			globalNetworkData.client_schedule = CreateLinkQueue();
#endif
#ifdef __LINUX__
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = TRUE;
#endif
		//globalNetworkData.hMonitorThreadControlEvent = CreateEvent( NULL, 0, FALSE, NULL );
	}

	this_thread.monitor_list = NULL;
#ifdef _WIN32
	this_thread.event_list = CreateDataList( sizeof( WSAEVENT ) );
	this_thread.hThread = WSACreateEvent();
	// setup this as if it was cleared already.
	this_thread.nEvents = 1;
	SetLink( &this_thread.monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
	SetDataItem( &this_thread.event_list, 0, &this_thread.hThread );
#else
	// have to fall back to poll() for __MAC__ builds. (probably client only)
	//this_thread.event_list = CreateDataList( sizeof( struct pollfd ) );
#ifdef __LINUX__
#ifdef __MAC__
	this_thread.kqueue = kqueue();
#else
	this_thread.epoll_fd = epoll_create1( EPOLL_CLOEXEC ); // close on exec (no inherit)
#endif
	{
#  ifdef __MAC__
#    ifdef __64__
		kevent64_s ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
		EV_SET64( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uint64_t)1, NULL, NULL );
		kevent64( this_thread.kqueue, &ev, 1, 0, 0, 0, 0 );
#    else
		kevent ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
		EV_SET( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)1 );
		kevent( this_thread.kqueue, &ev, 1, 0, 0, 0 );
#    endif
#  else
		struct epoll_event ev;
		ev.data.ptr = (void*)1;
		ev.events = EPOLLIN;
		epoll_ctl( this_thread.epoll_fd, EPOLL_CTL_ADD, GetThreadSleeper( thread ), &ev );
#  endif
	}
#endif

#endif
	this_thread.parent_peer = peer_thread;
	this_thread.child_peer = NULL;
	this_thread.thread = thread;

	if( peer_thread )
		peer_thread->child_peer = &this_thread;
	else {
		globalNetworkData.root_thread = &this_thread;
#ifdef _WIN32
		globalNetworkData.hMonitorThreadControlEvent = this_thread.hThread;
#endif
	}

	while( !globalNetworkData.pThreads ) // creator won't pass until bThreadInitComplete is set.
		Relinquish();

	globalNetworkData.flags.bThreadInitOkay = TRUE;
	globalNetworkData.flags.bThreadInitComplete = TRUE;
	while( !globalNetworkData.bQuit )
	{
		ProcessNetworkMessages( &this_thread, 0 );
	}

	xlprintf( 2100 )(WIDE( "Enter global network on shutdown... (thread exiting)" ));

	EnterCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE( "NetworkThread(exit) in global" ) );
#  endif

	if( !this_thread.parent_peer )
	{
		if( globalNetworkData.root_thread = this_thread.child_peer )
			this_thread.child_peer->parent_peer = NULL;
	}
	else
	{
		if( this_thread.parent_peer->child_peer = this_thread.child_peer )
			this_thread.child_peer->parent_peer = this_thread.parent_peer;
	}
	// this used to be done in the WM_DESTROY
	DeleteLink( &globalNetworkData.pThreads, thread );

	globalNetworkData.flags.bThreadExit = TRUE;

	xlprintf( 2100 )(WIDE( "Shut down network thread." ));

	globalNetworkData.flags.bThreadInitComplete = FALSE;
	globalNetworkData.flags.bNetworkReady = FALSE;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE( "NetworkThread(exit) left global" ) );
#  endif
	//DeleteCriticalSec( &globalNetworkData.csNetwork );	 //spv:980303
	return 0;
}


//----------------------------------------------------------------------------

struct peer_thread_info *IsNetworkThread( void )
{
	struct peer_thread_info *thread;
	PTHREAD this_thread = MakeThread();
	for( thread = globalNetworkData.root_thread; thread; thread = thread->child_peer )
	{
		if( thread->thread == this_thread )
			return thread;
	}
	return NULL;
}

//----------------------------------------------------------------------------

int NetworkQuit(void)
{
	if( !global_network_data )
		return 0;

	if( globalNetworkData.uPendingTimer )
	{
		RemoveTimer( globalNetworkData.uPendingTimer );
		globalNetworkData.uPendingTimer = 0;
	}
	while( globalNetworkData.ActiveClients )
	{
#ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE("NetworkQuit - Remove active client %p"), globalNetworkData.ActiveClients );
#endif
		InternalRemoveClientEx( globalNetworkData.ActiveClients, TRUE, FALSE );
	}
	globalNetworkData.bQuit = TRUE;
	{
		PTHREAD thread;
		INDEX idx;
		struct peer_thread_info *peer_thread;
#ifdef USE_WSA_EVENTS
		for( peer_thread = globalNetworkData.root_thread; peer_thread; peer_thread = peer_thread->child_peer ) {
			struct peer_thread_info *peer_thread = (struct peer_thread_info*)GetThreadParam( thread );
			if( peer_thread )
				WSASetEvent( peer_thread->hThread );
		}
#endif
		LIST_FORALL( globalNetworkData.pThreads, idx, PTHREAD, thread ) {
			WakeThread( thread );
		}
	}
#ifdef _WIN32
#  ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (trigger quit)" ) );
#  endif
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#else
#  ifndef __LINUX__
	if( IsWindow( globalNetworkData.ghWndNetwork ) )
	{
		// okay forget this... at exit, cannot guarantee that
		// any other thread other than myself has any rights to do anything.
#    ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE( "Post SOCKMSG_CLOSE" ) );
#    endif
		PostMessage( globalNetworkData.ghWndNetwork, SOCKMSG_CLOSE, 0, 0 );
		// also remove PCLIENT clients, and all client->pUserData allocated...
	}
#  else
	//while( globalNetworkData.pThread )
	//	Sleep(0);
	// should kill Our thread.... and close any active ockets...
#  endif
#endif
	globalNetworkData.flags.bThreadInitComplete = FALSE;
	//RemoveIdleProc( ProcessNetworkMessages );
	if( globalNetworkData.pThreads )
	{
		uint32_t started = timeGetTime() + 500;
		Relinquish(); // allow network thread to gracefully exit
		while( globalNetworkData.flags.bNetworkReady && timeGetTime() < started )
			IdleFor( 20 );
		if( globalNetworkData.flags.bNetworkReady )
		{
#ifdef LOG_STARTUP_SHUTDOWN
			lprintf( WIDE( "Network was locked up?  Failed to allow network to exit in half a second (500ms)" ) );
#endif
		}
	}
	globalNetworkData.root_thread = NULL;
	return -1;
}

ATEXIT( NetworkShutdown )
{
	NetworkQuit();
}

//----------------------------------------------------------------------------

LOGICAL NetworkAlive( void )
{
	return !globalNetworkData.flags.bThreadExit;
}

//----------------------------------------------------------------------------

static void AddClients( void ) {
	PCLIENT_SLAB pClientSlab;

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );

	{
		size_t n;
		//Log1( WIDE("Creating %d Client Resources"), MAX_NETCLIENTS );
		pClientSlab = NewPlus( CLIENT_SLAB, (MAX_NETCLIENTS - 1 )* sizeof( CLIENT ) );
		pClientSlab->pUserData = NewArray( uintptr_t, MAX_NETCLIENTS * globalNetworkData.nUserData );
		MemSet( pClientSlab->client, 0, (MAX_NETCLIENTS) * sizeof( CLIENT ) ); // can't clear the lpUserData Address!!!
		MemSet( pClientSlab->pUserData, 0, (MAX_NETCLIENTS) * globalNetworkData.nUserData * sizeof( uintptr_t ) );
		pClientSlab->count = MAX_NETCLIENTS;

		for( n = 0; n < pClientSlab->count; n++ )
		{
			pClientSlab->client[n].Socket = INVALID_SOCKET; // unused sockets on all clients.
			pClientSlab->client[n].lpUserData = pClientSlab->pUserData + (n * globalNetworkData.nUserData);
			InitializeCriticalSec( &pClientSlab->client[n].csLock );
			AddAvailable( pClientSlab->client + n );
		}
		AddLink( &globalNetworkData.ClientSlabs, pClientSlab );
	}

	LeaveCriticalSec( &globalNetworkData.csNetwork );
}

//----------------------------------------------------------------------------


static void ReallocClients( uint32_t wClients, int nUserData )
{

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );
	if( !wClients )
		wClients = 32;  // default 32 clients per slab...
	if( !nUserData )
		nUserData = 4;  // defualt to 4 pointer words per socket; most applications only use 1.

	// keep the max of specified data..
	if( nUserData < globalNetworkData.nUserData )
		nUserData = globalNetworkData.nUserData;

	// keep the max of specified connections..
	if( wClients < MAX_NETCLIENTS )
		wClients = MAX_NETCLIENTS;

	// if the client slab size increases, new slabs will be the new size; old slabs will still be the old size.

	if( nUserData > globalNetworkData.nUserData ) // have to reallocate the user data for all sockets
	{
		INDEX idx;
		PCLIENT_SLAB slab;
		uint32_t n;
		// for all existing client slabs...
		LIST_FORALL( globalNetworkData.ClientSlabs, idx, PCLIENT_SLAB, slab )
		{
			uintptr_t* pUserData;
			pUserData = NewArray( uintptr_t, nUserData * sizeof( uintptr_t ) * slab->count );// slab->pUserData;
			for( n = 0; n < slab->count; n++ )
			{
				if( slab->client[n].lpUserData )
					MemCpy( (char*)pUserData + (n * (nUserData * sizeof( uintptr_t )))
					      , slab->client[n].lpUserData
					      , globalNetworkData.nUserData * sizeof( uintptr_t ) );
				slab->client[n].lpUserData = pUserData + (n * nUserData);
			}
			Deallocate( uintptr_t*, slab->pUserData );
			slab->pUserData = pUserData;
		}
	}
	MAX_NETCLIENTS = wClients;
	globalNetworkData.nUserData = nUserData;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
}

#ifdef __LINUX__
NETWORK_PROC( LOGICAL, NetworkWait )(POINTER unused,uint32_t wClients,int wUserData)
#else
NETWORK_PROC( LOGICAL, NetworkWait )(HWND hWndNotify,uint32_t wClients,int wUserData)
#endif
{
	// want to start the thead; clear quit.
	if( !global_network_data )
		LowLevelNetworkInit();

	// allow network to restart with new NetworkWait after NetworkQuit
	globalNetworkData.bQuit = FALSE;

	ReallocClients( wClients, wUserData );

	//-------------------------
	// please be mindful of the following data declared immediate...
	if( GetLinkCount( globalNetworkData.pThreads ) )
	{
		xlprintf(200)( WIDE("Threads already active...") );
		// might do something... might not...
		return TRUE; // network thread active, do not realloc
	}

	AddLink( &globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)/*peer_thread==*/NULL ) );
	globalNetworkData.nPeers++;
	AddIdleProc( IdleProcessNetworkMessages, 1 );
	//lprintf( WIDE("Network Initialize..."));
	//lprintf( WIDE("Create network thread.") );
	while( !globalNetworkData.flags.bThreadInitComplete )
	{
		Relinquish();
	}
	if( !globalNetworkData.flags.bThreadInitOkay )
	{
		lprintf( WIDE("Abort network, init is NOT ok.") );
		return FALSE;
	}
	while( !globalNetworkData.flags.bNetworkReady )
		Relinquish(); // wait for actual network...
	{
		char buffer[256];
		if( gethostname( buffer, sizeof( buffer ) ) == 0)
			globalNetworkData.system_name = DupCStr( buffer );
	}
	LoadNetworkAddresses();
	return globalNetworkData.flags.bThreadInitOkay;  // return status of thread initialization
}

//----------------------------------------------------------------------------

PCLIENT GetFreeNetworkClientEx( DBG_VOIDPASS )
{
	PCLIENT pClient = NULL;
get_client:
	EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE("GetFreeNetworkClient in global") );
#endif
	if( !globalNetworkData.AvailableClients ) // if there's none available, add some with current config
		AddClients();
	for( pClient = globalNetworkData.AvailableClients; pClient; pClient = pClient->next )
		if( !( pClient->dwFlags & CF_CLOSING ) )
			break;
	if( pClient )
	{
		int d;
		// oterhwise we'll deadlock the closing client...
		// an opening condition has global lock (above)
		// and a closing socket will want the global lock before it's done.
		pClient = GrabClient( pClient );
		do {
#ifdef USE_NATIVE_CRITICAL_SECTION
			d = EnterCriticalSecNoWait( &pClient->csLock, NULL );
#else
			d = EnterCriticalSecNoWaitEx( &pClient->csLock, NULL DBG_RELAY );
#endif
			if( d < 1 ) {
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				goto get_client;
			}
		} while( d < 1 );
		if( pClient->dwFlags & CF_STATEFLAGS )
			DebugBreak();
		ClearClient( pClient DBG_SRC ); // clear client is redundant here... but saves the critical section now
		//Log1( WIDE("New network client %p"), client );
	}
	else
	{
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( WIDE("GetFreeNetworkClient left global") );
#endif

		RescheduleTimerEx( globalNetworkData.uPendingTimer, 1 );
		Relinquish();
		if( globalNetworkData.AvailableClients )
		{
			lprintf( WIDE( "there were clients available... just in a closing state..." ) );
			goto get_client;
		}
		lprintf( WIDE("No unused network clients are available.") );
		return NULL;
	}
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE("GetFreeNetworkClient left global") );
#endif
	return pClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetNetworkLong )(PCLIENT lpClient, int nLong, uintptr_t dwValue)
{
	if( lpClient && ( nLong < globalNetworkData.nUserData ) )
	{
		lpClient->lpUserData[nLong] = dwValue;
	}
	return;
}

//----------------------------------------------------------------------------

int GetAddressParts( SOCKADDR *sa, uint32_t *pdwIP, uint16_t *pdwPort )
{
	int result = TRUE;
	if( sa )
	{
		if( sa->sa_family == AF_INET ) {
			if( pdwIP )
				(*pdwIP) = (uint32_t)(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr);
		}
		else if( sa->sa_family == AF_INET6 ) {
			if( pdwIP )
				memcpy( pdwIP, &(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr), 16 );
		}
		else
			result = FALSE;
		if( (sa->sa_family == AF_INET) || (sa->sa_family = AF_INET6) ) {
			if( pdwPort ) 
				(*pdwPort) = ntohs((uint16_t)( (SOCKADDR_IN*)sa)->sin_port);
		}
		else 
			result = FALSE;
	}
	return result;
}

NETWORK_PROC( uintptr_t, GetNetworkLong )(PCLIENT lpClient,int nLong)
{
	if( !lpClient )
	{
		return (uintptr_t)-1;
	}
	if( nLong < 0 )
	{
		switch( nLong )
		{
		case GNL_IP:  // IP of destination
			if( lpClient->saClient )
				return *(uint32_t*)(lpClient->saClient->sa_data+2);
			break;
		case GNL_REMOTE_ADDRESS:  // IP of destination
  			return (uintptr_t)lpClient->saClient;
			break;
		case GNL_LOCAL_ADDRESS:  // IP of local side
  			return (uintptr_t)lpClient->saSource;
			break;
		case GNL_PORT:  // port of server...  STUPID PATCH?!  maybe...
			if( lpClient->saClient )
				return ntohs( *(uint16_t*)(lpClient->saClient->sa_data) );
			break;
		case GNL_MYPORT:  // port of server...  STUPID PATCH?!  maybe...
			if( lpClient->saSource )
				return ntohs( *(uint16_t*)(lpClient->saSource->sa_data) );
			break;
		case GNL_MYIP: // IP of myself (after connect?)
			if( lpClient->saSource )
				return *(uint32_t*)(lpClient->saSource->sa_data+2);
			break;

			//TODO if less than zero return a (high/low)portion of the  hardware address (MAC).
		}
	}
	else if( nLong < globalNetworkData.nUserData )
	{
		return lpClient->lpUserData[nLong];
	}

	return (uintptr_t)-1;   //spv:980303
}

//----------------------------------------------------------------------------


SOCKADDR* DuplicateAddressEx( SOCKADDR *pAddr DBG_PASS ) // return a copy of this address...
{
	POINTER tmp = (POINTER)( ( (uintptr_t)pAddr ) - 2*sizeof(uintptr_t) );
	SOCKADDR *dup = AllocAddrEx( DBG_VOIDRELAY );
	POINTER tmp2 = (POINTER)( ( (uintptr_t)dup ) - 2*sizeof(uintptr_t) );
	MemCpy( tmp2, tmp, SOCKADDR_LENGTH( pAddr ) + 2*sizeof(uintptr_t) );
	if( ((char**)( ( (uintptr_t)pAddr ) - sizeof(char*) ))[0] )
		( (char**)( ( (uintptr_t)dup ) - sizeof( char* ) ) )[0]
				= strdup( ((char**)( ( (uintptr_t)pAddr ) - sizeof( char* ) ))[0] );
	return dup;
}

//---------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateAddress_hton)( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = htonl(dwIP);
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------
#if defined( __LINUX__ ) && !defined( __CYGWIN__ )
 #define UNIX_PATH_MAX	 108

struct sockaddr_un {
#ifdef __MAC__
	u_char   sa_len;
#endif
	sa_family_t  sun_family;		/* AF_UNIX */
	char	       sun_path[UNIX_PATH_MAX]; /* pathname */
};

NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	struct sockaddr_un *lpsaAddr;
#ifdef UNICODE
	char *tmp_path = CStrDup( path );
#endif
   lpsaAddr=(struct sockaddr_un*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	((uintptr_t*)lpsaAddr)[-1] = StrLen( path ) + 1;
	lpsaAddr->sun_family = PF_UNIX;
#ifdef UNICODE
	strncpy( lpsaAddr->sun_path, tmp_path, 107 );
	Deallocate( char*, tmp_path );
#else
	strncpy( lpsaAddr->sun_path, path, 107 );
#endif

#ifdef __MAC__
	lpsaAddr->sa_len = 2+strlen( lpsaAddr->sun_path );
#endif
	return((SOCKADDR*)lpsaAddr);
}
#else
NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	lprintf( WIDE( "-- CreateUnixAddress -- not available. " ) );
	return NULL;
}
#endif
//---------------------------------------------------------------------------

SOCKADDR *CreateAddress( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family	    = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = dwIP;
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------

SOCKADDR *CreateRemote( CTEXTSTR lpName,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr;
	int conversion_success = FALSE;
	char *tmpName = NULL;
#ifdef UNICODE
	char *_lpName = CStrDup( lpName );
#  define lpName _lpName
#endif
#ifndef WIN32
	PHOSTENT phe;
	// a IP type name will never have a / in it, therefore
	// we can assume it's a unix type address....
	if( lpName && StrChr( lpName, '/' ) )
		return CreateUnixAddress( lpName );
#endif
	if( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) {
		size_t len;
		tmpName = NewArray( char, len = StrLen( lpName ) );
		memcpy( tmpName, lpName + 1, len - 2 );
		tmpName[len - 2] = 0;
		lpName = tmpName;
	}

	lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if( !lpsaAddr )
	{
#ifdef UNICODE
		Deallocate( char *, _lpName );
#endif
		return(NULL);
	}
	SetAddrName( (SOCKADDR*)lpsaAddr, lpName );

	// if it's a numeric name... attempt to use as an address.
#ifdef __LINUX__
	if( lpName &&
		( lpName[0] >= '0' && lpName[0] <= '9' )
	  && StrChr( lpName, '.' ) )
	{
#ifdef UNICODE
		char *tmp = CStrDup( lpName );
		if( inet_pton( AF_INET, tmp, (struct in_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
		Deallocate( char *, tmp );
#else
		if( inet_pton( AF_INET, lpName, (struct in6_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
#endif
	}
	else if( lpName 
		   && ( ( lpName[0] >= '0' && lpName[0] <= '9' )
		      || ( lpName[0] >= 'a' && lpName[0] <= 'f' )
		      || ( lpName[0] >= 'A' && lpName[0] <= 'F' )
		      || lpName[0] == ':'
		      || ( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) ) 
		   && StrChr( lpName, ':' )!=StrRChr( lpName, ':' ) )
	{
#ifdef UNICODE
		char *tmp = CStrDup( lpName );
		if( inet_pton( AF_INET6, tmp, (struct in_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET6;         // InetAddress Type.
			conversion_success = TRUE;
		}
		Deallocate( char *, tmp );
#else
		if( inet_pton( AF_INET6, lpName, (struct in6_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET6;         // InetAddress Type.
			conversion_success = TRUE;
		}
#endif
	}
#endif
	if( !conversion_success )
	{
		if( lpName )
		{
#ifdef WIN32
			{
				struct addrinfo *result;
				struct addrinfo *test;
				int error;
				error = getaddrinfo( lpName, NULL, NULL, (struct addrinfo**)&result );
				if( error == 0 )
				{
					for( test = result; test; test = test->ai_next )
					{
						//SOCKADDR *tmp;
						//AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
						MemCpy( lpsaAddr, test->ai_addr, test->ai_addrlen );
						SET_SOCKADDR_LENGTH( lpsaAddr, test->ai_addrlen );
						break;
					}
				}
				else
					lprintf( WIDE( "getaddrinfo Error: %d for [%s]" ), error, lpName );
			}
#else //WIN32

			char *tmp = CStrDup( lpName );
			if( 1 )//!(phe=gethostbyname(tmp)))
			{
				if( !(phe=gethostbyname2(tmp,AF_INET6) ) )
				{
					if( !(phe=gethostbyname2(tmp,AF_INET) ) )
					{
 						// could not find the name in the host file.
						Log1( WIDE("Could not Resolve to %s"), lpName );
						Deallocate(SOCKADDR_IN*, lpsaAddr);
						Deallocate( char*, tmp );
						if( tmpName ) Deallocate( char*, tmpName );
						return(NULL);
					}
					else
					{
						lprintf( WIDE( "Strange, gethostbyname failed, but AF_INET worked..." ) );
						SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
						lpsaAddr->sin_family = AF_INET;
						memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
							 phe->h_addr,
							 phe->h_length);
					}
				}
				else
				{
					SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
					lpsaAddr->sin_family = AF_INET6;         // InetAddress Type.
#if note
	{
		__SOCKADDR_COMMON (sin6_);
		n_port_t sin6_port;        /* Transport layer port # */
		uint32_t sin6_flowinfo;     /* IPv6 flow information */
		struct in6_addr sin6_addr;  /* IPv6 address */
		uint32_t sin6_scope_id;     /* IPv6 scope-id */
	};
#endif

					memcpy( ((struct sockaddr_in6*)lpsaAddr)->sin6_addr.s6_addr,           // save IP address from host entry.
							 phe->h_addr,
							 phe->h_length);
				}
			}
			else
			{
				Deallocate( char *, tmp );
				SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
				lpsaAddr->sin_family = AF_INET;         // InetAddress Type.
				memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
					 phe->h_addr,
					 phe->h_length);
			}
#endif
		}
		else
		{
			lpsaAddr->sin_family      = AF_INET;         // InetAddress Type.
			lpsaAddr->sin_addr.S_un.S_addr = 0;
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
		}
	}
#ifdef UNICODE
	Deallocate( char *, _lpName );
#  undef lpName
#endif
	// put in his(destination) port number...
	if( tmpName ) Deallocate( char*, tmpName );
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//----------------------------------------------------------------------------

#ifdef __cplusplus
namespace udp {
#endif
NETWORK_PROC( void, DumpAddrEx)( CTEXTSTR name, SOCKADDR *sa DBG_PASS )
	{
		if( !sa ) { _lprintf(DBG_RELAY)( "%s: NULL", name ); return; }
		LogBinary( (uint8_t *)sa, SOCKADDR_LENGTH( sa ) );
		if( sa->sa_family == AF_INET ) {
			_lprintf(DBG_RELAY)( WIDE("%s: (%s) %d.%d.%d.%d:%d "), name
			       , ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
			       //*(((unsigned char *)sa)+0),
			       //*(((unsigned char *)sa)+1),
			       ,*(((unsigned char *)sa)+4),
			       *(((unsigned char *)sa)+5),
			       *(((unsigned char *)sa)+6),
			       *(((unsigned char *)sa)+7) 
			       , ntohs( *(((unsigned short *)((unsigned char*)sa + 2))) )
			);
		} else if( sa->sa_family == AF_INET6 )
		{
			lprintf( WIDE( "Socket address binary: %s" ), name );
			_lprintf(DBG_RELAY)( WIDE("%s: (%s) %03d %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x ")
					 , name
					, ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+2))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+8))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+10))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+12))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+14))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+16))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+18))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+20))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+22))))
					 );
		}

}
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetAddressPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetNonDefaultPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr && !((SOCKADDR_IN *)pAddr)->sin_port )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateSockAddress)(CTEXTSTR name, uint16_t nDefaultPort )
{
// blah... should process a ip:port - but - default port?!
	uint32_t bTmpName = 0;
	char * tmp;
	SOCKADDR *sa = NULL;
	char *port;
	uint16_t wPort;
	CTEXTSTR portName = name;

#ifdef UNICODE
	char *_name = CStrDup( name );
#  define name _name
#endif
	if( name[0] == '[' ) {
		while( portName[0] && portName[0] != ']' )
			portName++;
		if( portName[0] ) portName++;
	}
	if( name && portName[0] && ( port = (char*)strrchr( portName, ':' ) ) )
	{
		tmp = StrDup( name );
		bTmpName = 1;
		port = tmp + (port-name);
		name = tmp;
		//Log1( WIDE("Found ':' assuming %s is IP:PORT"), name );
		*port = 0;
		port++;
		if( port[0] )  // a trailing : could be IPV6 abbreviation.
		{
			if( isdigit( *port ) )
			{
				wPort = (short)atoi( port );
			}
			else
			{
				struct servent *se;
				se = getservbyname( port, NULL );
				if( !se )
				{
#ifdef UNICODE
#define FMT WIDE("S")
#else
#define FMT WIDE("s")
#endif
					Log1( WIDE("Could not resolve \"%" ) FMT WIDE("\" as a valid service name"), port );
					//return NULL;
					wPort = nDefaultPort;
				}
				else
					wPort = htons(se->s_port);
				//Log1( WIDE("port alpha - name resolve to %d"), wPort );
			}
		}
		else
			wPort = nDefaultPort;
#ifdef UNICODE
#  undef name
#endif
		sa = CreateRemote( name, wPort );
		if( port )
		{
			port[-1] = ':';  // incase we obliterated it
		}
	}
	else  // no port specification...
	{
		//Log1( WIDE("%s does not have a ':'"), name );
		sa = CreateRemote( name, nDefaultPort );
	}
#ifdef UNICODE
	Deallocate( char *, _name );
#endif
	if( bTmpName ) Deallocate( char*, tmp );
	return sa;
}

//----------------------------------------------------------------------------

SOCKADDR *CreateLocal(uint16_t nMyPort)
{
	char lpHostName[HOSTNAME_LEN];
	if (gethostname(lpHostName,HOSTNAME_LEN))
	{
		return(NULL);
	}
	return CreateRemote( WIDE("0.0.0.0"), nMyPort );
}

//----------------------------------------------------------------------------

LOGICAL CompareAddressEx( SOCKADDR *sa1, SOCKADDR *sa2, int method )
{
	if( method == SA_COMPARE_FULL )
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						SOCKADDR_IN *sin1 = (SOCKADDR_IN*)sa1;
						SOCKADDR_IN *sin2 = (SOCKADDR_IN*)sa2;
						if( MemCmp( sin1, sin2, sizeof( SOCKADDR_IN ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( WIDE("unhandled address type passed to compare, resulting FAILURE") );
					return 0;
				}
			}
		}
	}
	else
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						if( MemCmp( &((SOCKADDR_IN*)sa1)->sin_addr, &((SOCKADDR_IN*)sa2)->sin_addr, sizeof( ((SOCKADDR_IN*)sa2)->sin_addr ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( WIDE("unhandled address type passed to compare, resulting FAILURE") );
					return 0;
				}
			}
		}
	}
	return 0;
}

//----------------------------------------------------------------------------

LOGICAL CompareAddress( SOCKADDR *sa1, SOCKADDR *sa2 )
{
	return CompareAddressEx( sa1, sa2, SA_COMPARE_FULL );
}

//----------------------------------------------------------------------------

PLIST GetLocalAddresses( void )
{
	return globalNetworkData.addresses;
}

//----------------------------------------------------------------------------

LOGICAL IsThisAddressMe( SOCKADDR *addr, uint16_t myport )
{
	struct interfaceAddress *test_addr;
	INDEX idx;

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, &((SOCKADDR_IN*)test_addr->sa)->sin_addr, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return TRUE;
					}
				}
				break;
			default:
				lprintf( WIDE( "Unknown comparison" ) );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

LOGICAL IsBroadcastAddressForInterface( struct interfaceAddress *address, SOCKADDR *addr ) {
	if( addr->sa_family == AF_INET ) {
      //lprintf( "can test for broadcast... %08x %08x %08x", ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ), ((uint32_t*)address->saMask->sa_data)[0] , ((uint32_t*)addr->sa_data)[0] );
		if( ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ) == 0xFFFFFFFFU )
         return TRUE;
	}
   return FALSE;
}

//----------------------------------------------------------------------------

struct interfaceAddress* GetInterfaceForAddress( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			default:
				lprintf( WIDE( "Unknown comparison" ) );
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------

SOCKADDR* GetBroadcastAddressForInterface( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			default:
				lprintf( WIDE( "Unknown comparison" ) );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

SOCKADDR* GetInterfaceAddressForBroadcast( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 4 ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 16  ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			default:
				lprintf( WIDE( "Unknown comparison" ) );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

void ReleaseAddress(SOCKADDR *lpsaAddr)
{
	// sockaddr is often skewed from what I would expect it. (contains its own length)
	if( lpsaAddr )
	{
		/* strdup is used for the addr part so use free instead of release */
		free( ((POINTER*)( ( (uintptr_t)lpsaAddr ) - sizeof(uintptr_t) ))[0] );
		Deallocate(POINTER, (POINTER)( ( (uintptr_t)lpsaAddr ) - 2 * sizeof(uintptr_t) ));
	}
}

//----------------------------------------------------------------------------
// creates class C broadcast address
SOCKADDR *CreateBroadcast(uint16_t nPort)
{
	SOCKADDR_IN *bcast=(SOCKADDR_IN*)AllocAddr();
	SOCKADDR *lpMyAddr;
	if (!bcast)
		return(NULL);
	lpMyAddr = CreateLocal(0);
	SET_SOCKADDR_LENGTH( bcast, IN_SOCKADDR_LENGTH );
	bcast->sin_family	    = AF_INET;
	bcast->sin_addr.S_un.S_addr  = ((SOCKADDR_IN*)lpMyAddr)->sin_addr.S_un.S_addr;
	bcast->sin_addr.S_un.S_un_b.s_b4 = 0xFF; // Fake a subnet broadcast address
	bcast->sin_port        = htons(nPort);
	ReleaseAddress(lpMyAddr);
	return((SOCKADDR*)bcast);
}

//----------------------------------------------------------------------------

void DumpSocket( PCLIENT pc )
{
	DumpAddr( WIDE("REMOT"), pc->saClient );
	DumpAddr( WIDE("LOCAL"), pc->saSource );
	return;
}

#ifdef __cplusplus
namespace udp {
#endif
#undef DumpAddr
NETWORK_PROC( void, DumpAddr)( CTEXTSTR name, SOCKADDR *sa )
{
	DumpAddrEx( name, sa DBG_SRC );
}

#ifdef __cplusplus
}
#endif


//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, NetworkLockEx)( PCLIENT lpClient DBG_PASS )
{
	if( lpClient )
	{
	start_lock:
		//lpClient->dwFlags |= CF_WANTS_GLOBAL_LOCK;
		//_lprintf(DBG_RELAY)( WIDE( "Lock %p" ), lpClient );
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( EnterCriticalSecNoWait( &globalNetworkData.csNetwork, NULL ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
			//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef LOG_NETWORK_LOCKING
			_lprintf(DBG_RELAY)( WIDE( "Failed enter global?" ) );
#endif
			return NULL;
			//DebugBreak();
		}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( WIDE( "Got global lock" ) );
#endif

		//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( !EnterCriticalSecNoWait( &lpClient->csLock, NULL ) )
#else
		if( EnterCriticalSecNoWaitEx( &lpClient->csLock, NULL DBG_RELAY ) < 1 )
#endif
		{
			// unlock the global section for a moment..
			// client may be requiring both local and global locks (already has local lock)

#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( &globalNetworkData.csNetwork);
#else
			LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
			//lprintf( "Idle... socket lock failed, had global though..." );
			Relinquish();
			goto start_lock;
		}
		//EnterCriticalSec( &lpClient->csLock );
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#else
		LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			// change to inactive status by the time we got here...
#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( &lpClient->csLock );
#else
			LeaveCriticalSecEx( &lpClient->csLock DBG_RELAY );
#endif
			_lprintf( DBG_RELAY )( WIDE( "Failed lock" ) );
			lprintf( WIDE( "%p  %08x %08x inactive, cannot lock." ), lpClient, lpClient->dwFlags, CF_ACTIVE );
			// this client is not available for client use!
			return NULL;
		}
	}
	return lpClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, NetworkUnlockEx)( PCLIENT lpClient DBG_PASS )
{
	//_lprintf(DBG_RELAY)( WIDE( "Unlock %p" ), lpClient );
	// simple unlock.
	if( lpClient )
	{
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( &lpClient->csLock );
#else
		LeaveCriticalSecEx( &lpClient->csLock DBG_RELAY );
#endif
	}
}

//----------------------------------------------------------------------------

void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef LOG_SOCKET_CREATION
	_lprintf( DBG_RELAY )( WIDE("InternalRemoveClient Removing this client %p (%d)"), lpClient, lpClient->Socket );
#endif
	if( lpClient && IsValid(lpClient->Socket) )
	{
		if( !bLinger )
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( WIDE("Setting quick close?!") );
#endif
			if( 0 )
			{
				int nAllowReuse = 1;
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEADDR,
									(char*)&nAllowReuse, sizeof(nAllowReuse)) <0 )
				{
					//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse addr." << endl;
				}

				/*
				// missing symbol in windows?
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEPORT,
									(char*)&nAllowReuse, sizeof(nAllowReuse)) <0 )
				{
					//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse port." << endl;
					}
				*/
			}
			if( 1 )
			{
				// www.serverframework.com/asynchronousevents/2011/01/time-wait-and-its-design-implications-for-protocols-and-scalable-servers.html
				//  the idea is to NEVER do this; but I had to do this for lots of parallel connections that were short lived...
				// windows registry http://technet.microsoft.com/en-us/library/cc938217.aspx 240 seconds time_wait timeout
				struct linger lingerSet;
				lingerSet.l_onoff = 1; // on , with no time = off.
				lingerSet.l_linger = 0; // 0 timeout sends reset.
				// set server to allow reuse of socket port
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_LINGER,
									(char*)&lingerSet, sizeof(lingerSet)) <0 )
				{
					lprintf( WIDE( "error setting no linger in close." ) );
					//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
				}
			}
		}
		else {
//#if 0
			struct linger lingerSet;
			// linger ON causes delay on close... otherwise close returns immediately
			lingerSet.l_onoff = 0; // on , with no time = off.
			lingerSet.l_linger = 2;
			// set server to allow reuse of socket port
			if( setsockopt( lpClient->Socket, SOL_SOCKET, SO_LINGER,
				(char*)&lingerSet, sizeof( lingerSet ) ) <0 )
			{
				lprintf( WIDE( "error setting no linger in close." ) );
				//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
			}
//#endif
		}

		if( !(lpClient->dwFlags & CF_ACTIVE) )
		{
			if( lpClient->dwFlags & CF_AVAILABLE )
			{
				lprintf( WIDE("Client was inactive?!?!?! removing from list and putting in available") );
				AddAvailable( GrabClient( lpClient ) );
			}
			// this is probably true, we've definatly already moved it from
			// active list to clsoed list.
			else if( !(lpClient->dwFlags & CF_CLOSED) )
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( WIDE("Client was NOT already closed?!?!") );
#endif
				AddClosed( GrabClient( lpClient ) );
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( WIDE("Client's state is CLOSED") );
#endif
			return;
		}
		if( lpClient->dwFlags & CF_WRITEPENDING ) {
			lprintf( "CLOSE WHILE WAITING FOR WRITE TO FINISH..." );
			lpClient->dwFlags |= CF_TOCLOSE;
			return;
		}
		while( !NetworkLockEx( lpClient DBG_SRC ) )
		{
			if( !(lpClient->dwFlags & CF_ACTIVE ) )
			{
				return;
			}
			Relinquish();
		}

		// allow application a chance to clean it's references
		// to this structure before closing and cleaning it.

		if( !bBlockNotify )
		{
			lpClient->dwFlags |= CF_CONNECT_CLOSED;
			if( lpClient->pWaiting )
			{
				WakeThread( lpClient->pWaiting );
				while( lpClient->dwFlags & CF_CONNECT_WAITING )
					Relinquish();
			}
			lpClient->dwFlags &= ~CF_CONNECT_CLOSED;
			if( !(lpClient->dwFlags & CF_CLOSING) ) // prevent multiple notifications...
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( WIDE( "Marked closing first, and dispatching callback?" ) );
#endif
				lpClient->dwFlags |= CF_CLOSING;
				if( lpClient->close.CloseCallback )
				{
					// during thisi if it wants a lock... and the application
					// is dispatching like
					if( lpClient->dwFlags & CF_CPPCLOSE )
						lpClient->close.CPPCloseCallback( lpClient->psvClose );
					else
						lpClient->close.CloseCallback( lpClient );
				}
#ifdef LOG_DEBUG_CLOSING
				else
					lprintf( WIDE( "no close callback!?" ) );
#endif
				// leave the flag closing set... we'll use that later
				// to avoid the double-lock;
				//lpClient->dwFlags &= ~CF_CLOSING;
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( WIDE( "socket was already ispatched callback?" ) );
#endif
		}
		else
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( WIDE( "blocknotify on close..." ) );
#endif
		}
		EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_DEBUG_CLOSING
		lprintf( WIDE( "Adding current client to closed clients." ) );
#endif
		AddClosed( GrabClient( lpClient ) );
#ifdef LOG_DEBUG_CLOSING
		lprintf( WIDE( "Leaving client critical section" ) );
#endif
		//lprintf( WIDE( "Leaving network critical section" ) );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( lpClient DBG_SRC );
	}
#ifdef LOG_DEBUG_CLOSING
	else
	{
		lprintf( WIDE("No Client, or socket already closed?") );
	}
#endif
}

void RemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
	InternalRemoveClientExx( lpClient, bBlockNotify, bLinger DBG_RELAY );
	TerminateClosedClient( lpClient );
}

CTEXTSTR GetSystemName( void )
{
	// start the network with defaults... we're able to reallocate later.
#ifdef __ANDROID__
#if 0
	// dont' actually have to start winsock; but we do have to do work to get our IP
	int sock_startup = socket( AF_INET, SOCK_RAW, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_DGRAM, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_STREAM, 0);
	if( sock_startup >= 0 )
	{
		struct ifconf buffer;
		struct ifreq ifr[10];
		int ifc_num;
		int n;
		buffer.ifc_len = sizeof(ifr);
		buffer.ifc_ifcu.ifcu_buf = (char*)ifr;
		ioctl( sock_startup, SIOCGIFCONF, &buffer);
		ifc_num = buffer.ifc_len / sizeof(struct ifreq);
#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

		for( n = 0; n < ifc_num; n++ )
		{
			int sd, ifc_num, addr, bcast, mask, network;
			lprintf( "interface %d : %s", n, ifr[n].ifr_name );
			if (ifr[n].ifr_addr.sa_family != AF_INET)
			{
				continue;
			}

			/* display the interface name */
			lprintf("%d) interface: %s\n", n+1, ifr[n].ifr_name);

			/* Retrieve the IP address, broadcast address, and subnet mask. */
			if (ioctl(sd, SIOCGIFADDR, &ifr[n]) == 0)
			{
				addr = ((struct sockaddr_in *)(&ifr[n].ifr_addr))->sin_addr.s_addr;
				lprintf("%d) address: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(addr));
			}
			if (ioctl(sd, SIOCGIFBRDADDR, &ifr[n]) == 0)
			{
				bcast = ((struct sockaddr_in *)(&ifr[n].ifr_broadaddr))->sin_addr.s_addr;
				lprintf("%d) broadcast: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(bcast));
			}
			if (ioctl(sd, SIOCGIFNETMASK, &ifr[n]) == 0)
			{
				mask = ((struct sockaddr_in *)(&ifr[n].ifr_netmask))->sin_addr.s_addr;
				lprintf("%d) netmask: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(mask));
			}

			/* Compute the current network value from the address and netmask. */
			network = addr & mask;
			lprintf("%d) network: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(network));
		}
		close( sock_startup );
	}
	else
#endif
		globalNetworkData.system_name = WIDE("No Name Available");
#else
	NetworkStart();
#endif
	return globalNetworkData.system_name;
}

#undef NetworkLock
#undef NetworkUnlock
NETWORK_PROC( PCLIENT, NetworkLock)( PCLIENT lpClient )
{
	return NetworkLockEx( lpClient DBG_SRC );
}

NETWORK_PROC( void, NetworkUnlock)( PCLIENT lpClient )
{
	NetworkUnlockEx( lpClient DBG_SRC );
}

NETWORK_PROC( void, GetNetworkAddressBinary )( SOCKADDR *addr, uint8_t **data, size_t *datalen ) {
	if( addr ) {
		size_t namelen;
		size_t addrlen = SOCKADDR_LENGTH( addr );
		const char * tmp = ((const char**)addr)[-1];
		if( !( (uintptr_t)tmp & 0xFFFF0000 ) )
		{
			lprintf( WIDE("corrupted sockaddr.") );
			DebugBreak();
		}
		if( tmp )
		{
			namelen = StrLen( tmp );
		}
		else
			namelen = 0;
		(*datalen) = namelen + 1 + 1 + SOCKADDR_LENGTH( addr );
		(*data) = NewArray( uint8_t, (*datalen) );
		MemCpy( (*data), tmp, namelen + 1 );
		(*data)[namelen+1] = (uint8_t)addrlen;
		MemCpy( (*data) + namelen + 1, addr, addrlen );
	}
}

NETWORK_PROC( SOCKADDR *, MakeNetworkAddressFromBinary )( uintptr_t *data, size_t datalen ) {
	SOCKADDR *addr = AllocAddr();
	size_t namelen = strlen( (const char*)data );
	if( namelen ) // if empty name, don't include it.
		SetAddrName( addr, (const char*)data );
	SET_SOCKADDR_LENGTH( addr, data[1] );
	MemCpy( addr, data + 2, data[1] );
	return addr;
}


#ifdef __LINUX__

void LoadNetworkAddresses( void ) {
	struct ifaddrs *addrs, *tmp;
	struct interfaceAddress *ia;

	getifaddrs( &addrs );
	tmp = addrs;

	ia = New( struct interfaceAddress );
	ia->sa = CreateRemote( "0.0.0.0", 0 );
	ia->saMask = NULL;
	ia->saBroadcast = CreateRemote( "255.255.255.255", 0 );
	AddLink( &globalNetworkData.addresses, ia );

	for( ; tmp; tmp = tmp->ifa_next )
	{
		SOCKADDR *dup;
		if( !tmp->ifa_addr )
			continue;
#  ifndef __MAC__
		if( tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET )
			continue;
#  endif
		ia = New( struct interfaceAddress );
		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			continue;
			//memcpy( dup, tmp->ifa_addr, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			memcpy( dup, tmp->ifa_addr, IN_SOCKADDR_LENGTH );
			SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
		}
		ia->sa = dup;

		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			//memcpy( dup, tmp->ifa_netmask, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			memcpy( dup, tmp->ifa_netmask, IN_SOCKADDR_LENGTH );
			SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
		}
		ia->saMask = dup;

		ia->saBroadcast = AllocAddr();
		ia->saBroadcast->sa_family = ia->sa->sa_family;
		ia->saBroadcast->sa_data[0] = 0;
		ia->saBroadcast->sa_data[1] = 0;
		ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
		ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
		ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
		ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
		SET_SOCKADDR_LENGTH( ia->saBroadcast, SOCKADDR_LENGTH( ia->sa ) );
		AddLink( &globalNetworkData.addresses, ia );
	}

	freeifaddrs( addrs );
}

#endif

#ifdef _WIN32
#if 0

#ifdef WIN32
	{
		struct addrinfo *result;
		struct addrinfo *test;
#ifdef _UNICODE
		char *tmp = WcharConvert( globalNetworkData.system_name );
		getaddrinfo( tmp, NULL, NULL, (struct addrinfo**)&result );
		Deallocate( char*, tmp );
#else
		getaddrinfo( globalNetworkData.system_name, NULL, NULL, (struct addrinfo**)&result );
#endif
		for( test = result; test; test = test->ai_next )
		{
			//if( test->ai_family == AF_INET )
			{
				SOCKADDR *tmp;
				AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
				((uintptr_t*)tmp)[-1] = test->ai_addrlen;
				MemCpy( tmp, test->ai_addr, test->ai_addrlen );
				//lprintf( "initialize addres..." );
				//DumpAddr( "blah", tmp );
			}
		}
	}
#endif
#endif

void LoadNetworkAddresses( void ) {
	// Declare and initialize variables
	PIP_INTERFACE_INFO pInfo;
	pInfo = (IP_INTERFACE_INFO *) malloc( sizeof(IP_INTERFACE_INFO) );
	DWORD dwRetVal = 0;

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = New(IP_ADAPTER_INFO);
	if (pAdapterInfo == NULL) {
		lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
		return;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		Deallocate( PIP_ADAPTER_INFO, pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) NewArray(uint8_t, ulOutBufLen);
		if (pAdapterInfo == NULL) {
			lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
			return;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {

			PIP_ADDR_STRING ipadd = &pAdapter->IpAddressList;
			for( ; ipadd; ipadd = ipadd->Next ) {
			/*
			typedef struct _IP_ADDR_STRING {
			  struct _IP_ADDR_STRING  *Next;
			  IP_ADDRESS_STRING      IpAddress;
			  IP_MASK_STRING         IpMask;
			  DWORD                  Context;
			} IP_ADDR_STRING, *PIP_ADDR_STRING;
			*/
				if( StrCmp( ipadd->IpAddress.String, "0.0.0.0" ) == 0 )
					continue;
				struct interfaceAddress *ia = New( struct interfaceAddress );
				ia->sa = CreateRemote( ipadd->IpAddress.String, 0 );
				ia->saMask = CreateRemote( ipadd->IpMask.String, 0 );
				ia->saBroadcast = AllocAddr();
				ia->saBroadcast->sa_family = ia->sa->sa_family;
				ia->saBroadcast->sa_data[0] = 0;
				ia->saBroadcast->sa_data[1] = 0;
				ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
				ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
				ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
				ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
				AddLink( &globalNetworkData.addresses, ia );
			}

			pAdapter = pAdapter->Next;
		}
	}
	else {
		lprintf( "GetAdaptersInfo failed with error: %d\n", dwRetVal );
	}
#if 0
https://msdn.microsoft.com/en-us/library/windows/desktop/aa365915%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

  do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(AF_UNSPEC
		, GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_FRIENDLY_NAME|GAA_FLAG_INCLUDE_ALL_INTERFACES
		, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));
#endif

}

#endif

SACK_NETWORK_NAMESPACE_END

