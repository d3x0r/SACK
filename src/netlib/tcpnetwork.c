// DEBUG FALGS in netstruc.h


//TODO: after the connect and just before the call to the connect callback fill in the PCLIENT's MAC addr field.
//TODO: After the accept, put in this code:
/*

NETWORK_PROC( int, GetMacAddress)(CTEXTSTR device, CTEXTSTR buffer )//int get_mac_addr (char *device, unsigned char *buffer)
{
int fd;
struct ifreq ifr;

fd = socket(PF_UNIX, SOCK_DGRAM, 0);
if (fd == -1)
{
perr ("Unable to create socket for device: %s", device);
return -1;
}

strcpy (ifr.ifr_name, device);

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

memcpy (buffer, ifr.ifr_hwaddr.sa_data, 6);

return 0;
}

*/
//#define LOG_SOCKET_CREATION
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

#undef s_addr
#include <netinet/in.h> // IPPROTO_TCP
//#include <linux/in.h>  // IPPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELAY
//#include <linux/tcp.h> // TCP_NODELAY
#include <fcntl.h>

#else
#endif

#include <sys/ioctl.h>
#include <signal.h> // SIGHUP defined

#define NetWakeSignal SIGHUP

#else
#define ioctl ioctlsocket

#endif

#include <sharemem.h>
#include <procreg.h>

//#define NO_LOGGING // force neverlog....
#include "logging.h"

SACK_NETWORK_NAMESPACE
	extern int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check );

_TCP_NAMESPACE

//----------------------------------------------------------------------------
LOGICAL TCPDrainRead( PCLIENT pClient );
//----------------------------------------------------------------------------

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
	if( !pNewClient )
	{
		SOCKADDR *junk = AllocAddr();
		nTemp = MAGIC_SOCKADDR_LENGTH;
		lprintf(WIDE( "GetFreeNetwork() returned NULL. Exiting AcceptClient, accept and drop connection" ));
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
   //lprintf( "Accept new client....%d", pNewClient->Socket );
#if WIN32
	SetHandleInformation( (HANDLE)pNewClient->Socket, HANDLE_FLAG_INHERIT, 0 );
#endif

#ifdef LOG_SOCKET_CREATION
	Log2( WIDE("Accepted socket %d  (%d)"), pNewClient->Socket, nTemp );
#endif
	//DumpAddr( WIDE("Client's Address"), pNewClient->saClient );
	{
#ifdef __LINUX__
		socklen_t
#else
			int
#endif
			nLen = MAGIC_SOCKADDR_LENGTH;
		if( !pNewClient->saSource )
			pNewClient->saSource = AllocAddr();
		if( getsockname( pNewClient->Socket, pNewClient->saSource, &nLen ) )
		{
			lprintf( WIDE("getsockname errno = %d"), errno );
		}
		//lprintf( "sockaddrlen: %d", nLen );
		if( pNewClient->saSource->sa_family == AF_INET )
			SET_SOCKADDR_LENGTH( pNewClient->saSource, IN_SOCKADDR_LENGTH );
		else if( pNewClient->saSource->sa_family == AF_INET6 )
			SET_SOCKADDR_LENGTH( pNewClient->saSource, IN6_SOCKADDR_LENGTH );
		else
			SET_SOCKADDR_LENGTH( pNewClient->saSource, nLen );
	}
	pNewClient->read.ReadComplete = pListen->read.ReadComplete;
	pNewClient->psvRead = pListen->psvRead;
	pNewClient->close.CloseCallback     = pListen->close.CloseCallback;
	pNewClient->psvClose = pListen->psvClose;
	pNewClient->write.WriteComplete     = pListen->write.WriteComplete;
	pNewClient->psvWrite = pListen->psvWrite;
	pNewClient->dwFlags |= CF_CONNECTED | ( pListen->dwFlags & CF_CALLBACKTYPES );
	if( IsValid(pNewClient->Socket) )
	{ // and we get one from the accept...
#ifdef _WIN32
#  ifdef USE_WSA_EVENTS
		pNewClient->event = WSACreateEvent();
#ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "New event on accepted %p", pNewClient->event );
#endif
		WSAEventSelect( pNewClient->Socket, pNewClient->event, /*FD_ACCEPT| FD_OOB| FD_CONNECT| FD_QOS| FD_GROUP_QOS| FD_ROUTING_INTERFACE_CHANGE| FD_ADDRESS_LIST_CHANGE|*/
			FD_READ|FD_WRITE|FD_CLOSE );
#  else
		if( WSAAsyncSelect( pNewClient->Socket,globalNetworkData.ghWndNetwork, SOCKMSG_TCP
		                  , FD_READ | FD_WRITE | FD_CLOSE))
		{ // if there was a select error...
			lprintf(WIDE( " Accept select Error" ));
			InternalRemoveClientEx( pNewClient, TRUE, FALSE );
			NetworkUnlockEx( pNewClient DBG_SRC );
			pNewClient = NULL;
		}
		else
#  endif
#else
		// yes this is an ugly transition from the above dangling
		// else...
			fcntl( pNewClient->Socket, F_SETFL, O_NONBLOCK );
#endif
		AddActive( pNewClient );
		{ 
			//lprintf( WIDE("Accepted and notifying...") );
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

			if( pNewClient->write.WriteComplete  &&
				!pNewClient->bWriteComplete )
			{
				pNewClient->bWriteComplete = TRUE;
				if( pNewClient->dwFlags & CF_CPPWRITE )
					pNewClient->write.CPPWriteComplete( pNewClient->psvWrite );
				else
					pNewClient->write.WriteComplete( pNewClient );
				pNewClient->bWriteComplete = FALSE;
			}
			NetworkUnlockEx( pNewClient DBG_SRC );
		}
		if( pNewClient->Socket ) {
#ifdef USE_WSA_EVENTS
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "SET GLOBAL EVENT (accepted socket added)  %p  %p" ), pNewClient, pNewClient->event );
			EnqueLink( &globalNetworkData.client_schedule, pNewClient );
			WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
			AddThreadEvent( pNewClient, 0 );
#endif
		}
	}
	else // accept failed...
	{
		InternalRemoveClientEx( pNewClient, TRUE, FALSE );
		NetworkUnlockEx( pNewClient DBG_SRC );
		pNewClient = NULL;
	}

	if( !pNewClient )
	{
		lprintf(WIDE( "Failed Accept..." ));
	}
}

