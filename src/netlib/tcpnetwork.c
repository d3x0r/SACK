// DEBUG FALGS in netstruc.h


//TODO: after the connect and just before the call to the connect callback fill in the PCLIENT's MAC addr field.

//#define DEBUG_SOCK_IO
#define LIBRARY_DEF
#include <stdhdrs.h>
#include <timers.h>
#include "netstruc.h"
#include <network.h>
#include <idle.h>
#ifndef UNDER_CE
#include <fcntl.h>
#endif
#ifdef __LINUX__
#include <unistd.h>
#ifdef __LINUX__
#  if !defined( __EMSCRIPTEN__ )
#    undef s_addr
#  endif
#  include <netinet/in.h> // IPPROTO_TCP
//#include <linux/in.h>  // IPPROTO_TCP
#  include <netinet/tcp.h> // TCP_NODELAY
//#include <linux/tcp.h> // TCP_NODELAY
#  include <fcntl.h>

#  else
#  endif

#  include <sys/ioctl.h>
#  include <signal.h> // SIGHUP defined

#  define NetWakeSignal SIGHUP

#else
#  define ioctl ioctlsocket

#endif

#include <sharemem.h>
#include <procreg.h>

//#define NO_LOGGING // force neverlog....
#include "logging.h"

#if !defined( _WIN32 )
#  ifndef MSG_NOSIGNAL
#    pragma message "Defining a default value for MSG_NOSIGNAL of 0"
#    define MSG_NOSIGNAL 0
#  endif
#else
// on windows define MSG_NOSIGNAL as 0
#  define MSG_NOSIGNAL 0
#endif

SACK_NETWORK_NAMESPACE
	extern int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check );

_TCP_NAMESPACE

//----------------------------------------------------------------------------
#if 0 && !DrainSupportDeprecated
LOGICAL TCPDrainRead( PCLIENT pClient );
#endif
//----------------------------------------------------------------------------

static inline void scheduleSocket( PCLIENT pc, struct peer_thread_info *this_thread ) {
#ifdef _WIN32
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "SET GLOBAL EVENT (wait for connect) %p %p", pc, pc->event );
	EnqueLink( &globalNetworkData.client_schedule, pc );
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
	// this_thread may be NULL, which won't be equal to root_thread?
	// the root thread is a wakeable object wait, which schedules other wsaasyncwait threads
	if( this_thread == globalNetworkData.root_thread ) {
		ProcessNetworkMessages( this_thread, 1 );
	}
#endif
#ifdef __LINUX__
	{
		//lprintf( "schedule socket %p %d", pc, pc->Socket );
		AddThreadEvent( pc, 0 );
	}
#endif

}


void SetNetworkListenerReady( PCLIENT pListen ) {
	if( pListen->pcOther ) pListen->pcOther->flags.bWaiting = 0;
	pListen->flags.bWaiting = 0;
}

void AcceptClient(PCLIENT pListen)
{
#ifdef __LINUX__
	socklen_t
#else
		int
#endif
		nTemp;
	PCLIENT pNewClient = NULL;// just to be safe.

	pNewClient = GetFreeNetworkClient();
	// new client will be locked...
	if( !pNewClient || !pListen || pListen->flags.bWaiting )
	{
		SOCKADDR *junk = AllocAddr();
		nTemp = MAGIC_SOCKADDR_LENGTH;
		lprintf("GetFreeNetwork() returned NULL. Exiting AcceptClient, accept and drop connection");
		closesocket( accept( pListen->Socket, junk ,&nTemp	));
		ReleaseAddress( junk );
		return;
	}

	// without setting this value - the pointer to the value
	// contains a value which may be less than a valid address
	// length... usually didn't JAB: 980203
	nTemp = MAGIC_SOCKADDR_LENGTH;
	pNewClient->saClient = AllocAddr();
	pNewClient->pcServer = pListen;
	pNewClient->flags.bSecure = pListen->flags.bSecure;
	pNewClient->flags.bAllowDowngrade = pListen->flags.bAllowDowngrade;
	pNewClient->Socket = accept( pListen->Socket
										, pNewClient->saClient
										,&nTemp
										);
	SET_SOCKADDR_LENGTH( pNewClient->saClient, pNewClient->saClient->sa_family == AF_INET6?IN6_SOCKADDR_LENGTH:IN_SOCKADDR_LENGTH );

	//lprintf( "Accept new client...%p %d", pNewClient, pNewClient->Socket );
#ifdef _WIN32
	SetHandleInformation( (HANDLE)pNewClient->Socket, HANDLE_FLAG_INHERIT, 0 );
#else
	{ 
		int flags = fcntl( pNewClient->Socket, F_GETFD, 0 );
		if( flags >= 0 ) fcntl( pNewClient->Socket, F_SETFD, flags | FD_CLOEXEC );
	}
#endif

#ifdef LOG_SOCKET_CREATION
	lprintf( "Accepted socket %p %d  (%d)", pNewClient, pNewClient->Socket, nTemp );
#endif
	//DumpAddr( "Client's Address", pNewClient->saClient );
	{
#ifdef __LINUX__
		socklen_t
#else
			int
#endif
		nLen = MAGIC_SOCKADDR_LENGTH;
		pNewClient->saSource = DuplicateAddress( pListen->saSource );
		if( getsockname( pNewClient->Socket, pNewClient->saSource, &nLen ) )
		{
			DumpAddr( "Failed adr:", pNewClient->saSource );
			DumpAddr( " Failed from:", pNewClient->saClient);
			lprintf( "getsockname errno = %d", errno );
		}
		if( SOCKADDR_NAME( pNewClient->saSource) ) free( SOCKADDR_NAME( pNewClient->saSource ) );
		SOCKADDR_NAME( pNewClient->saSource ) = NULL;
		//lprintf( "sockaddrlen: %d", nLen );
		if( pNewClient->saSource->sa_family == AF_INET )
			SET_SOCKADDR_LENGTH( pNewClient->saSource, IN_SOCKADDR_LENGTH );
		else if( pNewClient->saSource->sa_family == AF_INET6 )
			SET_SOCKADDR_LENGTH( pNewClient->saSource, IN6_SOCKADDR_LENGTH );
		else
			SET_SOCKADDR_LENGTH( pNewClient->saSource, nLen );
	}
	pNewClient->errorCallback           = pListen->errorCallback;
	pNewClient->psvErrorCallback        = pListen->psvErrorCallback;
	pNewClient->read.ReadComplete       = pListen->read.ReadComplete;
	pNewClient->psvRead                 = pListen->psvRead;
	pNewClient->close.CloseCallback     = pListen->close.CloseCallback;
	pNewClient->psvClose                = pListen->psvClose;
	pNewClient->write.WriteComplete     = pListen->write.WriteComplete;
	pNewClient->psvWrite                = pListen->psvWrite;
	pNewClient->dwFlags                |= CF_CONNECTED | ( pListen->dwFlags & CF_CALLBACKTYPES );
	if( IsValid(pNewClient->Socket) )
	{ // and we get one from the accept...
#ifdef _WIN32
		pNewClient->event = WSACreateEvent();
		WSAEventSelect( pNewClient->Socket, pNewClient->event, /*FD_ACCEPT| FD_OOB| FD_CONNECT| FD_QOS| FD_GROUP_QOS| FD_ROUTING_INTERFACE_CHANGE| FD_ADDRESS_LIST_CHANGE|*/
			FD_READ|FD_WRITE|FD_CLOSE );
#else
		// yes this is an ugly transition from the above dangling
		// else...
		fcntl( pNewClient->Socket, F_SETFL, O_NONBLOCK );
#endif
		AddActive( pNewClient );
		{
			//lprintf( "Accepted and notifying..." );
			if( pNewClient->Socket != INVALID_SOCKET ) {
				scheduleSocket( pNewClient, NULL );
			}
			if( pListen->connect.ClientConnected )
			{
				pNewClient->dwFlags |= CF_CONNECT_ISSUED;
				// SSL layer(if hooked) will clear CONNECT_ISSUED, and track that state itself.
				if( pListen->dwFlags & CF_CPPCONNECT )
					pListen->connect.CPPClientConnected( pListen->psvConnect, pNewClient );
				else 
					pListen->connect.ClientConnected( pListen, pNewClient );
			}

			// signal initial read.
			//lprintf(" Initial notifications...");
			if( pNewClient->read.ReadComplete )
			{
				pNewClient->dwFlags |= CF_READREADY; // may be... at least we can fail sooner...
				if( pListen->dwFlags & CF_CPPREAD )
					pNewClient->read.CPPReadComplete( pNewClient->psvRead, NULL, 0 );  // process read to get data already pending...
				else
					pNewClient->read.ReadComplete( pNewClient, NULL, 0 );  // process read to get data already pending...
			}

			NetworkUnlockEx( pNewClient, 0 DBG_SRC );
			NetworkUnlockEx( pNewClient, 1 DBG_SRC );
		}
	}
	else // accept failed...
	{
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pNewClient, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pNewClient, 0 DBG_SRC );
		NetworkUnlockEx( pNewClient, 1 DBG_SRC );
		pNewClient = NULL;
	}

	if( !pNewClient )
	{
		lprintf("Failed Accept...");
	}
}

//----------------------------------------------------------------------------

