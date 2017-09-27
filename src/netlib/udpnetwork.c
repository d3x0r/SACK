#define LIBRARY_DEF

#ifdef __LINUX__
#include <sys/ioctl.h>
#include <signal.h> // SIGHUP defined

#define NetWakeSignal SIGHUP
#else
#define ioctl ioctlsocket
#endif
#include <stdhdrs.h>

#include "netstruc.h"
#include <network.h>
#include <signed_unsigned_comparisons.h>
#include <sharemem.h>
//#define NO_LOGGING // force neverlog....
#define DO_LOGGING
#include "logging.h"


SACK_NETWORK_NAMESPACE

// local extern in network.c
void DumpSocket( PCLIENT pc );

_UDP_NAMESPACE

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PCLIENT CPPServeUDPAddrEx( SOCKADDR *pAddr
                         , cReadCompleteEx pReadComplete
                         , uintptr_t psvRead
                         , cCloseCallback Close
                         , uintptr_t psvClose
                         , int bCPP DBG_PASS )
{
	PCLIENT pc;
	// open a UDP Port to listen for Pings for server...

	pc = GetFreeNetworkClient();
	if( !pc )
	{
		Log( WIDE("Network Resource Fail"));
		return NULL;
	}

#ifdef WIN32
	pc->Socket = OpenSocket(((*(uint16_t*)pAddr) == AF_INET)?TRUE:FALSE,FALSE, FALSE, 0);
	if( pc->Socket == INVALID_SOCKET )
#endif
		pc->Socket = socket( PF_INET
		                   , SOCK_DGRAM
		                   , (((*(uint16_t*)pAddr) == AF_INET)||((*(uint16_t*)pAddr) == AF_INET6))?IPPROTO_UDP:0);
	if( pc->Socket == INVALID_SOCKET )
	{
		_lprintf(DBG_RELAY)( WIDE("UDP Socket Fail") );
		InternalRemoveClientEx( pc, TRUE, FALSE );
		NetworkUnlock( pc );
		return NULL;
	}
#if WIN32
	if( 0 )
	{
		DWORD dwFlags;
		GetHandleInformation( (HANDLE)pc->Socket, &dwFlags );
		lprintf( WIDE( "Natural was %d" ), dwFlags );
	}
	SetHandleInformation( (HANDLE)pc->Socket, HANDLE_FLAG_INHERIT, 0 );
#endif
#ifdef LOG_SOCKET_CREATION
	lprintf( WIDE( "Created UDP %p(%d)" ), pc, pc->Socket );
#endif
	pc->dwFlags |= CF_UDP;

	if( pAddr? pc->saSource = DuplicateAddress( pAddr ),1:0 )
	{
		if (bind(pc->Socket,pc->saSource,SOCKADDR_LENGTH(pc->saSource)))
		{
			uint32_t err = WSAGetLastError();
			_lprintf(DBG_RELAY)( WIDE("Bind Fail: %d"), err );
			DumpAddr( WIDE( "BIND FAIL:" ), pc->saSource );
			InternalRemoveClientEx( pc, TRUE, FALSE );
			NetworkUnlock( pc );
			return NULL;
		}
		// get the port address back immediately.
		getsockname(pc->Socket, pc->saSource, (socklen_t*)(((uintptr_t)pc->saSource)-2*sizeof(uintptr_t)));
	}
	else
	{
		Log( WIDE("Bind Will Fail") );
		InternalRemoveClientEx( pc, TRUE, FALSE );
		NetworkUnlock( pc );
		return NULL;
	}

#ifdef _WIN32
#  ifdef USE_WSA_EVENTS
	pc->event = WSACreateEvent();
	WSAEventSelect( pc->Socket, pc->event, FD_READ|FD_WRITE );
#  else
	if (WSAAsyncSelect( pc->Socket,
	                   globalNetworkData.ghWndNetwork,
	                   SOCKMSG_UDP,
	                   FD_READ|FD_WRITE ) )
	{
		Log( WIDE("Select Fail"));
		InternalRemoveClientEx( pc, TRUE, FALSE );
		NetworkUnlock( pc );
		return NULL;
	}
#  endif
#else
	{
		int t = TRUE;
		ioctl( pc->Socket, FIONBIO, &t );
	}
#endif
	pc->read.ReadCompleteEx = pReadComplete;
	pc->psvRead = psvRead;
	pc->close.CloseCallback = Close;
	pc->psvClose = psvClose;
	if( bCPP )
		pc->dwFlags |= (CF_CPPREAD|CF_CPPCLOSE );
#ifdef __LINUX__
	AddThreadEvent( pc, 0 );
#endif
	if( pReadComplete )
	{
		if( pc->dwFlags & CF_CPPREAD )
			pc->read.CPPReadCompleteEx( pc->psvRead, NULL, 0, pc->saSource );
		else
			pc->read.ReadCompleteEx( pc, NULL, 0, pc->saSource );
	}else
		lprintf( "NO READ CALLBACK IS SET?!" );

#ifdef LOG_SOCKET_CREATION
	DumpSocket( pc );
#endif
	AddActive( pc );

	NetworkUnlock( pc );
#ifdef USE_WSA_EVENTS
	if( globalNetworkData.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (new udp socket %p)" ), pc );
	EnqueLink( &globalNetworkData.client_schedule, pc );
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#endif
	return pc;
}

//----------------------------------------------------------------------------

PCLIENT ServeUDPAddrEx( SOCKADDR *pAddr
                      , cReadCompleteEx pReadComplete
                      , cCloseCallback Close DBG_PASS )
{
	PCLIENT result = CPPServeUDPAddrEx( pAddr, pReadComplete, 0, Close, 0, FALSE DBG_RELAY );
	if( result )
		result->dwFlags &= ~(CF_CPPREAD|CF_CPPCLOSE);
	return result;
}

//----------------------------------------------------------------------------

PCLIENT ServeUDPEx( CTEXTSTR pAddr, uint16_t wPort
                  , cReadCompleteEx pReadComplete
                  , cCloseCallback Close DBG_PASS )
{
	SOCKADDR *lpMyAddr;

	if( pAddr )
		lpMyAddr = CreateSockAddress( pAddr, wPort);
	else
	{
		// NOTE this is NOT create local which binds to only
		// one address and port - this IS "0.0.0.0" which is
		// any IP inteface on the box...
		lpMyAddr = CreateSockAddress( WIDE("0.0.0.0"), wPort ); // assume bind to any address...
	}

	return ServeUDPAddrEx( lpMyAddr, pReadComplete, Close DBG_RELAY );
}

//----------------------------------------------------------------------------

void UDPEnableBroadcast( PCLIENT pc, int bEnable )
{
	if( pc ) {
#ifdef __LINUX__
		if( bEnable ) {
			uint16_t port;
			SOCKADDR *broadcastAddr;
			//RemoveThreadEvent( pc );
			//pc->Socket = close( pc->Socket );
			pc->interfaceAddress = GetInterfaceForAddress( pc->saSource );
			if( pc->interfaceAddress ) {
				pc->SocketBroadcast = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
				{
					int t = TRUE;
					ioctl( pc->SocketBroadcast, FIONBIO, &t );
				}
				broadcastAddr = DuplicateAddress( GetBroadcastAddressForInterface( pc->saSource ) );
				GetAddressParts( pc->saSource, NULL, &port );
				SetAddressPort( broadcastAddr, port );
				if( bind( pc->SocketBroadcast, broadcastAddr, SOCKADDR_LENGTH( broadcastAddr ) ) ) {
					lprintf( "Failed to rebind to broadcast address when enabling... %d", errno );
				}
				if( setsockopt( pc->SocketBroadcast, SOL_SOCKET
				              , SO_BROADCAST, (char*)&bEnable, sizeof( bEnable ) ) )
				{
					uint32_t error = WSAGetLastError();
					lprintf( WIDE("Failed to set sock opt - BROADCAST(%d)"), error );
				}
				AddThreadEvent( pc, 1 );
#ifdef __LINUX__
				// need to set EAGAIN state on socket.
				FinishUDPRead( pc, 1 );  // do actual read.... (results in read callback)
#endif
				ReleaseAddress( broadcastAddr );
			} else
				lprintf( "Network interface for socket address not found." );
		}
#else
		if( setsockopt( pc->Socket, SOL_SOCKET
		              , SO_BROADCAST, (char*)&bEnable, sizeof( bEnable ) ) )
		{
			uint32_t error = WSAGetLastError();
			lprintf( WIDE("Failed to set sock opt - BROADCAST(%d)"), error );
		}
#endif
	}
}

//----------------------------------------------------------------------------

LOGICAL GuaranteeAddr( PCLIENT pc, SOCKADDR *sa )
{
	if( sa )
	{
		pc->saClient = DuplicateAddress( sa );
	}
	else
		return FALSE;

#ifdef VERBOSE_DEBUG
	DumpSocket( pc );
#endif
	return TRUE;
}

//----------------------------------------------------------------------------

LOGICAL Guarantee( PCLIENT pc, CTEXTSTR pAddr, uint16_t wPort )
{
	SOCKADDR *lpMyAddr = CreateSockAddress( pAddr, wPort);
	int res = GuaranteeAddr( pc, lpMyAddr );
	ReleaseAddress( lpMyAddr );
	return res;
}

//----------------------------------------------------------------------------

PCLIENT CPPConnectUDPAddrEx( SOCKADDR *sa
                           , SOCKADDR *saTo
                           , cReadCompleteEx pReadComplete
                           , uintptr_t psvRead
                           , cCloseCallback Close
                           , uintptr_t psvClose DBG_PASS )
{
	PCLIENT pc;
	int bFixed = FALSE;
	if( !sa )
	{
		sa = CreateLocal( 0 ); // use any address...
		bFixed = TRUE;
	}
	pc = ServeUDPAddrEx( sa, NULL, NULL DBG_RELAY );
	if( !pc )
	{
		Log( WIDE("Failed to establish incoming side of UDP Socket") );
		return NULL;
	}
	if( !GuaranteeAddr( pc, saTo ) )
	{
		Log( WIDE("Failed to set guaranteed UDP Send Address."));
		InternalRemoveClient( pc );
		return NULL;
	}

	if( bFixed )
		ReleaseAddress( sa );

	pc->read.ReadCompleteEx = pReadComplete;
	pc->psvRead = psvRead;
	pc->close.CloseCallback = Close;
	pc->psvClose = psvClose;
	pc->dwFlags |= (CF_CPPREAD|CF_CPPCLOSE );

	if( pReadComplete )
		pReadComplete( pc, NULL, 0, NULL ); // allow server to start a read method...

	return pc;
}

PCLIENT CPPConnectUDPAddr( SOCKADDR *sa
                         , SOCKADDR *saTo
                         , cReadCompleteEx pReadComplete
                         , uintptr_t psvRead
                         , cCloseCallback Close
                         , uintptr_t psvClose )
{
	return CPPConnectUDPAddrEx( sa, saTo, pReadComplete, psvRead, Close, psvClose DBG_SRC );
}
//----------------------------------------------------------------------------

PCLIENT ConnectUDPAddrEx( SOCKADDR *sa
                        , SOCKADDR *saTo
                        , cReadCompleteEx pReadComplete
                        , cCloseCallback Close DBG_PASS )
{
	PCLIENT result = CPPConnectUDPAddrEx( sa, saTo, pReadComplete, 0, Close, 0 DBG_RELAY );
	if( result )
		result->dwFlags &= ~( CF_CPPREAD|CF_CPPCLOSE );
	return result;	
}

//----------------------------------------------------------------------------

static PCLIENT CPPConnectUDPExx( CTEXTSTR pFromAddr, uint16_t wFromPort
                               , CTEXTSTR pToAddr, uint16_t wToPort
                               , cReadCompleteEx pReadComplete
                               , uintptr_t psvRead
                               , cCloseCallback Close
                               , uintptr_t psvClose
                               , LOGICAL bCPP
                               DBG_PASS )
{
	PCLIENT pc;

	pc = ServeUDPEx( pFromAddr, wFromPort, NULL, NULL DBG_RELAY );
	if( !pc )
	{
		Log( WIDE("Failed to establish incoming side of UDP Socket") );
		return NULL;
	}
	if( !Guarantee( pc, pToAddr, wToPort ) )
	{
		Log( WIDE("Failed to set guaranteed UDP Send Address."));
		InternalRemoveClient( pc );
		return NULL;
	}

	pc->read.ReadCompleteEx = pReadComplete;
	pc->psvRead = psvRead;
	pc->close.CloseCallback = Close;
	pc->psvClose = psvClose;
	if( bCPP )
	{
		pc->dwFlags |= (CF_CPPREAD|CF_CPPCLOSE );
		if( pReadComplete )
			pc->read.CPPReadCompleteEx( psvRead, NULL, 0, NULL ); // allow server to start a read method...
	}
	else
	{
		pc->dwFlags &= ~( CF_CPPREAD|CF_CPPCLOSE );
		if( pReadComplete )
			pReadComplete( pc, NULL, 0, NULL ); // allow server to start a read method...
	}
	return pc;
}

PCLIENT CPPConnectUDPEx( CTEXTSTR pFromAddr, uint16_t wFromPort,
                         CTEXTSTR pToAddr, uint16_t wToPort,
                         cReadCompleteEx pReadComplete,
                         uintptr_t psvRead,
                         cCloseCallback Close,
                         uintptr_t psvClose DBG_PASS )
{
	return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, psvRead, Close, psvClose, TRUE DBG_RELAY );
}

