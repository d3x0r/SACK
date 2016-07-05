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
#include <deadstart.h>
#include <procreg.h>
//#define NO_LOGGING // force neverlog....
#define DO_LOGGING
#include "logging.h"


SACK_NETWORK_NAMESPACE

void DumpSocket( PCLIENT pc );

_UDP_NAMESPACE

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


NETWORK_PROC( PCLIENT, CPPServeUDPAddrEx )( SOCKADDR *pAddr
                  , cReadCompleteEx pReadComplete
                  , PTRSZVAL psvRead
                  , cCloseCallback Close
													 , PTRSZVAL psvClose
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
	pc->Socket = OpenSocket(((*(_16*)pAddr) == AF_INET)?TRUE:FALSE,FALSE, FALSE, 0);
	if( pc->Socket == INVALID_SOCKET )
#endif
		pc->Socket = socket(PF_INET,SOCK_DGRAM,(((*(_16*)pAddr) == AF_INET)||((*(_16*)pAddr) == AF_INET6))?IPPROTO_UDP:0);
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
#ifdef LOG_SOCKET_CREATION
      Log8( WIDE(" %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d"),
       				*(((unsigned char *)pAddr)+0),
       				*(((unsigned char *)pAddr)+1),
       				*(((unsigned char *)pAddr)+2),
       				*(((unsigned char *)pAddr)+3),
       				*(((unsigned char *)pAddr)+4),
       				*(((unsigned char *)pAddr)+5),
       				*(((unsigned char *)pAddr)+6),
       				*(((unsigned char *)pAddr)+7) );
#endif
		if (bind(pc->Socket,pc->saSource,SOCKADDR_LENGTH(pc->saSource)))
		{
			_32 err = WSAGetLastError();
			_lprintf(DBG_RELAY)( WIDE("Bind Fail: %d"), err );
			DumpAddr( WIDE( "BIND FAIL:" ), pc->saSource );
			InternalRemoveClientEx( pc, TRUE, FALSE );
			NetworkUnlock( pc );
			return NULL;
		}
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
                       g.ghWndNetwork,
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
	if( pReadComplete )
	{
   		if( pc->dwFlags & CF_CPPREAD )
			pc->read.CPPReadCompleteEx( pc->psvRead, NULL, 0, pc->saSource );
		else
			pc->read.ReadCompleteEx( pc, NULL, 0, pc->saSource );
	}

#ifdef LOG_SOCKET_CREATION
	DumpSocket( pc );
#endif
	AddActive( pc );
	NetworkUnlock( pc );
#ifdef USE_WSA_EVENTS
	if( g.flags.bLogNotices )
		lprintf( WIDE( "SET GLOBAL EVENT (new udp socket %p)" ), pc );
	WSASetEvent( g.hMonitorThreadControlEvent );
#endif
   return pc;
}

#undef CPPServeUDPAddr
NETWORK_PROC( PCLIENT, CPPServeUDPAddr )( SOCKADDR *pAddr
                  , cReadCompleteEx pReadComplete
                  , PTRSZVAL psvRead
                  , cCloseCallback Close
													 , PTRSZVAL psvClose
													 , int bCPP )
{
   return CPPServeUDPAddrEx( pAddr, pReadComplete, psvRead, Close, psvClose, bCPP DBG_SRC );
}
//----------------------------------------------------------------------------

#undef ServeUDPAddr
NETWORK_PROC( PCLIENT, ServeUDPAddrEx )( SOCKADDR *pAddr,
                  cReadCompleteEx pReadComplete,
                  cCloseCallback Close DBG_PASS )
{
	PCLIENT result = CPPServeUDPAddrEx( pAddr, pReadComplete, 0, Close, 0, FALSE DBG_RELAY );
   if( result )
		result->dwFlags &= ~(CF_CPPREAD|CF_CPPCLOSE);
	return result;
}

