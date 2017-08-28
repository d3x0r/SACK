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

#if 0
#undef EnterCriticalSec
#undef LeaveCriticalSec

#define EnterCriticalSec(a) lprintf( WIDE( "Enter section %p" ), a ); EnterCriticalSection( a ); lprintf( WIDE( "In Section %p" ), a );
#define LeaveCriticalSec(a) lprintf( WIDE( "Leave section %p" ), a ); LeaveCriticalSection( a ); lprintf( WIDE( "Out Section %p" ), a );

#undef EnterCriticalSecNoWait
#define EnterCriticalSecNoWait(a,b) MyEnterCriticalSecNoWait(a)

static int MyEnterCriticalSecNoWait( CRITICALSECTION *cs )
{
	int result = TryEnterCriticalSection( cs );
	lprintf( WIDE( "try Enter Section %p - and result was %d" ), cs, result );
	return result;
}
#endif

//for GetMacAddress

SACK_NETWORK_NAMESPACE

PRELOAD( InitNetworkGlobalOptions )
{
#ifndef __NO_OPTIONS__
	globalNetworkData.flags.bLogProtocols = SACK_GetProfileIntEx( WIDE("SACK"), WIDE( "Network/Log Protocols" ), 0, TRUE );
	globalNetworkData.flags.bShortLogReceivedData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Received Data(64 byte max)" ), 0, TRUE );
	globalNetworkData.flags.bLogReceivedData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Received Data" ), 0, TRUE );
	globalNetworkData.flags.bLogSentData = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Sent Data" ), globalNetworkData.flags.bLogReceivedData, TRUE );
	globalNetworkData.flags.bLogNotices = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Log Network Notifications" ), 0, TRUE );
	globalNetworkData.dwReadTimeout = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Read wait timeout" ), 5000, TRUE );
	globalNetworkData.dwConnectTimeout = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "Network/Connect timeout" ), 10000, TRUE );
#else
	globalNetworkData.dwReadTimeout = 5000;
	globalNetworkData.dwConnectTimeout = 10000;
#endif
}

static void LowLevelNetworkInit( void )
{
	if( !global_network_data )
		SimpleRegisterAndCreateGlobal( global_network_data );
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

void DumpSocket( PCLIENT pc );
_TCP_NAMESPACE_END

//----------------------------------------------------------------------------
#if defined( WIN32 ) && defined( NETDLL_EXPORTS ) || defined( __LCC__ )
LOGICAL APIENTRY DllMain( HINSTANCE hModule,
                       uint32_t  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
   return TRUE;
}
#endif


//----------------------------------------------------------------------------
//
//    GetMacAddress( version cpg01032007 )
//
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

const char * GetAddrName( SOCKADDR *addr )
{
	char * tmp = ((char**)addr)[-1];
	if( !( (uintptr_t)tmp & 0xFFFF0000 ) )
	{
		lprintf( WIDE("corrupted sockaddr.") );
		DebugBreak();
	}
	if( !tmp )
	{
		{
			char buf[256];

			if( addr->sa_family == AF_INET )
				snprintf( buf, 256, "%03d.%03d.%03d.%03d"
						  ,*(((unsigned char *)addr)+4),
						  *(((unsigned char *)addr)+5),
						  *(((unsigned char *)addr)+6),
						  *(((unsigned char *)addr)+7) );
			else if( addr->sa_family == AF_INET6 )
			{
				snprintf( buf, 256, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x "
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+8))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+10))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+12))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+14))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+16))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+18))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+20))))
						 , ntohs(*(((unsigned short *)((unsigned char*)addr+22))))
						 );
			}
			else
				snprintf( buf, 256, "unknown protocol" );
			
			((char**)addr)[-1] = strdup( buf );
		}
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

void ClearClient( PCLIENT pc )
{
	POINTER pbtemp;
	PCLIENT next;
	PCLIENT *me;
	CRITICALSECTION cs;
	// keep the closing flag until it's really been closed. (getfreeclient will try to nab it)
	uint32_t   dwFlags = pc->dwFlags & (CF_STATEFLAGS|CF_CLOSING|CF_CONNECT_WAITING|CF_CONNECT_CLOSED);
#ifdef VERBOSE_DEBUG
	lprintf( WIDE("CLEAR CLIENT!") );
#endif
	me = pc->me;
	next = pc->next;
	pbtemp = pc->lpUserData;
	cs = pc->csLock;

	ReleaseAddress( pc->saClient );
	ReleaseAddress( pc->saSource );

	// sets socket to 0 - so it's not quite == INVALID_SOCKET
#ifdef LOG_NETWORK_EVENT_THREAD
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "Clear Client %p" ), pc );
#endif
	MemSet( pc, 0, sizeof( CLIENT ) ); // clear all information...
	pc->csLock = cs;
	pc->lpUserData = (uint8_t*)pbtemp;
	if( pc->lpUserData )
		MemSet( pc->lpUserData, 0, globalNetworkData.nUserData * sizeof( uintptr_t ) );
	pc->next = next;
	pc->me = me;
	pc->dwFlags = dwFlags;
}

//----------------------------------------------------------------------------

void TerminateClosedClientEx( PCLIENT pc DBG_PASS )
{
#ifdef VERBOSE_DEBUG
	_lprintf(DBG_RELAY)( WIDE( "terminate? " ) );
#endif
	if( !pc )
		return;
	if( pc->dwFlags & CF_CLOSED )
	{
		PendingBuffer * lpNext;
		//lprintf( "Terminate Closed Client" );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		//lprintf( WIDE( "Terminating closed client..." ) );
		if( IsValid( pc->Socket ) )
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE( "close socket." ) );
#endif
#if defined( USE_WSA_EVENTS )
			WSACloseEvent( pc->event );
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
		ClearClient( pc );
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

void CPROC PendingTimer( uintptr_t unused )
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
		next = pc->next; // will dissappear when closed so save it.
		if( GetTickCount() > (pc->LastEvent + 2000) )
		{
			//lprintf( WIDE("Closing delay terminated client.") );
			TerminateClosedClient( pc );
		}
	}
	//lprintf( WIDE("Leaving network lock.") );
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE("pending timer left global") );
#endif
}