PCLIENT ConnectUDPEx( CTEXTSTR pFromAddr, uint16_t wFromPort,
                      CTEXTSTR pToAddr, uint16_t wToPort,
                      cReadCompleteEx pReadComplete,
                      cCloseCallback Close DBG_PASS )
{
	return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, 0, Close, 0, FALSE DBG_RELAY );
}

//----------------------------------------------------------------------------

NETWORK_PROC( LOGICAL, SendUDPEx )( PCLIENT pc, CPOINTER pBuf, size_t nSize, SOCKADDR *sa )
{
	int nSent;
	SOCKET sendSocket = pc->Socket;
	if( !sa)
		sa = pc->saClient;
	if( !sa )
      return FALSE;
	if( !pc )
		return FALSE;
#ifdef __LINUX__
	if( IsBroadcastAddressForInterface( pc->interfaceAddress, sa ) ) {
		sendSocket = pc->SocketBroadcast;
	}
#endif
	//LogBinary( (uint8_t*)pBuf, nSize );
	nSent = sendto( sendSocket
	              , (const char*)pBuf
	              , (int)nSize
	              , 0
	              , (sa)
	              , SOCKADDR_LENGTH((sa))
	              );
	if( nSent < 0 )
	{
		Log1( WIDE("SendUDP: Error (%d)"), WSAGetLastError() );
		DumpAddr( WIDE( "SendTo Socket" ), (sa) );
		return FALSE;
	}
	else if( SUS_LT( nSent, int, nSize, size_t ) ) // this is all so very vague.....
	{
		Log( WIDE("SendUDP: Small send :(") );
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------

NETWORK_PROC( LOGICAL, ReconnectUDP )( PCLIENT pc, CTEXTSTR pToAddr, uint16_t wPort )
{
	return Guarantee( pc, pToAddr, wPort );
}

//----------------------------------------------------------------------------
extern uint32_t uNetworkPauseTimer, uTCPPendingTimer;

//----------------------------------------------------------------------------

NETWORK_PROC( int, doUDPRead )( PCLIENT pc, POINTER lpBuffer, int nBytes )
{
	if( pc->RecvPending.dwAvail )
	{
		lprintf( WIDE("Read already pending for %")_size_fs WIDE("... not doing anything for this one..")
		        , pc->RecvPending.dwAvail );
		return FALSE;
	}
	//Log1( WIDE("UDPRead Pending:%d bytes"), nBytes );
	pc->RecvPending.dwAvail = nBytes;
	pc->RecvPending.dwUsed = 0;
	pc->RecvPending.buffer.p = lpBuffer;
	{
		pc->dwFlags |= CF_READPENDING;
		// we are now able to read, so schedule the socket.
	}
#if 0
#ifdef __LINUX__
	// need to set EAGAIN state on socket.
	FinishUDPRead( pc, 0 );  // do actual read.... (results in read callback)
	if( pc->SocketBroadcast )
		FinishUDPRead( pc, 1 );  // do actual read.... (results in read callback)
#endif
#endif
	return TRUE;
}

//----------------------------------------------------------------------------

int FinishUDPRead( PCLIENT pc, int broadcastEvent )
{  // all UDP Reads return the address of the other side's message...
	int nReturn;
#ifdef __LINUX__
	socklen_t
#else
		int
#endif
		Size=MAGIC_SOCKADDR_LENGTH;  // echoed from server.

	if( !pc->RecvPending.buffer.p || !pc->RecvPending.dwAvail )
	{
		//lprintf( WIDE("UDP Read without queued buffer for result. %p %") _size_fs, pc->RecvPending.buffer.p, pc->RecvPending.dwAvail );
		return FALSE;
	}
	//do{
	if( !pc->saLastClient )
		pc->saLastClient = AllocAddr();
	nReturn = recvfrom( broadcastEvent?pc->SocketBroadcast:pc->Socket,
	                    (char*)pc->RecvPending.buffer.p,
	                    (int)pc->RecvPending.dwAvail,0,
	                    pc->saLastClient,
	                    &Size);// get address...
	uintptr_t name = (((uintptr_t*)pc->saLastClient) - 1)[0];
	if( name ) free( (void*)name );
	(((uintptr_t*)pc->saLastClient) - 1)[0] = 0;
	// address size in recvfrom on linux results with '256' which
	// then results with a sockaddr that can't sendto with.
	if( pc->saLastClient->sa_family == AF_INET )
		SET_SOCKADDR_LENGTH( pc->saLastClient, IN_SOCKADDR_LENGTH );
	else if( pc->saLastClient->sa_family == AF_INET6 )
		SET_SOCKADDR_LENGTH( pc->saLastClient, IN6_SOCKADDR_LENGTH );
	//lprintf( WIDE("Recvfrom result:%d"), nReturn );

	if (nReturn == SOCKET_ERROR)
	{
		uint32_t dwErr = WSAGetLastError();
		// shutdown the udp socket... not needed...???
		//lprintf( WIDE("Recvfrom result:%d"), dwErr );
		switch( dwErr )
		{
		case WSAEWOULDBLOCK: // NO data returned....
			//lprintf( "got EWOULDBLOCK(EAGAIN)..." );
			pc->dwFlags |= CF_READPENDING;
			return TRUE;
#ifdef _WIN32
		// this happens on WIN2K/XP - ICMP Port Unreachable (nothing listening there)
		case WSAECONNRESET: // just ignore this error.
  			Log( WIDE("ICMP Port unreachable on previous send.") );
			return TRUE;
#endif
		default:
				Log2( WIDE("FinishUDPRead Unknown error: %d %") _size_fs WIDE(""), WSAGetLastError(), pc->RecvPending.dwAvail );
			InternalRemoveClient( pc );
			return FALSE;
			break;
		}

	}
	//Log1( WIDE("UDPRead:%d bytes"), nReturn );
	if( globalNetworkData.flags.bShortLogReceivedData )
	{
		DumpAddr( WIDE("UDPRead at"), pc->saSource );
		LogBinary( (uint8_t*)pc->RecvPending.buffer.p +
					 pc->RecvPending.dwUsed, ( nReturn< 64 )?nReturn:64 );
	}
	if( globalNetworkData.flags.bLogReceivedData )
	{
		DumpAddr( WIDE("UDPRead at"), pc->saSource );
		LogBinary( (uint8_t*)pc->RecvPending.buffer.p +
					 pc->RecvPending.dwUsed, nReturn );
	}
	pc->dwFlags &= ~CF_READPENDING;
	pc->RecvPending.dwAvail = 0;  // allow further reads...
	pc->RecvPending.dwUsed += nReturn;

	if( pc->read.ReadCompleteEx )
	{
		if( pc->dwFlags & CF_CPPREAD )
			pc->read.CPPReadCompleteEx( pc->psvRead, pc->RecvPending.buffer.p, nReturn, pc->saLastClient );
		else
		{
			//lprintf( WIDE("Calling UDP complete %p %p %d"), pc, pc->RecvPending.buffer.p, nReturn );
			pc->read.ReadCompleteEx( pc, pc->RecvPending.buffer.p, nReturn, pc->saLastClient );
		}
	}
	//}while(1);
	return TRUE;
}

int SetSocketReuseAddress( PCLIENT lpClient, int32_t enable )
{
	if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEADDR,
						(char*)&enable, sizeof(enable)) <0 )
	{
		return 0;
		//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse addr." << endl;
	}
	return 1;
}

int SetSocketReusePort( PCLIENT lpClient, int32_t enable )
{
#ifdef __LINUX__
	if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEPORT,
						(char*)&enable, sizeof(enable)) < 0 )
	{
		return 0;
		//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse addr." << endl;
	}
#else
	SetSocketReuseAddress( lpClient, enable );
#endif
	return 1;
}


_UDP_NAMESPACE_END
SACK_NETWORK_NAMESPACE_END