NETWORK_PROC( PCLIENT, ServeUDPAddr )( SOCKADDR *pAddr,
                  cReadCompleteEx pReadComplete,
                  cCloseCallback Close)
{
   return ServeUDPAddrEx( pAddr, pReadComplete, Close DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, ServeUDPEx )( CTEXTSTR pAddr, _16 wPort,
                  cReadCompleteEx pReadComplete,
                  cCloseCallback Close DBG_PASS )
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

#undef ServeUDP
NETWORK_PROC( PCLIENT, ServeUDP )( CTEXTSTR pAddr, _16 wPort,
                  cReadCompleteEx pReadComplete,
                  cCloseCallback Close)
{
	return ServeUDPEx( pAddr, wPort, pReadComplete, Close DBG_SRC );
}
//----------------------------------------------------------------------------
NETWORK_PROC( void, UDPEnableBroadcast)( PCLIENT pc, int bEnable )
{
   if( pc )
      if( setsockopt( pc->Socket, SOL_SOCKET
                  , SO_BROADCAST, (char*)&bEnable, sizeof( bEnable ) ) )
		{
			_32 error = WSAGetLastError();
			lprintf( WIDE("Failed to set sock opt - BROADCAST(%d)"), error );
		}
}

//----------------------------------------------------------------------------

LOGICAL GuaranteeAddr( PCLIENT pc, SOCKADDR *sa )
{
   int broadcast;

   if( sa )
   {
      pc->saClient=DuplicateAddress( sa );
      if( *(int*)(pc->saClient->sa_data+2) == -1 )
         broadcast = TRUE;
      else
			broadcast = FALSE;
#ifdef VERBOSE_DEBUG
		if( broadcast )
			Log( WIDE("Setting socket to broadcast!") );
		else
			Log( WIDE("Setting socket as not broadcast!") );
#endif
      UDPEnableBroadcast( pc, broadcast );
      //if( setsockopt( pc->Socket, SOL_SOCKET
      //            , SO_BROADCAST, (char*)&broadcast, sizeof( broadcast) ) )
		//{
		//	Log( WIDE("Failed to set sock opt - BROADCAST") );
		//}

      // if we don't connect we can use SendTo ..(95 limit only)
//      if( connect( pc->Socket, lpMyAddr, sizeof(SOCKADDR ) ) )
//      {
//         ReleaseAddress( lpMyAddr );
//   	   Log(" Connect on UDP Fail..." );
//   	   return FALSE;
//      }
   }
   else
      return FALSE;

#ifdef VERBOSE_DEBUG
	DumpSocket( pc );
#endif
   return TRUE;
}

//----------------------------------------------------------------------------

LOGICAL Guarantee( PCLIENT pc, CTEXTSTR pAddr, _16 wPort )
{
	SOCKADDR *lpMyAddr = CreateSockAddress( pAddr, wPort);
	int res = GuaranteeAddr( pc, lpMyAddr );
	ReleaseAddress( lpMyAddr );
	return res;
}

//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, CPPConnectUDPAddrEx)( SOCKADDR *sa,
                        SOCKADDR *saTo, 
                    cReadCompleteEx pReadComplete,
                    PTRSZVAL psvRead, 
                    cCloseCallback Close,
                    PTRSZVAL psvClose DBG_PASS )
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

NETWORK_PROC( PCLIENT, CPPConnectUDPAddr)( SOCKADDR *sa, 
                        SOCKADDR *saTo, 
                    cReadCompleteEx pReadComplete,
                    PTRSZVAL psvRead, 
                    cCloseCallback Close,
														PTRSZVAL psvClose )
{
	return CPPConnectUDPAddrEx( sa, saTo, pReadComplete, psvRead, Close, psvClose DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, ConnectUDPAddrEx)( SOCKADDR *sa,
                        SOCKADDR *saTo, 
                    cReadCompleteEx pReadComplete,
                    cCloseCallback Close DBG_PASS )
{
	PCLIENT result = CPPConnectUDPAddrEx( sa, saTo, pReadComplete, 0, Close, 0 DBG_RELAY );
	if( result )
		result->dwFlags &= ~( CF_CPPREAD|CF_CPPCLOSE );
	return result;	
}

#undef ConnectUDPAddr
NETWORK_PROC( PCLIENT, ConnectUDPAddr)( SOCKADDR *sa, 
													SOCKADDR *saTo,
													cReadCompleteEx pReadComplete,
													cCloseCallback Close )
{
   return ConnectUDPAddrEx( sa, saTo, pReadComplete, Close DBG_SRC );
}
//----------------------------------------------------------------------------

static PCLIENT CPPConnectUDPExx ( CTEXTSTR pFromAddr, _16 wFromPort,
                    CTEXTSTR pToAddr, _16 wToPort,
                    cReadCompleteEx pReadComplete,
                    PTRSZVAL psvRead, 
                    cCloseCallback Close,
											PTRSZVAL psvClose
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

NETWORK_PROC( PCLIENT, CPPConnectUDPEx )( CTEXTSTR pFromAddr, _16 wFromPort,
                    CTEXTSTR pToAddr, _16 wToPort,
                    cReadCompleteEx pReadComplete,
                    PTRSZVAL psvRead, 
                    cCloseCallback Close,
                    PTRSZVAL psvClose DBG_PASS )
{
   return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, psvRead, Close, psvClose, TRUE DBG_RELAY );
}

#undef CPPConnectUDP
NETWORK_PROC( PCLIENT, CPPConnectUDP )( CTEXTSTR pFromAddr, _16 wFromPort,
                    CTEXTSTR pToAddr, _16 wToPort,
                    cReadCompleteEx pReadComplete,
                    PTRSZVAL psvRead, 
                    cCloseCallback Close,
													PTRSZVAL psvClose )
{
   return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, psvRead, Close, psvClose, TRUE DBG_SRC );
}