#ifndef __LINUX__

//----------------------------------------------------------------------------

static int NetworkStartup( void )
{
	static int attempt = 0;
	static int nStep = 0,
	          nError;
	static SOCKET sockMaster;
	static SOCKADDR remSin;
	//WSADATA ws;  // used to start up the socket services...
	// can tick before the timer happens...
	//if( !uNetworkPauseTimer )
	//	return 2;
	switch( nStep )
	{
	case 0 :
		//lprintf( "Global is %d %p", sizeof( globalNetworkData ), &globalNetworkData.uNetworkPauseTimer, &globalNetworkData.nProtos );
		SystemCheck();
		/*

		if( WSAStartup( MAKEWORD(2,0), &ws ) )
		{
			nError = WSAGetLastError();
			Log1( WIDE("Failed network startup! (%d)"), nError );
			nStep = 0; // reset...
			return NetworkQuit();
		}
		lprintf("Winsock Version: %d.%d", LOBYTE(ws.wVersion), HIBYTE(ws.wVersion));
		*/
		nStep++;
		attempt = 0;
	case 1 :
		// Sit around, waiting for the network to start...

		//--------------------
		// sorry this is really really ugly to read!
#ifdef WIN32
		sockMaster = OpenSocket( TRUE, FALSE, FALSE, 0 );
		if( sockMaster == INVALID_SOCKET )
		{
			lprintf( WIDE( "Clever OpenSocket failed... fallback... and survey sez..." ) );
#endif
			//--------------------


			sockMaster = socket( AF_INET, SOCK_DGRAM, 0);


			//--------------------
#ifdef WIN32
		}
#endif
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

void CPROC NetworkPauseTimer( uintptr_t psv )
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

#if defined( USE_WSA_EVENTS )

void HandleEvent( PCLIENT pClient )
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
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "Client event on %p" ), pClient );
#endif
	if( WSAEnumNetworkEvents( pClient->Socket, pClient->event, &networkEvents ) == ERROR_SUCCESS )
	{
		{
			if( pClient->dwFlags & CF_UDP )
			{
				if( networkEvents.lNetworkEvents & FD_READ )
				{
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "Got UDP FD_READ" ) );
#endif
					FinishUDPRead( pClient );
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
				while( !NetworkLock( pClient ) )
				{
					// done with events; inactive sockets can't have events
					if( !(pClient->dwFlags & CF_ACTIVE ) )
					{
						pClient->dwFlags &= ~CF_PROCESSING;
						return;
					}
					Relinquish();
				}

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
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "FD_READ" ) );
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
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE("FD_Write") );
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
						lprintf(WIDE( "FD_CLOSE... %p" ), pClient );
#endif
					if( pClient->dwFlags & CF_ACTIVE )
					{
						// might already be cleared and gone..
						InternalRemoveClient( pClient );
						TerminateClosedClient( pClient );
					}
					// section will be blank after termination...(correction, we keep the section state now)
					pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
				}
				if( networkEvents.lNetworkEvents & FD_ACCEPT )
				{
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE("FD_ACCEPT on %p"), pClient );
#endif
					AcceptClient(pClient);
				}
				//lprintf( WIDE("leaveing event handler...") );
				//lprintf( WIDE("Left event handler CS.") );
				NetworkUnlock( pClient );
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
#endif  //defined(USE_WSA_EVENTS)



#endif // if !defined( __LINUX__ )

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetNetworkWriteComplete)( PCLIENT pClient,
                                         cWriteComplete WriteComplete )
{
	if( pClient && IsValid( pClient->Socket ) )
	{
		pClient->write.WriteComplete = WriteComplete;
	}
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetCPPNetworkWriteComplete)( PCLIENT pClient
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

NETWORK_PROC( void, SetNetworkCloseCallback)( PCLIENT pClient,
                                         cCloseCallback CloseCallback )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CloseCallback = CloseCallback;
	}
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetCPPNetworkCloseCallback)( PCLIENT pClient
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

NETWORK_PROC( void, SetNetworkReadComplete)( PCLIENT pClient,
                                        cReadComplete pReadComplete )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.ReadComplete = pReadComplete;
	}
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetCPPNetworkReadComplete)( PCLIENT pClient
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


#ifndef __LINUX__
//----------------------------------------------------------------------------

#  if defined( USE_WSA_EVENTS )

static uintptr_t CPROC NetworkThreadProc( PTHREAD thread );

static void ClearThreadEvents( struct peer_thread_info *info )
{
	struct peer_thread_info *first_peer = info;
	struct peer_thread_info *peer;
#ifdef LOG_NETWORK_EVENT_THREAD
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "Clear Events." ) );
#endif
	{
		PCLIENT pc;
		PCLIENT next;
		for( pc = globalNetworkData.ClosedClients; pc; pc = next )
		{
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "reclaim closed client... %p" ), pc );
#endif
			next = pc->next; // will dissappear when closed so save it.
			if(+ GetTickCount() > (pc->LastEvent + 2000) )
			{
				//lprintf( WIDE("Closing delay terminated client.") );
				TerminateClosedClient( pc );
			}
		}
	}

	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		peer->nEvents = 1;
		// if he's still asleep on 1 event, don't bother waking him.
		// if a thread is 'processing' then we cannot wait for it to be waiting on the events;
		// it is likely waiting to be scheduled anyhow; so go ahead and schedule it.
		if( peer->parent_peer && ( peer->nEvents != peer->nWaitEvents ) && ( !peer->flags.bProcessing ) )
		{
			// have to make sure threads reset to the new list.
			WSASetEvent( peer->hThread );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			while( peer->nWaitEvents != 1 )
				Relinquish();
			EnterCriticalSec( &globalNetworkData.csNetwork );
		}
		// have to set the list to clear state after thread ack's having 1 event.
		// otherwise - it might wake and get a socket event.
		EmptyList( &peer->monitor_list );
		EmptyDataList( &peer->event_list );
		SetLink( &peer->monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
		SetDataItem( &peer->event_list, 0, &peer->hThread );
	}
}


