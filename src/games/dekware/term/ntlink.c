// well do stuff here....
#include <stdhdrs.h>

#include <network.h>

#include "plugin.h"


// common DLL plugin interface.....
#if defined( WIN32 ) || defined( _WIN32 )
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

#if 0
int Send( PSENTIENT ps, PTEXT parameters );
PDATAPATH Terminal( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
PDATAPATH ServeTerminal( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
int TerminalTransmit( PSENTIENT ps, PTEXT tokens );
PDATAPATH UDP( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
PDATAPATH UDPServer( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );

int GetAddr( PSENTIENT ps, PTEXT parameters );
int GetIP( PSENTIENT ps, PTEXT parameters );
int GetPort( PSENTIENT ps, PTEXT parameters );

int Ping( PSENTIENT ps, PTEXT parameters );
int Trace( PSENTIENT ps, PTEXT parameters );
int PortScan( PSENTIENT ps, PTEXT parameters );
int Whois( PSENTIENT ps, PTEXT parameters );
int SendBinary( PSENTIENT ps, PTEXT parameters );
int ReadBinary( PSENTIENT ps, PTEXT parameters );
int HTTP( PSENTIENT ps, PTEXT parameters );

int myTypeID; // supplied for uhmm... grins...
int myTypeID2; // supplied for uhmm... grins...
int myTypeID3;
int myTypeID4;

extern int b95;

PUBLIC( char *, RegisterRoutines )( void )
{
   myTypeID = RegisterDevice( "tcp", "Telnet type clear text socket connection...", Terminal );
   myTypeID2 = RegisterDevice( "tcpserver", "Telnet server socket connection...", ServeTerminal );

   myTypeID3 = RegisterDevice( "udp", "UDP datagram connection...", UDP ); 
   myTypeID4 = RegisterDevice( "udpserver", "UDP datagram connection...", UDPServer );

//   RegisterRoutine( "send", "output data to connection...", Send );
   RegisterRoutine( "getaddr" , "Get network address from message...", GetAddr );
   RegisterRoutine( "getip" , "Get network address from message...", GetIP );
   RegisterRoutine( "getport" , "Get network address from message...", GetPort );
   RegisterRoutine( "ping", "Ping a network address...", Ping );
   RegisterRoutine( "trace", "Route trace a network address...", Trace );
   RegisterRoutine( "portscan", "scan first 2000 ports at an address...", PortScan );
   RegisterRoutine( "whois", "Perform whois query on listed names.", Whois );
   RegisterRoutine( "ReadData", "Read binary data from socket connection", ReadBinary );
   RegisterRoutine( "SendData", "Send binary data to socket connection", SendBinary );
   RegisterRoutine( "HTTP", "Perform http command operation...", HTTP );
   // RegisterDefaultRoutine( TerminalTransmit );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
//   UnregisterRoutine( "send" );
   UnregisterRoutine( "getaddr" );
   UnregisterRoutine( "getip" );
   UnregisterRoutine( "getport" );
   UnregisterRoutine( "ping" );
   UnregisterRoutine( "trace" );
   UnregisterRoutine( "portscan" );
   UnregisterRoutine( "whois" );
   UnregisterRoutine( "ReadData" );
   UnregisterRoutine( "SendData" );
   UnregisterRoutine( "HTTP" );
   UnregisterDevice( "tcp" );
   UnregisterDevice( "udp" );
   UnregisterDevice( "tcpserver" );
   UnregisterDevice( "udpserver" );
   NetworkQuit();
}
#endif
// $Log: ntlink.c,v $
// Revision 1.6  2005/01/19 08:55:46  d3x0r
// Oh - those missing symbols - conflicts within its own shared library with static declarations
//
// Revision 1.5  2003/03/25 08:59:03  panther
// Added CVS logging
//
