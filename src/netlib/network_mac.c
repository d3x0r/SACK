///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_mac.C
//
// Description -  Network event handling for MAC(BSD) using kevent 
//
// Author      -  James Buckeyne
//
// Create Date -  2019-06-26
//
///////////////////////////////////////////////////////////////////////////

//
//  DEBUG FLAGS IN netstruc.h
//
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE  // for features.h
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#define FIX_RELEASE_COM_COLLISION
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stddef.h>
#include <ctype.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sqlgetoption.h>

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
#include <sys/un.h>
#include <arpa/inet.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#else
#include "android_ifaddrs.h"
#define EPOLLRDHUP EPOLLHUP
#define EPOLL_CLOEXEC 0
#endif
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
	if( !globalNetworkData.flags.bOptionsRead ) {
#ifdef __LINUX__
		signal(SIGPIPE, SIG_IGN);
#endif
#ifndef __NO_OPTIONS__
		globalNetworkData.flags.bLogProtocols = SACK_GetProfileIntEx( "SACK", "Network/Log Protocols", 0, TRUE );
		globalNetworkData.flags.bShortLogReceivedData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Received Data(64 byte max)", 0, TRUE );
		globalNetworkData.flags.bLogReceivedData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Received Data", 0, TRUE );
		globalNetworkData.flags.bLogSentData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Sent Data", globalNetworkData.flags.bLogReceivedData, TRUE );
#  ifdef LOG_NOTICES
		globalNetworkData.flags.bLogNotices = SACK_GetProfileIntEx( "SACK", "Network/Log Network Notifications", 0, TRUE );
#  endif
		globalNetworkData.dwReadTimeout = SACK_GetProfileIntEx( "SACK", "Network/Read wait timeout", 5000, TRUE );
		globalNetworkData.dwConnectTimeout = SACK_GetProfileIntEx( "SACK", "Network/Connect timeout", 10000, TRUE );
#else
		globalNetworkData.flags.bLogNotices = 0;
		globalNetworkData.dwReadTimeout = 5000;
		globalNetworkData.dwConnectTimeout = 10000;
#endif
		globalNetworkData.flags.bOptionsRead = 1;
	}
}

static void LowLevelNetworkInit( void )
{
	if( !global_network_data ) {
		SimpleRegisterAndCreateGlobal( global_network_data );
	}
	if( !globalNetworkData.ClientSlabs )
		InitializeCriticalSec( &globalNetworkData.csNetwork );
}

PRIORITY_PRELOAD( InitNetworkGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY - 1 )
{
	LowLevelNetworkInit();
	if( !globalNetworkData.system_name )
	{
		globalNetworkData.system_name = "no.network";
	}
}

//----------------------------------------------------------------------------
// forward declaration for the window proc...
_TCP_NAMESPACE
void AcceptClient(PCLIENT pc);
int TCPWriteEx(PCLIENT pc DBG_PASS);
#define TCPWrite(pc) TCPWriteEx(pc DBG_SRC)

int FinishPendingRead(PCLIENT lpClient DBG_PASS );
LOGICAL TCPDrainRead( PCLIENT pClient );

_TCP_NAMESPACE_END

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