static void AddThreadEvent( PCLIENT pc, struct peer_thread_info *info )
{
	struct peer_thread_info *first_peer;
	struct peer_thread_info *last_peer;
	struct peer_thread_info *peer;

	if( pc->dwFlags & CF_PROCESSING )
	{
#ifdef LOG_NETWORK_EVENT_THREAD
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE("This is a socket that is processing, don't put in schedule.") );
#endif
		return;
	}

	if( pc->flags.bRemoveFromEvents )
	{
		return;
	}

	for( first_peer = info; first_peer && first_peer->parent_peer; first_peer = first_peer->parent_peer );
	for( last_peer = info; last_peer && last_peer->child_peer; last_peer = last_peer->child_peer );
	for( peer = first_peer; peer && ( ( peer->nEvents >= 60 ) || peer->flags.bProcessing ); peer = peer->child_peer );

	if( !peer )
	{
#ifdef LOG_NETWORK_EVENT_THREAD
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE( "Now at event capacity, creating another thread" ) );
#endif
		AddLink( &globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)last_peer ) );
		while( !last_peer->child_peer )
			Relinquish();
		last_peer = last_peer->child_peer;
		peer = last_peer; // since we needed to create some more pc space; this is where we should add this pc;
	}


	// make sure to only add this handle when the first peer will also be added.
	// this means the list can be 61 and at this time no more.
	AddLink( &peer->monitor_list, pc );
	AddDataItem( &peer->event_list, &pc->event );
	pc->this_thread = peer;
	peer->nEvents++;
	//if( globalNetworkData.flags.bLogNotices )
	//	lprintf( WIDE( "Added event %p %p(%d) %d" ), pc->this_thread, pc, pc->Socket, pc->event );
}

static void WakeNewThreadEvents( struct peer_thread_info *info )
{
	struct peer_thread_info *first_peer = info;
	struct peer_thread_info *peer;
#ifdef LOG_NETWORK_EVENT_THREAD
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "Wake New Events." ) );
#endif

	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		// this will be the parent of all; so his parent will be null, and we're already awake.
		if( peer->parent_peer && peer->nEvents != peer->nWaitEvents )
		{
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "wake parent thread %p" ), peer );
#endif
			WSASetEvent( peer->hThread );
		}
	}
}

#endif //defined( USE_WSA_EVENTS )


void SetNetworkCallbackWindow( HWND hWnd )
{
}

// this is passed '0' when it is called internally
// this is passed '1' when it is called by idleproc
int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check )
{
	//lprintf( WIDE("Check messages.") );
	if( globalNetworkData.bQuit )
		return -1;
	if( IsNetworkThread( ) )
	{
#  if defined( USE_WSA_EVENTS )
		PCLIENT first = NULL;
		int first_added = 0;
		//int Count = 0;
		// this is the thread that should be waiting here.
		//static PLIST events = NULL;
		//static PLIST clients = NULL; // clients are added parallel, so events are in order too.
		PCLIENT pc;
		// disallow changing of the lists.
#ifdef LOG_NETWORK_EVENT_THREAD
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE("End - thread processing") );
#endif
		thread->flags.bProcessing = 0;
		if( !thread->parent_peer )
		{
			EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
			lprintf( WIDE( "Process Network in global" ) );
#endif
			if( thread->nEvents != 1 )
				ClearThreadEvents( thread );

			// if first is NULL, and first link in queue is NULL the equality will fail.
			// once first is set ot something else, then we have that, and we need to just test if the next link
			// dequeued is the first again.
			// if the scheduled one is the one we're looking for; don't put it back in the queue, and end searching.
			for( pc = globalNetworkData.ActiveClients; pc; pc = pc->next )
			{
				// use this for "added to schedule".  Closing removes from schedule.
				if( !pc->flags.bAddedToEvents )
				{
					if( pc->dwFlags & CF_CLOSED || ( !( pc->dwFlags & CF_ACTIVE ) ) )
					{
						lprintf(WIDE( " Found closed? %p" ), pc );
						continue;
					}
#ifdef LOG_NETWORK_EVENT_THREAD
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "Added to schedule : %p" ), pc );
#endif
					EnqueLink( &globalNetworkData.event_schedule, pc );
					pc->flags.bAddedToEvents = 1;
				}
			}

			for( pc = (PCLIENT)DequeLink( &globalNetworkData.event_schedule );
				 pc && pc != first;
				  pc = (PCLIENT)DequeLink( &globalNetworkData.event_schedule ) )
			{
				// have to set this in the loop; otherwise as an initial condition
				// it triggers the end of the loop
				if( !first )
					first = pc;
				// socket might not be quite opened yet... or maybe the event isn't created just yet
				if( !IsValid( pc->Socket ) || !(pc->event) || !(pc->Socket) )
				{
					pc->flags.bAddedToEvents = 0;
					lprintf( WIDE( "Failed - invalid socket. %p %d" ), pc, pc->Socket );
					continue;
				}
				// make sure we drop inactive clients immediately...
				if( !( pc->dwFlags & CF_ACTIVE ) )
				{
					pc->flags.bAddedToEvents = 0;
					lprintf( WIDE( "failed - not active %p" ), pc );
					continue;
				}

				if( first == pc ) // is first
					first_added = 1;
				// don't schedule things that are invalid... so move this down.
				EnqueLink( &globalNetworkData.event_schedule, pc );
				AddThreadEvent( pc, thread );
			}
			// and put the first back in... (it was just dequeued... so add it back) (provides rotation)
			if( pc && first_added )
				EnqueLink( &globalNetworkData.event_schedule, pc );
			WakeNewThreadEvents( thread );
			// okay, we have a picture of what was active
			// they could still go away in the middle.
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
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "%p Waiting on %d events" ), thread, thread->nEvents );
#endif
			thread->nWaitEvents = thread->nEvents;
			result = WSAWaitForMultipleEvents( thread->nEvents
														, (const HANDLE *)thread->event_list->data
														, FALSE
														, (quick_check)?0:WSA_INFINITE
														, FALSE
														);
			// this should never be 0, but we're awake, not sleeping, and should say we're in a place
			// where we probably do want to be woken on a 0 event.
			if( result != WSA_WAIT_EVENT_0 )
			{
#ifdef LOG_NETWORK_EVENT_THREAD
				if( globalNetworkData.flags.bLogNotices )
					lprintf( WIDE("Begin - thread processing %d"), result );
#endif
				thread->flags.bProcessing = 1;
				thread->nWaitEvents = 0;
			}
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE("Event Wait Result was %d"), result );
#endif
			if( result == WSA_WAIT_FAILED )
			{
				DWORD dwError = WSAGetLastError();
				if( dwError == WSA_INVALID_HANDLE )
				{
					lprintf( WIDE( "Rebuild list, have a bad event handle somehow." ) );
					break;
				}
				lprintf( WIDE( "error of wait is %d" ), WSAGetLastError() );
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
					//lprintf( "index is %d", result-(WSA_WAIT_EVENT_0) );
					HandleEvent( (PCLIENT)GetLink( &thread->monitor_list, result-(WSA_WAIT_EVENT_0) ) );
					if( thread->parent_peer )
					{
						// if this is a child worker, wait for main to rebuild events.
						// if this was the main thread, it would wake us anyway...
						WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
						while( thread->nEvents != 1 )
							Relinquish();
					}
				}
				else
				{
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE( "RESET GLOBAL EVENT" ) );
					WSAResetEvent( thread->hThread );
				}
				return 1;
			}
		}
		// result 0... we had nothing to do
		// but we are this thread.
		//DeleteList( &clients );
		//DeleteList( &events );
