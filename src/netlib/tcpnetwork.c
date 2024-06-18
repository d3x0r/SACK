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

void SetNetworkListenerReady( PCLIENT pListen ) {
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
	pNewClient->flags.bSecure = pListen->flags.bSecure;
	pNewClient->flags.bAllowDowngrade = pListen->flags.bAllowDowngrade;
	pNewClient->Socket = accept( pListen->Socket
										, pNewClient->saClient
										,&nTemp
										);
	SET_SOCKADDR_LENGTH( pNewClient->saClient, pNewClient->saClient->sa_family == AF_INET6?IN6_SOCKADDR_LENGTH:IN_SOCKADDR_LENGTH );

	//lprintf( "Accept new client...%p %d", pNewClient, pNewClient->Socket );
#ifdef WIN32
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
#ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "New event on accepted %p", pNewClient->event );
#endif
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
			if( pListen->connect.ClientConnected )
			{
				if( pListen->dwFlags & CF_CPPCONNECT )
					pListen->connect.CPPClientConnected( pListen->psvConnect, pNewClient );
				else
					pListen->connect.ClientConnected( pListen, pNewClient );
			}

			// signal initial read.
			//lprintf(" Initial notifications...");
			pNewClient->dwFlags |= CF_READREADY; // may be... at least we can fail sooner...

			if( pNewClient->read.ReadComplete )
			{
				if( pListen->dwFlags & CF_CPPREAD )
					pNewClient->read.CPPReadComplete( pNewClient->psvRead, NULL, 0 );  // process read to get data already pending...
				else
					pNewClient->read.ReadComplete( pNewClient, NULL, 0 );  // process read to get data already pending...
			}
			/*
			* really don't need a write complete on initial open... the initial read should suffice.
			if( pNewClient->write.WriteComplete  &&
				!pNewClient->bWriteComplete )
			{
				pNewClient->bWriteComplete = TRUE;
				if( pNewClient->dwFlags & CF_CPPWRITE )
					pNewClient->write.CPPWriteComplete( pNewClient->psvWrite, NULL, 0 );
				else
					pNewClient->write.WriteComplete( pNewClient, NULL, 0 );
				pNewClient->bWriteComplete = FALSE;
			}
			*/
			//lprintf( "Is it already closed HERE?""?""?");
			if( pNewClient->Socket ) {
#ifdef USE_WSA_EVENTS
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "SET GLOBAL EVENT (accepted socket added)  %p  %p", pNewClient, pNewClient->event );
				EnqueLink( &globalNetworkData.client_schedule, pNewClient );
				WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
				AddThreadEvent( pNewClient, 0 );
#endif
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

PCLIENT CPPOpenTCPListenerAddr_v2d( SOCKADDR *pAddr
                                 , cppNotifyCallback NotifyCallback
                                 , uintptr_t psvConnect
                                 , LOGICAL waitForReady
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
	//	pListen->Socket = socket( *(uint16_t*)pAddr, SOCK_STREAM, 0 );
#ifdef WIN32
	pListen->Socket = OpenSocket( ((*(uint16_t*)pAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
	if( pListen->Socket == INVALID_SOCKET )
#endif
#ifdef __MAC__
		pListen->Socket = socket( ((uint8_t*)pAddr)[1]
										, SOCK_STREAM
										, ((((uint8_t*)pAddr)[1] == AF_INET)||((((uint8_t*)pAddr)[1]) == AF_INET6))?IPPROTO_TCP:0 );
#else
		pListen->Socket = socket( *(uint16_t*)pAddr
										, SOCK_STREAM
										, (((*(uint16_t*)pAddr) == AF_INET)||((*(uint16_t*)pAddr) == AF_INET6))?IPPROTO_TCP:0 );
#endif
#ifdef LOG_SOCKET_CREATION
	lprintf( "Created new socket %d", pListen->Socket );
#endif
	pListen->dwFlags &= ~CF_UDP; // make sure this flag is clear!
	pListen->dwFlags |= CF_LISTEN;
	pListen->flags.bWaiting = waitForReady;
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

#ifdef WIN32
	SetHandleInformation( (HANDLE)pListen->Socket, HANDLE_FLAG_INHERIT, 0 );
#else
	{
		int flags = fcntl( pListen->Socket, F_GETFL, 0 );
		fcntl( pListen->Socket, F_SETFL, O_NONBLOCK );
	}
	{ 
		int flags = fcntl( pListen->Socket, F_GETFD, 0 );
		if( flags >= 0 ) fcntl( pListen->Socket, F_SETFD, flags | FD_CLOEXEC );
	}
#endif


#ifdef _WIN32
#  ifdef USE_WSA_EVENTS
	pListen->event = WSACreateEvent();
	WSAEventSelect( pListen->Socket, pListen->event, FD_ACCEPT|FD_CLOSE );
#  else
	if( WSAAsyncSelect( pListen->Socket, globalNetworkData.ghWndNetwork,
                       SOCKMSG_TCP, FD_ACCEPT|FD_CLOSE ) )
	{
		lprintf( "Windows AsynchSelect failed: %d", WSAGetLastError() );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pListen, 0 DBG_SRC );
		NetworkUnlockEx( pListen, 1 DBG_SRC );
		return NULL;
	}
#  endif
	{
		int t = FALSE;
		setsockopt( pListen->Socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&t, 4 );
	}
#else
	{
		int t = TRUE;
		setsockopt( pListen->Socket, SOL_SOCKET, SO_REUSEADDR, &t, 4 );
		fcntl( pListen->Socket, F_SETFL, O_NONBLOCK );
	}
#  ifdef SO_REUSEPORT
	{
		int t = TRUE;
		setsockopt( pListen->Socket, SOL_SOCKET, SO_REUSEPORT, &t, 4 );
	}
#  endif
#endif
#ifndef _WIN32
	if( pAddr->sa_family==AF_UNIX )
		unlink( (char*)(((uint16_t*)pAddr)+1));
#endif

	if (!pAddr ||
		 bind(pListen->Socket ,pAddr, SOCKADDR_LENGTH( pAddr ) ) )
	{
		_lprintf(DBG_RELAY)( "Cannot bind to address..:%d", WSAGetLastError() );
		DumpAddr( "Bind address:", pAddr );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pListen, 0 DBG_SRC );
		NetworkUnlockEx( pListen, 1 DBG_SRC );
		return NULL;
	}
	pListen->saSource = DuplicateAddress( pAddr );

	if(listen(pListen->Socket, SOMAXCONN ) == SOCKET_ERROR )
	{
		lprintf( "listen(5) failed: %d", WSAGetLastError() );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( pListen, 0 DBG_SRC );
		NetworkUnlockEx( pListen, 1 DBG_SRC );
		return NULL;
	}
	pListen->connect.CPPClientConnected = NotifyCallback;
	pListen->psvConnect = psvConnect;
	pListen->dwFlags |= CF_CPPCONNECT;
	NetworkUnlockEx( pListen, 0 DBG_SRC );
	NetworkUnlockEx( pListen, 1 DBG_SRC );
	AddActive( pListen );
   // make sure to schedule this socket for events (connect)
#ifdef USE_WSA_EVENTS
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "SET GLOBAL EVENT (listener added)" );
	EnqueLink( &globalNetworkData.client_schedule, pListen );
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
	EnterCriticalSec( &globalNetworkData.csNetwork );
	AddThreadEvent( pListen, 0 );
	LeaveCriticalSec( &globalNetworkData.csNetwork );
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
	if( result )
		result->dwFlags &= ~CF_CPPCONNECT;
	return result;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPListenerAddrExx( SOCKADDR *pAddr
                              , cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerAddr_v2d( pAddr, (cppNotifyCallback)NotifyCallback, 0, FALSE DBG_RELAY );
	if( result )
		result->dwFlags &= ~CF_CPPCONNECT;
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
	PCLIENT pc = CPPOpenTCPListenerAddrExx( lpMyAddr, NotifyCallback, psvConnect DBG_RELAY );
	ReleaseAddress( lpMyAddr );
	if( pc )
	{
		// have to have the base one open or pcOther cannot be set.
		lpMyAddr = CreateSockAddress( ":::", wPort );
		pc->pcOther = CPPOpenTCPListenerAddrExx( lpMyAddr, NotifyCallback, psvConnect DBG_RELAY );
		ReleaseAddress( lpMyAddr );
	}
	return pc;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPListenerExx(uint16_t wPort, cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerExx( wPort, (cppNotifyCallback)NotifyCallback, 0 DBG_RELAY );
	if( result )
		result->dwFlags &= ~CF_CPPCONNECT;
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
	//DumpAddr( "Connect to", &pResult->saClient );
	if( (err = connect( pc->Socket, pc->saClient
		, SOCKADDR_LENGTH( pc->saClient ) )) )
	{
		uint32_t dwError;
		dwError = WSAGetLastError();
		if( dwError != WSAEWOULDBLOCK
#ifdef __LINUX__
			&& dwError != EINPROGRESS
#else
			&& dwError != WSAEINPROGRESS
#endif
			)
		{
			if( pc->connect.CPPThisConnected ) {
				if( pc->dwFlags & CF_CPPCONNECT )
					pc->connect.CPPThisConnected( pc->psvConnect, dwError );
				else
					pc->connect.ThisConnected( pc, dwError );
			}
			_lprintf( DBG_RELAY )("Connect FAIL: %d %d %" _32f, pc->Socket, err, dwError);
			EnterCriticalSec( &globalNetworkData.csNetwork );
			InternalRemoveClientEx( pc, TRUE, FALSE );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			NetworkUnlockEx( pc, 0 DBG_SRC );
			pc = NULL;
			return -1;
		}
		else
		{
			//lprintf( "Pending connect has begun..." );
		}
	}
	else
	{
#ifdef VERBOSE_DEBUG
		lprintf( "Connected before we even get a chance to wait" );
#endif
	}
	NetworkUnlockEx( pc, 0 DBG_SRC );

	return 0;
}

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
#ifdef WIN32
		pResult->Socket = OpenSocket( ((*(uint16_t*)lpAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
		if( pResult->Socket == INVALID_SOCKET )
#endif
#ifdef __MAC__
			pResult->Socket=socket( ((uint8_t*)lpAddr)[1]
			                      , SOCK_STREAM
			                      , (((((uint8_t*)lpAddr)[1]) == AF_INET)||((((uint8_t*)lpAddr)[1]) == AF_INET6))?IPPROTO_TCP:0 );
#else
			pResult->Socket=socket( *(uint16_t*)lpAddr
			                      , SOCK_STREAM
			                      , (((*(uint16_t*)lpAddr) == AF_INET)||((*(uint16_t*)lpAddr) == AF_INET6))?IPPROTO_TCP:0 );
#endif
#ifdef LOG_SOCKET_CREATION
		lprintf( "Created new socket %p %d", pResult, pResult->Socket );
#endif
		if (pResult->Socket==INVALID_SOCKET)
		{
			lprintf( "Create socket failed. %d", WSAGetLastError() );
			EnterCriticalSec( &globalNetworkData.csNetwork );
			InternalRemoveClientEx( pResult, TRUE, FALSE );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			NetworkUnlockEx( pResult, 1 DBG_SRC );
			NetworkUnlockEx( pResult, 0 DBG_SRC );
			return NULL;
		}
		else
		{
			int err;
#ifdef _WIN32
			if( 0 )
			{
				DWORD dwFlags;
				GetHandleInformation( (HANDLE)pResult->Socket, &dwFlags );
				lprintf( "Natural was %d", dwFlags );
			}
			SetHandleInformation( (HANDLE)pResult->Socket, HANDLE_FLAG_INHERIT, 0 );
#  ifdef USE_WSA_EVENTS
			pResult->event = WSACreateEvent();
#    ifdef LOG_NETWORK_EVENT_THREAD
			lprintf( "new event is %p", pResult->event );
#    endif
			WSAEventSelect( pResult->Socket, pResult->event, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE );
#  else
			if( WSAAsyncSelect( pResult->Socket,globalNetworkData.ghWndNetwork,SOCKMSG_TCP
			                  , FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT) )
			{
				lprintf( " Select NewClient Fail! %d", WSAGetLastError() );
				EnterCriticalSec( &globalNetworkData.csNetwork );
				InternalRemoveClientEx( pResult, TRUE, FALSE );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				NetworkUnlockEx( pResult, 1 DBG_SRC );
				NetworkUnlockEx( pResult, 0 DBG_SRC );
				pResult = NULL;
				goto LeaveNow;
			}
#  endif
#else
			{ 
				int flags = fcntl( pResult->Socket, F_GETFD, 0 );
				if( flags >= 0 ) fcntl( pResult->Socket, F_SETFD, flags | FD_CLOEXEC );
			}
			{
				int flags = fcntl( pResult->Socket, F_GETFL, 0 );
				fcntl( pResult->Socket, F_SETFL, O_NONBLOCK );
			}
#endif
			if( pFromAddr )
			{

				LOGICAL opt = 1;
				err = setsockopt( pResult->Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof( opt ) );
				if( err )
				{
					uint32_t dwError = WSAGetLastError();
					lprintf( "Failed to set socket option REUSEADDR : %d", dwError );
				}
				pResult->saSource = DuplicateAddress( pFromAddr );
				//DumpAddr( "source", pResult->saSource );
				if( ( err = bind( pResult->Socket, pResult->saSource
				                , SOCKADDR_LENGTH( pResult->saSource ) ) ) )
				{
					uint32_t dwError;
					dwError = WSAGetLastError();
					lprintf( "Error binding connecting socket to source address... continuing with connect : %d", dwError );
				}
			}

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

			// socket should now get scheduled for events, after unlocking it?
#ifdef USE_WSA_EVENTS
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "SET GLOBAL EVENT (wait for connect) new %p %p  %08x %p", pResult, pResult->event, pResult->dwFlags, globalNetworkData.hMonitorThreadControlEvent );
			EnqueLink( &globalNetworkData.client_schedule, pResult );
			if( this_thread == globalNetworkData.root_thread ) {
				ProcessNetworkMessages( this_thread, 1 );
				if( !pResult->this_thread ) {
					WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
					//lprintf( "Failed to schedule myself in a single run of root thread that I am running on." );
				}
			}
			else {
				int tries = 0;
				WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
				while( !pResult->this_thread ) {
					INDEX idx;
					POINTER client;
					IdleFor(1); // wait for it to be added to waiting lists?
					if( tries++ > 10 ) {
						tries = 0;
						for( idx = 0; client = PeekQueueEx( globalNetworkData.client_schedule, (int)idx); idx++ ){
							if( client == pResult ) break;
						}
						if( !pResult->this_thread && !client ) {
							lprintf( "Lost client in schedule list:%p (Requeuing)", pResult );
							EnqueLink( &globalNetworkData.client_schedule, pResult );
							WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
						}
					}
				}
				//if( tries > 1 ) lprintf( "Took %d tries.", tries );
			}

#endif
#ifdef __LINUX__
			AddThreadEvent( pResult, 0 );
#endif
			if (!(flags & OPEN_TCP_FLAG_DELAY_CONNECT)) {
				NetworkConnectTCPEx(pResult DBG_RELAY);
			}
			if( !pConnectComplete && !(flags & OPEN_TCP_FLAG_DELAY_CONNECT) )
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
					pResult = NULL;
					goto LeaveNow;
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
					if( getsockname( pResult->Socket, pResult->saSource, &nLen ) )
					{
						lprintf( "getsockname errno = %d", errno );
					}
				}
				//lprintf( "Connect did complete... returning to application");
			}
#ifdef VERBOSE_DEBUG
			else
				lprintf( "Connect in progress, will notify application when done." );
#endif
			pResult->dwFlags &= ~CF_CONNECT_WAITING;
		}
	}

LeaveNow:
	if( !pResult )  // didn't break out of the loop with a good return.
	{
		//lprintf( "Failed Open TCP Client." );
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
	if( lpName &&
	   (lpsaDest = CreateSockAddress(lpName,wPort) ) )
	{
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
	} else if( !lpsaDest && pConnectComplete ) {
#ifdef WIN32
		pConnectComplete( psvConnect, globalNetworkData.lastAddressError );
#else
		pConnectComplete( psvConnect, errno );
#endif
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
	if( lpName &&
		(lpsaDest = CreateSockAddress(lpName,wPort) ) )
	{
		pClient = OpenTCPClientAddrExxx( lpsaDest
		                               , pReadComplete
		                               , CloseCallback
		                               , WriteComplete
		                               , pConnectComplete
		                               , flags DBG_RELAY );
		ReleaseAddress( lpsaDest );
	} else {
		if( !lpsaDest && pConnectComplete ) {
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
			//lprintf( "Network receive %p %d %d %d", lpClient, nRecv, lpClient->RecvPending.dwUsed, lpClient->RecvPending.dwAvail );
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
						Log5( "Failed reading from %d (err:%d) into %p %" _size_f " bytes %" _size_f " read already.",
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
					lprintf( "Send to application...." );
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
					lprintf( "back from applciation... (loop to next)" ); // new read probably pending ehre...
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
#ifdef LOG_PENDING
	{
		lprintf( "Pending %d Bytes to network..." , nLen );
	}
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
		return TRUE;
	while (pc->lpFirstPending)
	{
#ifdef VERBOSE_DEBUG
		if( pc->dwFlags & CF_CONNECTING )
			lprintf( "Sending previously queued data." );
#endif

		if( pc->lpFirstPending->dwAvail )
		{
  			uint32_t dwError;
			if( pc->flags.bAggregateOutput && pc->lpFirstPending->lpNext ){
				uint32_t size = 0;
				uint32_t blocks = 0;
				PendingBuffer *lpNext = pc->lpFirstPending;
				// collapse all pending into first pending block.
				while( lpNext ) { size += lpNext->dwAvail; blocks++; lpNext = lpNext->lpNext;}
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
					lprintf( "Pending write..." );
#endif
					pc->dwFlags &= ~CF_WRITEREADY;
					pc->dwFlags |= CF_WRITEPENDING;
					return TRUE;
				}
				if( dwError == EPIPE ) {
					_lprintf(DBG_RELAY)( "EPIPE on send() to socket...");
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
					return FALSE; // get out of here!
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
		}
		else
			nSent = 0;

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
	return FALSE; // 0 = everything sent / nothing left to send...
}

//----------------------------------------------------------------------------

void SetTCPWriteAggregation( PCLIENT pc, int bAggregate )
{
	pc->flags.bAggregateOutput = bAggregate;
}

static void triggerWrite( uintptr_t psv ){
	PCLIENT pc = (PCLIENT)psv;
	//lprintf( "Timer fired %p", psv );
	if( pc )
	{
		int32_t timer = pc->writeTimer;
		pc->writeTimer = 0;
		if( pc->dwFlags & CF_WRITEPENDING )
		{
			int tries = 0;
			//lprintf( "Triggered write event..." );
			while( tries++ < 20 && !NetworkLockEx( pc, 0 DBG_SRC ) ) {
				Relinquish();
			}
			if( tries >= 20 ){
				lprintf( "Failed to lock network? waiting in timer to generate write? %p", pc );
				if( (!(pc->dwFlags & CF_ACTIVE )) || (pc->dwFlags & CF_TOCLOSE) ) 
					return;
				lprintf( "Use write on unlock flag to trigger write on unlock. %p", pc );
				pc->flags.bWriteOnUnlock = 1;
				return;
			}
			if( pc->dwFlags & CF_ACTIVE )
					TCPWrite( pc );
			NetworkUnlockEx( pc, 0 DBG_SRC );
		} else {
			lprintf( "Triggered write shouldn't hve no pending...%p (Try Again?)", psv );
			//RescheduleTimerEx( timer, 3 );
			//pc->writeTimer = timer;
		}
	}
}


static PDATAQUEUE pdqPendingWrites = NULL;
static PTHREAD writeWaitThread = NULL;

uintptr_t WaitToWrite( PTHREAD thread ) {
	PLIST requeued = NULL;
	if( !pdqPendingWrites )
		pdqPendingWrites = CreateDataQueue( sizeof( struct PendingWrite ) );
	while( 1 ) {
		WakeableSleep( 100 );
		struct PendingWrite pending;
		struct PendingWrite *lpPending = &pending;
		INDEX i;
		i = 0;
		while( PeekDataQueueEx( &pdqPendingWrites, struct PendingWrite*, &pending, i ) ){
			if( !NetworkLockEx( pending.pc, 0 DBG_SRC ) ) {
				if( !requeued || !FindLink( &requeued, pending.pc ) ) {
					AddLink( &requeued, pending.pc );
				}
				i++;
				continue;
			}
			LOGICAL stillPend = doTCPWriteExx( pending.pc, pending.buffer, pending.len, pending.bLong, pending.failpending DBG_SRC );
			if( stillPend == -1 ) {
				lprintf( "--- This should not happen - have the lock already..." );
				AddLink( &requeued, lpPending->pc );
				break;  // this might have beeen await for another write?
			}
			else 
				lpPending->pc->wakeOnUnlock = NULL;
			NetworkUnlockEx( lpPending->pc, 0 DBG_SRC );
			DequeData( &pdqPendingWrites, &pending );
			i++;
		}
		EmptyList( &requeued );
	}
}

LOGICAL doTCPWriteExx( PCLIENT lpClient
                     , CPOINTER pInBuffer
                     , size_t nInLen
                     , int bLongBuffer
                     , int failpending
                     DBG_PASS
                     )
{
	if( !lpClient )
	{
#ifdef VERBOSE_DEBUG
		lprintf( "TCP Write failed - invalid client." );
#endif
		return FALSE;  // cannot process a closed channel. data not sent.
	}

	while( !NetworkLockEx( lpClient, 0 DBG_SRC ) )
	{
		if( (!(lpClient->dwFlags & CF_ACTIVE )) || (lpClient->dwFlags & CF_TOCLOSE) )
		{
#ifdef LOG_NETWORK_LOCKING
			_lprintf(DBG_RELAY)( "Failing send... inactive or closing" );
			LogBinary( (uint8_t*)pInBuffer, nInLen );
#endif
			return FALSE;
		}
		if( !writeWaitThread) 
			writeWaitThread = ThreadTo( WaitToWrite, 0 );


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
		pw.failpending = failpending;
		EnqueData( &pdqPendingWrites, &pw );
		lpClient->wakeOnUnlock = writeWaitThread;
		return -1;
		Relinquish();
	}
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
	{
#ifdef VERBOSE_DEBUG
		lprintf( "TCP Write failed - client is inactive" );
#endif
		// change to inactive status by the time we got here...
		NetworkUnlockEx( lpClient, 0 DBG_SRC );
		return FALSE;
	}

	if( lpClient->lpFirstPending ) // will already be in a wait on network state...
	{
#ifdef VERBOSE_DEBUG
		_lprintf( DBG_RELAY )(  "Data already pending, pending buffer...%p %d", pInBuffer, nInLen );
#endif
		if( !failpending )
		{
#ifdef VERBOSE_DEBUG
			lprintf( "Queuing pending data anyhow..." );
#endif
			// this doesn't re-trigger sending; it assumes the network write-ready event will do that.
			PendWrite( lpClient, pInBuffer, nInLen, bLongBuffer );
			if( lpClient->flags.bAggregateOutput) 
			{
				//_lprintf(DBG_RELAY)( "Write with aggregate... %d %d %p", nInLen, lpClient->writeTimer, lpClient );
				if( lpClient->writeTimer ) {
					RescheduleTimerEx( lpClient->writeTimer, 3 );
				} else lpClient->writeTimer = AddTimerExx( 3, 0, triggerWrite, (uintptr_t)lpClient DBG_SRC );
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
			int wasTimer = lpClient->writeTimer;
			//_lprintf(DBG_RELAY)( "Write with aggregate... %d %d %d %p", bLongBuffer, nInLen, lpClient->writeTimer, lpClient );
			if( lpClient->writeTimer ) RescheduleTimerEx( lpClient->writeTimer, 3 ); 
			else lpClient->writeTimer = AddTimerExx( 3, 0, triggerWrite, (uintptr_t)lpClient DBG_SRC );
			lpClient->dwFlags |= CF_WRITEPENDING;
			//lprintf( "First Write with aggregate... %d %d %d %p", nInLen, lpClient->writeTimer, wasTimer, lpClient );
			NetworkUnlockEx( lpClient, 0 DBG_SRC );
			return TRUE;
		}
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

SACK_NETWORK_TCP_NAMESPACE_END