//----------------------------------------------------------------------------

PCLIENT CPPOpenTCPListenerAddrExx( SOCKADDR *pAddr
                                 , cppNotifyCallback NotifyCallback
                                 , uintptr_t psvConnect
                                 DBG_PASS )
{
	PCLIENT pListen;
	if( !pAddr )
		return NULL;
	pListen = GetFreeNetworkClient();
	if( !pListen )
	{
		lprintf( WIDE("Network has not been started.") );
		return NULL;
	}
	//	pListen->Socket = socket( *(uint16_t*)pAddr, SOCK_STREAM, 0 );
#ifdef WIN32
	pListen->Socket = OpenSocket( ((*(uint16_t*)pAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
	if( pListen->Socket == INVALID_SOCKET )
#endif
		pListen->Socket = socket( *(uint16_t*)pAddr
										, SOCK_STREAM
										, (((*(uint16_t*)pAddr) == AF_INET)||((*(uint16_t*)pAddr) == AF_INET6))?IPPROTO_TCP:0 );
#if WIN32
	SetHandleInformation( (HANDLE)pListen->Socket, HANDLE_FLAG_INHERIT, 0 );
#endif

#ifdef LOG_SOCKET_CREATION
	lprintf( WIDE( "Created new socket %d" ), pListen->Socket );
#endif
	pListen->dwFlags &= ~CF_UDP; // make sure this flag is clear!
	pListen->dwFlags |= CF_LISTEN;
	if( pListen->Socket == INVALID_SOCKET )
	{
		lprintf( WIDE(" Open Listen Socket Fail... %d"), errno);
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlockEx( pListen DBG_SRC );
		pListen = NULL;
		return NULL;
	}
#ifdef _WIN32
#  ifdef USE_WSA_EVENTS
	pListen->event = WSACreateEvent();
	WSAEventSelect( pListen->Socket, pListen->event, FD_ACCEPT|FD_CLOSE );
#  else
	if( WSAAsyncSelect( pListen->Socket, globalNetworkData.ghWndNetwork,
                       SOCKMSG_TCP, FD_ACCEPT|FD_CLOSE ) )
	{
		lprintf( WIDE("Windows AsynchSelect failed: %d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlockEx( pListen DBG_SRC );
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
		t = TRUE;
		fcntl( pListen->Socket, F_SETFL, O_NONBLOCK );
	}
#endif
#ifndef _WIN32
	if( pAddr->sa_family==AF_UNIX )
		unlink( (char*)(((uint16_t*)pAddr)+1));
#endif

	if (!pAddr || 
		 bind(pListen->Socket ,pAddr, SOCKADDR_LENGTH( pAddr ) ) )
	{
		_lprintf(DBG_RELAY)( WIDE("Cannot bind to address..:%d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlockEx( pListen DBG_SRC );
		return NULL;
	}
	pListen->saSource = DuplicateAddress( pAddr );

	if(listen(pListen->Socket, SOMAXCONN ) == SOCKET_ERROR )
	{
		lprintf( WIDE("listen(5) failed: %d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlockEx( pListen DBG_SRC );
		return NULL;
	}
	pListen->connect.CPPClientConnected = NotifyCallback;
	pListen->psvConnect = psvConnect;
	pListen->dwFlags |= CF_CPPCONNECT;
	NetworkUnlockEx( pListen DBG_SRC );
	AddActive( pListen );
   // make sure to schedule this socket for events (connect)
#ifdef USE_WSA_EVENTS
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (listener added)" ) );
	EnqueLink( &globalNetworkData.client_schedule, pListen );
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
	AddThreadEvent( pListen, 0 );
#endif
	return pListen;
}

//----------------------------------------------------------------------------

PCLIENT OpenTCPListenerAddrExx( SOCKADDR *pAddr
                              , cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerAddrExx( pAddr, (cppNotifyCallback)NotifyCallback, 0 DBG_RELAY );
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
		lpMyAddr = CreateSockAddress( WIDE(":::"), wPort );
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

	while( !NetworkLockEx( pc DBG_SRC ) )
	{
		if( !(pc->dwFlags & CF_ACTIVE) )
		{
			return -1;
		}
		Relinquish();
	}

	pc->dwFlags |= CF_CONNECTING;
	//DumpAddr( WIDE("Connect to"), &pResult->saClient );
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
			_lprintf( DBG_RELAY )(WIDE( "Connect FAIL: %d %d %" ) _32f, pc->Socket, err, dwError);
			InternalRemoveClientEx( pc, TRUE, FALSE );
			NetworkUnlockEx( pc DBG_SRC );
			pc = NULL;
			return -1;
		}
		else
		{
			//lprintf( WIDE("Pending connect has begun...") );
		}
	}
	else
	{
#ifdef VERBOSE_DEBUG
		lprintf( WIDE( "Connected before we even get a chance to wait" ) );
#endif
	}
	NetworkUnlockEx( pc DBG_SRC );

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
			pResult->Socket=socket( *(uint16_t*)lpAddr
										 , SOCK_STREAM
										 , (((*(uint16_t*)lpAddr) == AF_INET)||((*(uint16_t*)lpAddr) == AF_INET6))?IPPROTO_TCP:0 );
#ifdef LOG_SOCKET_CREATION
		lprintf( WIDE( "Created new socket %d" ), pResult->Socket );
#endif
		if (pResult->Socket==INVALID_SOCKET)
		{
			lprintf( WIDE("Create socket failed. %d"), WSAGetLastError() );
			InternalRemoveClientEx( pResult, TRUE, FALSE );
			NetworkUnlockEx( pResult DBG_SRC );
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
				lprintf( WIDE( "Natural was %d" ), dwFlags );
			}
			SetHandleInformation( (HANDLE)pResult->Socket, HANDLE_FLAG_INHERIT, 0 );
#  ifdef USE_WSA_EVENTS
			pResult->event = WSACreateEvent();
#    ifdef LOG_NETWORK_EVENT_THREAD
			lprintf( "new event is %p", pResult->event );
#    endif
			WSAEventSelect( pResult->Socket, pResult->event, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE );
#  else
			if( WSAAsyncSelect( pResult->Socket,globalNetworkData.ghWndNetwork,SOCKMSG_TCP,
									 FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT) )
			{
				lprintf( WIDE(" Select NewClient Fail! %d"), WSAGetLastError() );
				InternalRemoveClientEx( pResult, TRUE, FALSE );
				NetworkUnlockEx( pResult DBG_SRC );
				pResult = NULL;
				goto LeaveNow;
			}
#  endif
#else
			fcntl( pResult->Socket, F_SETFL, O_NONBLOCK );
#endif
			if( pFromAddr )
			{
				
				LOGICAL opt = 1;
				err = setsockopt( pResult->Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof( opt ) );
				if( err )
				{
					uint32_t dwError = WSAGetLastError();
					lprintf( WIDE("Failed to set socket option REUSEADDR : %d"), dwError );
				}
				pResult->saSource = DuplicateAddress( pFromAddr );
				//DumpAddr( "source", pResult->saSource );
				if( ( err = bind( pResult->Socket, pResult->saSource
								, SOCKADDR_LENGTH( pResult->saSource ) ) ) )
				{
					uint32_t dwError;
					dwError = WSAGetLastError();
					lprintf( WIDE("Error binding connecting socket to source address... continuing with connect : %d"), dwError );
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
			if( !(flags & OPEN_TCP_FLAG_DELAY_CONNECT) ) {
				NetworkConnectTCPEx( pResult DBG_RELAY );
			}
			//lprintf( WIDE("Leaving Client's critical section") );
			NetworkUnlockEx( pResult DBG_SRC );

			// socket should now get scheduled for events, after unlocking it?
#ifdef USE_WSA_EVENTS
			if( globalNetworkData.flags.bLogNotices )
				lprintf( WIDE( "SET GLOBAL EVENT (wait for connect) new %p %p  %08x %p" ), pResult, pResult->event, pResult->dwFlags, globalNetworkData.hMonitorThreadControlEvent );
			EnqueLink( &globalNetworkData.client_schedule, pResult );
			if( this_thread == globalNetworkData.root_thread ) {
				ProcessNetworkMessages( this_thread, 1 );
				if( !pResult->this_thread ) {
					WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
					lprintf( "Failed to schedule myself in a single run of root thread that I am running on." );
				}
			} 
			else {
				WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
				while( !pResult->this_thread )
					Relinquish(); // wait for it to be added to waiting lists?
			}

#endif
#ifdef __LINUX__
			AddThreadEvent( pResult, 0 );
#endif
			if( !pConnectComplete && !(flags & OPEN_TCP_FLAG_DELAY_CONNECT) )
			{
				int Start, bProcessing = 0;

				// should trigger a rebuild (if it's the root thread)
				Start = (GetTickCount()&0xFFFFFFF);
				pResult->dwFlags |= CF_CONNECT_WAITING;
				// caller was expecting connect to block....
				while( !( pResult->dwFlags & (CF_CONNECTED|CF_CONNECTERROR|CF_CONNECT_CLOSED) ) &&
						( ( (GetTickCount()&0xFFFFFFF) - Start ) < globalNetworkData.dwConnectTimeout ) )
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
							lprintf( WIDE( "Falling asleep 3 seconds waiting for connect on %p." ), pResult );
						pResult->tcp_delay_count++;
						WakeableSleep( 3000 );
						pResult->pWaiting = NULL;
						if( pResult->dwFlags & CF_CLOSING )
							return NULL;
					}
					else
					{
						lprintf( WIDE( "Spin wait for connect" ) );
						Relinquish();
					}
				}
				if( (( (GetTickCount()&0xFFFFFFF) - Start ) >= 10000)
					|| (pResult->dwFlags &  CF_CONNECTERROR ) )
				{
					if( pResult->dwFlags &  CF_CONNECTERROR )
					{
						//DumpAddr( WIDE("Connect to: "), lpAddr );
						//lprintf( WIDE("Connect FAIL: message result") );
					}
					else
						lprintf( WIDE("Connect FAIL: Timeout") );
					InternalRemoveClientEx( pResult, TRUE, FALSE );
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
						lprintf( WIDE("getsockname errno = %d"), errno );
					}
				}
				//lprintf( WIDE("Connect did complete... returning to application"));
			}
#ifdef VERBOSE_DEBUG
			else
				lprintf( WIDE("Connect in progress, will notify application when done.") );
#endif
			pResult->dwFlags &= ~CF_CONNECT_WAITING;
		}
	}

LeaveNow:
	if( !pResult )  // didn't break out of the loop with a good return.
	{
		//lprintf( WIDE("Failed Open TCP Client.") );
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
		//lprintf( WIDE( "Finish pending - return, no pending read. %08x" ), lpClient->dwFlags );
	}

	do
	{
		if( lpClient->bDraining )
		{
			lprintf(WIDE( "LOG:ERROR trying to read during a drain state..." ) );
			return -1; // why error on draining with pending finish??
		}

		if( !(lpClient->dwFlags & CF_CONNECTED)  )
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE( "Finsih pending - return, not connected." ) );
#endif
			return (int)lpClient->RecvPending.dwUsed; // amount of data available...
		}
		//lprintf( WIDE(WIDE( "FinishPendingRead of %d" )), lpClient->RecvPending.dwAvail );
		if( !( lpClient->dwFlags & CF_READPENDING ) )
		{
			//lpClient->dwFlags |= CF_READREADY; // read ready is set if FinishPendingRead returns 0; and it's from the core read...
			lprintf( WIDE( "Finish pending - return, no pending read. %08x" ), lpClient->dwFlags );
			// without a pending read, don't read, the buffers are not correct.
			return 0;
		}
		while( lpClient->RecvPending.dwAvail )  // if any room is availiable.
		{
#ifdef DEBUG_SOCK_IO
			//nCount++;
			_lprintf( DBG_RELAY )( WIDE("FinishPendingRead %d %d" )
				, lpClient->RecvPending.dwUsed, lpClient->RecvPending.dwAvail );
#endif
			nRecv = recv(lpClient->Socket,
							 (char*)lpClient->RecvPending.buffer.p +
							 lpClient->RecvPending.dwUsed,
							 (int)lpClient->RecvPending.dwAvail,0);
			if (nRecv == SOCKET_ERROR)
			{
				dwError = WSAGetLastError();
#ifdef DEBUG_SOCK_IO
				lprintf( "Received error (-1) %d", nRecv );
#endif
				switch( dwError)
				{
				case WSAEWOULDBLOCK: // no data avail yet...
					//lprintf( WIDE("Pending Receive would block...") );
					lpClient->dwFlags &= ~CF_READREADY;
					return (int)lpClient->RecvPending.dwUsed;
#ifdef __LINUX__
				case ECONNRESET:
#else
				case WSAECONNRESET:
				case WSAECONNABORTED:
#endif
#ifdef LOG_DEBUG_CLOSING
					lprintf( WIDE("Read from reset connection - closing. %p"), lpClient );
#endif
					if(0)
					{
					default:
						Log5( WIDE("Failed reading from %d (err:%d) into %p %") _size_f WIDE(" bytes %") _size_f WIDE(" read already."),
							  lpClient->Socket,
							  WSAGetLastError(),
							  lpClient->RecvPending.buffer.p,
							  lpClient->RecvPending.dwAvail,
							  lpClient->RecvPending.dwUsed );
						lprintf(WIDE("LOG:ERROR FinishPending discovered unhandled error (closing connection) %") _32f WIDE(""), dwError );
					}
					//InternalRemoveClient( lpClient );  // invalid channel now.
					lpClient->dwFlags |= CF_TOCLOSE;
					return -1;   // return pending finished...
				}
			}
			else if (!nRecv) // channel closed if received 0 bytes...
			{   // otherwise WSAEWOULDBLOCK would be generated.
#ifdef DEBUG_SOCK_IO
				lprintf( "Received (0) %d", nRecv );
#endif
				//_lprintf( DBG_RELAY )( WIDE("Closing closed socket... Hope there's also an event... "));
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
				lpClient->RecvPending.dwUsed  += nRecv;
				if( lpClient->RecvPending.s.bStream &&
					lpClient->RecvPending.dwAvail )
					break;
				//else
				//	lprintf( WIDE("Was not a stream read - try reading more...") );
			}
		}

		// if read notification okay - then do callback.
		if( !( lpClient->dwFlags & CF_READWAITING ) )
		{
#ifdef LOG_PENDING
			lprintf( WIDE("Waiting on a queued read... result to callback.") );
#endif
			if( ( !lpClient->RecvPending.dwAvail || // completed all of the read
				  ( lpClient->RecvPending.dwUsed &&   // or completed some of the read
					lpClient->RecvPending.s.bStream ) ) )
			{
#ifdef LOG_PENDING
				lprintf( WIDE("Sending completed read to application") );
#endif
				lpClient->dwFlags &= ~CF_READPENDING;
				if( lpClient->read.ReadComplete )  // and there's a read complete callback available
				{
					// need to clear dwUsed...
					// otherwise the on-close notificatino can cause this to dispatch again.
					size_t length = lpClient->RecvPending.dwUsed;
					lpClient->RecvPending.dwUsed = 0;
#ifdef LOG_PENDING
					lprintf( WIDE( "Send to application...." ) );
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
					if( !lpClient->Socket )
						return -1;
#ifdef LOG_PENDING
					lprintf( WIDE( "back from applciation... (loop to next)" ) ); // new read probably pending ehre...
#endif
					continue;
				}
			}
			if( !( lpClient->dwFlags & CF_READPENDING ) )
			{
				lprintf( WIDE("somehow we didn't get a good read.") );
			}
		}
		else
		{
#ifdef LOG_PENDING
			lprintf( WIDE("Client is waiting for this data... should we wake him? %d"), lpClient->RecvPending.s.bStream );
#endif
			if( ( !lpClient->RecvPending.dwAvail || // completed all of the read
				  ( lpClient->RecvPending.dwUsed &&   // or completed some of the read
					lpClient->RecvPending.s.bStream ) ) )
			{
				lprintf( WIDE("Wake waiting thread... clearing pending read flag.") );
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
	_lprintf(DBG_RELAY)( WIDE( "Reading ... %p(%d) int %p %d (%s,%s)" ), lpClient, lpClient->Socket, lpBuffer, (uint32_t)nBytes, bIsStream?WIDE( "stream" ):WIDE( "block" ), bWait?WIDE( "Wait" ):WIDE( "NoWait" ) );
#endif
	if( !lpClient || !lpBuffer )
		return 0; // nothing read.... ???
	// don't try to read closed/inactive sockets.
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
		return 0;

	if( TCPDrainRead( lpClient ) &&  //draining....
		lpClient->RecvPending.dwAvail ) // and already queued next read.
	{
		lprintf(WIDE( "LOG:ERROR is draining, and NEXT pending queued ALSO..." ));
		return -1;  // read not queued... (ERROR)
	}

	if( !lpClient->RecvPending.s.bStream && // existing read was not a stream...
		lpClient->RecvPending.dwAvail )   // AND is not at completion...
	{
		lprintf(WIDE("LOG:ERROR was not a stream, and is not complete...%") _32f WIDE(" left "),
			  (uint32_t)lpClient->RecvPending.dwAvail );
		return -1; // this I guess is an error since we're queueing another
		// read on top of existing incoming 'guaranteed data'
	}
	while( !NetworkLockEx( lpClient DBG_RELAY ) )
	{
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			return -1;
		}
		Relinquish();
	}
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
	{
		// like say the callback we're being invoked from closed it;
		lprintf( WIDE( "inactive client, will not pend read." ) );
		NetworkUnlockEx( lpClient DBG_SRC );
		return -1;
	}
	//lprintf( "read %d", nBytes );
	if( nBytes )   // only worry if there IS data to read.
	{
		// we can assume there is nothing now pending...
		lpClient->RecvPending.buffer.p = lpBuffer;
		lpClient->RecvPending.dwAvail = nBytes;
		lpClient->RecvPending.dwUsed = 0;
		lpClient->RecvPending.s.bStream = bIsStream;
		// if the pending finishes it will call the ReadComplete Callback.
		// otherwise there will be more data to read...
		//lprintf( WIDE("Ok ... buffers set up - now we can handle read events") );
#ifdef LOG_PENDING
		lprintf( WIDE( "Setup read pending %08x" ), lpClient->dwFlags );
#endif
		lpClient->dwFlags |= CF_READPENDING;
#ifdef LOG_PENDING
		lprintf( WIDE( "Setup read pending %p %08x" ), lpClient, lpClient->dwFlags );
#endif
		if( bWait )
		{
			//lprintf( WIDE("setting read waiting so we get awoken... and callback dispatch does not happen.") );
			lpClient->dwFlags |= CF_READWAITING;
		}
		//else
		//   lprintf( WIDE("No read waiting... allow forward going...") );
		if( lpClient->dwFlags & CF_READREADY )
		{
#ifdef LOG_PENDING
			lprintf( WIDE("Data already present for read...") );
#endif
			FinishPendingRead( lpClient DBG_SRC );
		}
	}
	else
	{
		lprintf( WIDE( "zero byte read...." ) );
		if( !bWait )
		{
			if( lpClient->read.ReadComplete )  // and there's a read complete callback available
			{
				lprintf( WIDE( "Read complete with 0 bytes immediate..." ) );
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
		//lprintf( WIDE("Waiting for TCP data result...") );
		{
			uint32_t tick = timeGetTime();
			lpClient->pWaiting = MakeThread();
			NetworkUnlockEx( lpClient DBG_SRC );
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
					//lprintf( WIDE("Nothing significant to idle on... going to sleep forever.") );
					WakeableSleep( 1000 );
				}
			}
		}
		while( !NetworkLockEx( lpClient DBG_SRC ) )
		{
			if( !(lpClient->dwFlags & CF_ACTIVE ) )
			{
				return 0;
			}
			Relinquish();
		}
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			NetworkUnlockEx( lpClient DBG_SRC );
			return -1;
		}
 		lpClient->dwFlags &= ~CF_READWAITING;
		NetworkUnlockEx( lpClient DBG_SRC);
		if( timeout )
			return 0;
		else
			return lpClient->RecvPending.dwUsed;
	}
	else
		NetworkUnlockEx( lpClient DBG_SRC );

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
		lprintf( WIDE("Pending %d Bytes to network...") , nLen );
	}
#endif
	lpPend = New( PendingBuffer );

	lpPend->dwAvail  = nLen;
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
			lprintf( WIDE("Sending previously queued data.") );
#endif

		if( pc->lpFirstPending->dwAvail )
		{
			if( globalNetworkData.flags.bLogSentData )
			{
				LogBinary( (uint8_t*)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail );
			}
#ifdef DEBUG_SOCK_IO
			_lprintf(DBG_RELAY)( "Try to send... %d  %d", pc->lpFirstPending->dwUsed, pc->lpFirstPending->dwAvail );
#endif
			nSent = send(pc->Socket,
							 (char*)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail,
							 0);
			if (nSent == SOCKET_ERROR) {
            DWORD dwError;
				dwError = WSAGetLastError();
				if( dwError == WSAEWOULDBLOCK )  // this is alright.
				{
#ifdef VERBOSE_DEBUG
					lprintf( WIDE("Pending write...") );
#endif
					pc->dwFlags |= CF_WRITEPENDING;
#ifdef __LINUX__
					//if( !(pc->dwFlags & CF_WRITEISPENDED ) )
					//{
					//	   lprintf( WIDE("Sending signal") );
					//    WakeThread( globalNetworkData.pThread );
					//}
					//else
					//    lprintf( WIDE("Yes... it was already pending..(no signal)") );
#endif

					return TRUE;
				}
				{
					_lprintf(DBG_RELAY)(WIDE(" Network Send Error: %5d(buffer:%p ofs: %") _size_f WIDE("  Len: %") _size_f WIDE(")"),
											  dwError,
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
						InternalRemoveClient( pc );
					}
					return FALSE; // get out of here!
				}
			} else if (!nSent) { // other side closed.
				lprintf( WIDE("sent zero bytes - assume it was closed - and HOPE there's an event...") );
				InternalRemoveClient( pc );
				// if this happened - don't return TRUE result which would
				// result in queuing a pending buffer...
				return FALSE;  // no sence processing the rest of this.
			} else if( nSent < (int)pc->lpFirstPending->dwAvail ) {
				//pc->lpFirstPending->dwUsed += nSent;
				//pc->lpFirstPending->dwAvail -= nSent;
				pc->dwFlags |= CF_WRITEPENDING;
				//lprintf( "THIS IS ANOTHER PENDING CONDITION THAT WASN'T ACCOUNTED %d of %d", nSent, pc->lpFirstPending->dwAvail  );
			}
		}
		else
			nSent = 0;

#ifdef DEBUG_SOCK_IO
		lprintf( "sent... %d", nSent );
#endif

		{  // sent some data - update pending buffer status.
			if( pc->lpFirstPending )
			{
				pc->lpFirstPending->dwAvail -= nSent;
				pc->lpFirstPending->dwUsed  += nSent;
				if (!pc->lpFirstPending->dwAvail)  // no more to send...
				{
					lpNext = pc->lpFirstPending -> lpNext;
					if( pc->lpFirstPending->s.bDynBuffer )
						Release(pc->lpFirstPending->buffer.p );
					// there is one pending holder in the client
					// structure that was NOT allocated...
					if( pc->lpFirstPending != &pc->FirstWritePending )
					{
#ifdef LOG_PENDING
						lprintf( WIDE("Finished sending a pending buffer.") );
#endif
						Release(pc->lpFirstPending );
					}
					else
					{
#ifdef LOG_PENDING
						if(0) // this happens 99.99% of the time.
						{
							lprintf( WIDE("Normal send complete.") );
						}
#endif
					}
					if (!lpNext)
						pc->lpLastPending = NULL;
					pc->lpFirstPending = lpNext;

					if( pc->write.WriteComplete &&
						!pc->bWriteComplete )
					{
						pc->bWriteComplete = TRUE;
						if( pc->dwFlags & CF_CPPWRITE )
							pc->write.CPPWriteComplete( pc->psvWrite );  // SOME WRITE!!!
						else
							pc->write.WriteComplete( pc );  // SOME WRITE!!!
						pc->bWriteComplete = FALSE;
					}
					if( !pc->lpFirstPending )
						pc->dwFlags &= ~CF_WRITEPENDING;
				}
				else
				{
					if( !(pc->dwFlags & CF_WRITEPENDING) )
					{
						pc->dwFlags |= CF_WRITEPENDING;
#ifdef USE_WSA_EVENTS
						if( globalNetworkData.flags.bLogNotices )
							lprintf( WIDE( "SET GLOBAL EVENT (write pending)" ) );
						EnqueLink( &globalNetworkData.client_schedule, pc );
						WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
						AddThreadEvent( pc, 0 );
#endif
					}
					return TRUE;
				}
			}
		}
	}
	return FALSE; // 0 = everything sent / nothing left to send...
}

//----------------------------------------------------------------------------

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
		lprintf( WIDE("TCP Write failed - invalid client.") );
#endif
		return FALSE;  // cannot process a closed channel. data not sent.
	}

	while( !NetworkLockEx( lpClient DBG_SRC ) )
	{
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			_lprintf(DBG_RELAY)( "Failing send..." );
			LogBinary( (uint8_t*)pInBuffer, nInLen );
			return FALSE;
		}
		Relinquish();
	}
	if( !(lpClient->dwFlags & CF_ACTIVE ) )
	{
#ifdef VERBOSE_DEBUG
		lprintf( WIDE("TCP Write failed - client is inactive") );
#endif
		// change to inactive status by the time we got here...
		NetworkUnlockEx( lpClient DBG_SRC );
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
			lprintf( WIDE("Queuing pending data anyhow...") );
#endif
			PendWrite( lpClient, pInBuffer, nInLen, bLongBuffer );
			//TCPWriteEx( lpClient DBG_SRC ); // make sure we don't lose a write event during the queuing...
			NetworkUnlockEx( lpClient DBG_SRC );
			return TRUE;
		}
		else
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE("Failing pend.") );
#endif
			NetworkUnlockEx( lpClient DBG_SRC );
			return FALSE;
		}
	}
	else
	{
		// have to steal the buffer - :(

		lpClient->FirstWritePending.buffer.c   = pInBuffer;
		lpClient->FirstWritePending.dwAvail    = nInLen;
		lpClient->FirstWritePending.dwUsed     = 0;

		lpClient->FirstWritePending.s.bStream    = FALSE;
		lpClient->FirstWritePending.s.bDynBuffer = FALSE;
		lpClient->FirstWritePending.lpNext       = NULL;

		lpClient->lpLastPending = 
		lpClient->lpFirstPending = &lpClient->FirstWritePending;
		if( TCPWriteEx( lpClient DBG_SRC ) )
		{
#ifdef VERBOSE_DEBUG
			Log2( WIDE("Data not sent, pending buffer... %d bytes %d remain"), nInLen, lpClient->FirstWritePending.dwAvail );
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
			_xlprintf( 1 DBG_RELAY )( WIDE("Data has been compeltely sent.") );
#endif
	}
	NetworkUnlockEx( lpClient DBG_SRC );
	return TRUE; // assume the data was sent.
}