#  else
		MSG Msg;
		if( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( !Msg.hwnd ||  // try something NEW here - an AND instead of OR
				Msg.message == SOCKMSG_CLOSE )
			{
				//lprintf( WIDE("Got a message... dispatching to network.") );
				StandardNetworkProcess( globalNetworkData.ghWndNetwork, Msg.message,
                                    Msg.wParam, Msg.lParam );
			}
			else
			{
				DispatchMessage( &Msg );
			}
			return 1;
		}
#  endif
		return 0;
	}
	// return -1, in case we are an idle proc, this will
	// de-elect this proc as an idle candiate for this thread
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


uintptr_t CPROC NetworkThreadProc( PTHREAD thread )
{
#  ifdef USE_WSA_EVENTS
	struct peer_thread_info *peer_thread = (struct peer_thread_info*)GetThreadParam( thread );
	struct peer_thread_info this_thread;
	// and when unloading should remove these timers.
	if( !peer_thread )
	{
		//globalNetworkData.uPendingTimer = AddTimer( 5000, PendingTimer, 0 );
		globalNetworkData.uNetworkPauseTimer = AddTimerEx( 1, 1000, NetworkPauseTimer, 0 );
		if( !globalNetworkData.event_schedule )
			globalNetworkData.event_schedule = CreateLinkQueue();
		//globalNetworkData.hMonitorThreadControlEvent = CreateEvent( NULL, 0, FALSE, NULL );
	}

	this_thread.monitor_list = NULL;
	this_thread.event_list = CreateDataList( sizeof( WSAEVENT ) );
	this_thread.hThread = WSACreateEvent();
	this_thread.parent_peer = peer_thread;
	this_thread.child_peer = NULL;
	this_thread.thread = thread;

	// setup this as if it was cleared already.
	this_thread.nEvents = 1;
	SetLink( &this_thread.monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
	SetDataItem( &this_thread.event_list, 0, &this_thread.hThread );
	
	if( peer_thread )
		peer_thread->child_peer = &this_thread;
	else
		globalNetworkData.root_thread = &this_thread;

	if( !peer_thread )
		globalNetworkData.hMonitorThreadControlEvent = this_thread.hThread;

#  endif
	while( !globalNetworkData.pThreads ) // creator won't pass until bThreadInitComplete is set.
		Relinquish();

	globalNetworkData.flags.bThreadInitOkay = TRUE;
	globalNetworkData.flags.bThreadInitComplete = TRUE;
	while( !globalNetworkData.bQuit )
	{
		if( !ProcessNetworkMessages( &this_thread, 0) )
		{
#  ifndef UNDER_CE
			WaitMessage();
#  endif
		}
	}

	xlprintf(2100)( WIDE( "Enter global network on shutdown... (thread exiting)" ) );

	EnterCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE( "NetworkThread(exit) in global" ) );
#  endif

#  ifdef USE_WSA_EVENTS
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
#  endif
	globalNetworkData.flags.bThreadExit = TRUE;

	xlprintf(2100)( WIDE("Shut down network thread.") );

#if 0
#  ifdef USE_WSA_EVENTS
	if( !globalNetworkData.root_thread )
#  endif
	{
		Deallocate( uint8_t*, globalNetworkData.pUserData ); // should be first one pointed to...
		{
			INDEX idx;
			PCLIENT_SLAB slab;
			LIST_FORALL( globalNetworkData.ClientSlabs, idx, PCLIENT_SLAB, slab )
			{
				Release( slab );
			}
			DeleteList( &globalNetworkData.ClientSlabs );
		}
		globalNetworkData.AvailableClients = NULL;
		globalNetworkData.ActiveClients = NULL;
		globalNetworkData.ClosedClients = NULL;
		globalNetworkData.pUserData = NULL;
	}
#endif
	globalNetworkData.flags.bThreadInitComplete = FALSE;
	globalNetworkData.pThread = NULL;
	globalNetworkData.flags.bNetworkReady = FALSE;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( WIDE( "NetworkThread(exit) left global" ) );
#  endif
	DeleteCriticalSec( &globalNetworkData.csNetwork );	 //spv:980303
	return 0;
}
#else // if !__LINUX__

