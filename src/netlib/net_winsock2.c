#ifndef __LINUX__

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

#include <stdhdrs.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#include "netstruc.h"
SACK_NETWORK_NAMESPACE


   // should pass ipv4? v6? to switch?
SOCKET OpenSocket( LOGICAL v4, LOGICAL bStream, LOGICAL bRaw, int another_offset )
{
	if( another_offset )
	{
		SOCKET result;
      // need to index into saved sockets and try another provider.
		result = WSASocketW(v4?AF_INET:AF_INET6
										 , bRaw?SOCK_RAW:0
										 , 0
										 , v4
										  ?bStream
										  ?globalNetworkData.pProtos+globalNetworkData.tcp_protocol
										  :globalNetworkData.pProtos+globalNetworkData.udp_protocol
										  :bStream
										  ?globalNetworkData.pProtos+globalNetworkData.tcp_protocolv6
										  :globalNetworkData.pProtos+globalNetworkData.udp_protocolv6
										 , 0, 0 );
		return result;
	}
	else
	{
		SOCKET result = WSASocketW(v4?AF_INET:AF_INET6
										 , bRaw?SOCK_RAW:0
										 , 0
										 , v4
										  ?bStream
										  ?globalNetworkData.pProtos+globalNetworkData.tcp_protocol
										  :globalNetworkData.pProtos+globalNetworkData.udp_protocol
										  :bStream
										  ?globalNetworkData.pProtos+globalNetworkData.tcp_protocolv6
										  :globalNetworkData.pProtos+globalNetworkData.udp_protocolv6
										 , 0, 0 );
		return result;
	}
}

int SystemCheck( void )
{
	WSADATA wd;
	int i;
	int protoIndex = -1;
	int size;
	//lprintf( "Global is %d %p", sizeof( globalNetworkData ), &globalNetworkData.uNetworkPauseTimer, &globalNetworkData.nProtos );

	if (WSAStartup(MAKEWORD(2, 0), &wd) != 0)
	{
		lprintf(WIDE( "WSAStartup 2.0 failed: %d" ), h_errno);
		return 0;
	}
	if( globalNetworkData.flags.bLogProtocols )
		lprintf(WIDE( "Winsock Version: %d.%d" ), LOBYTE(wd.wVersion), HIBYTE(wd.wVersion));

	size = 0;
	if ((globalNetworkData.nProtos = WSAEnumProtocolsW(NULL, NULL, (DWORD *) &size)) == -1)
	{
		if( WSAGetLastError() != WSAENOBUFS )
		{
			lprintf(WIDE( "WSAEnumProtocols: %d" ), h_errno);
			return 0;
		}
	}

	globalNetworkData.pProtos = (WSAPROTOCOL_INFOW*)Allocate( size );
	if ((globalNetworkData.nProtos = WSAEnumProtocolsW(NULL, globalNetworkData.pProtos, (DWORD *) &size)) == -1)
	{
		lprintf(WIDE( "WSAEnumProtocols: %d" ), h_errno);
		return 0;
	}
	for (i = 0; i < globalNetworkData.nProtos; i++)
	{
		// IPv6
		if (wcscmp(globalNetworkData.pProtos[i].szProtocol, L"MSAFD Tcpip [TCP/IP]" ) == 0)
		{
			globalNetworkData.tcp_protocol = i;
			protoIndex = i;
		}
		if (wcscmp(globalNetworkData.pProtos[i].szProtocol, L"MSAFD Tcpip [UDP/IP]" ) == 0)
		{
			globalNetworkData.udp_protocol = i;
		}
		if (wcscmp(globalNetworkData.pProtos[i].szProtocol, L"MSAFD Tcpip [TCP/IPv6]" ) == 0)
		{
			globalNetworkData.tcp_protocolv6 = i;
		}
		if (wcscmp(globalNetworkData.pProtos[i].szProtocol, L"MSAFD Tcpip [UDP/IPv6]" ) == 0)
		{
			globalNetworkData.udp_protocolv6 = i;
		}
		if( globalNetworkData.flags.bLogProtocols )
			lprintf(WIDE( "Index #%d - name: '%S', type: %d, proto: %d" ), i, globalNetworkData.pProtos[i].szProtocol,
					  globalNetworkData.pProtos[i].iSocketType, globalNetworkData.pProtos[i].iProtocol);
	}
	if (protoIndex == -1)
	{
		lprintf(WIDE( "no valid TCP/IP protocol available" ));
		return 0;
	}
	return 0;
}

SACK_NETWORK_NAMESPACE_END

#endif
