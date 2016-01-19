#include <stdhdrs.h>
#define LIBRARY_DEF
#include <stdio.h>
#ifdef __LINUX__
// close() for closesocket() alias.
#include <unistd.h>
#endif
#include <network.h>

#include <logging.h>
#include <string.h>

SACK_NETWORK_NAMESPACE

TEXTCHAR DefaultServer[] = WIDE( "whois.nsiregistry.net:43" ); //whois";

LOGICAL DoWhois( CTEXTSTR pHost, CTEXTSTR pServer, PVARTEXT pvtResult )
{
	//TEXTCHAR *pStartResult = pResult;
	SOCKET S;
	SOCKADDR *sa1;
	// hey if noone started it by now - they ain't gonna, and I will.
	NetworkStart();
	//SetSystemLog( SYSLOG_FILE, stdout );
	if( !pHost )
	{
		vtprintf( pvtResult, WIDE("Must specify a host, handle, or domain name\n") );
		return FALSE;
	}
	S = socket( 2,1,6 );
	if( S == INVALID_SOCKET )
	{
		vtprintf( pvtResult, WIDE("Could not allocate socket resource\n") );
		return FALSE;
	}

	if( !pServer )
		pServer = DefaultServer;

	sa1 = CreateSockAddress( pServer, 43 );
	if( connect( S, sa1, sizeof(*sa1) ) )
	{
		vtprintf( pvtResult, WIDE("Failed to connect (%d)\n"), WSAGetLastError());
		closesocket( S );
		return FALSE;
	}

	{
		static TEXTCHAR buf[4096];
		int  l;

		l = tnprintf( buf, sizeof( buf ), WIDE("%s\n"), pHost );

		if( send( S, (char*)buf, l, 0 ) != l )
		{
			vtprintf( pvtResult, WIDE("Failed to be able to write data to the network\n") );
			closesocket( S );
			return FALSE;
		}

		// insert WAIT FOR RESPONCE code....
		//vtprintf( pvtResult, WIDE("Version 1.0   ADA Software Developers, Inc.  Copyright 1999.\n") );
		while( ( l = recv( S, (char*)buf, sizeof( buf ), 0 ) ) > 0 )
		{
			if( l < sizeof(buf) )
				buf[l] = 0;
			vtprintf( pvtResult, WIDE("%s"), buf );
		}
		{
			TEXTSTR pNext, pEnd;
			PTEXT result = VarTextGet( pvtResult );
			if( ( pNext = (TEXTSTR)StrCaseStr( GetText( result ), WIDE("Whois Server: ") ) ) )
			{
				pNext += 14;
				pEnd = pNext;
				while( *pEnd != '\n' )
					pEnd++;
				pEnd[0] = ':';
				pEnd[1] = '4';
				pEnd[2] = '3';
				pEnd[3] = 0;
				//StrCpyEx( pEnd, WIDE(":43"),  ); // whois" );
				DoWhois( pHost, pNext, pvtResult );
			}
		}
	}
	closesocket( S );

	return TRUE;
}

SACK_NETWORK_NAMESPACE_END

// $Log: whois.c,v $
// Revision 1.8  2005/01/27 07:37:11  panther
// Linux cleaned.
//
// Revision 1.7  2004/01/11 23:24:43  panther
// Enable network in whois - if noone has noone will
//
// Revision 1.6  2002/04/18 15:01:42  panther
// Added Log History
// Stage 1: Delay schedule closes - only network thread will close.
//