int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t unused )
{
	fd_set read, write, except;
	int cnt, maxcnt;
	PCLIENT pc;
	struct timeval time;
	if( globalNetworkData.bQuit )
		return -1;
	if( IsThisThread( globalNetworkData.pThread ) )
	{
		FD_ZERO( &read );
		FD_ZERO( &write );
		FD_ZERO( &except );
		maxcnt = 0;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		for( pc = globalNetworkData.ActiveClients; pc; pc = pc->next )
		{
			// socket might not be quite opened yet...
			if( !IsValid( pc->Socket ) )
				continue;
			if( pc->dwFlags & (CF_LISTEN|CF_READPENDING) )
			{
				if( pc->Socket >= maxcnt )
					maxcnt = pc->Socket+1;
				FD_SET( pc->Socket, &read );
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( WIDE( "Added for read select %p(%d)" ), pc, pc->Socket );
#endif
			}

			if( pc->dwFlags & CF_LISTEN ) // only check read...
				continue;

			if( pc->dwFlags & ( CF_WRITEPENDING | CF_CONNECTING ) )
			{
				if( pc->Socket >= maxcnt )
					maxcnt = pc->Socket+1;
				pc->dwFlags |= CF_WRITEISPENDED;
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( WIDE("Pending write... Setting socket into write list...") );
#endif
				FD_SET( pc->Socket, &write );
			}

			if( pc->Socket >= maxcnt )
				maxcnt = pc->Socket+1;
			FD_SET( pc->Socket, &except );
#ifdef LOG_NOTICES
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "Added for except event %p(%d)" ), pc, pc->Socket );
#endif

		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		if( !maxcnt )
			return 0;
		time.tv_sec = 5;
		time.tv_usec = 0; // need timer to be quick to watch new sockets
		// should be able to make a signal handle
#ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE("Entering select....") );
#endif
		cnt = select( maxcnt, &read, &write, &except, &time );
		if( cnt < 0 )
		{
			if( errno == EINTR )
				return 1;
			if( errno == EBADF )
			{
				PCLIENT next;
				EnterCriticalSec( &globalNetworkData.csNetwork );
				for( pc = globalNetworkData.ActiveClients; pc; pc = next )
				{
					next = pc->next;
					if( IsValid( pc->Socket ) )
					{
						if( pc->Socket >= maxcnt )
							maxcnt = pc->Socket+1;
						FD_ZERO( &except );
						FD_SET( pc->Socket, &except );
						time.tv_sec = 0;
						time.tv_usec = 0;
						cnt = select( maxcnt, NULL, NULL, &except, &time );
						if( cnt < 0 )
						{
							if( errno == EBADF )
							{
								Log1( WIDE("Bad descriptor is : %d"), pc->Socket );
								InternalRemoveClient( pc );
								LeaveCriticalSec( &globalNetworkData.csNetwork );
								return 1;
							}
						}
					}
				}
				LeaveCriticalSec( &globalNetworkData.csNetwork );
			}
			Log1( WIDE("Sorry Select call failed... %d"), errno );
		}
		restart_scan:
		if( cnt > 0 )
		{
			THREAD_ID prior = 0;
			PCLIENT next;
		start_lock:
			EnterCriticalSec( &globalNetworkData.csNetwork );
			for( pc = globalNetworkData.ActiveClients; pc; pc = next )
			{
				next = pc->next;
				if( !IsValid( pc->Socket ) )
					continue;
				if( pc->dwFlags & CF_WANTS_GLOBAL_LOCK)
				{
					LeaveCriticalSec( &globalNetworkData.csNetwork );
					goto start_lock;
				}

				if( FD_ISSET( pc->Socket, &read ) )
				{
					if( EnterCriticalSecNoWait( &pc->csLock, &prior ) < 1 )
					{
						// unlock the global section for a moment..
						// client may be requiring both local and global locks (already has local lock)
						LeaveCriticalSec( &globalNetworkData.csNetwork );
						goto start_lock;
					}
					//EnterCriticalSec( &pc->csLock );
					LeaveCriticalSec( &globalNetworkData.csNetwork );
					if( !(pc->dwFlags & CF_ACTIVE ) )
					{
						// change to inactive status by the time we got here...
						LeaveCriticalSec( &pc->csLock );
						goto start_lock;
					}

					if( pc->dwFlags & CF_LISTEN )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("accepting...") );
#endif
						AcceptClient( pc );
					}
					else if( pc->dwFlags & CF_UDP )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("UDP Read Event..."));
#endif
						FinishUDPRead( pc );
					}
					else if( pc->bDraining )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("TCP Drain Event..."));
#endif
						TCPDrainRead( pc );
					}
					else if( pc->dwFlags & CF_READPENDING )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("TCP Read Event..."));
#endif
						// packet oriented things may probably be reading only
						// partial messages at a time...
						FinishPendingRead( pc DBG_SRC );
						if( pc->dwFlags & CF_TOCLOSE )
						{
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( WIDE( "Pending read failed - reset connection." ) );
#endif
							InternalRemoveClientEx( pc, FALSE, FALSE );
							TerminateClosedClient( pc );
						}
						else if( !pc->RecvPending.s.bStream )
							pc->dwFlags |= CF_READREADY;
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("TCP Set read ready...") );
#endif
						pc->dwFlags |= CF_READREADY;
					}
					cnt--;
					FD_CLR( pc->Socket, &read );
					LeaveCriticalSec( &pc->csLock );
					goto restart_scan;
				}
				if( FD_ISSET( pc->Socket, &write ) )
				{
					if( EnterCriticalSecNoWait( &pc->csLock, &prior ) < 1 )
					{
						// unlock the global section for a moment..
						// client may be requiring both local and global locks (already has local lock)
						LeaveCriticalSec( &globalNetworkData.csNetwork );
						goto start_lock;
					}
					//EnterCriticalSec( &pc->csLock );
					LeaveCriticalSec( &globalNetworkData.csNetwork );
					if( !(pc->dwFlags & CF_ACTIVE ) )
					{
						// change to inactive status by the time we got here...
						LeaveCriticalSec( &pc->csLock );
						goto start_lock;
					}

					if( pc->dwFlags & CF_CONNECTING )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("Connected!") );