NETWORK_PROC( PCLIENT, ConnectUDPEx )( CTEXTSTR pFromAddr, _16 wFromPort,
                    CTEXTSTR pToAddr, _16 wToPort,
                    cReadCompleteEx pReadComplete,
                    cCloseCallback Close DBG_PASS )
{
	return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, 0, Close, 0, FALSE DBG_RELAY );
}

#undef ConnectUDP
NETWORK_PROC( PCLIENT, ConnectUDP )( CTEXTSTR pFromAddr, _16 wFromPort,
                    CTEXTSTR pToAddr, _16 wToPort,
                    cReadCompleteEx pReadComplete,
                    cCloseCallback Close )
{
   return CPPConnectUDPExx( pFromAddr, wFromPort, pToAddr, wToPort, pReadComplete, 0, Close, 0, FALSE DBG_SRC );
}
//----------------------------------------------------------------------------

NETWORK_PROC( LOGICAL, SendUDPEx )( PCLIENT pc, CPOINTER pBuf, size_t nSize, SOCKADDR *sa )
{
	int nSent;
	if( !sa)
		sa = pc->saClient;
	if( !pc )
		return FALSE;
	nSent = sendto( pc->Socket
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

NETWORK_PROC( LOGICAL, ReconnectUDP )( PCLIENT pc, CTEXTSTR pToAddr, _16 wPort )
{
   return Guarantee( pc, pToAddr, wPort );
}

//----------------------------------------------------------------------------
extern _32 uNetworkPauseTimer,
           uTCPPendingTimer;

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
#ifdef USE_WSA_EVENTS
		if( g.flags.bLogNotices )
			lprintf( WIDE( "SET GLOBAL EVENT (set readpending)" ) );
		WSASetEvent( g.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
		{
			WakeThread( g.pThread );
		}
#endif
	}
   //FinishUDPRead( pc );  // do actual read.... (results in read callback)
   return TRUE;
}

//----------------------------------------------------------------------------

int FinishUDPRead( PCLIENT pc )
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
		lprintf( WIDE("UDP Read without queued buffer for result. %p %") _size_fs, pc->RecvPending.buffer.p, pc->RecvPending.dwAvail );
		return FALSE;
	}
	//do{
	if( !pc->saLastClient )
		pc->saLastClient = AllocAddr();
	nReturn = recvfrom( pc->Socket,
                       (char*)pc->RecvPending.buffer.p,
                       (int)pc->RecvPending.dwAvail,0,
                       pc->saLastClient,
							 &Size);// get address...
   SET_SOCKADDR_LENGTH( pc->saLastClient, Size );
	//lprintf( WIDE("Recvfrom result:%d"), nReturn );

	if (nReturn == SOCKET_ERROR)
	{
		_32 dwErr = WSAGetLastError();
		// shutdown the udp socket... not needed...???
		//lprintf( WIDE("Recvfrom result:%d"), dwErr );
		switch( dwErr )
		{
		case WSAEWOULDBLOCK: // NO data returned....
			pc->dwFlags |= CF_READPENDING;
#ifdef USE_WSA_EVENTS
			if( g.flags.bLogNotices )
				 lprintf( WIDE( "SET GLOBAL EVENT (set read pending)" ) );
			WSASetEvent( g.hMonitorThreadControlEvent );
#endif
#ifdef __LINUX__
			{
				WakeThread( g.pThread );
			}
#endif
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
	if( g.flags.bShortLogReceivedData )
	{
		DumpAddr( WIDE("UDPRead at"), pc->saSource );
		LogBinary( (P_8)pc->RecvPending.buffer.p +
					 pc->RecvPending.dwUsed, min( nReturn, 64 ) );
	}
	if( g.flags.bLogReceivedData )
	{
		DumpAddr( WIDE("UDPRead at"), pc->saSource );
		LogBinary( (P_8)pc->RecvPending.buffer.p +
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
_UDP_NAMESPACE_END
SACK_NETWORK_NAMESPACE_END