#ifdef LOG_CLIENT_LISTS
void DumpLists( void )
{
	int c = 0;
	PCLIENT pc;
	for( pc = globalNetworkData.AvailableClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( "Available %p", pc );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow available clients." );
		//DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ActiveClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( WIDE( "Active %p(%d)" ), pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow active clients." );
		DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ClosedClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( "Closed %p(%d)", pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow closed clients." );
		DebugBreak();
	}
}
#endif

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

PCLIENT GrabClientEx( PCLIENT pClient DBG_PASS )
#define GrabClient(pc) GrabClientEx( pc DBG_SRC )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		_lprintf(DBG_RELAY)( "grabbed client %p(%d)", pClient, pClient->Socket );
		lprintf( "grabbed client %p Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))"
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

static PCLIENT AddAvailable( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Avail client %p(%d)", pClient, pClient->Socket );
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
// used externally by udp/tcp
PCLIENT AddActive( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Active client %p(%d)", pClient, pClient->Socket );
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

LOGICAL sack_network_is_active( PCLIENT pc ) {
	if( pc && ( pc->dwFlags & ( CF_ACTIVE )) && !( pc->dwFlags & (CF_CLOSED)) ) return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------

static PCLIENT AddClosed( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Closed client %p(%d)", pClient, pClient->Socket );
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

static void ClearClient( PCLIENT pc DBG_PASS )
{
	uintptr_t* pbtemp;
	PCLIENT next;
	PCLIENT *me;
	CRITICALSECTION csr;
	CRITICALSECTION csw;
	// keep the closing flag until it's really been closed. (getfreeclient will try to nab it)
	enum NetworkConnectionFlags  dwFlags = pc->dwFlags & (CF_STATEFLAGS | CF_CLOSING | CF_CONNECT_WAITING | CF_CONNECT_CLOSED);
#ifdef VERBOSE_DEBUG
	lprintf( "CLEAR CLIENT!" );
#endif
	me = pc->me;
	next = pc->next;
	pbtemp = pc->lpUserData;
	csr = pc->csLockRead;
	csw = pc->csLockWrite;

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
		_lprintf(DBG_RELAY)( "Clear Client %p  %08x   %08x", pc, pc->dwFlags, dwFlags );
#endif
	MemSet( pc, 0, sizeof( CLIENT ) ); // clear all information...

	// Socket is now 0; which for linux is a valid handle... which is what I get for events...
	pc->csLockRead = csr;
	pc->csLockWrite = csw;
	pc->lpUserData = pbtemp;
	if( pc->lpUserData )
		MemSet( pc->lpUserData, 0, globalNetworkData.nUserData * sizeof( uintptr_t ) );
	pc->next = next;
	pc->me = me;
	pc->dwFlags = dwFlags;
}

//----------------------------------------------------------------------------

static void NetworkGlobalLock( DBG_VOIDPASS ) {
	LOGICAL locked = FALSE;
	do {
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( TryEnterCriticalSection( &globalNetworkData.csNetwork ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
#ifdef LOG_NETWORK_LOCKING
			_lprintf( DBG_RELAY )("Failed enter global? %lld", globalNetworkData.csNetwork.dwThreadID );
#endif
			Relinquish();
		}
		else
			locked = TRUE;
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )("Got global lock");
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

// this removes the client from lists, and thread scheduleing.

void TerminateClosedClientEx( PCLIENT pc DBG_PASS )
{
#ifdef VERBOSE_DEBUG
	_lprintf(DBG_RELAY)( "terminate client %p ", pc );
#endif
	if( !pc )
		return;
	if( pc->dwFlags & CF_TOCLOSE ) {
#ifdef VERBOSE_DEBUG
		lprintf( "WAIT FOR CLOSE LATER... %x", pc->dwFlags );
#endif
		//return;
	}
	if( pc->dwFlags & CF_CLOSED )
	{
		PendingBuffer * lpNext;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		RemoveThreadEvent( pc );
#ifdef VERBOSE_DEBUG
		lprintf( "REMOVED EVENT...." );
#endif

		//lprintf( "Terminating closed client..." );
		if( IsValid( pc->Socket ) )
		{
#ifdef VERBOSE_DEBUG
			lprintf( "close soekcet." );
#endif
#if !defined( SHUT_WR ) && defined( _WIN32 )
#  define SHUT_WR SD_SEND
#endif
			shutdown( pc->Socket, SHUT_WR );
#if defined( _WIN32 )
#undef SHUT_WR
#endif
			//lprintf( "Win32:ShutdownWR+closesocket %p", pc );
			closesocket( pc->Socket );
			while( pc->lpFirstPending )
			{
				lpNext = pc->lpFirstPending -> lpNext;

				if( pc->lpFirstPending->s.bDynBuffer )
					Deallocate( POINTER, pc->lpFirstPending->buffer.p );

				if( pc->lpFirstPending != &pc->FirstWritePending )
				{
#ifdef LOG_PENDING
					lprintf( "Data queued...Deleting in remove." );
#endif
					Deallocate( PendingBuffer*, pc->lpFirstPending);
				}
				else
				{
#ifdef LOG_PENDING
					lprintf( "Normal send queued...Deleting in remove." );
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
#ifdef LOG_PENDING
	else
		lprintf( "Client's state was not CLOSED..." );
#endif
}

//----------------------------------------------------------------------------


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
			lprintf( "Clever OpenSocket failed... fallback... and survey sez..." );
			//--------------------
			sockMaster = socket( AF_INET, SOCK_DGRAM, 0);
			//--------------------
		}
		//--------------------

		if( sockMaster == INVALID_SOCKET )
		{
			nError = WSAGetLastError();

			lprintf( "Failed to create a socket - error is %ld", WSAGetLastError() );
			if( nError == 10106 ) // provvider init fail )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				return -2;
			}
			if( nError == WSAENETDOWN )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				//else return NetworkPause("Socket is delaying...");
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
				//else return NetworkPause("Bind is Delaying");
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
		lprintf( "Task is shutting down network... don't handle event." );
		return;
	}
	if( !pClient )
	{
		lprintf( "How did a NULL client get here?!" );
		return;
	}

	pClient->dwFlags |= CF_PROCESSING;
#ifdef LOG_NETWORK_EVENT_THREAD
	//if( globalNetworkData.flags.bLogNotices )
	//	lprintf( "Client event on %p", pClient );
#endif
	if( WSAEnumNetworkEvents( pClient->Socket, pClient->event, &networkEvents ) == ERROR_SUCCESS )
	{
		if( networkEvents.lNetworkEvents == 0 ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "zero events...%p %p %p", pClient, pClient->Socket, pClient->event );
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
						lprintf( "Got UDP FD_READ" );
#endif
					FinishUDPRead( pClient, 0 );
				}
			}
			else
			{
				THREAD_ID prior = 0;
#ifdef LOG_CLIENT_LISTS
				lprintf( "client lists Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))"
						 , &globalNetworkData.ActiveClients, globalNetworkData.ActiveClients, globalNetworkData.ActiveClients?globalNetworkData.ActiveClients->Socket:0
						 , &globalNetworkData.AvailableClients, globalNetworkData.AvailableClients, globalNetworkData.AvailableClients?globalNetworkData.AvailableClients->Socket:0
						 , &globalNetworkData.ClosedClients, globalNetworkData.ClosedClients, globalNetworkData.ClosedClients?globalNetworkData.ClosedClients->Socket:0
						 );
#endif

#ifdef LOG_NETWORK_LOCKING
				lprintf( "Handle Event left global" );
#endif
				// if an unknown socket issued a
				// notification - close it - unknown handling of unknown socket.
#ifdef LOG_NETWORK_EVENT_THREAD
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "events : %08x on %p", networkEvents.lNetworkEvents, pClient );
#endif
				if( networkEvents.lNetworkEvents & FD_CONNECT )
				{
					{
						uint16_t wError = networkEvents.iErrorCode[FD_CONNECT_BIT];
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "FD_CONNECT on %p", pClient );
#endif
						if( !wError )
							pClient->dwFlags |= CF_CONNECTED;
						else
						{
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connect error: %d", wError );
#endif
							pClient->dwFlags |= CF_CONNECTERROR;
						}
						if( !( pClient->dwFlags & CF_CONNECTERROR ) )
						{
							// since this is done before connecting is clear, tcpwrite
							// may make notice of previously queued data to
							// connection opening...
							//lprintf( "Sending any previously queued data." );

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
									lprintf( "getsockname errno = %d", errno );
								}
							}
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Post connect to application %p  error:%d", pClient, wError );
#endif
							if( pClient->dwFlags & CF_CPPCONNECT )
								pClient->connect.CPPThisConnected( pClient->psvConnect, wError );
							else
								pClient->connect.ThisConnected( pClient, wError );
						}
						//lprintf( "Returned from application connected callback" );
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
							lprintf( "FD_CONNECT Completed" );
#endif
						//lprintf( "Returned from application inital read complete." );
					}
				}
				if( networkEvents.lNetworkEvents & FD_READ )
				{
					PCLIENT pcLock;
					while( !( pcLock = NetworkLockEx( pClient, 1 DBG_SRC ) ) ) {
						// done with events; inactive sockets can't have events
						if( !( pClient->dwFlags & CF_ACTIVE ) ) {
							pcLock = NULL;
							break;
						}
						Relinquish();
					}
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "FD_READ" );
#endif
  					if( ( pClient->dwFlags & CF_ACTIVE ) ) {
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
								//lprintf( "Pending read failed - and wants to close." );
								//InternalRemoveClientEx( pc, TRUE, FALSE );
							}
						}
						NetworkUnlock( pClient, 1 );
					}
				}
				if( networkEvents.lNetworkEvents & FD_WRITE )
				{
					PCLIENT pcLock;
					while( !( pcLock = NetworkLockEx( pClient, 0 DBG_SRC ) ) ) {
						// done with events; inactive sockets can't have events
						if( !( pClient->dwFlags & CF_ACTIVE ) ) {
							pcLock = NULL;
							break;
						}
						Relinquish();
					}
					if( pClient->dwFlags & CF_ACTIVE ) {
#ifdef LOG_NOTICES
						//if( globalNetworkData.flags.bLogNotices )
						//	lprintf( "FD_Write" );
#endif
						// returns true while it wrote or there is data to write
						if( pClient->lpFirstPending )
							TCPWrite(pClient);
						if( !pClient->lpFirstPending ) {
							if( pClient->dwFlags & CF_TOCLOSE )
							{
								pClient->dwFlags &= ~CF_TOCLOSE;
								//lprintf( "Pending read failed - and wants to close." );
								EnterCriticalSec( &globalNetworkData.csNetwork );
								InternalRemoveClientEx( pClient, FALSE, TRUE );
								TerminateClosedClient( pClient );
								LeaveCriticalSec( &globalNetworkData.csNetwork );
							}
						}
						NetworkUnlock( pClient, 0 );
					}
				}

				if( networkEvents.lNetworkEvents & FD_CLOSE )
				{
					//lprintf( "FD_CLOSE %p", pClient );
					if( !pClient->bDraining )
					{
						size_t bytes_read;
						// act of reading can result in a close...
						// there are things like IE which close and send
						// adn we might get the close notice at application level indicating there might still be data...
						//lprintf( "closed, try pending read %p", pClient );
						while( ( bytes_read = FinishPendingRead( pClient DBG_SRC) ) > 0
							&& bytes_read != (size_t)-1 ) // try and read...
						//if( pClient->dwFlags & CF_TOCLOSE )
						{
							//lprintf( "Pending read failed - reset connection. (well this is FD_CLOSE so yeah...???)" );
							//InternalRemoveClientEx( pc, TRUE, FALSE );
						}
					}
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf("FD_CLOSE... %p  %08x", pClient, pClient->dwFlags );
#endif
					if( pClient->dwFlags & CF_ACTIVE )
					{
						// might already be cleared and gone..
						EnterCriticalSec( &globalNetworkData.csNetwork );
						InternalRemoveClientEx( pClient, FALSE, TRUE );
						TerminateClosedClient( pClient );
						LeaveCriticalSec( &globalNetworkData.csNetwork );
					}
					// section will be blank after termination...(correction, we keep the section state now)
					pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
				}
				if( networkEvents.lNetworkEvents & FD_ACCEPT )
				{
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "FD_ACCEPT on %p", pClient );
#endif
					AcceptClient(pClient);
					//NetworkUnlock( pClient, 1 );
				}
				//lprintf( "leaveing event handler..." );
				//lprintf( "Left event handler CS." );
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
			lprintf( "Event enum failed... do what? close socket? %p %" _32f, pClient, dwError );
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

void SetNetworkErrorCallback( PCLIENT pc, cErrorCallback callback, uintptr_t psvUser ) {
	if( pc ) {
		pc->errorCallback = callback;
		pc->psvErrorCallback = psvUser;
	}
}

//----------------------------------------------------------------------------

void TriggerNetworkErrorCallback( PCLIENT pc, enum SackNetworkErrorIdentifier error ) {
	if( pc && pc->errorCallback )
		pc->errorCallback( pc->psvErrorCallback, pc, error );
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
	thread->counting = TRUE;
	thread->nEvents = 1;
	if( thread->thread != MakeThread() && !thread->flags.bProcessing )
		while( thread->nEvents != thread->nWaitEvents ) {
			if( !thread->flags.bProcessing )
			{
				// have to make sure threads reset to the new list.
				//lprintf( "have to wait for thread to be in wait state..." );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				while( (thread->nWaitEvents > 1) || thread->flags.bProcessing ) {
					WSASetEvent( thread->hThread );
					Relinquish();
				}
				EnterCriticalSec( &globalNetworkData.csNetwork );
				EnterCriticalSec( &globalNetworkData.csNetwork );
			}
			else
				break;  // if it's processing, race it; build new list, count it.
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
		thread->nEvents = (int)c;
		thread->counting = FALSE;

		DeleteListEx( &thread->monitor_list DBG_SRC );
		thread->monitor_list = newList;
		pc->this_thread = NULL;
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
		AddLink( (PLIST*)&globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)peer ) );
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
	lprintf( "peer %p now has %d events", peer, peer->nEvents );
#endif
	if( !peer->flags.bProcessing && peer->parent_peer ) // scheduler thread already awake do not wake him.
		WSASetEvent( peer->hThread );
}