#endif
						pc->dwFlags |= CF_CONNECTED;
						pc->dwFlags &= ~CF_CONNECTING;
						{
							int error;
							socklen_t errlen = sizeof(error);
							getsockopt( pc->Socket, SOL_SOCKET
										 , SO_ERROR
										 , &error, &errlen );
							lprintf( WIDE("Error checking for connect is: %s"), strerror(error) );
							if( pc->pWaiting )
							{
#ifdef LOG_NOTICES
								if( globalNetworkData.flags.bLogNotices )
									lprintf( WIDE( "Got connect event, waking waiter.." ) );
#endif
								 WakeThread( pc->pWaiting );
							}
							if( pc->connect.ThisConnected )
								pc->connect.ThisConnected( pc, error );
							// if connected okay - issue first read...
							if( !error )
							{
								if( pc->read.ReadComplete )
								{
									pc->read.ReadComplete( pc, NULL, 0 );
								}
								if( pc->lpFirstPending )
								{
									lprintf( WIDE("Data was pending on a connecting socket, try sending it now") );
									TCPWrite( pc );
								}
							}
							else
							{
								pc->dwFlags |= CF_CONNECTERROR;
								FD_CLR( pc->Socket, &read );
								LeaveCriticalSec( &pc->csLock );
								goto restart_scan;
							}
						}
					}
					else if( pc->dwFlags & CF_UDP )
					{
						// udp write event complete....
						// do we ever care? probably sometime...
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE("TCP Write Event...") );
#endif
						pc->dwFlags &= ~CF_WRITEISPENDED;
						TCPWrite( pc );
					}
					cnt--;
					FD_CLR( pc->Socket, &write );
					LeaveCriticalSec( &pc->csLock );
					goto restart_scan;
				}

				if( FD_ISSET( pc->Socket, &except ) )
				{
					if( EnterCriticalSecNoWait( &pc->csLock, &prior ) < 1 )
					{
						// unlock the global section for a moment..
						// client may be requiring both local and global locks (already has local lock)
						LeaveCriticalSec( &globalNetworkData.csNetwork );
						goto start_lock;
					}
					//EnterCriticalSec( &pc->csLock );
					LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( WIDE("Except Event...") );
#endif
					if( !(pc->dwFlags & CF_ACTIVE ) )
					{
						// change to inactive status by the time we got here...
						LeaveCriticalSec( &pc->csLock );
						goto restart_scan;
					}
					if( !pc->bDraining )
					{
						size_t bytes_read;
						while( ( bytes_read = FinishPendingRead( pc DBG_SRC ) ) > 0
								&& bytes_read != (size_t)-1 ); // try and read...
					}
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf(" FD_CLOSE...\n");
#endif
					InternalRemoveClient( pc );
					TerminateClosedClient( pc );
					cnt--;
					FD_CLR( pc->Socket, &except );
					LeaveCriticalSec( &pc->csLock );
					goto restart_scan;
				}
			}
			// had some event  - return 1 to continue working...
			LeaveCriticalSec( &globalNetworkData.csNetwork );
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

static uintptr_t CPROC NetworkThreadProc( PTHREAD thread )
{
// idle loop to handle select() call....
// this thread has many other protections against starting
// a second time... and this may result from the parent thread
// before even starting the first, invalidating this wait
// for global thread.
	//if( globalNetworkData.pThread )
	//{
   //   lprintf( WIDE("Thread has already been started.") );
	//	return 0;
	//}
	//Relinquish();
	globalNetworkData.uPendingTimer = AddTimer( 5000, PendingTimer, 0 );
#if defined( __ANDROID__ ) && defined( __ANDROID_OLD_PLATFORM_SUPPORT__ )
	bsd_signal( SIGPIPE, SIG_IGN );
#else
	signal( SIGPIPE, SIG_IGN );
#endif
	while( !globalNetworkData.pThreads )
	{
		Relinquish(); // wait for pThread to be done
	}
	globalNetworkData.pThread = thread;
	globalNetworkData.flags.bThreadInitOkay = TRUE;
	globalNetworkData.flags.bThreadInitComplete = TRUE;
	globalNetworkData.flags.bNetworkReady = TRUE;
	//lprintf( "So we're good to go; set all flags as sucess, and begin doing nothing." );
	//lprintf( WIDE("Network Thread Began...") );

	while( !globalNetworkData.bQuit )
	{
		int n = ProcessNetworkMessages( NULL, 0 );
		if( n < 0 )
			break;
		else if( !n )
		{
			// should sleep when there's no sockets to listen to...
			WakeableSleep( SLEEP_FOREVER ); // groovy we can Sleep!!!
		}
	}
	{
		INDEX idx;
		PCLIENT_SLAB slab;
		LIST_FORALL( globalNetworkData.ClientSlabs, idx, PCLIENT_SLAB, slab )
		{
			Deallocate( PCLIENT_SLAB, slab );
		}
	}
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( WIDE("Exiting network thread...") );
#endif
	DeleteListEx( &globalNetworkData.ClientSlabs DBG_SRC );
	Deallocate( uint8_t*, globalNetworkData.pUserData );
	globalNetworkData.pUserData = NULL;
	globalNetworkData.pThreads = NULL; // confirm thread exit.
	globalNetworkData.flags.bNetworkReady = FALSE;
	return 0;
}

#endif

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
		if( globalNetworkData.flags.bLogNotices )
			lprintf( WIDE("Remove active client %p"), globalNetworkData.ActiveClients );
		InternalRemoveClient( globalNetworkData.ActiveClients );
	}
	globalNetworkData.bQuit = TRUE;
#ifdef USE_WSA_EVENTS
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (trigger quit)" ) );
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

