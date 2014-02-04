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
#define LIBRARY_DEF
#include <stdhdrs.h>
#include <timers.h>
#include "netstruc.h"
#include <network.h>
#ifndef UNDER_CE
#include <fcntl.h>
#endif
#include <idle.h>
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
	extern int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, PTRSZVAL quick_check );

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
	pNewClient->Socket = accept( pListen->Socket
										, pNewClient->saClient
										,&nTemp
										);
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
		WSAEventSelect( pNewClient->Socket, pNewClient->event, FD_READ|FD_WRITE|FD_CLOSE );
#  else
		if( WSAAsyncSelect( pNewClient->Socket,g.ghWndNetwork,SOCKMSG_TCP,
                               FD_READ | FD_WRITE | FD_CLOSE))
		{ // if there was a select error...
			lprintf(WIDE( " Accept select Error" ));
			InternalRemoveClientEx( pNewClient, TRUE, FALSE );
			NetworkUnlock( pNewClient );
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
			NetworkUnlock( pNewClient );
		}
	}
	else // accept failed...
	{
		InternalRemoveClientEx( pNewClient, TRUE, FALSE );
		NetworkUnlock( pNewClient );
		pNewClient = NULL;
	}

	if( !pNewClient )
	{
		lprintf(WIDE( "Failed Accept..." ));
	}
}

//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, CPPOpenTCPListenerAddrExx )( SOCKADDR *pAddr
																 , cppNotifyCallback NotifyCallback
																 , PTRSZVAL psvConnect
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
	//	pListen->Socket = socket( *(_16*)pAddr, SOCK_STREAM, 0 );