// this is passed '0' when it is called internally
// this is passed '1' when it is called by idleproc
int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check )
{
	//lprintf( "Check messages." );
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
					//lprintf( "Remove thread event on closed thread (should be terminate here..)" );
					// also does the remove.
					TerminateClosedClient( pc );
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
						lprintf( " Found closed? %p", pc );
						continue;
					}
#ifdef LOG_NETWORK_EVENT_THREAD
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "Added to schedule : %p %08x", pc, pc->dwFlags );
#endif
					AddThreadEvent( pc, 0 );
				}
				else
					lprintf( "Client in schedule queue, but it is already schedule?! %p", pc );
			}
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( "Process Network left global" );
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
		//	lprintf( "%p Waiting on %d events", thread, thread->nEvents );
#endif
		thread->nWaitEvents = thread->nEvents;
		thread->flags.bProcessing = 0;
		while( thread->counting ) { thread->nWaitEvents = thread->nEvents; Relinquish(); }
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
		//	lprintf( "Event Wait Result was %d", result );
#endif
		// this should never be 0, but we're awake, not sleeping, and should say we're in a place
		// where we probably do want to be woken on a 0 event.
		if( result != WSA_WAIT_EVENT_0 )
		{
#ifdef LOG_NETWORK_EVENT_THREAD
			//if( globalNetworkData.flags.bLogNotices )
			//	lprintf( "Begin - thread processing %d", result );
#endif
			thread->flags.bProcessing = 1;
		}
		thread->nWaitEvents = 0;
		if( result == WSA_WAIT_FAILED )
		{
			DWORD dwError = WSAGetLastError();
			if( dwError == WSA_INVALID_HANDLE )
			{
				lprintf( "Rebuild list, have a bad event handle somehow." );
				break;
			}
			lprintf( "error of wait is %d   %p", dwError, thread );
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
				//if( pcLock ) {
					if( !pc || ( pc->dwFlags & CF_AVAILABLE ) ) {
						//lprintf( "thread event happened on a now available client." );
					}
					else
						HandleEvent( pc );
				//}
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
					lprintf( thread->parent_peer?"RESET THREAD EVENT":"RESET GLOBAL EVENT" );
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
	if( !thread ) {
	 	lprintf( "didn't have one? %p", pc );
		return;
	}
	{
#  ifdef __EMSCRIPTEN__
#  else
#    ifdef __MAC__
#      ifdef __64__
		struct kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		} else {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET64( &ev, pc->SocketBroadcast, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#      else
		struct kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		} else {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET( &ev, pc->SocketBroadcsat, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		}
#      endif
#    else
		int r;
#      ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "Removing event %p   %d from poll %d", pc, pc->Socket, thread->epoll_fd );
#      endif
		//r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DISABLE, pc->Socket, NULL );
		//if( r < 0 ) lprintf( "Error removing:%d", errno );
		
		r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->Socket, NULL );
		if( r < 0 ) lprintf( "Error removing:%d", errno );
		if( pc->SocketBroadcast ) {
			r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->SocketBroadcast, NULL );
			if( r < 0 ) lprintf( "Error removing:%d", errno );
		}
		pc->flags.bAddedToEvents = 0;
		pc->this_thread = NULL;
#    endif
#  endif // __EMSCRIPTEN__
	}
	LockedDecrement( &thread->nEvents );