void ReallocClients( uint32_t wClients, int nUserData )
{
	uint8_t* pUserData;
	PCLIENT_SLAB pClientSlab;
	if( !global_network_data )
		LowLevelNetworkInit();

	if( !MAX_NETCLIENTS )
	{
		//lprintf( WIDE("Starting Network Init!") );
		InitializeCriticalSec( &globalNetworkData.csNetwork );
	}
	// else lprintf( WIDE("Restarting network init!") );

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );
	if( !wClients )
		wClients = 16;  // default 16 clients...
	if( !nUserData )
		nUserData = 16;

	// keep the max of specified connections..
	if( wClients < MAX_NETCLIENTS )
		wClients = MAX_NETCLIENTS;
	if( wClients > MAX_NETCLIENTS )
	{
		size_t n;
		//Log1( WIDE("Creating %d Client Resources"), MAX_NETCLIENTS );
		pClientSlab = NULL;
		pClientSlab = (PCLIENT_SLAB)NewArray( CLIENT_SLAB, wClients - MAX_NETCLIENTS );
		MemSet( pClientSlab, 0, (wClients - MAX_NETCLIENTS)*sizeof(CLIENT_SLAB) ); // can't clear the lpUserData Address!!!
		pClientSlab->count = wClients - MAX_NETCLIENTS;
		for( n = 0; n < pClientSlab->count; n++ )
		{
			pClientSlab->client[n].Socket = INVALID_SOCKET; // unused sockets on all clients.
			AddAvailable( pClientSlab->client + n );
		}
		AddLink( &globalNetworkData.ClientSlabs, pClientSlab );
	}

	// keep the max of specified data..
	if( nUserData < globalNetworkData.nUserData )
		nUserData = globalNetworkData.nUserData;

	if( nUserData > globalNetworkData.nUserData || wClients > MAX_NETCLIENTS )
	{
		INDEX idx;
		PCLIENT_SLAB slab;
		uint32_t n;
		int tot = 0;
		//globalNetworkData.nUserData = nUserData;
		pUserData = (uint8_t*)NewArray( uint8_t, nUserData * sizeof( uintptr_t ) * wClients );
		MemSet( pUserData, 0, nUserData * sizeof( uintptr_t ) * wClients );
		LIST_FORALL( globalNetworkData.ClientSlabs, idx, PCLIENT_SLAB, slab )
		{
			for( n = 0; n < slab->count; n++ )
			{
				if( slab->client[n].lpUserData )
					MemCpy( (char*)pUserData + (tot * (nUserData * sizeof( uintptr_t )))
							, slab->client[n].lpUserData
                     , globalNetworkData.nUserData * sizeof( uintptr_t ) );
				slab->client[n].lpUserData = (unsigned char*)pUserData + (tot * (nUserData * sizeof( uintptr_t )));

				InitializeCriticalSec( &slab->client[n].csLock );

				tot++;
			}
		}
		if( globalNetworkData.pUserData )
			Deallocate( uint8_t*, globalNetworkData.pUserData );
		globalNetworkData.pUserData = pUserData;
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
	globalNetworkData.bQuit = FALSE;

	ReallocClients( wClients, wUserData );

	//-------------------------
	// please be mindful of the following data declared immediate...
	if( globalNetworkData.pThreads )
	{
		xlprintf(2400)( WIDE("Thread already active...") );
		// might do something... might not...
		return TRUE; // network thread active, do not realloc
	}
	AddLink( &globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)/*peer_thread==*/NULL ) );
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
			}
		}
	}
#endif

	//lprintf( WIDE("Network Initialize Complete!") );
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
	for( pClient = globalNetworkData.AvailableClients; pClient; pClient = pClient->next )
		if( !( pClient->dwFlags & CF_CLOSING ) )
			break;
	if( pClient )
	{
		// oterhwise we'll deadlock the closing client...
		// an opening condition has global lock (above)
		// and a closing socket will want the global lock before it's done.
		pClient = GrabClient( pClient );
#ifdef USE_NATIVE_CRITICAL_SECTION
		EnterCriticalSec( &pClient->csLock );
#else
		EnterCriticalSecEx( &pClient->csLock DBG_RELAY );
#endif
		ClearClient( pClient ); // clear client is redundant here... but saves the critical section now
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
		*(uintptr_t*)(lpClient->lpUserData+(nLong * sizeof(uintptr_t))) = dwValue;
	}
	return;
}

//----------------------------------------------------------------------------

int GetAddressParts( SOCKADDR *sa, uint32_t *pdwIP, uint16_t *pdwPort )
{
	if( sa )
	{
		if( sa->sa_family == AF_INET )
		{
			if( pdwIP )
				(*pdwIP) = (uint32_t)(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr);
			if( pdwPort )
				(*pdwPort) = ntohs((uint16_t)( (SOCKADDR_IN*)sa)->sin_port);
			return TRUE;
		}
	}
	return FALSE;
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
		return(*(uintptr_t*)(lpClient->lpUserData + (nLong * sizeof(uintptr_t))));
	}

	return (uintptr_t)-1;   //spv:980303
}

//----------------------------------------------------------------------------

/*
NETWORK_PROC( uint16_t, GetNetworkWord )(PCLIENT lpClient,int nWord)
{
	if( !lpClient )
		return 0xFFFF;
	if( nWord < (globalNetworkData.nUserData *2) )
		return(*(uint16_t*)(lpClient->lpUserData + (nWord * 2)));
	return 0xFFFF;
}
*/
//---------------------------------------------------------------------------



NETWORK_PROC( SOCKADDR *, DuplicateAddress )( SOCKADDR *pAddr ) // return a copy of this address...
{
	POINTER tmp = (POINTER)( ( (uintptr_t)pAddr ) - 2*sizeof(uintptr_t) );
	SOCKADDR *dup = AllocAddr();
	POINTER tmp2 = (POINTER)( ( (uintptr_t)dup ) - 2*sizeof(uintptr_t) );
	MemCpy( tmp2, tmp, MAGIC_SOCKADDR_LENGTH + 2*sizeof(uintptr_t) );
	if( (POINTER)( ( (uintptr_t)pAddr ) - sizeof(uintptr_t) ) )
		( (char**)( ( (uintptr_t)dup ) - sizeof(uintptr_t) ) )[0]
				= strdup( ((char**)( ( (uintptr_t)pAddr ) - sizeof(uintptr_t) ))[0] );
	return dup;
}


//---------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateAddress_hton)( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
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
	SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
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
			SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
		Deallocate( char *, tmp );