//----------------------------------------------------------------------------

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
			DWORD dwError;
         dwError = WSAGetLastError();
			if( dwError == WSAEWOULDBLOCK )
			{
				if( !pClient->bDrainExact )
					pClient->nDrainLength = 0;
				break;
			}
			lprintf(WIDE(" Network Error during drain: %d (from: %p  to: %p  has: %") _size_f WIDE("  toget: %") _size_f WIDE(")")
			       , dwError
			       , pClient->Socket
			       , pClient->RecvPending.buffer.p
			       , pClient->RecvPending.dwUsed
			       , pClient->RecvPending.dwAvail );
			InternalRemoveClient( pClient );
			NetworkUnlockEx( pClient DBG_SRC );
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
			NetworkUnlockEx( pClient DBG_SRC );
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
		while( !NetworkLockEx( pClient DBG_SRC ) )
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
		NetworkUnlockEx( pClient DBG_SRC );
		return bytes;
	}
	return 0;
}

//----------------------------------------------------------------------------

void SetTCPNoDelay( PCLIENT pClient, int bEnable )
{
	if( setsockopt( pClient->Socket, IPPROTO_TCP,
						TCP_NODELAY,
						(const char *)&bEnable, sizeof(bEnable) ) == SOCKET_ERROR )
	{
		lprintf( WIDE("Error(%d) setting Delay to : %d"), WSAGetLastError(), bEnable );
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
		lprintf( WIDE("Error(%d) setting KeepAlive to : %d"), WSAGetLastError(), bEnable );
		// log some sort of error... and ignore...
	}
}
SACK_NETWORK_TCP_NAMESPACE_END