#    ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "peer %p now has %d events", thread, thread->nEvents );
#    endif
	// don't bubble sort root thread
	if( thread->parent_peer )
		while( (thread->nEvents < thread->parent_peer->nEvents) && thread->parent_peer->parent_peer ) {
#    ifdef LOG_NETWORK_EVENT_THREAD
			//lprintf( "swapping this with parent peer ... this events %d  parent %d", thread->nEvents, thread->parent_peer->nEvents );
#    endif
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
	if( pc->Socket <= 0 ) {
		lprintf( "SAVED FROM A FATAL INFINITE EVENT LOOP");
		return;
	}
#ifdef LOG_NOTICES
	//if( globalNetworkData.flags.bLogNotices )
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
			AddLink( (PLIST*)&globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)peer ) );
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
#ifdef __EMSCRIPTEN__
#else

	{
#  ifdef __MAC__
		struct event_data *data = New( struct event_data );
		data->pc = pc;
		data->broadcast = broadcast;
#    ifdef __64__
		struct kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		else {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#    else
		struct kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
		else {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (uintptr_t)data );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD|EV_ENABLE|EV_CLEAR, 0, 0, (uintptr_t)data );
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
#    ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "peer add socket %d to %d now has 0x%x events", pc->Socket, peer->epoll_fd, ev.events );
#    endif
		r = epoll_ctl( peer->epoll_fd, EPOLL_CTL_ADD, broadcast?pc->SocketBroadcast:pc->Socket, &ev );
		if( r < 0 ) lprintf( "Error adding:%d %d", errno, broadcast?pc->SocketBroadcast:pc->Socket );
#  endif
	}
#endif // __EMSCRIPTEN__
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "added thread: %p  %p  %p  ", pc, pc->this_thread, peer );
#endif
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
#ifdef __EMSCRIPTEN__
		lprintf( "Need Old Select Code Here" );
		return 1;
