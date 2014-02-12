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
		result = WSASocket(v4?AF_INET:AF_INET6
										 , bRaw?SOCK_RAW:0
										 , 0
										 , v4
										  ?bStream
										  ?g.pProtos+g.tcp_protocol
										  :g.pProtos+g.udp_protocol
										  :bStream
										  ?g.pProtos+g.tcp_protocolv6
										  :g.pProtos+g.udp_protocolv6
										 , 0, 0 );
		return result;
	}
	else
	{
		SOCKET result = WSASocket(v4?AF_INET:AF_INET6
										 , bRaw?SOCK_RAW:0
										 , 0
										 , v4
										  ?bStream
										  ?g.pProtos+g.tcp_protocol
										  :g.pProtos+g.udp_protocol
										  :bStream
										  ?g.pProtos+g.tcp_protocolv6
										  :g.pProtos+g.udp_protocolv6
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
	//lprintf( "Global is %d %p", sizeof( g ), &g.uNetworkPauseTimer, &g.nProtos );

	if (WSAStartup(MAKEWORD(2, 0), &wd) != 0)
	{
		lprintf(WIDE( "WSAStartup 2.0 failed: %d" ), h_errno);
		return 0;
	}
	if( g.flags.bLogProtocols )
		lprintf(WIDE( "Winsock Version: %d.%d" ), LOBYTE(wd.wVersion), HIBYTE(wd.wVersion));

	size = 0;
	if ((g.nProtos = WSAEnumProtocols(NULL, NULL, (DWORD *) &size)) == -1)
	{
		if( WSAGetLastError() != WSAENOBUFS )
		{
			lprintf(WIDE( "WSAEnumProtocols: %d" ), h_errno);
			return 0;
		}
	}

	g.pProtos = (WSAPROTOCOL_INFO*)Allocate( size );
	if ((g.nProtos = WSAEnumProtocols(NULL, g.pProtos, (DWORD *) &size)) == -1)
	{
		lprintf(WIDE( "WSAEnumProtocols: %d" ), h_errno);
		return 0;
	}
	for (i = 0; i < g.nProtos; i++)
	{
		// IPv6
		if (strcmp(g.pProtos[i].szProtocol, WIDE( "MSAFD Tcpip [TCP/IP]" )) == 0)
		{
			g.tcp_protocol = i;
			protoIndex = i;
		}
		if (strcmp(g.pProtos[i].szProtocol, WIDE( "MSAFD Tcpip [UDP/IP]" )) == 0)
		{
			g.udp_protocol = i;
		}
		if (strcmp(g.pProtos[i].szProtocol, WIDE( "MSAFD Tcpip [TCP/IPv6]" )) == 0)
		{
			g.tcp_protocolv6 = i;
		}
		if (strcmp(g.pProtos[i].szProtocol, WIDE( "MSAFD Tcpip [UDP/IPv6]" )) == 0)
		{
			g.udp_protocolv6 = i;
		}
		if( g.flags.bLogProtocols )
			lprintf(WIDE( "Index #%d - name: '%s', type: %d, proto: %d" ), i, g.pProtos[i].szProtocol,
					  g.pProtos[i].iSocketType, g.pProtos[i].iProtocol);
	}
	if (protoIndex == -1)
	{
		lprintf(WIDE( "no valid TCP/IP protocol available" ));
		return 0;
	}
	return 0;
}

SACK_NETWORK_NAMESPACE_END