#else
		if( inet_pton( AF_INET, lpName, (struct in6_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
#endif
	}
	else 	if( lpName &&
				( ( lpName[0] >= '0' && lpName[0] <= '9' )
				 || ( lpName[0] >= 'a' && lpName[0] <= 'f' )
				 || ( lpName[0] >= 'A' && lpName[0] <= 'F' )
				 || lpName[0] == ':' )
				&& StrChr( lpName, ':' )!=StrRChr( lpName, ':' ) )
	{
#ifdef UNICODE
		char *tmp = CStrDup( lpName );
		if( inet_pton( AF_INET6, tmp, (struct in_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, 24 );
			lpsaAddr->sin_family       = AF_INET6;         // InetAddress Type.
			conversion_success = TRUE;
		}
		Deallocate( char *, tmp );
#else
		if( inet_pton( AF_INET6, lpName, (struct in6_addr*)&lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, 24 );
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
						return(NULL);
					}
					else
					{
						lprintf( WIDE( "Strange, gethostbyname failed, but AF_INET worked..." ) );
						SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
						lpsaAddr->sin_family = AF_INET;
						memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
							 phe->h_addr,
							 phe->h_length);
					}
				}
				else
				{
					SET_SOCKADDR_LENGTH( lpsaAddr, 26 );
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
				SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
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
			SET_SOCKADDR_LENGTH( lpsaAddr, 16 );
		}
	}
#ifdef UNICODE
	Deallocate( char *, _lpName );
#  undef lpName
#endif
	// put in his(destination) port number...
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//----------------------------------------------------------------------------

#ifdef __cplusplus
namespace udp {
#endif
NETWORK_PROC( void, DumpAddrEx)( CTEXTSTR name, SOCKADDR *sa DBG_PASS )
	{
		LogBinary( (uint8_t *)sa, SOCKADDR_LENGTH( sa ) );
		if( sa->sa_family == AF_INET ) {
			lprintf( WIDE("%s: (%s) %03d %03d.%03d.%03d.%03d "), name
					, ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
					  //*(((unsigned char *)sa)+0),
					  //*(((unsigned char *)sa)+1),
					  , ntohs(*(((unsigned short *)((unsigned char*)sa+2))))
					  ,*(((unsigned char *)sa)+4),
					  *(((unsigned char *)sa)+5),
					  *(((unsigned char *)sa)+6),
					  *(((unsigned char *)sa)+7) );
		} else if( sa->sa_family == AF_INET6 )
		{
			lprintf( WIDE( "Socket address binary: %s" ), name );
			lprintf( WIDE("%s: (%s) %03d %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x ")
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
//		EventLog(SOH_VER_HOST_NAME,gwCategory,0,NULL,0,NULL);
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
	SOCKADDR *test_addr;
	INDEX idx;
	LIST_FORALL( globalNetworkData.addresses, idx, SOCKADDR*, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, &((SOCKADDR_IN*)test_addr)->sin_addr, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
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
	SET_SOCKADDR_LENGTH( bcast, 16 );
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
		THREAD_ID prior = 0;
		THREAD_ID prior_g = 0;
	start_lock:
		//lpClient->dwFlags |= CF_WANTS_GLOBAL_LOCK;
		//_lprintf(DBG_RELAY)( WIDE( "Lock %p" ), lpClient );
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( EnterCriticalSecNoWait( &globalNetworkData.csNetwork, &prior_g ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, &prior_g DBG_RELAY ) < 1 )
#endif
		{
			lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
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
		if( !EnterCriticalSecNoWait( &lpClient->csLock, &prior ) )
#else
		if( EnterCriticalSecNoWaitEx( &lpClient->csLock, &prior DBG_RELAY ) < 1 )
#endif
		{
			// unlock the global section for a moment..
			// client may be requiring both local and global locks (already has local lock)

#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( &globalNetworkData.csNetwork);
#else
			LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
			Idle();
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

void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNofity, LOGICAL bLinger DBG_PASS )
{
#ifdef LOG_SOCKET_CREATION
	_lprintf( DBG_RELAY )( WIDE("Removing this client %p (%d)"), lpClient, lpClient->Socket );
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
			struct linger lingerSet;
			// linger ON causes delay on close... otherwise close returns immediately
			lingerSet.l_onoff = 0; // on , with no time = off.
			lingerSet.l_linger = 0;
			// set server to allow reuse of socket port
			if( setsockopt( lpClient->Socket, SOL_SOCKET, SO_LINGER,
				(char*)&lingerSet, sizeof( lingerSet ) ) <0 )
			{
				lprintf( WIDE( "error setting no linger in close." ) );
				//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
			}
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

		while( !NetworkLock( lpClient ) )
		{
			if( !(lpClient->dwFlags & CF_ACTIVE ) )
			{
				return;
			}
			Relinquish();
		}

		// allow application a chance to clean it's references
		// to this structure before closing and cleaning it.

		if( !bBlockNofity )
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
		{
			PCLIENT first = NULL;
			PCLIENT tmp_scheduled;
			// if first is NULL, and first link in queue is NULL the equality will fail.
			// once first is set ot something else, then we have that, and we need to just test if the next link
			// dequeued is the first again.
			// if the scheduled one is the one we're looking for; don't put it back in the queue, and end searching.
			for( tmp_scheduled = (PCLIENT)DequeLink( &globalNetworkData.event_schedule );
					tmp_scheduled && ( tmp_scheduled != first ) && ( tmp_scheduled != lpClient );
					tmp_scheduled = (PCLIENT)DequeLink( &globalNetworkData.event_schedule ) )
			{
				if( !first )
					first = tmp_scheduled;
				EnqueLink( &globalNetworkData.event_schedule, tmp_scheduled );
			}
			if( tmp_scheduled && tmp_scheduled != lpClient )
			{
				EnqueLink( &globalNetworkData.event_schedule, tmp_scheduled );
			}
			else
			{
				if( globalNetworkData.flags.bLogNotices )
					lprintf( WIDE( "Removed from schedule : %p" ), tmp_scheduled );
			}
			// no longer in events.
			lpClient->flags.bAddedToEvents = 0;
		}
		AddClosed( GrabClient( lpClient ) );
#ifdef LOG_DEBUG_CLOSING
		lprintf( WIDE( "Leaving client critical section" ) );
#endif
		//LeaveCriticalSec( &lpClient->csLock );
		//lprintf( WIDE( "Leaving network critical section" ) );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlock( lpClient );
		//lprintf( WIDE( "And no nothing is locked?!" ) );
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

NETWORK_PROC( SOCKADDR *, MakeNetworkAddressFromBinary )( uint8_t *data, size_t datalen ) {
	SOCKADDR *addr = AllocAddr();
	size_t namelen = strlen( (const char*)data );
	if( namelen ) // if empty name, don't include it.
		SetAddrName( addr, (const char*)data );
	SET_SOCKADDR_LENGTH( addr, data[namelen+1] );
	MemCpy( addr, data + namelen + 2, data[namelen+1] );
	return addr;
}

SACK_NETWORK_NAMESPACE_END