#else
	{
#  ifdef __MAC__
#    ifdef __64__
		struct kevent64_s events[10];
		cnt = kevent64( thread->kqueue, NULL, 0, events, 10, 0, NULL );
#    else
		kevent events[10];
		cnt = kevent( thread->kqueue, NULL, 0, events, 10, NULL );
#    endif
#  else
		struct epoll_event events[10];
#    ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "Wait on %d   %d", thread->epoll_fd, thread->nEvents );
#    endif
		cnt = epoll_wait( thread->epoll_fd, events, 10, -1 );
#  endif
		if( cnt < 0 )
		{
			int err = errno;
			if( err == EINTR )
				return 1;
			Log1( "Sorry epoll_pwait/kevent call failed... %d", err );
			return 1;
		}
		if( cnt > 0 )
		{
			int closed;
			int n;
			struct event_data *event_data;
			THREAD_ID prior = 0;
			PCLIENT next;
#  ifdef LOG_NETWORK_EVENT_THREAD
			lprintf( "process %d events", cnt );
#  endif
			for( n = 0; n < cnt; n++ ) {
				closed = 0;
#  ifdef __MAC__
				event_data = (struct event_data*)events[n].udata;
#  ifdef LOG_NOTICES
				lprintf( "Process %d %x %x"
				       , ((uintptr_t)event_data == 1) ?0:event_data->broadcast?event_data->pc->SocketBroadcast:event_data->pc->Socket
				       , ((uintptr_t)event_data == 1) ?0:event_data->pc->dwFlags
				       , events[n].filter );
#  endif
#  else
				event_data = (struct event_data*)events[n].data.ptr;
#  ifdef LOG_NOTICES
				if( event_data != (struct event_data*)1 ) {
					lprintf( "Process %d %x", event_data->broadcast?event_data->pc->SocketBroadcast:event_data->pc->Socket
							 , events[n].events );
				}
#  endif
#  endif
				if( event_data == (struct event_data*)1 ) {
					//char buf;
					//int stat;
					//stat = read( GetThreadSleeper( thread->thread ), &buf, 1 );
					//call wakeable sleep to just clear the sleep; because this is an event on the sleep pipe.
					//WakeableSleep( 1 );
					//lprintf( "This should sleep forever..." );
					return 1;
				}

#  ifdef __MAC__
				if( events[n].filter == EVFILT_READ )
#  else
				if( events[n].events & EPOLLIN )
#  endif
				{
					int locked;
					locked = 1;

					while( !NetworkLock( event_data->pc, 1 ) ) {
						if( !( event_data->pc->dwFlags & CF_ACTIVE ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
							lprintf( "failed lock dwFlags : %8x", event_data->pc->dwFlags );
#  endif
							locked = 0;
							break;
						}
						if( event_data->pc->dwFlags & CF_AVAILABLE ) {
							locked = 0;
							break;
						}
						Relinquish();
					}
					if( !( event_data->pc->dwFlags & ( CF_ACTIVE | CF_CLOSED ) ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
						lprintf( "not active but locked? dwFlags : %8x", event_data->pc->dwFlags );
#  endif
						continue;
					}
					if( event_data->pc->dwFlags & CF_AVAILABLE )
						continue;

					if( !IsValid( event_data->pc->Socket ) ) {
						NetworkUnlock( event_data->pc, 1 );
						continue;
					}

#  ifdef LOG_NETWORK_EVENT_THREAD
					lprintf( "EPOLLIN/EVFILT_READ %x", event_data->pc->dwFlags );
#  endif
					if( event_data->pc->dwFlags & CF_CLOSED ) {
						PCLIENT pClient = event_data->pc;
						// close notice went to application; all resources for application are gone.
						// any pending reads are no longre valid.

						//lprintf( "socket is already closed... what do we need to do?");
						//WakeableSleep( 100 );
						if( 0 && !pClient->bDraining )
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
#  ifdef LOG_NOTICES
						//if( globalNetworkData.flags.bLogNotices )
							lprintf("FD_CLOSE... %p  %08x", pClient, pClient->dwFlags );
#  endif
						//if( pClient->dwFlags & CF_ACTIVE )
						{
							// might already be cleared and gone..
							//InternalRemoveClientEx( pClient, FALSE, TRUE );
							TerminateClosedClient( pClient );
							closed = 1;
						}
						// section will be blank after termination...(correction, we keep the section state now)
						pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
					} else if( !(event_data->pc->dwFlags & (CF_ACTIVE) ) ) {
						lprintf( "Event on socket no longer active..." );
						// change to inactive status by the time we got here...
					} else if( event_data->pc->dwFlags & CF_LISTEN )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "accepting..." );
#endif
						AcceptClient( event_data->pc );
					}
					else if( event_data->pc->dwFlags & CF_UDP )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "UDP Read Event..." );
#endif
						//lprintf( "UDP READ" );
						FinishUDPRead( event_data->pc, event_data->broadcast );
					}
					else if( event_data->pc->bDraining )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Drain Event..." );
#endif
						TCPDrainRead( event_data->pc );
					}
					else if( event_data->pc->dwFlags & CF_READPENDING )
					{
						size_t read;
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Read Event..." );
#endif
						// packet oriented things may probably be reading only
						// partial messages at a time...
						read = FinishPendingRead( event_data->pc DBG_SRC );
						//lprintf( "Read %d", read );
						if( ( read == -1 ) && ( event_data->pc->dwFlags & CF_TOCLOSE ) )
						{
#ifdef LOG_NOTICES
							//if( globalNetworkData.flags.bLogNotices )
							lprintf( "Pending read failed - reset connection." );
#endif
							EnterCriticalSec( &globalNetworkData.csNetwork );
							InternalRemoveClientEx( event_data->pc, FALSE, FALSE );
							TerminateClosedClient( event_data->pc );
							LeaveCriticalSec( &globalNetworkData.csNetwork );
							closed = 1;
						}
						else if( !event_data->pc->RecvPending.s.bStream )
							event_data->pc->dwFlags |= CF_READREADY;
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Set read ready..." );
#endif
						event_data->pc->dwFlags |= CF_READREADY;
					}
					if( locked )
						LeaveCriticalSec( &event_data->pc->csLockRead );
				}

				if( !closed && ( event_data->pc->dwFlags & CF_ACTIVE ) ) {
					int locked;
					locked = 1;
#  ifdef __MAC__
					if( events[n].filter == EVFILT_WRITE )
#  else
					if( events[n].events & EPOLLOUT )
#  endif
					{
#  ifdef LOG_NETWORK_EVENT_THREAD
						lprintf( "EPOLLOUT %s", ( event_data->pc->dwFlags & CF_CONNECTING ) ? "connecting"
							: ( !( event_data->pc->dwFlags & CF_ACTIVE ) ) ? "closed" : "writing" );
#  endif

						while( !NetworkLock( event_data->pc, 0 ) ) {
							if( !( event_data->pc->dwFlags & CF_WRITEISPENDED ) ) {
								locked = 0;
								break;
							}
							if( !( event_data->pc->dwFlags & CF_ACTIVE ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
								lprintf( "failed lock dwFlags : %8x", event_data->pc->dwFlags );
#  endif
								locked = 0;
								break;
							}
							if( event_data->pc->dwFlags & CF_AVAILABLE ) {
								locked = 0;
								break;
							}
							Relinquish();
						}
						if( !( event_data->pc->dwFlags & ( CF_ACTIVE | CF_CLOSED ) ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
							lprintf( "not active but locked? dwFlags : %8x", event_data->pc->dwFlags );
#  endif
							continue;
						}
						if( event_data->pc->dwFlags & CF_AVAILABLE )
							continue;

						if( !IsValid( event_data->pc->Socket ) ) {
							NetworkUnlock( event_data->pc, 0 );
							continue;
						}
						if( !( event_data->pc->dwFlags & CF_ACTIVE ) ) {
							//lprintf( "FLAGS IS NOT ACTIVE BUT: %x", event_data->pc->dwFlags );
							// change to inactive status by the time we got here...
						} else if( event_data->pc->dwFlags & CF_CONNECTING ) {
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connected!" );
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
								if( getsockname( pc->Socket, pc->saSource, &nLen ) ) {
									lprintf( "getsockname errno = %d", errno );
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
								//lprintf( "Error checking for connect is: %s on %d", strerror( error ), event_data->pc->Socket );
								if( event_data->pc->pWaiting ) {
#ifdef LOG_NOTICES
									if( globalNetworkData.flags.bLogNotices )
										lprintf( "Got connect event, waking waiter.." );
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
								if( !error ) {
#ifdef LOG_NOTICES
									lprintf( "Read Complete" );
#endif
									if( event_data->pc->read.ReadComplete ) {
#ifdef LOG_NOTICES
										lprintf( "Initial Read Complete" );
#endif
										event_data->pc->read.ReadComplete( event_data->pc, NULL, 0 );
									}
									if( event_data->pc->lpFirstPending ) {
										lprintf( "Data was pending on a connecting socket, try sending it now" );
										TCPWrite( event_data->pc );
									}
								} else {
									event_data->pc->dwFlags |= CF_CONNECTERROR;
								}
							}
						} else if( event_data->pc->dwFlags & CF_UDP ) {
							//lprintf( "UDP WRITE IS NEVER QUEUED." );
							// udp write event complete....
							// do we ever care? probably sometime...
						} else {
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "TCP Write Event..." );
#endif
							// if write did not get locked, a write is in progress
							// wait until it finished there?
							// did this wake up because that wrote?
							if( locked ) {
								event_data->pc->dwFlags &= ~CF_WRITEISPENDED;
								TCPWrite( event_data->pc );
							}
						}
						if( locked )
							NetworkUnlock( event_data->pc, 0 );
					}
				} else {
					//lprintf( "Already closed? Stop looping on this event? %p %d %x", event_data->pc, event_data->pc->Socket, event_data->pc->dwFlags );
				}
			}
			// had some event  - return 1 to continue working...
		}
		return 1;
	}
