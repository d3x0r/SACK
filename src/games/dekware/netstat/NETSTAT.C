//------------------------------------------------------------
//
// Netstatp
//
// Copyright (C) 1998 Mark Russinovich
// Systems Internals
// http://www.sysinternals.com
//
// This program implements a subset of the Netstat program's
// functionality. Specifically, it enumerates and displays
// information about all UDP and TCP endpoints.
//
//------------------------------------------------------------
#include "windows.h"
#include "stdio.h"
#include "snmp.h"
#include "winsock.h"

#define HOSTNAMELEN 256
#define PORTNAMELEN 256
#define ADDRESSLEN HOSTNAMELEN+PORTNAMELEN

typedef struct _tcpinfo {
	struct _tcpinfo		*prev;
	struct _tcpinfo		*next;
	UINT				state;
	UINT				localip;
	UINT				localport;
	UINT				remoteip;
	UINT				remoteport;
} TCPINFO, *PTCPINFO;


BOOL (__stdcall *pSnmpExtensionInit)(
    IN  DWORD               dwTimeZeroReference,
    OUT HANDLE              *hPollForTrapEvent,
    OUT AsnObjectIdentifier *supportedView);

BOOL (__stdcall *pSnmpExtensionQuery)(
    IN BYTE                   requestType,
    IN OUT RFC1157VarBindList *variableBindings,
    OUT AsnInteger            *errorStatus,
    OUT AsnInteger            *errorIndex);

//
// Possible TCP endpoint states
//
static char TcpState[][32] = {
	"???",
	"CLOSED",
	"LISTENING",
	"SYN_SENT",
	"SEN_RECEIVED",
	"ESTABLISHED",
	"FIN_WAIT",
	"FIN_WAIT2",
	"CLOSE_WAIT",
	"CLOSING",
	"LAST_ACK",
	"TIME_WAIT"
};
	
//
// Lists of endpoints
//
TCPINFO		TcpInfoTable;
TCPINFO		UdpInfoTable;


//------------------------------------------------------------
//
// GetPortName
//
// Translate port numbers into their text equivalent if 
// there is one
//
//------------------------------------------------------------
char *GetPortName( UINT port, char *proto, char *name, int namelen ) 
{
	struct servent *psrvent;

	if( psrvent = getservbyport( htons( (USHORT) port ), proto )) {

		strcpy( name, psrvent->s_name );

	} else {

		sprintf(name, "%d", port);

	}		
	return name;
}


//------------------------------------------------------------
//
// GetIpHostName
//
// Translate IP addresses into their name-resolved form
// if possible.
//
//------------------------------------------------------------
char *GetIpHostName( BOOL local, UINT ipaddr, char *name, int namelen ) 
{
	struct hostent			*phostent;
	UINT					nipaddr;

	nipaddr = htonl( ipaddr );
	if( !ipaddr  ) {

		if( !local ) {

			sprintf( name, "%d.%d.%d.%d", 
				(nipaddr >> 24) & 0xFF,
				(nipaddr >> 16) & 0xFF,
				(nipaddr >> 8) & 0xFF,
				(nipaddr) & 0xFF);

		} else {

			gethostname(name, namelen);
		}

	} else if( ipaddr == 0x0100007f ) {

		if( local ) {

			gethostname(name, namelen);
		} else {

			strcpy( name, "localhost" );
		}

	} else if( phostent = gethostbyaddr( (char *) &ipaddr,
		sizeof( nipaddr ), PF_INET )) {

		strcpy( name, phostent->h_name );

	} else {

		sprintf( name, "%d.%d.%d.%d", 
			(nipaddr >> 24) & 0xFF,
			(nipaddr >> 16) & 0xFF,
			(nipaddr >> 8) & 0xFF,
			(nipaddr) & 0xFF);
	}
	return name;
}


//------------------------------------------------------------
//
// LoadInetMibEntryPoints
//
// Load the TCP/IP SNMP extension DLL and locate the entry
// points we will use.
//
//------------------------------------------------------------
BOOLEAN LoadInetMibEntryPoints()
{
	HINSTANCE	hInetLib;

	if( !(hInetLib = LoadLibrary( "inetmib1.dll" ))) {

		return FALSE;
	}

	if( !(pSnmpExtensionInit = (void *) GetProcAddress( hInetLib,
			"SnmpExtensionInit" )) ) {

		return FALSE;
	}

	if( !(pSnmpExtensionQuery = (void *) GetProcAddress( hInetLib,
			"SnmpExtensionQuery" )) ) {

		return FALSE;
	}
	
	return TRUE;
}