static void openSocket( PCLIENT pClient, SOCKADDR *pFromAddr, SOCKADDR *pAddr, LOGICAL autoSocket )
{
#ifdef __LINUX__	
	int replaced = 0;
#endif	
	if( !IsInvalid( pClient->Socket ) )
	{
		//lprintf( "CLOSE SOCKET IN OPEN: %d", pClient->Socket );
#ifdef __LINUX__	
		// closing a handle and re-opening it doesn't trick epoll...  have to probably at last add the events...
		// but there's other work associated with related sockets that needs to be done... not just reset epoll handle
		RemoveThreadEvent( pClient );
		replaced = 1;
#endif
		closesocket( pClient->Socket );
	}
#ifndef _WIN32
	if( pAddr->sa_family==AF_UNIX ) {
		// delete the old socket file if it exists
		if( pFromAddr == pAddr ) // this would only be true for listeners(?)
			unlink( (char*)(((uint16_t*)pAddr)+1));
	}
#endif
	//	pListen->Socket = socket( *(uint16_t*)pAddr, SOCK_STREAM, 0 );
#ifdef _WIN32
	pClient->event = WSACreateEvent();
	pClient->Socket = OpenSocket( ((*(uint16_t*)pAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
	if( pClient->Socket == INVALID_SOCKET )
#endif
#ifdef __MAC__
		pClient->Socket = socket( ((uint8_t*)pAddr)[1]
										, SOCK_STREAM
										, ((((uint8_t*)pAddr)[1] == AF_INET)||((((uint8_t*)pAddr)[1]) == AF_INET6))?IPPROTO_TCP:0 );
#else
		pClient->Socket = socket( *(uint16_t*)pAddr
										, SOCK_STREAM
										, (((*(uint16_t*)pAddr) == AF_INET)||((*(uint16_t*)pAddr) == AF_INET6))?IPPROTO_TCP:0 );
#endif
#ifdef LOG_SOCKET_CREATION
	// accept() also is 'created new Socket'
	lprintf( "Created new socket %p %p %d %d", pClient, pFromAddr, pClient->Socket, errno );
#endif
	if( pClient->Socket != INVALID_SOCKET )
	{
		int err;
		if( pFromAddr )
		{
			LOGICAL opt = 1;
			err = setsockopt( pClient->Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof( opt ) );
			if( err )
			{
				uint32_t dwError = WSAGetLastError();
				lprintf( "Failed to set socket option REUSEADDR : %d", dwError );
			}
#ifndef WIN32
			// reuse port isn't a windows feature.
			err = setsockopt( pClient->Socket, SOL_SOCKET, SO_REUSEPORT, (const char *)&opt, sizeof( opt ) );
			if( err )
			{
				uint32_t dwError = WSAGetLastError();
				lprintf( "Failed to set socket option REUSEPORT : %d", dwError );
			}
#endif
			// client's don't normally have a from, but when they do, it gets overwritten with another duplicate...
			if( pClient->saSource != pFromAddr ) {
				if( !pClient->saSource )
					pClient->saSource = DuplicateAddress( pFromAddr );
				else {
					lprintf( "Source address already set... %p", pClient->saSource );
					DumpAddr( "Source address is already", pClient->saSource );
					DumpAddr( "new from address is", pFromAddr );
				}
			}
			//DumpAddr( " in pFromAddr source", pClient->saSource );
			if( ( err = bind( pClient->Socket, pClient->saSource
											, SOCKADDR_LENGTH( pClient->saSource ) ) ) )
			{
				uint32_t dwError;
				dwError = WSAGetLastError();
				lprintf( "Error binding connecting socket to source address...: %d", dwError );
				DumpAddr( "Bind address:", pAddr );
				closesocket( pClient->Socket );
				pClient->Socket = INVALID_SOCKET;
			}
			if( !autoSocket ) {
				if( pClient->saSource->sa_family == AF_INET ) {
					if( !(*(uint32_t*)(pClient->saSource->sa_data+2) ) ) {
						// have to have the base one open or pcOther cannot be set.
						SOCKADDR* lpMyAddr = CreateSockAddress( ":::", ntohs(((uint16_t*)(pClient->saSource->sa_data+0))[0]) );
						pClient->pcOther = CPPOpenTCPListenerAddr_v3d( lpMyAddr, pClient->connect.CPPClientConnected, pClient->psvConnect, pClient->flags.bWaiting, TRUE DBG_SRC );
						pClient->pcOther->pcOther = pClient;
						ReleaseAddress( lpMyAddr );
					}
				} else if( pClient->saSource->sa_family == AF_INET6 ) {
					// maybe we only do the 'other' for IPV4 source?
					if( MemCmp( pClient->saSource->sa_data+6, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16 ) == 0 ) {
						SOCKADDR* lpMyAddr = CreateSockAddress( "0.0.0.0", ntohs(((uint16_t*)(pClient->saSource->sa_data+0))[0]) );
						pClient->pcOther = CPPOpenTCPListenerAddr_v3d( lpMyAddr, pClient->connect.CPPClientConnected, pClient->psvConnect, pClient->flags.bWaiting, TRUE DBG_SRC );
						pClient->pcOther->pcOther = pClient;
						ReleaseAddress( lpMyAddr );
					}
				}
			}
		}

#ifdef WIN32
		SetHandleInformation( (HANDLE)pClient->Socket, HANDLE_FLAG_INHERIT, 0 );
#else
		{
			int flags = fcntl( pClient->Socket, F_GETFL, 0 );
			fcntl( pClient->Socket, F_SETFL, O_NONBLOCK );
		}
		{ 
			int flags = fcntl( pClient->Socket, F_GETFD, 0 );
			if( flags >= 0 ) fcntl( pClient->Socket, F_SETFD, flags | FD_CLOEXEC );
		}
#endif
	}
#ifdef __LINUX__	
		// closing a handle and re-opening it doesn't trick epoll...  have to probably at last add the events...
		// but there's other work associated with related sockets that needs to be done... not just reset epoll handle
	if( replaced )
		AddThreadEvent( pClient, 0 );
#endif

}

PCLIENT CPPOpenTCPListenerAddr_v2d( SOCKADDR *pAddr
                                 , cppNotifyCallback NotifyCallback
                                 , uintptr_t psvConnect
                                 , LOGICAL waitForReady
                                 DBG_PASS )
{
	return CPPOpenTCPListenerAddr_v3d( pAddr, NotifyCallback, psvConnect, waitForReady, FALSE DBG_RELAY ); 
}

PCLIENT CPPOpenTCPListenerAddr_v3d( SOCKADDR *pAddr
                                 , cppNotifyCallback NotifyCallback
                                 , uintptr_t psvConnect
                                 , LOGICAL waitForReady
											, LOGICAL autoSocket
                                 DBG_PASS )
{
	PCLIENT pListen;
	if( !pAddr )
		return NULL;
	pListen = GetFreeNetworkClient();
	if( !pListen )
	{
		lprintf( "Network has not been started." );
		return NULL;
	}
	// openSocket also schedules it for event handling... so setup what events.
	pListen->dwFlags &= ~CF_UDP; // make sure this flag is clear!
	pListen->dwFlags |= CF_LISTEN;
	pListen->flags.bWaiting = waitForReady;
	pListen->connect.CPPClientConnected = NotifyCallback;
	pListen->psvConnect = psvConnect;
	pListen->dwFlags |= CF_CPPCONNECT;

	// this does the bind part of the socket also... (if any)
	openSocket( pListen, pAddr, pAddr, autoSocket /*second parameter just selects link protocol, and stream */ );
	if( pListen->Socket == INVALID_SOCKET )
	{
		lprintf( " Open Listen Socket Fail... %d", errno);
		DumpAddr( "passed address to select:", pAddr );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pListen, 0 DBG_SRC );
		NetworkUnlockEx( pListen, 1 DBG_SRC );
		pListen = NULL;
		return NULL;
	}

	AddActive( pListen );
	pListen->saSource = DuplicateAddress( pAddr );
	scheduleSocket( pListen, NULL );

	if(listen(pListen->Socket, SOMAXCONN ) == SOCKET_ERROR )
	{
		PCLIENT other = pListen->pcOther;
		if( other ) {
			pListen->pcOther = NULL;
			other->pcOther = NULL;
		} else 
			lprintf( "listen(5) failed: (autosocket %d)%d", autoSocket, WSAGetLastError() );

		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pListen, 0 DBG_SRC );
		NetworkUnlockEx( pListen, 1 DBG_SRC );
		return other;
	}
	NetworkUnlockEx( pListen, 0 DBG_SRC );
	NetworkUnlockEx( pListen, 1 DBG_SRC );
	// make sure to schedule this socket for events (connect)
#ifdef USE_WSA_EVENTS
	WSAEventSelect( pListen->Socket, pListen->event, FD_ACCEPT|FD_CLOSE );
#endif
	return pListen;
}

PCLIENT CPPOpenTCPListenerAddrExx( SOCKADDR *pAddr
	, cppNotifyCallback NotifyCallback
	, uintptr_t psvConnect
	DBG_PASS )
{
	return CPPOpenTCPListenerAddr_v2d( pAddr, NotifyCallback, psvConnect, FALSE DBG_RELAY );
}
//----------------------------------------------------------------------------
PCLIENT OpenTCPListenerAddr_v2d( SOCKADDR *pAddr
                              , cNotifyCallback NotifyCallback
                              , LOGICAL waitForReady DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerAddr_v2d( pAddr, (cppNotifyCallback)NotifyCallback, 0, waitForReady DBG_RELAY );
	if( result ) {
		result->dwFlags &= ~CF_CPPCONNECT;
		if( result->pcOther ) result->pcOther->dwFlags &= ~CF_CPPCONNECT;
	}
	return result;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPListenerAddrExx( SOCKADDR *pAddr
                              , cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerAddr_v2d( pAddr, (cppNotifyCallback)NotifyCallback, 0, FALSE DBG_RELAY );
	if( result ) {
		result->dwFlags &= ~CF_CPPCONNECT;
		if( result->pcOther ) result->pcOther->dwFlags &= ~CF_CPPCONNECT;
	}
	return result;
}

//----------------------------------------------------------------------------

PCLIENT CPPOpenTCPListenerExx( uint16_t wPort
                             , cppNotifyCallback NotifyCallback
                             , uintptr_t psvConnect
                             DBG_PASS
                             )
{
	SOCKADDR *lpMyAddr = CreateLocal(wPort);
	PCLIENT pc = CPPOpenTCPListenerAddr_v2d( lpMyAddr, NotifyCallback, psvConnect, FALSE DBG_RELAY );
	ReleaseAddress( lpMyAddr );
	return pc;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPListenerExx(uint16_t wPort, cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerExx( wPort, (cppNotifyCallback)NotifyCallback, 0 DBG_RELAY );
	if( result ) {
		result->dwFlags &= ~CF_CPPCONNECT;
		if( result->pcOther ) result->pcOther->dwFlags &= ~CF_CPPCONNECT;
	}
	return result;
}

//----------------------------------------------------------------------------

int NetworkConnectTCPEx( PCLIENT pc DBG_PASS ) {
	int err;

	while( !NetworkLockEx( pc, 0 DBG_SRC ) )
	{
		if( !(pc->dwFlags & CF_ACTIVE) )
		{
			return -1;
		}
		Relinquish();
	}

	pc->dwFlags |= CF_CONNECTING;

	while( 1 ) {
		if( (err = connect( pc->Socket, pc->saClient
		                  , SOCKADDR_LENGTH( pc->saClient ) )) )
		{
			uint32_t dwError;
			dwError = WSAGetLastError();
			//lprintf( "err: %d %d %d", err, dwError, EHOSTUNREACH );
			if( 
#ifdef __LINUX__
			    dwError == EHOSTUNREACH 
			  || dwError == ENETUNREACH
#else
			    dwError == WSAENETUNREACH 
#endif			
			)
			{
				//DumpAddr( "Unreachable address:", pc->saClient );
				if( pc->saClient->sa_family == AF_INET6 )
				{
					char * tmp = ((char**)(pc->saClient))[-1];
					if( tmp ) {
						SOCKADDR *addr = (SOCKADDR*)CreateSockAddressV2( tmp, ntohs((uint16_t)( (SOCKADDR_IN*)(pc->saClient))->sin_port), NETWORK_ADDRESS_FLAG_PREFER_V4 );
						if( addr->sa_family == AF_INET ) {
							pc->saClient = addr;
							// the old pc->Socket was scheduled (in wait events?)
							// and this creates a new One...
							//lprintf( "Re-open the socket... was %d", pc->Socket );
							// if saSource == saClient (which it never should, they should be indepdenatly duplicated)
							// this behaves like a listener 
							if( pc->saSource ) {
								lprintf( "Why is saSource not NULL? (if it wasn't... ) %p", pc->saSource );
								ReleaseAddress( pc->saSource );
								pc->saSource = NULL;
							}
							openSocket( pc, pc->saSource /* used to bind to an address on the client side */, pc->saClient /* just determins socket parameters with family */, FALSE );
							//lprintf( "Re-open the socket... and is %d", pc->Socket );
							// should kick the thread waiting to wait on the new one now...
							// but it didn't have to be scheduled, connect() bombed before an event...
							continue;
						}else {
							DumpAddr( "The resulting request for a V4 was still v6?", addr );
							DumpAddr( "and it started V6", pc->saClient );
							ReleaseAddress( addr );
						}
					}
				}
			} else if( dwError != WSAEWOULDBLOCK
#ifdef __LINUX__
			  && dwError != EINPROGRESS
#else
			  && dwError != WSAEINPROGRESS
#endif
				)
			{
				// is a union, either is valid to test
				if( pc->connect.CPPThisConnected ) {
					//lprintf( "connect callback dispatch... %p", pc->saClient );
					pc->dwFlags |= CF_CONNECT_ISSUED;
					if( pc->dwFlags & CF_CPPCONNECT )
						pc->connect.CPPThisConnected( pc->psvConnect, dwError );
					else
						pc->connect.ThisConnected( pc, dwError );
				}
				//_lprintf( DBG_RELAY )("Connect FAIL: %p %d %d %" _32f, pc->saClient, pc->Socket, err, dwError);
				EnterCriticalSec( &globalNetworkData.csNetwork );
				InternalRemoveClientEx( pc, TRUE, FALSE );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				NetworkUnlockEx( pc, 0 DBG_SRC );
				pc = NULL;
				return dwError;
			}
			else
			{
				// is EINPROGRESS on linux, WSAEWOULDBLOCK on windows...
				//lprintf( "Pending connect has begun...%p %d", pc, dwError );
			}
		}
		else
		{
			// if FD_CONNECT isn't selected, then this section is called
			// otherwise this is mostly dead code in this block.
			pc->dwFlags &= ~CF_CONNECTING;
			pc->dwFlags |= CF_CONNECTED;

			if( !pc->saSource ) {
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
				SET_SOCKADDR_LENGTH( pc->saSource
					, pc->saSource->sa_family == AF_INET
					? IN_SOCKADDR_LENGTH
					: IN6_SOCKADDR_LENGTH );
			}


			if( pc->connect.CPPThisConnected ) {
				//lprintf( "connect callback dispatch... %p", pc->saClient );
				pc->dwFlags |= CF_CONNECT_ISSUED;
				if( pc->dwFlags & CF_CPPCONNECT )
					pc->connect.CPPThisConnected( pc->psvConnect, 0 );
				else
					pc->connect.ThisConnected( pc, 0 );
			}
			if( pc->read.ReadComplete ) {
				if( pc->dwFlags & CF_CPPREAD )
					// process read to get data already pending...
					pc->read.CPPReadComplete( pc->psvRead, NULL, 0 );
				else
					// process read to get data already pending...
					pc->read.ReadComplete( pc, NULL, 0 );
			}

//#ifdef VERBOSE_DEBUG
			lprintf( "Connected before we even get a chance to wait" );
//#endif
		}
		break;
	}
	NetworkUnlockEx( pc, 0 DBG_SRC );

	return 0;
}

#if 0
static inline void waitScheduleSocket( PCLIENT pc ) {
#ifdef _WIN32
	int tries = 0;
	while( !pc->this_thread ) {
		INDEX idx;
		POINTER client;
		IdleFor(1); // wait for it to be added to waiting lists?
		if( tries++ > 10 ) {
			tries = 0;
			for( idx = 0; client = PeekQueueEx( globalNetworkData.client_schedule, (int)idx); idx++ ){
				if( client == pc ) break;
			}
			if( !pc->this_thread && !client ) {
				lprintf( "Lost client in schedule list:%p (Requeuing)", pc );
				EnqueLink( &globalNetworkData.client_schedule, pc );
				WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
			}
		}
	}
	//if( tries > 1 ) lprintf( "Took %d tries.", tries );
#endif
}
#endif
//----------------------------------------------------------------------------

static PCLIENT InternalTCPClientAddrFromAddrExxx( SOCKADDR *lpAddr, SOCKADDR *pFromAddr,
                                                  int bCPP,
                                                  cppReadComplete  pReadComplete,
                                                  uintptr_t psvRead,
                                                  cppCloseCallback CloseCallback,
                                                  uintptr_t psvClose,
                                                  cppWriteComplete WriteComplete,
                                                  uintptr_t psvWrite,
                                                  cppConnectCallback pConnectComplete,
                                                  uintptr_t psvConnect,
                                                  int flags
                                                  DBG_PASS
                                                )
{
	// Server's Port and Name.
	PCLIENT pResult;
	if( !lpAddr )
		return NULL;
	pResult = GetFreeNetworkClient();

	if( pResult )
	{
		struct peer_thread_info *this_thread = IsNetworkThread();
		// use the sockaddr to switch what type of socket this is.
		
		openSocket( pResult, pFromAddr, lpAddr, FALSE ); // addr determins some socket flags
		if (pResult->Socket==INVALID_SOCKET)
		{
			// this is a windows failure like 'network not started'
			lprintf( "Create socket failed (network not started?). %d", WSAGetLastError() );
			EnterCriticalSec( &globalNetworkData.csNetwork );
			InternalRemoveClientEx( pResult, TRUE, FALSE );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			NetworkUnlockEx( pResult, 1 DBG_SRC );
			NetworkUnlockEx( pResult, 0 DBG_SRC );
			return NULL;
		}
		else
		{
			// allow caller to forge their copy (Hold, but this structure has negative indices)
			pResult->saClient = DuplicateAddress( lpAddr );

			// set up callbacks before asynch select...
			pResult->connect.CPPThisConnected  = pConnectComplete;
			pResult->psvConnect                = psvConnect;
			pResult->read.CPPReadComplete      = pReadComplete;
			pResult->psvRead                   = psvRead;
			pResult->close.CPPCloseCallback    = CloseCallback;
			pResult->psvClose                  = psvClose;
			pResult->write.CPPWriteComplete    = WriteComplete;
			pResult->psvWrite                  = psvWrite;
			if( bCPP )
				pResult->dwFlags |= ( CF_CALLBACKTYPES );

			AddActive( pResult );
			NetworkUnlockEx(pResult, 1 DBG_SRC);
			NetworkUnlockEx(pResult, 0 DBG_SRC);
			//lprintf( "Leaving Client's critical section" );

			scheduleSocket( pResult, this_thread );
#ifdef USE_WSA_EVENTS
			WSAEventSelect( pResult->Socket, pResult->event, FD_CONNECT|FD_READ|FD_WRITE|FD_CLOSE );
#endif

			// socket should now get scheduled for events, after unlocking it?
			if (!(flags & OPEN_TCP_FLAG_DELAY_CONNECT)) {
				NetworkConnectTCPEx(pResult DBG_RELAY);

				// if there is not a network callback, wait for connect inline...
				if( !pConnectComplete )
				{
					uint64_t Start = timeGetTime64();
					int bProcessing = 0;

					// should trigger a rebuild (if it's the root thread)
					pResult->dwFlags |= CF_CONNECT_WAITING;
					// caller was expecting connect to block....
					while( !( pResult->dwFlags & (CF_CONNECTED|CF_CONNECTERROR|CF_CONNECT_CLOSED) ) &&
							( ( timeGetTime64() - Start ) < globalNetworkData.dwConnectTimeout ) )
					{
						// may be this thread itself which connects...
						if( this_thread )
						{
							if( !bProcessing )
							{
								if( ProcessNetworkMessages( this_thread, 0 ) >= 0 )
									bProcessing = 1;
								else
									bProcessing = 2;
							}
							else
							{
								if( bProcessing >= 0 )
									ProcessNetworkMessages( this_thread, 0 );
							}
						}
						else
						{
							// isn't a network thread; so wait as an external thread
							bProcessing = 2;
						}
						if( bProcessing == 2 )
						{
							pResult->pWaiting = MakeThread();
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Falling asleep 3 seconds waiting for connect on %p.", pResult );
							//pResult->tcp_delay_count++;
							WakeableSleep( 3000 );
							pResult->pWaiting = NULL;
							if( pResult->dwFlags & CF_CLOSING )
								return NULL;
						}
						else
						{
							lprintf( "Spin wait for connect" );
							Relinquish();
						}
					}
					if( (( timeGetTime64() - Start ) >= 10000)
						|| (pResult->dwFlags &  CF_CONNECTERROR ) )
					{
						if( pResult->dwFlags &  CF_CONNECTERROR )
						{
							//DumpAddr( "Connect to: ", lpAddr );
							//lprintf( "Connect FAIL: message result" );
						}
						else
							lprintf( "Connect FAIL: Timeout" );
						EnterCriticalSec( &globalNetworkData.csNetwork );
						InternalRemoveClientEx( pResult, TRUE, FALSE );
						LeaveCriticalSec( &globalNetworkData.csNetwork );
						pResult->dwFlags &= ~CF_CONNECT_WAITING;
						return pResult = NULL;
					}
					{
#ifdef __LINUX__
						socklen_t
#else
						int
#endif
							nLen = MAGIC_SOCKADDR_LENGTH;
						if( !pResult->saSource )
							pResult->saSource = AllocAddr();
						//lprintf( "Setup sa Source to connect (reconnect?) %p", pResult->saSource);
						if( getsockname( pResult->Socket, pResult->saSource, &nLen ) )
						{
							lprintf( "getsockname errno = %d", errno );
						}
					}
					//lprintf( "Connect did complete... returning to application");
					pResult->dwFlags &= ~CF_CONNECT_WAITING;
				}
#ifdef VERBOSE_DEBUG
				else
					lprintf( "Connect in progress, will notify application when done." );
#endif
			}
		}
	}
	return pResult;
}

//----------------------------------------------------------------------------
NETWORK_PROC( PCLIENT, CPPOpenTCPClientAddrExxx )(SOCKADDR *lpAddr
                                                 , cppReadComplete  pReadComplete
                                                 , uintptr_t psvRead
                                                 , cppCloseCallback CloseCallback
                                                 , uintptr_t psvClose
                                                 , cppWriteComplete WriteComplete
                                                 , uintptr_t psvWrite
                                                 , cppConnectCallback pConnectComplete
                                                 , uintptr_t psvConnect
                                                 , int flags
                                                 DBG_PASS  )
{
	return InternalTCPClientAddrFromAddrExxx( lpAddr, NULL, TRUE
											 , pReadComplete, psvRead
											 , CloseCallback, psvClose
											 , WriteComplete, psvWrite
											 , pConnectComplete, psvConnect, flags DBG_RELAY );
}

//----------------------------------------------------------------------------
PCLIENT OpenTCPClientAddrExxx( SOCKADDR *lpAddr
                             , cReadComplete     pReadComplete
                             , cCloseCallback    CloseCallback
                             , cWriteComplete    WriteComplete
                             , cConnectCallback  pConnectComplete
                             , int flags
                             DBG_PASS
                             )
{
	return InternalTCPClientAddrFromAddrExxx( lpAddr, NULL, FALSE
											 , (cppReadComplete)pReadComplete, 0
											 , (cppCloseCallback)CloseCallback, 0
											 , (cppWriteComplete)WriteComplete, 0
											 , (cppConnectCallback)pConnectComplete, 0, flags DBG_RELAY );
}

PCLIENT OpenTCPClientAddrFromAddrEx(SOCKADDR *lpAddr, SOCKADDR *pFromAddr
															  , cReadComplete     pReadComplete
															  , cCloseCallback    CloseCallback
															  , cWriteComplete    WriteComplete
															  , cConnectCallback  pConnectComplete
	, int flags
                                               DBG_PASS
															 )
{

	return InternalTCPClientAddrFromAddrExxx( lpAddr, pFromAddr, FALSE
											 , (cppReadComplete)pReadComplete, 0
											 , (cppCloseCallback)CloseCallback, 0
											 , (cppWriteComplete)WriteComplete, 0
											 , (cppConnectCallback)pConnectComplete, 0, flags DBG_RELAY );
}

PCLIENT OpenTCPClientAddrFromEx(SOCKADDR *lpAddr, int port
															  , cReadComplete     pReadComplete															  , cCloseCallback    CloseCallback
															  , cWriteComplete    WriteComplete
															  , cConnectCallback  pConnectComplete
	, int flags
                                               DBG_PASS
															 )
{
	PCLIENT result;
	SOCKADDR *pFromAddr = CreateSockAddress( NULL, port );
	result = InternalTCPClientAddrFromAddrExxx( lpAddr, pFromAddr, FALSE
											 , (cppReadComplete)pReadComplete, 0
											 , (cppCloseCallback)CloseCallback, 0
											 , (cppWriteComplete)WriteComplete, 0
											 , (cppConnectCallback)pConnectComplete, 0, flags DBG_RELAY );
	ReleaseAddress( pFromAddr );
	return result;
}


//----------------------------------------------------------------------------
PCLIENT OpenTCPClientAddrExEx( SOCKADDR *lpAddr
                             , cReadComplete  pReadComplete
                             , cCloseCallback CloseCallback
                             , cWriteComplete WriteComplete
                             DBG_PASS )
{
   return OpenTCPClientAddrExxx( lpAddr, pReadComplete, CloseCallback, WriteComplete, NULL, 0 DBG_RELAY );
}

//----------------------------------------------------------------------------

PCLIENT CPPOpenTCPClientExEx(CTEXTSTR lpName,uint16_t wPort,
             cppReadComplete	 pReadComplete, uintptr_t psvRead,
             cppCloseCallback CloseCallback, uintptr_t psvClose,
             cppWriteComplete WriteComplete, uintptr_t psvWrite,
             cppConnectCallback pConnectComplete, uintptr_t psvConnect, int flags DBG_PASS )
{
	PCLIENT pClient;
	SOCKADDR *lpsaDest;
	pClient = NULL;
	if( lpName ) {
		if(lpsaDest = CreateSockAddressV2(lpName,wPort
		                                 , (flags&OPEN_TCP_FLAG_PREFER_V6)
		                                   ? NETWORK_ADDRESS_FLAG_PREFER_V6
		                                   : (flags&OPEN_TCP_FLAG_PREFER_V4)
		                                   ? NETWORK_ADDRESS_FLAG_PREFER_V4 
		                                   : NETWORK_ADDRESS_FLAG_PREFER_NONE
		   ) ) {
			pClient = CPPOpenTCPClientAddrExxx( lpsaDest
			                                  , pReadComplete
			                                  , psvRead
			                                  , CloseCallback
			                                  , psvClose
			                                  , WriteComplete
			                                  , psvWrite
			                                  , pConnectComplete
			                                  , psvConnect
			                                  , flags
			                                    DBG_RELAY
			                                  );
			ReleaseAddress( lpsaDest );
		} else if( pConnectComplete ) {
#ifdef WIN32
			pConnectComplete( psvConnect, globalNetworkData.lastAddressError );
#else
			pConnectComplete( psvConnect, errno );
#endif
		}
	}
	return pClient;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPClientExxx(CTEXTSTR lpName,uint16_t wPort,
             cReadComplete  pReadComplete,
             cCloseCallback CloseCallback,
             cWriteComplete WriteComplete,
             cConnectCallback pConnectComplete,
             int flags
             DBG_PASS )
{
	PCLIENT pClient;
	SOCKADDR *lpsaDest;
	pClient = NULL;
	if( lpName ) {
		if(lpsaDest = CreateSockAddressV2(lpName,wPort
		                                 , (flags&OPEN_TCP_FLAG_PREFER_V6)
		                                   ? NETWORK_ADDRESS_FLAG_PREFER_V6
		                                   : (flags&OPEN_TCP_FLAG_PREFER_V4)
		                                   ? NETWORK_ADDRESS_FLAG_PREFER_V4 
		                                   : NETWORK_ADDRESS_FLAG_PREFER_NONE
		   ) ) {
			pClient = OpenTCPClientAddrExxx( lpsaDest
			                               , pReadComplete
			                               , CloseCallback
			                               , WriteComplete
			                               , pConnectComplete
			                               , flags DBG_RELAY );
			ReleaseAddress( lpsaDest );
		} else if( pConnectComplete ) {
#ifdef WIN32
			pConnectComplete( NULL, globalNetworkData.lastAddressError );
#else
			pConnectComplete( NULL, errno );
#endif
		}
	}
	return pClient;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPClientExEx(CTEXTSTR lpName,uint16_t wPort,
                          cReadComplete	 pReadComplete,
                          cCloseCallback CloseCallback,
                          cWriteComplete WriteComplete
                          DBG_PASS )
{
	return OpenTCPClientExxx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete, NULL, 0 DBG_RELAY );
}


//----------------------------------------------------------------------------

int FinishPendingRead(PCLIENT lpClient DBG_PASS )  // only time this should be called is when there IS, cause
                                 // we definaly have already gotten SOME data to leave in
                                 // a pending state...
{
	int nRecv;
#ifdef VERBOSE_DEBUG
	int nCount;
#endif
	uint32_t dwError;
	// returns amount of information left to get.
#ifdef VERBOSE_DEBUG
	nCount = 0;
#endif
	if( !( lpClient->dwFlags & CF_READPENDING ) )
	{
		//lpClient->dwFlags |= CF_READREADY; // read ready is set if FinishPendingRead returns 0; and it's from the core read...
		//lprintf( "Finish pending - return, no pending read. %08x", lpClient->dwFlags );
	}

	do
	{
#if 0 && !DrainSupportDeprecated
		if( lpClient->bDraining )
		{
			lprintf("LOG:ERROR trying to read during a drain state..." );
			return -1; // why error on draining with pending finish??
		}
#endif

		if( !(lpClient->dwFlags & CF_CONNECTED)  )
		{
#ifdef DEBUG_SOCK_IO
			lprintf( "Finsih pending - return, not connected." );
#endif
			return (int)lpClient->RecvPending.dwUsed; // amount of data available...
		}
		//lprintf( ("FinishPendingRead of %d"), lpClient->RecvPending.dwAvail );
		if( !( lpClient->dwFlags & CF_READPENDING ) )
		{
			//lpClient->dwFlags |= CF_READREADY; // read ready is set if FinishPendingRead returns 0; and it's from the core read...
			if( lpClient->dwFlags & CF_WANTCLOSE ) {
				// the application didn't queue a buffer to read into, have to force accepting a close.
				lpClient->dwFlags |= CF_TOCLOSE;
				return -1;   // return pending finished...
			}
#ifdef DEBUG_SOCK_IO
			lprintf( "Finish pending - return, no pending read. %08x", lpClient->dwFlags );
#endif
			// without a pending read, don't read, the buffers are not correct.
			return 0;
		}
		while( lpClient->RecvPending.dwAvail )  // if any room is availiable.
		{
#ifdef DEBUG_SOCK_IO
			//nCount++;
			_lprintf( DBG_RELAY )( "FinishPendingRead %p %d %d", lpClient
				, lpClient->RecvPending.dwUsed, lpClient->RecvPending.dwAvail );
#endif
			nRecv = recv(lpClient->Socket,
							 (char*)lpClient->RecvPending.buffer.p +
							 lpClient->RecvPending.dwUsed,
							 (int)lpClient->RecvPending.dwAvail,0);
 			dwError = WSAGetLastError();
			//lprintf( "Network receive %p %d %zd %zd", lpClient, nRecv, lpClient->RecvPending.dwUsed, lpClient->RecvPending.dwAvail );
			if (nRecv == SOCKET_ERROR)
			{
#ifdef DEBUG_SOCK_IO
				lprintf( "Received error (-1) %d", nRecv );
#endif
				switch( dwError)
				{
				case WSAEWOULDBLOCK: // no data avail yet...
					//lprintf( "Pending Receive would block..." );
					lpClient->dwFlags &= ~CF_READREADY;
					return (int)lpClient->RecvPending.dwUsed;
#ifdef __LINUX__
				case ECONNRESET:
				// sometimes this leaks past connect and read happens?
				case ECONNREFUSED: 
#else
				case WSAECONNRESET:
				case WSAECONNABORTED:
#endif
#ifdef LOG_DEBUG_CLOSING
					lprintf( "Read from reset connection - closing. %p", lpClient );
#endif
					if(0)
					{
					default:
						lprintf( "Failed reading from %p %d (err:%d) into %p %" _size_f " bytes %" _size_f " read already.",
							  lpClient,
							  lpClient->Socket,
							  WSAGetLastError(),
							  lpClient->RecvPending.buffer.p,
							  lpClient->RecvPending.dwAvail,
							  lpClient->RecvPending.dwUsed );
						lprintf("LOG:ERROR FinishPending discovered unhandled error (closing connection) %" _32f "", dwError );
					}
					lpClient->dwFlags |= CF_TOCLOSE;
					return -1;   // return pending finished...
				}
			}
			else if (!nRecv) // channel closed if received 0 bytes...
			{   // otherwise WSAEWOULDBLOCK would be generated.
#ifdef DEBUG_SOCK_IO
				lprintf( "Received (0) %d", nRecv );
#endif
				//_lprintf( DBG_RELAY )( "Closing closed socket... Hope there's also an event... ");
				lpClient->dwFlags |= CF_TOCLOSE;
				break; // while dwAvail... try read...
				//return -1;
			}
			else
			{
#ifdef DEBUG_SOCK_IO
				lprintf( "Received %d", nRecv );
#endif
				if( globalNetworkData.flags.bShortLogReceivedData )
				{
					LogBinary( (uint8_t*)lpClient->RecvPending.buffer.p
					         + lpClient->RecvPending.dwUsed,  (nRecv < 64) ? nRecv:64 );
				}
				if( globalNetworkData.flags.bLogReceivedData )
				{
					LogBinary( (uint8_t*)lpClient->RecvPending.buffer.p
					         + lpClient->RecvPending.dwUsed, nRecv );
				}
				if( lpClient->RecvPending.s.bStream )
					lpClient->dwFlags &= ~CF_READREADY;
				lpClient->RecvPending.dwLastRead = nRecv;
				lpClient->RecvPending.dwAvail -= nRecv;
				//lprintf( "Receive pending is now %d after %d", lpClient->RecvPending.dwAvail, nRecv );
				lpClient->RecvPending.dwUsed  += nRecv;
				if( lpClient->RecvPending.s.bStream &&
				    lpClient->RecvPending.dwAvail )
					break;
				//else
				//	lprintf( "Was not a stream read - try reading more..." );
			}
		}

		// if read notification okay - then do callback.
		if( !( lpClient->dwFlags & CF_READWAITING ) )
		{
#ifdef LOG_PENDING
			//lprintf( "Waiting on a queued read... result to callback." );
#endif
			if( ( !lpClient->RecvPending.dwAvail || // completed all of the read
				  ( lpClient->RecvPending.dwUsed &&   // or completed some of the read
					lpClient->RecvPending.s.bStream ) ) )
			{
#ifdef LOG_PENDING
				//lprintf( "Sending completed read to application" );
#endif
				lpClient->dwFlags &= ~CF_READPENDING;
				if( lpClient->read.ReadComplete )  // and there's a read complete callback available
				{
					// need to clear dwUsed...
					// otherwise the on-close notificatino can cause this to dispatch again.
					size_t length = lpClient->RecvPending.dwUsed;
					lpClient->RecvPending.dwUsed = 0;
#ifdef LOG_PENDING
					//lprintf( "Send to application...." );
#endif
					if( lpClient->dwFlags & CF_CPPREAD )
					{
						lpClient->read.CPPReadComplete( lpClient->psvRead
																, lpClient->RecvPending.buffer.p
																, length );
					}
					else
					{
						lpClient->read.ReadComplete( lpClient,
															 lpClient->RecvPending.buffer.p,
															 length );
					}
					if( !IsValid( lpClient->Socket ) ) // closed
						return -1;
#ifdef LOG_PENDING
					//lprintf( "back from applciation... (loop to next)" ); // new read probably pending ehre...
#endif
					continue;
				}
			}
			if( !( lpClient->dwFlags & CF_READPENDING ) )
			{
				lprintf( "somehow we didn't get a good read." );
			}
		}
		else
		{
#ifdef LOG_PENDING
			lprintf( "Client is waiting for this data... should we wake him? %d", lpClient->RecvPending.s.bStream );
#endif
			if( ( !lpClient->RecvPending.dwAvail || // completed all of the read
				  ( lpClient->RecvPending.dwUsed &&   // or completed some of the read
					lpClient->RecvPending.s.bStream ) ) )
			{
				lprintf( "Wake waiting thread... clearing pending read flag." );
				lpClient->dwFlags &= ~CF_READPENDING;
				if( lpClient->pWaiting )
					WakeThread( lpClient->pWaiting );
			}
		}
		if( lpClient->dwFlags & CF_TOCLOSE )
			return -1;
		return (int)lpClient->RecvPending.dwUsed; // returns amount of information which is available NOW.
	} while ( 1 );
}

//----------------------------------------------------------------------------

size_t doReadExx2(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream, LOGICAL bWait, int user_timeout DBG_PASS )
{
#ifdef LOG_PENDING
	_lprintf(DBG_RELAY)( "Reading ... %p(%d) int %p %d (%s,%s)", lpClient, lpClient->Socket, lpBuffer, (uint32_t)nBytes, bIsStream?"stream":"block", bWait?"Wait":"NoWait" );
#endif
	if( !lpClient || !lpBuffer )
		return 0; // nothing read.... ???
	// don't try to read closed/inactive sockets.
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
		return 0;

#if 0 && !DrainSupportDeprecated
	if( TCPDrainRead( lpClient ) &&  //draining....
		lpClient->RecvPending.dwAvail ) // and already queued next read.
	{
		lprintf("LOG:ERROR is draining, and NEXT pending queued ALSO...");
		return -1;  // read not queued... (ERROR)
	}
#endif

	if( !lpClient->RecvPending.s.bStream && // existing read was not a stream...
		lpClient->RecvPending.dwAvail )   // AND is not at completion...
	{
		lprintf("LOG:ERROR was not a stream, and is not complete...%" _32f " left ",
			  (uint32_t)lpClient->RecvPending.dwAvail );
		return -1; // this I guess is an error since we're queueing another
		// read on top of existing incoming 'guaranteed data'
	}
#ifdef REQUIRE_READ_LOCK
	while( !NetworkLockEx( lpClient, 1 DBG_RELAY ) )
	{
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			return -1;
		}
		Relinquish();
	}
#endif
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
	{
		// like say the callback we're being invoked from closed it;
		lprintf( "inactive client, will not pend read." );
#ifdef REQUIRE_READ_LOCK
		NetworkUnlockEx( lpClient, 1 DBG_SRC );
#endif
		return -1;
	}
	//lprintf( "read %d", nBytes );
	if( nBytes )   // only worry if there IS data to read.
	{
		// we can assume there is nothing now pending...
		lpClient->RecvPending.buffer.p = lpBuffer;
		//_lprintf(DBG_RELAY)( "Setup read avail: %d", nBytes );
		lpClient->RecvPending.dwAvail = nBytes;
		lpClient->RecvPending.dwUsed = 0;
		lpClient->RecvPending.s.bStream = bIsStream;
		// if the pending finishes it will call the ReadComplete Callback.
		// otherwise there will be more data to read...
		//lprintf( "Ok ... buffers set up - now we can handle read events" );
		lpClient->dwFlags |= CF_READPENDING;
#ifdef LOG_PENDING
		lprintf( "Setup read pending %p %08x", lpClient, lpClient->dwFlags );
#endif
		if( bWait )
		{
			//lprintf( "setting read waiting so we get awoken... and callback dispatch does not happen." );
			lpClient->dwFlags |= CF_READWAITING;
		}
		//else
		//   lprintf( "No read waiting... allow forward going..." );
		if( lpClient->dwFlags & CF_READREADY )
		{
#ifdef LOG_PENDING
			lprintf( "Data already present for read..." );
#endif
			FinishPendingRead( lpClient DBG_SRC );
		}
	}
	else
	{
		lprintf( "zero byte read...." );
		if( !bWait )
		{
			if( lpClient->read.ReadComplete )  // and there's a read complete callback available
			{
				lprintf( "Read complete with 0 bytes immediate..." );
				lpClient->read.ReadComplete( lpClient, lpBuffer, 0 );
			}
		}
		else
		{
			lpClient->dwFlags &= ~CF_READWAITING;
 		}
	}
	if( bWait )
	{
		int this_timeout = user_timeout?user_timeout:globalNetworkData.dwReadTimeout;
		int timeout = 0;
		//lprintf( "Waiting for TCP data result..." );
		{
			uint32_t tick = timeGetTime();
			lpClient->pWaiting = MakeThread();
#ifdef REQUIRE_READ_LOCK
			NetworkUnlockEx( lpClient, 1 DBG_SRC );
#endif
			while( lpClient->dwFlags & CF_READPENDING )
			{
				// wait 5 seconds, then bail.
				if( ( tick + this_timeout ) < timeGetTime() )
				{
					//lprintf( "pending has timed out! return now." );
					timeout = 1;
					break;
				}
				if( !Idle() )
				{
					//lprintf( "Nothing significant to idle on... going to sleep forever." );
					WakeableSleep( 1000 );
				}
			}
		}
		while( !NetworkLockEx( lpClient, 1 DBG_SRC ) )
		{
			if( !(lpClient->dwFlags & CF_ACTIVE ) )
			{
				return 0;
			}
			Relinquish();
		}
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
#ifdef REQUIRE_READ_LOCK
			NetworkUnlockEx( lpClient, 1 DBG_SRC );
#endif
			return -1;
		}
 		lpClient->dwFlags &= ~CF_READWAITING;
#ifdef REQUIRE_READ_LOCK
		NetworkUnlockEx( lpClient, 1 DBG_SRC);
#endif
		if( timeout )
			return 0;
		else
			return lpClient->RecvPending.dwUsed;
	}
#ifdef REQUIRE_READ_LOCK
	else
		NetworkUnlockEx( lpClient, 1 DBG_SRC );
#endif

	return 0; // unknown result really... success prolly
}

//----------------------------------------------------------------------------

static void PendWrite( PCLIENT pClient
                     , CPOINTER lpBuffer
                     , size_t nLen, int bLongBuffer )
{
	PendingBuffer *lpPend;
#if defined( LOG_PENDING ) || defined( LOG_WRITE_NOTICES )
	lprintf( "Pending %d Bytes to network... %p" , nLen, pClient );
#endif
	lpPend = New( PendingBuffer );
	//lprintf( "Write pend %d", nLen );
	lpPend->dwAvail = nLen;
	lpPend->dwUsed	= 0;
	lpPend->lpNext	= NULL;
	if( !bLongBuffer )
	{
		 lpPend->s.bDynBuffer = TRUE;
		 lpPend->buffer.p = Allocate( nLen );
		 MemCpy( lpPend->buffer.p, lpBuffer, nLen );
	}
	else
	{
		 lpPend->s.bDynBuffer = FALSE;
		 lpPend->buffer.c = lpBuffer;
	}
	if (pClient->lpLastPending)
		pClient->lpLastPending->lpNext = lpPend;
	else
		pClient->lpFirstPending = lpPend;
	pClient->lpLastPending = lpPend;
}

//----------------------------------------------------------------------------

int TCPWriteEx(PCLIENT pc DBG_PASS)
{
	int nSent;
	PendingBuffer *lpNext;
	if( !pc || !(pc->dwFlags & CF_CONNECTED ) )
		return 1;
	while (pc->lpFirstPending)
	{
#ifdef VERBOSE_DEBUG
//		if( pc->dwFlags & CF_CONNECTING )
			lprintf( "Sending previously queued data." );
#endif

		if( pc->lpFirstPending->dwAvail )
		{
			uint32_t dwError;
			if( pc->flags.bAggregateOutput && pc->lpFirstPending->lpNext ){
				uint32_t size = 0;
				//uint32_t blocks = 0;
				PendingBuffer *lpNext = pc->lpFirstPending;
				// collapse all pending into first pending block.
				while( lpNext ) { size += lpNext->dwAvail; /*blocks++; */lpNext = lpNext->lpNext;}
				if( size > 0 ) {
					POINTER newbuffer =  Allocate( size );
					// 
					//lprintf( "Allocate total aggregate size:%d in %d chunks", size, blocks );
					lpNext = pc->lpFirstPending;
					size = 0;
					while( lpNext ) {
						//lprintf( "Copy:%d %d",size, lpNext->dwAvail );
						MemCpy( ((uint8_t*)newbuffer) + size
								, ((const uint8_t*)lpNext->buffer.c)+lpNext->dwUsed
								, lpNext->dwAvail );
						if( lpNext->s.bDynBuffer )
							Release( lpNext->buffer.p );
						size += lpNext->dwAvail;
						PendingBuffer *next = lpNext->lpNext;
						if( lpNext != &pc->FirstWritePending )
							Release( lpNext );
						lpNext = next;
					}
					pc->FirstWritePending.buffer.c = newbuffer;
					pc->FirstWritePending.dwAvail = size;
					pc->FirstWritePending.dwUsed = 0;
					pc->FirstWritePending.lpNext = NULL;
					pc->FirstWritePending.s.bDynBuffer = TRUE;
#ifdef LOG_WRITE_AGGREGATION
					lprintf( "Aggregated %d bytes into single buffer %p", size, pc );
#endif
					pc->lpLastPending =	pc->lpFirstPending = &pc->FirstWritePending;
				}
			}
			if( globalNetworkData.flags.bLogSentData )
			{
				LogBinary( (uint8_t*)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail );
			}
#ifdef DEBUG_SOCK_IO
			_lprintf(DBG_RELAY)( "Try to send... %p  %d  %d", pc, pc->lpFirstPending->dwUsed, pc->lpFirstPending->dwAvail );
#endif
			nSent = send(pc->Socket,
							 (char*)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail,
							 MSG_NOSIGNAL );
  			dwError = WSAGetLastError();
#ifdef DEBUG_SOCK_IO
			lprintf( "sent result: %d %d %d", nSent, pc->lpFirstPending->dwUsed, pc->lpFirstPending->dwAvail );
#endif
			if (nSent == SOCKET_ERROR) {
				if( dwError == WSAEWOULDBLOCK )  // this is alright.
				{
#ifdef VERBOSE_DEBUG
					lprintf( "Pending write...(EBLOCK) %p %zd", pc, pc->lpFirstPending->dwAvail );
#endif
					pc->dwFlags &= ~CF_WRITEREADY;
					pc->dwFlags |= CF_WRITEPENDING;
					return TRUE;
				}
				if( dwError == EPIPE ) {
					//_lprintf(DBG_RELAY)( "EPIPE on send() to socket...");
					pc->dwFlags |= CF_TOCLOSE;
					return FALSE;
				}
				if( dwError == ECONNREFUSED ) {
					_lprintf(DBG_RELAY)( "ECONNREFUSED on send() to socket...");
					pc->dwFlags |= CF_TOCLOSE;
					return FALSE;
				}
#ifdef _WIN32
				// especially websockets that try to gracefully cloose
				if( dwError == WSAESHUTDOWN) {
					if( pc->dwFlags & CF_WANTCLOSE )
						pc->dwFlags |= CF_TOCLOSE;
					else
						_lprintf(DBG_RELAY)( "WSAESHUTDOWN on send() to socket...");
					pc->dwFlags |= CF_TOCLOSE;
					return FALSE;
				}
				if( dwError == WSAECONNABORTED ) {
					pc->dwFlags |= CF_TOCLOSE;
					return FALSE;
				}
#endif
				{
					_lprintf(DBG_RELAY)(" Network Send Error: %5d(sock:%p, buffer:%p ofs: %" _size_f "  Len: %" _size_f ")",
											  dwError,
											  pc,
											  pc->lpFirstPending->buffer.c,
											  pc->lpFirstPending->dwUsed,
											  pc->lpFirstPending->dwAvail );
					if( dwError == 10057 // ENOTCONN
						||dwError == 10014 // EFAULT
#ifdef __LINUX__
						|| dwError == EPIPE
#endif
					  )
					{
						pc->dwFlags |= CF_TOCLOSE;
					}
					return FALSE; // get out of here! (say we sent whatever was pending...; don't repend long buffers)
				}
			} else if (!nSent) { // other side closed.
				//lprintf( "sent zero bytes - assume it was closed - and HOPE there's an event..." );
				// this is more likely to show up as an EPIPE
				pc->dwFlags |= CF_TOCLOSE;
				// if this happened - don't return TRUE result which would
				// result in queuing a pending buffer...
				return FALSE;  // no sence processing the rest of this.
			} else if( pc->lpFirstPending && ( nSent < (int)pc->lpFirstPending->dwAvail ) ) {
				pc->dwFlags &= ~CF_WRITEREADY;
				pc->dwFlags |= CF_WRITEPENDING;
			}
#ifdef _WIN32
			// windows only gives a FD_WRITE when a send gets an ewouldblock...
			else
				pc->dwFlags |= CF_WRITEREADY;
#endif
		}
		else {
			lprintf( "nothing was pending?" );
			nSent = 0;
		}

#ifdef DEBUG_SOCK_IO
		lprintf( "sent... %d %d %d", nSent, pc->lpFirstPending?pc->lpFirstPending->dwUsed:0, pc->lpFirstPending?pc->lpFirstPending->dwAvail:0 );
#endif

		{  // sent some data - update pending buffer status.
			if( pc->lpFirstPending )
			{
				pc->lpFirstPending->dwAvail -= nSent;
				//lprintf( "Subtracted %d got %d", nSent, pc->lpFirstPending->dwAvail );
				pc->lpFirstPending->dwUsed  += nSent;
				if (!pc->lpFirstPending->dwAvail)  // no more to send...
				{
					CPOINTER p = pc->lpFirstPending->buffer.p;
					size_t len = pc->lpFirstPending->dwUsed;
					lpNext = pc->lpFirstPending -> lpNext;
					if( pc->lpFirstPending->s.bDynBuffer ) {
						Release( pc->lpFirstPending->buffer.p );
						p = NULL; // already released the buffer.
					}
					// there is one pending holder in the client
					// structure that was NOT allocated...
					if( pc->lpFirstPending != &pc->FirstWritePending )
					{
#ifdef LOG_PENDING
						lprintf( "Finished sending a pending buffer." );
#endif
						Release(pc->lpFirstPending );
					}
					else
					{
#ifdef LOG_PENDING
						if(0) // this happens 99.99% of the time.
						{
							lprintf( "Normal send complete." );
						}
#endif
					}
					if (!lpNext)
						pc->lpLastPending = NULL;
					pc->lpFirstPending = lpNext;

					if( p && pc->write.WriteComplete &&
						!pc->bWriteComplete )
					{
						pc->bWriteComplete = TRUE;
						if( pc->dwFlags & CF_CPPWRITE )
							pc->write.CPPWriteComplete( pc->psvWrite, p, len );  // SOME WRITE!!!
						else
							pc->write.WriteComplete( pc, p, len );  // SOME WRITE!!!
						pc->bWriteComplete = FALSE;
					}
					if( !pc->lpFirstPending ) {
						pc->dwFlags &= ~CF_WRITEPENDING;
						//lprintf( "nothing left pending.... %p", pc );
					}
				}
				else
				{
					// still have more to send...
					//lprintf( "wasn't able to send all at this time, there's still pending data %p", pc );
					if( !(pc->dwFlags & CF_WRITEPENDING) )
					{
						//lprintf( "And set writepending again?  Do a scheduled timer?" );
						pc->dwFlags |= CF_WRITEPENDING;
					}
					return TRUE;
				}
			}
		}
	}
#ifdef LOG_WRITE_NOTICES
	lprintf( "everything is written. %p", pc );
#endif	

	return FALSE; // 0 = everything sent / nothing left to send...
}

//----------------------------------------------------------------------------

void SetTCPWriteAggregation( PCLIENT pc, int bAggregate )
{
	lprintf( "Write Aggregation causes failures:%d", bAggregate );
	bAggregate = 0;
	pc->flags.bAggregateOutput = bAggregate;
}

static void triggerWrite( uintptr_t psv ){
	PCLIENT pc = (PCLIENT)psv;
#ifdef LOG_WRITE_AGGREGATION
	lprintf( "Timer fired %p  %d", psv, pc->writeTimer );
#endif	
	if( pc )
	{
#ifdef LOG_WRITE_AGGREGATION
		int32_t timer = pc->writeTimer;
#endif
		pc->writeTimer = 0;
		if( pc->dwFlags & CF_WRITEPENDING )
		{
			int tries = 0;
			//lprintf( "Triggered write event..." );
			while( tries++ < 20 && !NetworkLockEx( pc, 0 DBG_SRC ) ) {
				if( pc->writeTimer ) return; // while waiting to write, got more to aggregate.
				Relinquish();
			}
			if( tries >= 10 ){
				lprintf( "Failed to lock network? waiting in timer to generate write? %p %p", pc, pc->lpFirstPending );
				if( (!(pc->dwFlags & CF_ACTIVE )) || (pc->dwFlags & CF_TOCLOSE) ) 
					return;
				lprintf( "Use write on unlock flag to trigger write on unlock. %p", pc );
				pc->flags.bWriteOnUnlock = 1;
				return;
			}
			if( pc->dwFlags & CF_ACTIVE ) {
#ifdef LOG_WRITE_AGGREGATION
				lprintf( "locked and can write: %p %d %d", pc, timer, pc->writeTimer );
#endif				
				if( pc->writeTimer ) {
					NetworkUnlockEx( pc, 0 DBG_SRC );
					return;
				}
				TCPWrite( pc );
			}
#ifdef LOG_WRITE_AGGREGATION
			lprintf( "Unlocking after write... %p", pc );
#endif			
			NetworkUnlockEx( pc, 0 DBG_SRC );
		} else {
			lprintf( "Triggered write shouldn't have no pending...%p (Try Again?) %s", (POINTER)psv, NetworkExpandFlags(pc) );
			//RescheduleTimerEx( timer, 3 );
			//pc->writeTimer = timer;
		}
	}
}

static PDATAQUEUE pdqPendingWrites = NULL;
static PTHREAD writeWaitThread = NULL;

void clearPending( PCLIENT pc ) {
	INDEX i;
	struct PendingWrite pending;
	i = 0;
	while( PeekDataQueueEx( &pdqPendingWrites, struct PendingWrite*, &pending, i++ ) ){
		if( pending.pc == pc ) {
			pending.pc = NULL;
		}
	}
}

static LOGICAL hasPending( PCLIENT pc ) {
	INDEX i;
	struct PendingWrite pending;
	i = 0;
	while( PeekDataQueueEx( &pdqPendingWrites, struct PendingWrite*, &pending, i++ ) ){
		if( pending.pc == pc ) {
			lprintf( "Might have still been able to get the lock...");
			return TRUE;
		}
	}
	return FALSE;
}

uintptr_t WaitToWrite( PTHREAD thread ) {
	PDATALIST requeued = CreateDataList( sizeof( struct PendingWrite ) );
	INDEX lastCount = 0;
	if( !pdqPendingWrites )
		pdqPendingWrites = CreateDataQueue( sizeof( struct PendingWrite ) );
	//lprintf( "started waittowrite" );
	while( 1 ) {
		//uint32_t tick; tick = timeGetTime();
		if( lastCount == GetDataQueueLength( pdqPendingWrites ) || IsDataQueueEmpty( &pdqPendingWrites ) ) {
			//lprintf( "WaitToWrite sleeps..." );
			WakeableSleep( 10000 );
			/*
			uint32_t now; now = timeGetTime();
			if( now -tick < 9000 ) {
				lprintf( "woke to write writes %d", now-tick);
			}else {
				if( !IsDataQueueEmpty( &pdqPendingWrites ))
					lprintf( "delayed like 10 seconds and now checking writes");
			}
			*/
		} else {
			;//lprintf( "already have writes to check" );
		}
		struct PendingWrite pending;
		struct PendingWrite *lpPending = &pending;
#ifdef LOG_PENDING_WRITES
		lprintf( "WaitToWrite is checking for writes" );
#endif
		while( DequeData( &pdqPendingWrites, &pending ) ) {
			if( !pending.pc ) {
				Release( pending.buffer );
				lprintf( "Socket closed while data was pending");
				continue;
			}
#ifdef LOG_PENDING_WRITES
			lprintf( "Handling pending writes... %p %zd", pending.pc, pending.len );
#endif
			{
				INDEX idx;
				struct PendingWrite* lpPending;
				DATA_FORALL( requeued, idx, struct PendingWrite*, lpPending ) {
					if( lpPending->pc == pending.pc ) {
						lprintf( "socket is already pending, requeue more:%p", pending.pc );
						AddDataItem( &requeued, lpPending );
						break;
					}
				}
				if( lpPending ) {
					lprintf( "This was already found as pending, saved for requeue");
					continue;
				}
			}
			if( !( pending.pc->dwFlags & CF_ACTIVE )  || (pending.pc->dwFlags & CF_TOCLOSE) ) {
				if(pending.pc->dwFlags & CF_TOCLOSE) 
					lprintf( "Socket is intended to close already... %08x %p" , pending.pc->dwFlags, pending.pc );
				else
					lprintf( "Socket is is inactive already... %08x %p" , pending.pc->dwFlags, pending.pc );
				continue;
			}
			if( !NetworkLockEx( pending.pc, 0 DBG_SRC ) ) {
				if( !( pending.pc->dwFlags & CF_ACTIVE ) || (pending.pc->dwFlags & CF_TOCLOSE) ) {
					lprintf( "Pending write on socket can't happen, no longer active." );
					continue; // do not requeue this... the socket has closed.
				}
#ifdef LOG_PENDING_WRITES
				lprintf( "(pending writer)Failed to lock network... requeueing %p", pending.pc );
#endif
				AddDataItem( &requeued, &pending );
			} else {
#ifdef LOG_PENDING_WRITES
				LogBinary( (const uint8_t*)pending.buffer, pending.len );
				lprintf( "Send pending block %p %p %zd", pending.pc, pending.buffer, pending.len );
#endif
				LOGICAL stillPend = doTCPWriteV2( pending.pc, pending.buffer, pending.len, pending.bLong, pending.failpending, FALSE DBG_SRC );
				if( stillPend == -1 ) {
#ifdef LOG_PENDING_WRITES
					lprintf( "--- This should not happen - have the lock already... %08x", pending.pc->dwFlags );
#endif
					AddDataItem( &requeued, &pending );
				} else {
					if( !hasPending( pending.pc ))
						pending.pc->wakeOnUnlock = NULL;
#ifdef LOG_PENDING_WRITES
					else
						lprintf( "Still has pending writes... %p", pending.pc );
#endif
				}
				NetworkUnlockEx( lpPending->pc, 0|0x10 DBG_SRC );
			}
			;
			//i++;
		}

		{
			INDEX idx;
			struct PendingWrite* lpPending;
			DATA_FORALL( requeued, idx, struct PendingWrite*, lpPending ) {
#ifdef LOG_PENDING_WRITES
				lprintf( "Requeing block %p %p %zd", lpPending->pc, lpPending->buffer, lpPending->len );
#endif
				EnqueData( &pdqPendingWrites, lpPending );
				lpPending->pc->wakeOnUnlock = thread;
			}
			lastCount = idx;
			//lprintf( "Emptied requeue list...");
			EmptyDataList( &requeued );
		}

	}
}


LOGICAL doTCPWriteV2( PCLIENT lpClient
                     , CPOINTER pInBuffer
                     , size_t nInLen
                     , int bLongBuffer
                     , int failpending
                     , int pend_on_fail
                     DBG_PASS
                     )
{
	if( !lpClient )
	{
//#ifdef VERBOSE_DEBUG
		lprintf( "TCP Write failed - invalid client." );
//#endif
		return FALSE;  // cannot process a closed channel. data not sent.
	}

	while( ( pend_on_fail && lpClient->wakeOnUnlock/*hasPending(lpClient)*/ ) || !NetworkLockEx( lpClient, 0 DBG_SRC ) )
	{
#ifdef LOG_NETWORK_LOCKING
		if( lpClient->wakeOnUnlock )
			lprintf( "client is already waiting for wake on unlock? %p  %p", lpClient, lpClient->wakeOnUnlock);
#endif		
		if( (!(lpClient->dwFlags & CF_ACTIVE )) || (lpClient->dwFlags & CF_TOCLOSE) )
		{
#ifdef LOG_NETWORK_LOCKING
			_lprintf(DBG_RELAY)( "Failing send... inactive or closing" );
			LogBinary( (uint8_t*)pInBuffer, nInLen );
#endif
			return FALSE;
		}

		if( pend_on_fail ) {
			if( !writeWaitThread ) {
				writeWaitThread = ThreadTo( WaitToWrite, 0 );
				while( !pdqPendingWrites ) Relinquish();
			}


			struct PendingWrite pw;
			pw.pc = lpClient;
			if( bLongBuffer )
				pw.buffer = (POINTER)pInBuffer;
			else {
				pw.buffer = Allocate( nInLen );
				MemCpy( pw.buffer, pInBuffer, nInLen );
			}
			pw.len = nInLen;
			pw.bLong = bLongBuffer;
			// when re-queuing this don't want to pend the same packet again...
			pw.failpending = FALSE;//failpending;
#ifdef LOG_PENDING_WRITES		
			if( pw.len < 16 ) LogBinary( (uint8_t*)pw.buffer, pw.len );
			fprintf( stderr, DBG_FILELINEFMT "Saving buffer to queue %p %d %p %zd\n" DBG_RELAY, lpClient, bLongBuffer, pw.buffer, pw.len );
#endif			
			EnqueData( &pdqPendingWrites, &pw );
			lpClient->wakeOnUnlock = writeWaitThread;
			if( NetworkLockEx( lpClient, 0 DBG_SRC ) ) {
#ifdef LOG_PENDING_WRITES		
				lprintf( "Network lock would not have been unlocked (wakeOnUnlock)... %p", lpClient );
#endif				
				NetworkUnlockEx( lpClient, 0 DBG_SRC );
			}
#ifdef LOG_PENDING_WRITES		
			else lprintf( "pended on fail - queued buffer as pending... %p ", lpClient );
#endif			
		}
		return -1;
	}
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
	{
//#ifdef LOG_WRITE_AGGREGATION
		lprintf( "TCP Write failed - client is inactive" );
//#endif
		// change to inactive status by the time we got here...
		NetworkUnlockEx( lpClient, 0 DBG_SRC );
		return FALSE;
	}

	if( lpClient->lpFirstPending ) // will already be in a wait on network state...
	{
#ifdef LOG_WRITE_AGGREGATION
		_lprintf( DBG_RELAY )(  "Data already pending, pending buffer...%p %d", pInBuffer, nInLen );
#endif
		if( !failpending )
		{
#ifdef LOG_WRITE_AGGREGATION
			lprintf( "Queuing pending data anyhow..." );
#endif
			// this doesn't re-trigger sending; it assumes the network write-ready event will do that.
			PendWrite( lpClient, pInBuffer, nInLen, bLongBuffer );
			if( lpClient->flags.bAggregateOutput) 
			{
#ifdef LOG_WRITE_AGGREGATION
				_lprintf(DBG_RELAY)( "Write with aggregate... %d (timer?)%d %p", nInLen, lpClient->writeTimer, lpClient );
#endif				
				if( lpClient->writeTimer ) {
					RescheduleTimerEx( lpClient->writeTimer, 3 );
				} else lpClient->writeTimer = AddTimerExx( 3, 0, triggerWrite, (uintptr_t)lpClient DBG_SRC );
#ifdef LOG_WRITE_AGGREGATION
				_lprintf( DBG_RELAY )( "Write with aggregate... (timer?)%d", lpClient->writeTimer );
#endif				
			}
			lpClient->dwFlags |= CF_WRITEPENDING; // shouldn't have to do this - this there is a race condition somewhere...
			//TCPWriteEx( lpClient DBG_SRC ); // make sure we don't lose a write event during the queuing...
			NetworkUnlockEx( lpClient, 0 DBG_SRC );
			return TRUE;
		}
		else
		{
#ifdef VERBOSE_DEBUG
			lprintf( "Failing pend." );
#endif
			NetworkUnlockEx( lpClient, 0 DBG_SRC );
			return FALSE;
		}
	}
	else
	{
		// have to steal the buffer - :(
		
		lpClient->FirstWritePending.buffer.c   = pInBuffer;
		//lprintf( "First pending Write set to %d", nInLen );
		lpClient->FirstWritePending.dwAvail    = nInLen;
		lpClient->FirstWritePending.dwUsed     = 0;

		lpClient->FirstWritePending.s.bStream    = FALSE;
		lpClient->FirstWritePending.s.bDynBuffer = FALSE;
		lpClient->FirstWritePending.lpNext       = NULL;

		lpClient->lpLastPending =
		lpClient->lpFirstPending = &lpClient->FirstWritePending;
		if( lpClient->flags.bAggregateOutput) 
		{
			if( !bLongBuffer ) // caller will assume the buffer usable on return
			{
				lpClient->FirstWritePending.buffer.p = Allocate( nInLen );
				MemCpy( lpClient->FirstWritePending.buffer.p, pInBuffer, nInLen );
				lpClient->FirstWritePending.s.bDynBuffer = TRUE;
			}
#ifdef LOG_WRITE_AGGREGATION
			int wasTimer = lpClient->writeTimer;
#endif
			if( lpClient->writeTimer ) RescheduleTimerEx( lpClient->writeTimer, 3 ); 
			else lpClient->writeTimer = AddTimerExx( 3, 0, triggerWrite, (uintptr_t)lpClient DBG_SRC );
#ifdef LOG_WRITE_AGGREGATION
			_lprintf(DBG_RELAY)("Write with aggregate (firstpending forced)... %d %d %d %p", bLongBuffer, nInLen, lpClient->writeTimer, lpClient);
#endif			
			lpClient->dwFlags |= CF_WRITEPENDING;
			//lprintf( "First Write with aggregate... %d %d %d %p", nInLen, lpClient->writeTimer, wasTimer, lpClient );
			NetworkUnlockEx( lpClient, 0 DBG_SRC );
			return TRUE;
		}
		//lprintf("Sending immediate? %p", lpClient);
		if( TCPWriteEx( lpClient DBG_RELAY ) )
		{
#ifdef VERBOSE_DEBUG
			Log2( "Data not sent, pending buffer... %d bytes %d remain", nInLen, lpClient->FirstWritePending.dwAvail );
#endif
			if( !bLongBuffer ) // caller will assume the buffer usable on return
			{
				lpClient->FirstWritePending.buffer.p = Allocate( nInLen );
				MemCpy( lpClient->FirstWritePending.buffer.p, pInBuffer, nInLen );
				lpClient->FirstWritePending.s.bDynBuffer = TRUE;
			}
		}
#ifdef VERBOSE_DEBUG
		else
			_xlprintf( 1 DBG_RELAY )( "Data has been compeltely sent." );
#endif
	}
	NetworkUnlockEx( lpClient, 0 DBG_SRC );
	return TRUE; // assume the data was sent.
}

//----------------------------------------------------------------------------

#if 0 && !DrainSupportDeprecated
#define DRAIN_MAX_READ 2048
// used internally to read directly to drain buffer.
LOGICAL TCPDrainRead( PCLIENT pClient )
{
	size_t nDrainRead;
	char byBuffer[DRAIN_MAX_READ];
	while( pClient && pClient->nDrainLength )
	{
		nDrainRead = pClient->nDrainLength;
		if( nDrainRead > DRAIN_MAX_READ )
			nDrainRead = DRAIN_MAX_READ;
		nDrainRead = recv( pClient->Socket
							  , byBuffer
						 , (int)nDrainRead, 0 );
		if( nDrainRead == 0 )//SOCKET_ERROR )
		{
			uint32_t dwError;
			dwError = WSAGetLastError();
			if( dwError == WSAEWOULDBLOCK )
			{
				if( !pClient->bDrainExact )
					pClient->nDrainLength = 0;
				break;
			}
			lprintf(" Network Error during drain: %d (from: %p  to: %p  has: %" _size_f "  toget: %" _size_f ")"
			       , dwError
			       , pClient->Socket
			       , pClient->RecvPending.buffer.p
			       , pClient->RecvPending.dwUsed
			       , pClient->RecvPending.dwAvail );
			InternalRemoveClient( pClient );
			NetworkUnlockEx( pClient, 1 DBG_SRC );
			return FALSE;
		}
		else
		{
			if( globalNetworkData.flags.bShortLogReceivedData )
			{
				LogBinary( (uint8_t*)byBuffer, (nDrainRead<64 )?nDrainRead:64 );
			}
			if( globalNetworkData.flags.bLogReceivedData )
			{
				LogBinary( (uint8_t*)byBuffer, nDrainRead );
			}
		}
		if( nDrainRead == 0 )
		{
			InternalRemoveClient( pClient ); // closed.
			NetworkUnlockEx( pClient, 1 DBG_SRC );
			return FALSE;
		}
		if( pClient->bDrainExact )
			pClient->nDrainLength -= nDrainRead;
	}
	if( pClient )
		return pClient->bDraining = (pClient->nDrainLength != 0);
	return 0; // no data available....
}

//----------------------------------------------------------------------------

NETWORK_PROC( LOGICAL, TCPDrainEx)( PCLIENT pClient, size_t nLength, int bExact )
{
	if( pClient )
	{
		LOGICAL bytes;
		while( !NetworkLockEx( pClient, 0 DBG_SRC ) )
		{
			if( !(pClient->dwFlags & CF_ACTIVE ) )
			{
				return FALSE;
			}
			Relinquish();
		}
		if( pClient->bDraining )
		{
			pClient->nDrainLength += nLength;
		}
		else
		{
			pClient->bDraining = TRUE;
			pClient->nDrainLength = nLength;
		}
		pClient->bDrainExact = bExact;
		if( !pClient->bDrainExact )
			pClient->nDrainLength = DRAIN_MAX_READ; // default optimal read
		bytes = TCPDrainRead( pClient );
		NetworkUnlockEx( pClient, 1 DBG_SRC );
		return bytes;
	}
	return 0;
}
#endif

//----------------------------------------------------------------------------

void SetTCPNoDelay( PCLIENT pClient, int bEnable )
{
	if( setsockopt( pClient->Socket, IPPROTO_TCP,
						TCP_NODELAY,
						(const char *)&bEnable, sizeof(bEnable) ) == SOCKET_ERROR )
	{
		lprintf( "Error(%d) setting Delay to : %d", WSAGetLastError(), bEnable );
		// log some sort of error... and ignore...
	}
}

//----------------------------------------------------------------------------

void SetClientKeepAlive( PCLIENT pClient, int bEnable )
{
	if( setsockopt( pClient->Socket, SOL_SOCKET,
						SO_KEEPALIVE,
						(const char *)&bEnable, sizeof(bEnable) ) == SOCKET_ERROR )
	{
		lprintf( "Error(%d) setting KeepAlive to : %d", WSAGetLastError(), bEnable );
		// log some sort of error... and ignore...
	}
}

#ifndef __LINUX__
#  undef ioctl 
#endif

#undef doTCPWriteExx
LOGICAL doTCPWriteExx( PCLIENT lpClient
	, CPOINTER pInBuffer
	, size_t nInLen
	, int bLongBuffer
	, int failpending
	DBG_PASS
) {
	return doTCPWriteV2( lpClient, pInBuffer, nInLen, bLongBuffer, failpending, 1 DBG_SRC );
}
#define doTCPWriteExx( c,b,l,f1,f2,fop,...) doTCPWriteV2( (c),(b),(l),(f1),(f2),(fop),##__VA_ARGS__ )


SACK_NETWORK_TCP_NAMESPACE_END