#endif
	//lprintf( "Attempting to wake thread (send sighup!)" );
	//WakeThread( globalNetworkData.pThread );
	//lprintf( "Only reason should get here is if it's not this thread..." );
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
#ifdef _WIN32
		globalNetworkData.uNetworkPauseTimer = AddTimerEx( 1, 1000, NetworkPauseTimer, 0 );
		if( !globalNetworkData.client_schedule )
			globalNetworkData.client_schedule = CreateLinkQueue();
#endif
#ifdef __LINUX__
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = TRUE;
#endif
	}
	memset( &this_thread, 0, sizeof( this_thread ) );

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

#ifdef __EMSCRIPTEN__
#else
#  ifdef __LINUX__
#    ifdef __MAC__
	this_thread.kqueue = kqueue();
#    else
#      ifdef __ANDROID__
	this_thread.epoll_fd = epoll_create( 128 ); // close on exec (no inherit)
#      else
	this_thread.epoll_fd = epoll_create1( EPOLL_CLOEXEC ); // close on exec (no inherit)
#      endif
#    endif
	{
#    ifdef __MAC__
#      ifdef __64__
		struct kevent64_s ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
#        ifdef USE_PIPE_SEMS
		EV_SET64( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uint64_t)1, NULL, NULL );
#        endif
		kevent64( this_thread.kqueue, &ev, 1, 0, 0, 0, 0 );
#      else
		struct kevent ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
#        ifdef USE_PIPE_SEMS
		EV_SET( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)1 );
#        endif
		kevent( this_thread.kqueue, &ev, 1, 0, 0, 0 );
#      endif
#    else
		//struct epoll_event ev;
		//ev.data.ptr = (void*)1;
		//ev.events = EPOLLIN;
		//epoll_ctl( this_thread.epoll_fd, EPOLL_CTL_ADD, GetThreadSleeper( thread ), &ev );
#    endif
	}
#  endif
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

	xlprintf( 2100 )("Enter global network on shutdown... (thread exiting)");

	EnterCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( "NetworkThread(exit) in global" );
#  endif
	if( !this_thread.parent_peer )
	{
		if( ( globalNetworkData.root_thread = this_thread.child_peer ) )
			this_thread.child_peer->parent_peer = NULL;
	}
	else
	{
		if( ( this_thread.parent_peer->child_peer = this_thread.child_peer ) )
			this_thread.child_peer->parent_peer = this_thread.parent_peer;
	}
	// this used to be done in the WM_DESTROY
	DeleteLink( (PLIST*)&globalNetworkData.pThreads, thread );

	globalNetworkData.flags.bThreadExit = TRUE;

	xlprintf( 2100 )("Shut down network thread.");

	globalNetworkData.flags.bThreadInitComplete = FALSE;
	globalNetworkData.flags.bNetworkReady = FALSE;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( "NetworkThread(exit) left global" );
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

#if 0
	if( globalNetworkData.uPendingTimer )
	{
		RemoveTimer( globalNetworkData.uPendingTimer );
		globalNetworkData.uPendingTimer = 0;
	}