//------------------------------------------------------------
//
// Main
//
// Do it all. Load and initialize the SNMP extension DLL and
// then build a table of TCP endpoints and UDP endpoints. After
// each table is built resolve addresses to names and print
// out the information
//
//------------------------------------------------------------
int main( int argc, char *argv[] )
{
	HANDLE					hTrapEvent;
	AsnObjectIdentifier		hIdentifier;
	RFC1157VarBindList		bindList;
	RFC1157VarBind			bindEntry;
	UINT					tcpidentifiers[] = { 1,3,6,1,2,1,6,13,1,1};
	UINT					udpidentifiers[] = { 1,3,6,1,2,1,7,5,1,1};
	AsnInteger				errorStatus, errorIndex;
	TCPINFO					*currentEntry, *newEntry;
	UINT					currentIndex;
	WORD					wVersionRequested;
	WSADATA					wsaData;
	char					localname[HOSTNAMELEN], remotename[HOSTNAMELEN];
	char					remoteport[PORTNAMELEN], localport[PORTNAMELEN];
	char					localaddr[ADDRESSLEN], remoteaddr[ADDRESSLEN];

	//
	// Initialize winsock
	//
	wVersionRequested = MAKEWORD( 1, 1 );
	if( WSAStartup(  wVersionRequested, &wsaData ) ) {

		printf("Could not initialize Winsock.\n");
		return 1;
	}

	//
	// Locate and initialize INETMIB1
	//
	if( !LoadInetMibEntryPoints()) {

		printf("Could not load extension DLL.\n");
		return 1;
	}

	if( !pSnmpExtensionInit( GetCurrentTime(), &hTrapEvent, &hIdentifier )) {

		printf("Could not initialize extension DLL.\n");
		return 1;
	}

	//
	// Initialize the query structure once
	//
	bindEntry.name.idLength = 0xA;
	bindEntry.name.ids = tcpidentifiers;
	bindList.list = &bindEntry;
	bindList.len  = 1;

	TcpInfoTable.prev = &TcpInfoTable;
	TcpInfoTable.next = &TcpInfoTable;

	//
	// Roll through TCP connections
	//
	currentIndex = 1;
	currentEntry = &TcpInfoTable;
	while(1) {

		if( !pSnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
			&bindList, &errorStatus, &errorIndex )) {

			return 1;
		}

		//
		// Terminate when we're no longer seeing TCP information
		//
		if( bindEntry.name.idLength < 0xA ) break;

		//
		// Go back to start of table if we're reading info
		// about the next byte
		//
		if( currentIndex != bindEntry.name.ids[9] ) {

			currentEntry = TcpInfoTable.next;
			currentIndex = bindEntry.name.ids[9];
		}

		//
		// Build our TCP information table 
		//
		switch( bindEntry.name.ids[9] ) {

		case 1:
			
			//
			// Always allocate a new structure
			//
			newEntry = (TCPINFO *) malloc( sizeof(TCPINFO ));
			newEntry->prev = currentEntry;
			newEntry->next = &TcpInfoTable;
			currentEntry->next = newEntry;
			currentEntry = newEntry;

			currentEntry->state = bindEntry.value.asnValue.number;
			break;

		case 2:

			currentEntry->localip = 
				*(UINT *) bindEntry.value.asnValue.address.stream;
			currentEntry = currentEntry->next;
			break;

		case 3:
			
			currentEntry->localport = 
				bindEntry.value.asnValue.number;
			currentEntry = currentEntry->next;
			break;

		case 4:

			currentEntry->remoteip = 
				*(UINT *) bindEntry.value.asnValue.address.stream;
			currentEntry = currentEntry->next;
			break;

		case 5:

			currentEntry->remoteport = 
				bindEntry.value.asnValue.number;
			currentEntry = currentEntry->next;
			break;
		}

	}
	
	//
	// Now print the connection information
	//
	printf("%7s %-30s %-30s %s\n", "Proto", "Local", "Remote", "State" );
	currentEntry = TcpInfoTable.next;
	while( currentEntry != &TcpInfoTable ) {

		sprintf( localaddr, "%s:%s", 
			GetIpHostName( TRUE, currentEntry->localip, localname, HOSTNAMELEN), 
			GetPortName( currentEntry->localport, "tcp", localport, PORTNAMELEN ));

		sprintf( remoteaddr, "%s:%s",
			GetIpHostName( FALSE, currentEntry->remoteip, remotename, HOSTNAMELEN), 
			currentEntry->remoteip ? 
				GetPortName( currentEntry->remoteport, "tcp", remoteport, PORTNAMELEN ):
				"0" );

		printf("%7s %-30s %-30s %s\n", "TCP", 
			localaddr, remoteaddr,
			TcpState[currentEntry->state]);
		
		currentEntry = currentEntry->next;
	}
	printf("\n");

	//
	// Initialize the query structure once
	//
	bindEntry.name.idLength = 0xA;
	bindEntry.name.ids = udpidentifiers;
	bindList.list = &bindEntry;
	bindList.len  = 1;

	UdpInfoTable.prev = &UdpInfoTable;
	UdpInfoTable.next = &UdpInfoTable;

	//
	// Roll through UDP endpoints
	//
	currentIndex = 1;
	currentEntry = &UdpInfoTable;
	while(1) {

		if( !pSnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
			&bindList, &errorStatus, &errorIndex )) {

			return 1;
		}

		//
		// Terminate when we're no longer seeing TCP information
		//
		if( bindEntry.name.idLength < 0xA ) break;

		//
		// Go back to start of table if we're reading info
		// about the next byte
		//
		if( currentIndex != bindEntry.name.ids[9] ) {

			currentEntry = UdpInfoTable.next;
			currentIndex = bindEntry.name.ids[9];
		}

		//
		// Build our TCP information table 
		//
		switch( bindEntry.name.ids[9] ) {

		case 1:
			
			//
			// Always allocate a new structure
			//
			newEntry = (TCPINFO *) malloc( sizeof(TCPINFO ));
			newEntry->prev = currentEntry;
			newEntry->next = &UdpInfoTable;
			currentEntry->next = newEntry;
			currentEntry = newEntry;

			currentEntry->localip = 
				*(UINT *) bindEntry.value.asnValue.address.stream;
			break;

		case 2:
			
			currentEntry->localport = 
				bindEntry.value.asnValue.number;
			currentEntry = currentEntry->next;
			break;
		}
	}
	
	//
	// Now print the connection information
	//
	currentEntry = UdpInfoTable.next;
	while( currentEntry != &UdpInfoTable ) {

		printf("%7s %s:%s\n", "UDP",
				GetIpHostName( TRUE, currentEntry->localip, localname, HOSTNAMELEN), 
				GetPortName( currentEntry->localport, "udp", localport, PORTNAMELEN ) );
		
		currentEntry = currentEntry->next;
	}
	printf("\n");
	return 0;
}