#ifdef WIN32
	pListen->Socket = OpenSocket( ((*(_16*)pAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
	if( pListen->Socket == INVALID_SOCKET )
#endif
		pListen->Socket = socket( *(_16*)pAddr
										, SOCK_STREAM
										, (((*(_16*)pAddr) == AF_INET)||((*(_16*)pAddr) == AF_INET6))?IPPROTO_TCP:0 );
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
		NetworkUnlock( pListen );
		pListen = NULL;
		return NULL;
	}
#ifdef _WIN32
#  ifdef USE_WSA_EVENTS
	pListen->event = WSACreateEvent();
	WSAEventSelect( pListen->Socket, pListen->event, FD_ACCEPT|FD_CLOSE );
#  else
	if( WSAAsyncSelect( pListen->Socket, g.ghWndNetwork,
                       SOCKMSG_TCP, FD_ACCEPT|FD_CLOSE ) )
	{
		DebugBreak();
		lprintf( WIDE("Windows AsynchSelect failed: %d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlock( pListen );
		return NULL;
	}
#  endif
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
		unlink( (char*)(((_16*)pAddr)+1));
#endif

   if (!pAddr || 
		 bind(pListen->Socket ,pAddr
#ifdef WIN32
			  ,sizeof(SOCKADDR)
#else
			  ,(pAddr->sa_family==AF_INET)?sizeof(struct sockaddr):110
#endif
			  ))
	{
		_lprintf(DBG_RELAY)( WIDE("Cant bind to address..:%d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlock( pListen );
		return NULL;
	}
	pListen->saSource = DuplicateAddress( pAddr );

	if(listen(pListen->Socket,5) == SOCKET_ERROR )
	{
		lprintf( WIDE("listen(5) failed: %d"), WSAGetLastError() );
		InternalRemoveClientEx( pListen, TRUE, FALSE );
		NetworkUnlock( pListen );
		return NULL;
	}
	pListen->connect.CPPClientConnected = (void(CPROC*)(PTRSZVAL,PCLIENT))NotifyCallback;
	pListen->psvConnect = psvConnect;
	pListen->dwFlags |= CF_CPPCONNECT;
	NetworkUnlock( pListen );
	AddActive( pListen );
   // make sure to schedule this socket for events (connect)
#ifdef USE_WSA_EVENTS
	if( g.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (listener added)" ) );
	WSASetEvent( g.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
	WakeThread( g.pThread );
#endif
	return pListen;
}

#undef CPPOpenTCPListenerAddrEx
NETWORK_PROC( PCLIENT, CPPOpenTCPListenerAddrEx )( SOCKADDR *pAddr
																 , cppNotifyCallback NotifyCallback
																 , PTRSZVAL psvConnect
																  DBG_PASS )
{
   return CPPOpenTCPListenerAddrExx( pAddr, NotifyCallback, psvConnect DBG_RELAY );
}

//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, OpenTCPListenerAddrExx )(SOCKADDR *pAddr
															, cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerAddrExx( pAddr, (cppNotifyCallback)NotifyCallback, 0 DBG_RELAY );
	if( result )
		result->dwFlags &= ~CF_CPPCONNECT;
	return result;
}

#undef OpenTCPListenerAddrEx
NETWORK_PROC( PCLIENT, OpenTCPListenerAddrEx )(SOCKADDR *pAddr
															 , cNotifyCallback NotifyCallback )
{
   return OpenTCPListenerAddrExx( pAddr, NotifyCallback DBG_SRC );
}

//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, CPPOpenTCPListenerExx )(_16 wPort
															 , cppNotifyCallback NotifyCallback
															 , PTRSZVAL psvConnect
                                               DBG_PASS
															 )
{
	SOCKADDR *lpMyAddr = CreateLocal(wPort);
	PCLIENT pc = CPPOpenTCPListenerAddrExx( lpMyAddr, NotifyCallback, psvConnect DBG_RELAY );
	ReleaseAddress( lpMyAddr );
	return pc;
}

#undef CPPOpenTCPListenerEx
NETWORK_PROC( PCLIENT, CPPOpenTCPListenerEx )(_16 wPort
                                          , cppNotifyCallback NotifyCallback
                                          , PTRSZVAL psvConnect )
{
   return CPPOpenTCPListenerExx( wPort, NotifyCallback, psvConnect DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, OpenTCPListenerExx )(_16 wPort, cNotifyCallback NotifyCallback DBG_PASS )
{
	PCLIENT result = CPPOpenTCPListenerExx( wPort, (cppNotifyCallback)NotifyCallback, 0 DBG_RELAY );
	if( result )
		result->dwFlags &= ~CF_CPPCONNECT;
	return result;
}

#undef OpenTCPListenerEx
NETWORK_PROC( PCLIENT, OpenTCPListenerEx )(_16 wPort, cNotifyCallback NotifyCallback )
{
   return OpenTCPListenerExx( wPort, NotifyCallback DBG_SRC );
}
//----------------------------------------------------------------------------

static PCLIENT InternalTCPClientAddrExxx(SOCKADDR *lpAddr,
													  int bCPP,
													  cppReadComplete  pReadComplete,
													  PTRSZVAL psvRead,
													  cppCloseCallback CloseCallback,
													  PTRSZVAL psvClose,
													  cppWriteComplete WriteComplete,
													  PTRSZVAL psvWrite,
													  cppConnectCallback pConnectComplete,
													  PTRSZVAL psvConnect
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
		pResult->Socket = OpenSocket( ((*(_16*)lpAddr) == AF_INET)?TRUE:FALSE, TRUE, FALSE, 0 );
		if( pResult->Socket == INVALID_SOCKET )
#endif
			pResult->Socket=socket( *(_16*)lpAddr
										 , SOCK_STREAM
										 , (((*(_16*)lpAddr) == AF_INET)||((*(_16*)lpAddr) == AF_INET6))?IPPROTO_TCP:0 );
#ifdef LOG_SOCKET_CREATION
		lprintf( WIDE( "Created new socket %d" ), pResult->Socket );
#endif
		if (pResult->Socket==INVALID_SOCKET)
		{
			lprintf( WIDE("Create socket failed. %d"), WSAGetLastError() );
			InternalRemoveClientEx( pResult, TRUE, FALSE );
			NetworkUnlock( pResult );
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
			WSAEventSelect( pResult->Socket, pResult->event, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE );
#  else
			if( WSAAsyncSelect( pResult->Socket,g.ghWndNetwork,SOCKMSG_TCP,
									 FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT) )
			{
				lprintf( WIDE(" Select NewClient Fail! %d"), WSAGetLastError() );
				InternalRemoveClientEx( pResult, TRUE, FALSE );
				NetworkUnlock( pResult );
				pResult = NULL;
				goto LeaveNow;
			}
#  endif
#else
			fcntl( pResult->Socket, F_SETFL, O_NONBLOCK );
#endif
			pResult->saClient = DuplicateAddress( lpAddr );

			// set up callbacks before asynch select...
			pResult->connect.CPPThisConnected= pConnectComplete;
			pResult->psvConnect = psvConnect;
			pResult->read.CPPReadComplete    = pReadComplete;
			pResult->psvRead = psvRead;
			pResult->close.CPPCloseCallback        = CloseCallback;
			pResult->psvClose = psvClose;
			pResult->write.CPPWriteComplete        = WriteComplete;
			pResult->psvWrite = psvWrite;
			if( bCPP )
				pResult->dwFlags |= ( CF_CALLBACKTYPES );

			pResult->dwFlags |= CF_CONNECTING;
			//DumpAddr( WIDE("Connect to"), &pResult->saClient );
			if( ( err = connect( pResult->Socket, pResult->saClient
									 , SOCKADDR_LENGTH( pResult->saClient ) ) ) )
			{
				_32 dwError;
				dwError = WSAGetLastError();
				if( dwError != WSAEWOULDBLOCK
#ifdef __LINUX__
					&& dwError != EINPROGRESS
#else
					&& dwError != WSAEINPROGRESS
#endif
				  )
				{
					_lprintf(DBG_RELAY)( WIDE("Connect FAIL: %d %d %") _32f, pResult->Socket, err, dwError );
					InternalRemoveClientEx( pResult, TRUE, FALSE );
					NetworkUnlock( pResult );
					pResult = NULL;
					goto LeaveNow;
				}
				else
				{
					//lprintf( WIDE("Pending connect has begun...") );
				}
			}
			else
			{
#ifdef VERBOSE_DEBUG
				lprintf( WIDE("Connected before we even get a chance to wait") );
#endif
			}

			AddActive( pResult );

			//lprintf( WIDE("Leaving Client's critical section") );
			NetworkUnlock( pResult );

			// socket should now get scheduled for events, after unlocking it?
#ifdef USE_WSA_EVENTS
			if( g.flags.bLogNotices )
				lprintf( WIDE( "SET GLOBAL EVENT (wait for connect)" ) );
			WSASetEvent( g.hMonitorThreadControlEvent );
			if( this_thread == g.root_thread )
				ProcessNetworkMessages( this_thread, 1 );
			else
				WSASetEvent( g.hMonitorThreadControlEvent );
			while( !pResult->this_thread )
				Idle(); // wait for it to be added to waiting lists?

#endif
#ifdef __LINUX__
			{
				//kill( (_32)(g.pThread->ThreadID), SIGHUP );
				WakeThread( g.pThread );
			}
#endif
			if( !pConnectComplete )
			{
				int Start, bProcessing = 0;

				// should trigger a rebuild (if it's the root thread)
				Start = (GetTickCount()&0xFFFFFFF);
				pResult->dwFlags |= CF_CONNECT_WAITING;
				// caller was expecting connect to block....
				while( !( pResult->dwFlags & (CF_CONNECTED|CF_CONNECTERROR|CF_CONNECT_CLOSED) ) &&
						( ( (GetTickCount()&0xFFFFFFF) - Start ) < g.dwConnectTimeout ) )
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
						if( g.flags.bLogNotices )
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
NETWORK_PROC( PCLIENT, CPPOpenTCPClientAddrExxx )(SOCKADDR *lpAddr,
																  cppReadComplete  pReadComplete,
																  PTRSZVAL psvRead,
																  cppCloseCallback CloseCallback,
																  PTRSZVAL psvClose,
																  cppWriteComplete WriteComplete,
																  PTRSZVAL psvWrite,
																  cppConnectCallback pConnectComplete,
																  PTRSZVAL psvConnect
																  DBG_PASS  )
{
	return InternalTCPClientAddrExxx( lpAddr, TRUE
											 , pReadComplete, psvRead
											 , CloseCallback, psvClose
											 , WriteComplete, psvWrite
											 , pConnectComplete, psvConnect DBG_RELAY );
}

#undef CPPOpenTCPClientAddrExx
NETWORK_PROC( PCLIENT, CPPOpenTCPClientAddrExx )(SOCKADDR *lpAddr, 
             cppReadComplete  pReadComplete,
             PTRSZVAL psvRead,
             cppCloseCallback CloseCallback,
             PTRSZVAL psvClose,
             cppWriteComplete WriteComplete,
             PTRSZVAL psvWrite,
             cppConnectCallback pConnectComplete,
             PTRSZVAL psvConnect
																)
{
	return InternalTCPClientAddrExxx( lpAddr, TRUE
											 , pReadComplete, psvRead
											 , CloseCallback, psvClose
											 , WriteComplete, psvWrite
											 , pConnectComplete, psvConnect DBG_SRC );
}
//----------------------------------------------------------------------------
NETWORK_PROC( PCLIENT, OpenTCPClientAddrExxx )(SOCKADDR *lpAddr,
															  cReadComplete     pReadComplete,
															  cCloseCallback    CloseCallback,
															  cWriteComplete    WriteComplete,
															  cConnectCallback  pConnectComplete
                                               DBG_PASS
															 )
{
	return InternalTCPClientAddrExxx( lpAddr, FALSE
											 , (cppReadComplete)pReadComplete, 0
											 , (cppCloseCallback)CloseCallback, 0
											 , (cppWriteComplete)WriteComplete, 0
											 , (cppConnectCallback)pConnectComplete, 0 DBG_RELAY );
}
#undef OpenTCPClientAddrExx
NETWORK_PROC( PCLIENT, OpenTCPClientAddrExx )(SOCKADDR *lpAddr, 
             cReadComplete     pReadComplete,
             cCloseCallback    CloseCallback,
             cWriteComplete    WriteComplete,
															 cConnectCallback  pConnectComplete )
{
   return OpenTCPClientAddrExxx( lpAddr, pReadComplete, CloseCallback, WriteComplete, pConnectComplete DBG_SRC );
}
//----------------------------------------------------------------------------
NETWORK_PROC( PCLIENT, OpenTCPClientAddrExEx )(SOCKADDR *lpAddr,
															 cReadComplete  pReadComplete,
															 cCloseCallback CloseCallback,
															 cWriteComplete WriteComplete
															 DBG_PASS )
{
   return OpenTCPClientAddrExxx( lpAddr, pReadComplete, CloseCallback, WriteComplete, NULL DBG_RELAY );
}

#undef OpenTCPClientAddrEx
NETWORK_PROC( PCLIENT, OpenTCPClientAddrEx )(SOCKADDR *lpAddr, 
															cReadComplete  pReadComplete,
															cCloseCallback CloseCallback,
															cWriteComplete WriteComplete )
{
	return OpenTCPClientAddrExEx( lpAddr, pReadComplete, CloseCallback, WriteComplete DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, CPPOpenTCPClientExEx )(CTEXTSTR lpName,_16 wPort,
             cppReadComplete	 pReadComplete, PTRSZVAL psvRead,
				 cppCloseCallback CloseCallback, PTRSZVAL psvClose,
             cppWriteComplete WriteComplete, PTRSZVAL psvWrite,
             cppConnectCallback pConnectComplete, PTRSZVAL psvConnect DBG_PASS )
{
   PCLIENT pClient;
   SOCKADDR *lpsaDest;
   pClient = NULL;
   if( lpName && 
       (lpsaDest = CreateSockAddress(lpName,wPort) ) )
   {
      pClient = CPPOpenTCPClientAddrExxx( lpsaDest,
													  pReadComplete,
													  psvRead,
													  CloseCallback,
													  psvClose,
													  WriteComplete,
													  psvWrite,
													  pConnectComplete,
													  psvConnect
                                         DBG_RELAY
													 );
      ReleaseAddress( lpsaDest );
   }   
   return pClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, OpenTCPClientExxx )(CTEXTSTR lpName,_16 wPort,
             cReadComplete	 pReadComplete,
				 cCloseCallback CloseCallback,
             cWriteComplete WriteComplete,
             cConnectCallback pConnectComplete DBG_PASS )
{
	PCLIENT pClient;
	SOCKADDR *lpsaDest;
	pClient = NULL;
	if( lpName &&
		(lpsaDest = CreateSockAddress(lpName,wPort) ) )
	{
		pClient = OpenTCPClientAddrExxx( lpsaDest,
												  pReadComplete,
												  CloseCallback,
												  WriteComplete,
												  pConnectComplete DBG_RELAY );
		ReleaseAddress( lpsaDest );
	}
	return pClient;
}

#undef OpenTCPClientExx
NETWORK_PROC( PCLIENT, OpenTCPClientExx )(CTEXTSTR lpName,_16 wPort,
             cReadComplete	 pReadComplete,
				 cCloseCallback CloseCallback,
             cWriteComplete WriteComplete,
														cConnectCallback pConnectComplete )
{
	return OpenTCPClientExxx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete, pConnectComplete DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, OpenTCPClientExEx)(CTEXTSTR lpName,_16 wPort,
														cReadComplete	 pReadComplete,
														cCloseCallback CloseCallback,
														cWriteComplete WriteComplete
														DBG_PASS )
{
	return OpenTCPClientExxx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete, NULL DBG_RELAY );
}


#undef OpenTCPClientEx
NETWORK_PROC( PCLIENT, OpenTCPClientEx)(CTEXTSTR lpName,_16 wPort,
             cReadComplete	 pReadComplete,
				 cCloseCallback CloseCallback,
													 cWriteComplete WriteComplete )
{
   return OpenTCPClientExEx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete DBG_SRC );
}
//----------------------------------------------------------------------------

size_t FinishPendingRead(PCLIENT lpClient DBG_PASS )  // only time this should be called is when there IS, cause
                                 // we definaly have already gotten SOME data to leave in
                                 // a pending state...
{
	int nRecv;
#ifdef VERBOSE_DEBUG
	int nCount;
#endif
	_32 dwError;
	// returns amount of information left to get.
#ifdef VERBOSE_DEBUG
	nCount = 0;
#endif
	if( !( lpClient->dwFlags & CF_READPENDING ) )
	{
		//lpClient->dwFlags |= CF_READREADY; // read ready is set if FinishPendingRead returns 0; and it's from the core read...
		lprintf( WIDE( "Finish pending - return, no pending read. %08x" ), lpClient->dwFlags );
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
			return lpClient->RecvPending.dwUsed; // amount of data available...
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
#ifdef VERBOSE_DEBUG
			nCount++;
			_lprintf( DBG_RELAY )( WIDE("FinishPendingRead %d %") _32f WIDE(""), nCount
										, lpClient->RecvPending.dwAvail );
#endif
			nRecv = recv(lpClient->Socket,
							 (char*)lpClient->RecvPending.buffer.p +
							 lpClient->RecvPending.dwUsed,
							 (int)lpClient->RecvPending.dwAvail,0);
			if (nRecv == SOCKET_ERROR)
			{
				dwError = WSAGetLastError();
				switch( dwError)
				{
				case WSAEWOULDBLOCK: // no data avail yet...
					//lprintf( WIDE("Pending Receive would block...") );
					lpClient->dwFlags &= ~CF_READREADY;
					return lpClient->RecvPending.dwAvail;
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
						Log5( WIDE("Failed reading from %d (err:%d) into %p %") _32f WIDE(" bytes %") _32f WIDE(" read already."),
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
			{           // otherwise WSAEWOULDBLOCK would be generated.
				//_lprintf( DBG_RELAY )( WIDE("Closing closed socket... Hope there's also an event... "));
				lpClient->dwFlags |= CF_TOCLOSE;
				break; // while dwAvail... try read...
				//return -1;
			}
			else
			{
				if( g.flags.bShortLogReceivedData )
				{
					LogBinary( (P_8)lpClient->RecvPending.buffer.p +
							 lpClient->RecvPending.dwUsed, min( nRecv, 64 ) );
				}
				if( g.flags.bLogReceivedData )
				{
					LogBinary( (P_8)lpClient->RecvPending.buffer.p +
							 lpClient->RecvPending.dwUsed, nRecv );
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
					if( lpClient->dwFlags & CF_CPPREAD )
					{
						lpClient->read.CPPReadComplete( lpClient->psvRead
																, lpClient->RecvPending.buffer.p
																, length );
					}
					else
					{
#ifdef LOG_PENDING
						lprintf( WIDE( "Send to application...." ) );
#endif
						lpClient->read.ReadComplete( lpClient,
															 lpClient->RecvPending.buffer.p,
															 length );
#ifdef LOG_PENDING
						lprintf( WIDE( "back from applciation... (loop to next)" ) ); // new read probably pending ehre...
#endif
						if( !(lpClient->dwFlags & CF_READPENDING ) )
						{
							lprintf( "somehow we didn't get a good read." );
						}
						continue;
					}
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
NETWORK_PROC( size_t, doReadExx)(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream, LOGICAL bWait )
{
   return doReadExx2( lpClient, lpBuffer, nBytes, bIsStream, bWait, 0 );
}

NETWORK_PROC( size_t, doReadExx2)(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream, LOGICAL bWait, int user_timeout )
{
#ifdef LOG_PENDING
	lprintf( WIDE( "Reading ... %p(%d) int %p %d (%s,%s)" ), lpClient, lpClient->Socket, lpBuffer, (_32)nBytes, bIsStream?WIDE( "stream" ):WIDE( "block" ), bWait?WIDE( "Wait" ):WIDE( "NoWait" ) );
#endif
	if( !lpClient || !lpBuffer )
		return 0; // nothing read.... ???
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
			  (_32)lpClient->RecvPending.dwAvail );
		return -1; // this I guess is an error since we're queueing another
		// read on top of existing incoming 'guaranteed data'
	}
	while( !NetworkLock( lpClient ) )
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
		NetworkUnlock( lpClient );
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
		//Log2( WIDE("Setting Read buffer pending for next select...%d(%d)")
		//				, lpClient - Clients
		//				, lpClient->Socket);
		if( lpClient->dwFlags & CF_READREADY )
		{
			lprintf( WIDE("Data already present for read...") );
			FinishPendingRead( lpClient DBG_SRC );
		}
#ifdef __LINUX__
		// if not unix, then the socket is already generating
		// WindowsMessage_ReadReady when there is something to
		// read...
		if( lpClient->dwFlags & CF_READREADY )
		{
			lprintf( WIDE("Data already present for read...") );
			int status = FinishPendingRead( lpClient DBG_SRC );
			if( lpClient->dwFlags & CF_ACTIVE )
			{
				NetworkUnlock( lpClient );
				return status; // returns bytes pending...
			}
			// else we shouldn't leave a critical section
			// of a client object which is not active...
			lprintf( WIDE("Leaving read from a bad state... adn we do not unlock.") );
			NetworkUnlock( lpClient );
			return 0;
		}
		else
		{
			lprintf( WIDE( "Not sure if READREADY" ) );
			WakeThread( g.pThread );
		}
#else
		//FinishPendingRead( lpClient DBG_SRC );
#endif
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
		int this_timeout = user_timeout?user_timeout:g.dwReadTimeout;
		int timeout = 0;
		//lprintf( WIDE("Waiting for TCP data result...") );
		{
			_32 tick = timeGetTime();
			lpClient->pWaiting = MakeThread();
			NetworkUnlock( lpClient );
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
		while( !NetworkLock( lpClient ) )
		{
			if( !(lpClient->dwFlags & CF_ACTIVE ) )
			{
				return 0;
			}
			Relinquish();
		}
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
			NetworkUnlock( lpClient );
			return -1;
		}
 		lpClient->dwFlags &= ~CF_READWAITING;
		NetworkUnlock( lpClient );
		if( timeout )
			return 0;
		else
			return lpClient->RecvPending.dwUsed;
	}
	else
		NetworkUnlock( lpClient );

	return 0; // unknown result really... success prolly
}

NETWORK_PROC( size_t, doReadEx)(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream)
{
   return doReadExx( lpClient, lpBuffer, nBytes, bIsStream, FALSE );
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
   lpPend->dwUsed   = 0;
   lpPend->lpNext   = NULL;
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
#define TCPWrite(pc) TCPWriteEx(pc DBG_SRC)
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
			if( g.flags.bLogSentData )
			{
				LogBinary( (P_8)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail );
			}
			nSent = send(pc->Socket,
							 (char*)pc->lpFirstPending->buffer.c +
							 pc->lpFirstPending->dwUsed,
							 (int)pc->lpFirstPending->dwAvail,
							 0);
			if (nSent == SOCKET_ERROR)
			{
				if( WSAGetLastError() == WSAEWOULDBLOCK )  // this is alright.
				{
#ifdef VERBOSE_DEBUG
					lprintf( WIDE("Pending write...") );
#endif
					pc->dwFlags |= CF_WRITEPENDING;
#ifdef __LINUX__
					//if( !(pc->dwFlags & CF_WRITEISPENDED ) )
					//{
					//	   lprintf( WIDE("Sending signal") );
					//    WakeThread( g.pThread );
					//}
					//else
					//    lprintf( WIDE("Yes... it was already pending..(no signal)") );
#endif

					return TRUE;
				}
				{
					_lprintf(DBG_RELAY)(WIDE(" Network Send Error: %5d(buffer:%p ofs: %") _32f WIDE("  Len: %") _32f WIDE(")"),
											  WSAGetLastError(),
											  pc->lpFirstPending->buffer.c,
											  pc->lpFirstPending->dwUsed,
											  pc->lpFirstPending->dwAvail );
					if( WSAGetLastError() == 10057 // ENOTCONN
						||WSAGetLastError() == 10014 // EFAULT
#ifdef __LINUX__
						|| WSAGetLastError() == EPIPE
#endif
					  )
					{
						InternalRemoveClient( pc );
					}
					return FALSE; // get out of here!
				}
			}
			else if (!nSent)  // other side closed.
			{
				lprintf( WIDE("sent zero bytes - assume it was closed - and HOPE there's an event...") );
				InternalRemoveClient( pc );
				// if this happened - don't return TRUE result which would
				// result in queuing a pending buffer...
				return FALSE;  // no sence processing the rest of this.
			}
		}
		else
			nSent = 0;

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
						{
							lprintf( WIDE("Finished sending a pending buffer.") );
						}
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
						if( g.flags.bLogNotices )
							lprintf( WIDE( "SET GLOBAL EVENT (write pending)" ) );
						WSASetEvent( g.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
						//kill( (_32)(g.pThread->ThreadID), SIGHUP );
						WakeThread( g.pThread );
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

NETWORK_PROC( LOGICAL, doTCPWriteExx)( PCLIENT lpClient
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

	while( !NetworkLock( lpClient ) )
	{
		if( !(lpClient->dwFlags & CF_ACTIVE ) )
		{
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
		NetworkUnlock( lpClient );
		return FALSE;
   }

   if( lpClient->lpFirstPending ) // will already be in a wait on network state...
   {
#ifdef VERBOSE_DEBUG
		Log2(  "%s(%d)Data pending, pending buffer... ", pFile, nLine );
#endif
		if( !failpending )
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE("Queuing pending data anyhow...") );
#endif
			PendWrite( lpClient, pInBuffer, nInLen, bLongBuffer );
			TCPWrite( lpClient ); // make sure we don't lose a write event during the queuing...
			NetworkUnlock( lpClient );
			return TRUE;
		}
		else
		{
#ifdef VERBOSE_DEBUG
			lprintf( WIDE("Failing pend.") );
#endif
			NetworkUnlock( lpClient );
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
		if( TCPWrite( lpClient ) )
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
	NetworkUnlock( lpClient );
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
		if( nDrainRead == SOCKET_ERROR )
		{
			if( WSAGetLastError() == WSAEWOULDBLOCK )
			{
         		if( !pClient->bDrainExact )
         			pClient->nDrainLength = 0;
				break;
			}
			Log5(WIDE(" Network Error during drain: %d (from: %d  to: %p  has: %") _32f WIDE("  toget: %") _32f WIDE(")"),
                      WSAGetLastError(),
                      pClient->Socket,
                      pClient->RecvPending.buffer.p,
                      pClient->RecvPending.dwUsed,
                      pClient->RecvPending.dwAvail );
			InternalRemoveClient( pClient );
			NetworkUnlock( pClient );
			return FALSE;
		}
		else
		{
			if( g.flags.bShortLogReceivedData )
			{
				LogBinary( byBuffer, min( nDrainRead, 64 ) );
			}
			if( g.flags.bLogReceivedData )
			{
				LogBinary( byBuffer, nDrainRead );
			}
		}
		if( nDrainRead == 0 )
		{
			InternalRemoveClient( pClient ); // closed.
			NetworkUnlock( pClient );
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
		while( !NetworkLock( pClient ) )
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
		NetworkUnlock( pClient );
		return bytes;
	}
	return 0;
}


//----------------------------------------------------------------------------

NETWORK_PROC( void, SetTCPNoDelay)( PCLIENT pClient, int bEnable )
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

NETWORK_PROC( void, SetClientKeepAlive)( PCLIENT pClient, int bEnable )
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