#endif
	//while( globalNetworkData.ActiveClients )
	{
#ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( "NetworkQuit - Remove active client %p", globalNetworkData.ActiveClients );
#endif
		//InternalRemoveClientEx( globalNetworkData.ActiveClients, TRUE, FALSE );
	}
	globalNetworkData.bQuit = TRUE;
	{
		PTHREAD thread;
		INDEX idx;
#ifdef USE_WSA_EVENTS
		PLIST wakeEvents = NULL;
		struct peer_thread_info *peer_thread;
		WSAEVENT hThread;
		peer_thread = globalNetworkData.root_thread;
		globalNetworkData.root_thread = NULL;
		for( ; peer_thread; peer_thread = peer_thread->child_peer ) {
			AddLink( &wakeEvents, peer_thread->hThread );
		}
		LIST_FORALL( wakeEvents, idx, WSAEVENT, hThread )
			WSASetEvent( hThread );
#endif
		LIST_FORALL( globalNetworkData.pThreads, idx, PTHREAD, thread ) {
			WakeThread( thread );
		}
	}
#ifdef _WIN32
#  ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "SET GLOBAL EVENT (trigger quit)" );
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
			lprintf( "Post SOCKMSG_CLOSE" );
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
			lprintf( "Network was locked up?  Failed to allow network to exit in half a second (500ms)" );
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
		//Log1( "Creating %d Client Resources", MAX_NETCLIENTS );
		pClientSlab = NewPlus( CLIENT_SLAB, (MAX_NETCLIENTS - 1 )* sizeof( CLIENT ) );
		pClientSlab->pUserData = NewArray( uintptr_t, MAX_NETCLIENTS * globalNetworkData.nUserData );
		MemSet( pClientSlab->client, 0, (MAX_NETCLIENTS) * sizeof( CLIENT ) ); // can't clear the lpUserData Address!!!
		MemSet( pClientSlab->pUserData, 0, (MAX_NETCLIENTS) * globalNetworkData.nUserData * sizeof( uintptr_t ) );
		pClientSlab->count = MAX_NETCLIENTS;

		for( n = 0; n < pClientSlab->count; n++ )
		{
			pClientSlab->client[n].Socket = INVALID_SOCKET; // unused sockets on all clients.
			pClientSlab->client[n].lpUserData = pClientSlab->pUserData + (n * globalNetworkData.nUserData);
			InitializeCriticalSec( &pClientSlab->client[n].csLockRead );
			InitializeCriticalSec( &pClientSlab->client[n].csLockWrite );
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
		//xlprintf(200)( "Threads already active..." );
		// might do something... might not...
		return TRUE; // network thread active, do not realloc
	}

	AddLink( (PLIST*)&globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)/*peer_thread==*/NULL ) );
	globalNetworkData.nPeers++;
	AddIdleProc( IdleProcessNetworkMessages, 1 );
	//lprintf( "Network Initialize...");
	//lprintf( "Create network thread." );
	while( !globalNetworkData.flags.bThreadInitComplete )
	{
		Relinquish();
	}
	if( !globalNetworkData.flags.bThreadInitOkay )
	{
		lprintf( "Abort network, init is NOT ok." );
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
	lprintf( "GetFreeNetworkClient in global" );
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

#ifdef USE_NATIVE_CRITICAL_SECTION
		d = EnterCriticalSecNoWait( &pClient->csLockRead, NULL );
#else
		d = EnterCriticalSecNoWaitEx( &pClient->csLockRead, NULL DBG_RELAY );
#endif
		if( d < 1 ) {
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			goto get_client;
		}

#ifdef USE_NATIVE_CRITICAL_SECTION
		d = EnterCriticalSecNoWait( &pClient->csLockWrite, NULL );
#else
		d = EnterCriticalSecNoWaitEx( &pClient->csLockWrite, NULL DBG_RELAY );
#endif
		if( d < 1 ) {
			LeaveCriticalSec( &pClient->csLockRead );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			goto get_client;
		}

		if( pClient->dwFlags & ( CF_STATEFLAGS & (~CF_AVAILABLE)) )
			DebugBreak();
		ClearClient( pClient DBG_SRC ); // clear client is redundant here... but saves the critical section now
		//Log1( "New network client %p", client );
	}
	else
	{
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( "GetFreeNetworkClient left global" );
#endif
		Relinquish();
		if( globalNetworkData.AvailableClients )
		{
			lprintf( "there were clients available... just in a closing state..." );
			goto get_client;
		}
		lprintf( "No unused network clients are available." );
		return NULL;
	}
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( "GetFreeNetworkClient left global" );
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


//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, NetworkLockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	if( lpClient )
	{
		//lpClient->dwFlags |= CF_WANTS_GLOBAL_LOCK;
		//_lprintf(DBG_RELAY)( "Lock %p", lpClient );
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( EnterCriticalSecNoWait( &globalNetworkData.csNetwork, NULL ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
			//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef LOG_NETWORK_LOCKING
			_lprintf(DBG_RELAY)( "Failed enter global? %llx", globalNetworkData.csNetwork.dwThreadID  );
#endif
			Relinquish();
			return NULL;
			//DebugBreak();
		}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Got global lock %p %d", lpClient, readWrite );
#endif

		//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( !EnterCriticalSecNoWait( (readWrite? &lpClient->csLockRead:&lpClient->csLockWrite), NULL ) )
#else
		if( EnterCriticalSecNoWaitEx( ( readWrite ?&lpClient->csLockRead : &lpClient->csLockWrite ), NULL DBG_RELAY ) < 1 )
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
			return NULL;
			//goto start_lock;
		}
		//EnterCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#else
		LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
		if( !(lpClient->dwFlags & (CF_ACTIVE|CF_CLOSED) ) )
		{
			// change to inactive status by the time we got here...
#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#else
			LeaveCriticalSecEx( readWrite?&lpClient->csLockRead:&lpClient->csLockWrite DBG_RELAY );
#endif
			_lprintf( DBG_RELAY )( "Failed lock" );
			lprintf( "%p  %08x %08x inactive, cannot lock.", lpClient, lpClient->dwFlags, CF_ACTIVE );
			// this client is not available for client use!
			return NULL;
		}
	}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Got private lock %p %d", lpClient, readWrite );
#endif
	return lpClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, NetworkUnlockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	//_lprintf(DBG_RELAY)( "Unlock %p", lpClient );
	// simple unlock.
	if( lpClient )
	{
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Leave private lock %p %d", lpClient, readWrite );
#endif
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#else
		LeaveCriticalSecEx( readWrite?&lpClient->csLockRead:&lpClient->csLockWrite DBG_RELAY );
#endif
	}
}

//----------------------------------------------------------------------------

void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef LOG_SOCKET_CREATION
	_lprintf( DBG_RELAY )( "InternalRemoveClient Removing this client %p (%d)", lpClient, lpClient->Socket );
#endif
	if( lpClient && IsValid(lpClient->Socket) )
	{
		if( !bLinger )
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( "Setting quick close?!" );
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
            //lprintf( "Set no linger" );
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_LINGER,
									(char*)&lingerSet, sizeof(lingerSet)) <0 )
				{
					lprintf( "error setting no linger in close." );
					//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
				}
			}
		}
		else {
			struct linger lingerSet;
			// linger ON causes delay on close... otherwise close returns immediately
			lingerSet.l_onoff = 1; // on , with no time = off.
			lingerSet.l_linger = 2;
			// set server to allow reuse of socket port
			if( setsockopt( lpClient->Socket, SOL_SOCKET, SO_LINGER,
				(char*)&lingerSet, sizeof( lingerSet ) ) <0 )
			{
				lprintf( "error setting 2 second linger in close." );
			}
		}

		if( !(lpClient->dwFlags & CF_ACTIVE) )
		{
			if( lpClient->dwFlags & CF_AVAILABLE )
			{
				lprintf( "Client was inactive?!?!?! removing from list and putting in available" );
				AddAvailable( GrabClient( lpClient ) );
			}
			// this is probably true, we've definatly already moved it from
			// active list to clsoed list.
			else if( !(lpClient->dwFlags & CF_CLOSED) )
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( "Client was NOT already closed?!?!" );
#endif
				AddClosed( GrabClient( lpClient ) );
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( "Client's state is CLOSED" );
#endif
			return;
		}
		if( lpClient->lpFirstPending || ( lpClient->dwFlags & CF_WRITEPENDING ) ) {
#ifdef LOG_DEBUG_CLOSING
			lprintf( "CLOSE WHILE WAITING FOR WRITE TO FINISH..." );
#endif
			//lpClient->dwFlags |= CF_TOCLOSE;
			//return;
			// continue on; otherwise the close event gets lost...
		}
		{
			int notLocked = TRUE;
			do {
				if( !NetworkLockEx( lpClient, 0 DBG_SRC ) )
				{
					if( !(lpClient->dwFlags & CF_ACTIVE ) ) // if it's already been closed
					{
						return;
					}
					Relinquish();
					continue;
				}
				if( !NetworkLockEx( lpClient, 1 DBG_SRC ) )
				{
					NetworkUnlock( lpClient, 0 );
					if( !(lpClient->dwFlags & CF_ACTIVE) )  // if it's already been closed
					{
						return;
					}
					Relinquish();
					continue;
				}
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				notLocked = FALSE;
				EnterCriticalSec( &globalNetworkData.csNetwork );
			} while( notLocked );
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
				lprintf( "Marked closing first, and dispatching callback?" );
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

					lpClient->close.CloseCallback = NULL;
				}
#ifdef LOG_DEBUG_CLOSING
				else
					lprintf( "no close callback!? (or duplicate close?)" );
#endif
				// leave the flag closing set... we'll use that later
				// to avoid the double-lock;
				//lpClient->dwFlags &= ~CF_CLOSING;
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( "socket was already ispatched callback?" );
#endif
		}
		else
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( "blocknotify on close..." );
#endif
		}
		EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_DEBUG_CLOSING
		lprintf( "Adding current client to closed clients." );
#endif
		AddClosed( GrabClient( lpClient ) );
#ifdef LOG_DEBUG_CLOSING
		lprintf( "Leaving client critical section" );
#endif
		//lprintf( "Leaving network critical section" );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( lpClient, 0 DBG_SRC );
		NetworkUnlockEx( lpClient, 1 DBG_SRC );
	}
#ifdef LOG_DEBUG_CLOSING
	else
	{
		lprintf( "No Client, or socket already closed?" );
	}
#endif
}

void RemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef _WIN32
#  define SHUT_WR SD_SEND
#endif
	if( !lpClient ) return;

	if( !( lpClient->dwFlags & CF_UDP ) 
		&& ( lpClient->dwFlags & ( CF_CONNECTED ) ) ) {
		shutdown( lpClient->Socket, SHUT_WR );
	} else

	{
		int n = 0;
		// UDP still needs to be done this way...
				//
		//lprintf( "This will end up resetting the socket?" );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientExx( lpClient, bBlockNotify, bLinger DBG_RELAY );
		if( NetworkLock( lpClient, 0 ) && ((n=1),NetworkLock( lpClient, 1 )) ) {
			TerminateClosedClient( lpClient );
			NetworkUnlock( lpClient, 0 );
			NetworkUnlock( lpClient, 1 );
		}
		else if( n ) {
			NetworkUnlock( lpClient, 0 );
			lpClient->dwFlags |= CF_TOCLOSE;
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
	}
}

#undef NetworkLock
#undef NetworkUnlock
NETWORK_PROC( PCLIENT, NetworkLock)( PCLIENT lpClient, int readWrite )
{
	return NetworkLockEx( lpClient, readWrite DBG_SRC );
}

NETWORK_PROC( void, NetworkUnlock)( PCLIENT lpClient, int readWrite )
{
	NetworkUnlockEx( lpClient, readWrite DBG_SRC );
}

SACK_NETWORK_NAMESPACE_END
