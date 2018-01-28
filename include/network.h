#ifndef NETWORK_HEADER_INCLUDED
#define NETWORK_HEADER_INCLUDED
#include "sack_types.h"
#include "loadsock.h"

#ifdef NETWORK_SOURCE
#define NETWORK_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define NETWORK_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
#define _NETWORK_NAMESPACE  namespace network {
#define _NETWORK_NAMESPACE_END }
#define _TCP_NAMESPACE  namespace tcp {
#define _TCP_NAMESPACE_END }
#define USE_TCP_NAMESPACE using namespace tcp;
#define _UDP_NAMESPACE  namespace udp {
#define _UDP_NAMESPACE_END }
#define USE_UDP_NAMESPACE using namespace tcp;
#else
#define _NETWORK_NAMESPACE
#define _NETWORK_NAMESPACE_END
#define _TCP_NAMESPACE
#define _TCP_NAMESPACE_END
#define _UDP_NAMESPACE
#define _UDP_NAMESPACE_END
#define USE_TCP_NAMESPACE
#define USE_UDP_NAMESPACE

#endif

#define SACK_NETWORK_NAMESPACE  SACK_NAMESPACE _NETWORK_NAMESPACE
#define SACK_NETWORK_NAMESPACE_END _NETWORK_NAMESPACE_END SACK_NAMESPACE_END
#define SACK_NETWORK_TCP_NAMESPACE  SACK_NAMESPACE _NETWORK_NAMESPACE _TCP_NAMESPACE
#define SACK_NETWORK_TCP_NAMESPACE_END _TCP_NAMESPACE_END _NETWORK_NAMESPACE_END SACK_NAMESPACE_END
#define SACK_NETWORK_UDP_NAMESPACE  SACK_NAMESPACE _NETWORK_NAMESPACE _UDP_NAMESPACE
#define SACK_NETWORK_UDP_NAMESPACE_END _UDP_NAMESPACE_END _NETWORK_NAMESPACE_END SACK_NAMESPACE_END

SACK_NAMESPACE
	/* Event based networking interface.




	   Example
	   \Example One : A simple client side application. Reads
	   standard input, and writes it to a server it connects to. Read
	   the network and write as standard output.
	   <code lang="c++">
	   \#include \<network.h\>
	   </code>
	   <code>
	   \#include \<logging.h\>
	   \#include \<sharemem.h\>
	   </code>
	   <code lang="c++">

	   void CPROC ReadComplete( PCLIENT pc, void *bufptr, int sz )
	   {
	      char *buf = (char*)bufptr;
	       if( buf )
	       {
	           buf[sz] = 0;
	           printf( "%s", buf );
	           fflush( stdout );
	       }
	       else
	       {
	           buf = (char*)Allocate( 4097 );
	           //SendTCP( pc, "Yes, I've connected", 12 );
	       }
	       ReadTCP( pc, buf, 4096 );
	   }

	   PCLIENT pc_user;

	   void CPROC Closed( PCLIENT pc )
	   {
	      pc_user = NULL;
	   }

	   int main( int argc, char** argv )
	   {
	       SOCKADDR *sa;
	       if( argc \< 2 )
	       {
	           printf( "usage: %s \<Telnet IP[:port]\>\\n", argv[0] );
	           return 0;
	       }
	       SystemLog( "Starting the network" );
	       NetworkStart();
	       SystemLog( "Started the network" );
	       sa = CreateSockAddress( argv[1], 23 );
	       pc_user = OpenTCPClientAddrEx( sa, ReadComplete, Closed, NULL, 0 );
	       if( !pc_user )
	       {
	           SystemLog( "Failed to open some port as telnet" );
	           printf( "failed to open %s%s\\n", argv[1], strchr(argv[1],':')?"":":telnet[23]" );
	           return 0;
	       }
	      //SendTCP( pc_user, "Some data here...", 12 );
	       while( pc_user )
	       {
	           char buf[256];
	           if( !fgets( buf, 256, stdin ) )
	           {
	               RemoveClient( pc_user );
	               return 0;
	           }
	           SendTCP( pc_user, buf, strlen( buf ) );
	       }
	       return -1;
	   }
	   </code>
	   \Example Two : A server application, opens a socket that it
	   accepts connections on. Reads the socket, and writes the
	   information it reads back to the socket as an echo.
	   <code lang="c++">
	   \#include \<stdhdrs.h\>
	   \#include \<sharemem.h\>
	   \#include \<timers.h\>
	   \#include \<network.h\>

	   void CPROC ServerRecieve( PCLIENT pc, POINTER buf, int size )
	   {
	       //int bytes;
	       if( !buf )
	       {
	           buf = Allocate( 4096 );
	           //SendTCP( pc, (void*)"Hi, welccome to...", 15 );
	       }
	       //else
	           //SendTCP( pc, buf, size );

	       // test for waitread support...
	       // read will not result until the data is read.
	       //bytes = WaitReadTCP( pc, buf, 4096 );
	       //if( bytes \> 0 )
	       //   SendTCP( pc, buf, bytes );

	       ReadTCP( pc, buf, 4095 );
	       // buffer does not have anything in it....
	   }

	   void CPROC ClientConnected( PCLIENT pListen, PCLIENT pNew )
	   {
	       SetNetworkReadComplete( pNew, ServerRecieve );
	   }

	   int main( int argc, char **argv )
	   {
	       PCLIENT pcListen;
	       SOCKADDR *port;
	       if( argc \< 2 )
	       {
	           printf( "usage: %s \<listen port\> (defaulting to telnet)\\n", argv[0] );
	           port = CreateSockAddress( "localhost:23", 23 );
	       }
	       else
	           port = CreateSockAddress( argv[1], 23 );
	       NetworkStart();
	       pcListen = OpenTCPListenerAddrEx( port, ClientConnected );
	       if(pcListen)
	           while(1) WakeableSleep( SLEEP_FOREVER );
	       else
	           printf( "Failed to listen on port %s\\n", argv[1] );
	       return 0;
	   }

	   </code>                                                                                    */
	_NETWORK_NAMESPACE

//#ifndef CLIENT_DEFINED
typedef struct NetworkClient *PCLIENT;
//typedef struct Client
//{
//   unsigned char Private_Structure_information_here;
//}CLIENT, *PCLIENT;
//#endif


NETWORK_PROC( CTEXTSTR, GetSystemName )( void );

NETWORK_PROC( PCLIENT, NetworkLockEx )( PCLIENT pc, int readWrite DBG_PASS );
NETWORK_PROC( void, NetworkUnlockEx )( PCLIENT pc, int readWrite DBG_PASS );
/* <combine sack::network::NetworkLockEx@PCLIENT pc>

   \ \                                               */
#define NetworkLock(pc,rw) NetworkLockEx( pc,rw DBG_SRC )
/* <combine sack::network::NetworkUnlockEx@PCLIENT pc>

   \ \                                                 */
#define NetworkUnlock(pc,rw) NetworkUnlockEx( pc,rw DBG_SRC )

typedef void (CPROC*cReadComplete)(PCLIENT, POINTER, size_t );
typedef void (CPROC*cReadCompleteEx)(PCLIENT, POINTER, size_t, SOCKADDR * );
typedef void (CPROC*cCloseCallback)(PCLIENT);
typedef void (CPROC*cWriteComplete)(PCLIENT );
typedef void (CPROC*cNotifyCallback)(PCLIENT server, PCLIENT newClient);
typedef void (CPROC*cConnectCallback)(PCLIENT, int);
typedef void (CPROC*cppReadComplete)(uintptr_t, POINTER, size_t );
typedef void (CPROC*cppReadCompleteEx)(uintptr_t,POINTER, size_t, SOCKADDR * );
typedef void (CPROC*cppCloseCallback)(uintptr_t);
typedef void (CPROC*cppWriteComplete)(uintptr_t );
typedef void (CPROC*cppNotifyCallback)(uintptr_t, PCLIENT newClient);
typedef void (CPROC*cppConnectCallback)(uintptr_t, int);

NETWORK_PROC( void, SetNetworkWriteComplete )( PCLIENT, cWriteComplete );
#ifdef __cplusplus
/* <combine sack::network::SetNetworkWriteComplete@PCLIENT@cWriteComplete>

   \ \                                                                     */
NETWORK_PROC( void, SetCPPNetworkWriteComplete )( PCLIENT, cppWriteComplete, uintptr_t );
#endif
/* <combine sack::network::SetNetworkWriteComplete@PCLIENT@cWriteComplete>

   \ \                                                                     */
#define SetWriteCallback SetNetworkWriteComplete
NETWORK_PROC( void, SetNetworkReadComplete )( PCLIENT, cReadComplete );
#ifdef __cplusplus
/* <combine sack::network::SetNetworkReadComplete@PCLIENT@cReadComplete>

   \ \                                                                   */
NETWORK_PROC( void, SetCPPNetworkReadComplete )( PCLIENT, cppReadComplete, uintptr_t );
#endif
/* <combine sack::network::SetNetworkReadComplete@PCLIENT@cReadComplete>

   \ \                                                                   */
#define SetReadCallback SetNetworkReadComplete
NETWORK_PROC( void, SetNetworkCloseCallback )( PCLIENT, cCloseCallback );
#ifdef __cplusplus
/* <combine sack::network::SetNetworkCloseCallback@PCLIENT@cCloseCallback>

   \ \                                                                     */
NETWORK_PROC( void, SetCPPNetworkCloseCallback )( PCLIENT, cppCloseCallback, uintptr_t );
#endif

/* <combine sack::network::SetNetworkCloseCallback@PCLIENT@cCloseCallback>

   \ \                                                                     */
#define SetCloseCallback SetNetworkCloseCallback

 // wwords is BYTES and wClients=16 is defaulted to 16
#ifdef __LINUX__
NETWORK_PROC( LOGICAL, NetworkWait )(POINTER unused,uint32_t wClients,int wUserData);
#else
NETWORK_PROC( LOGICAL, NetworkWait )(HWND hWndNotify,uint32_t wClients,int wUserData);
#endif
/* <combine sack::network::NetworkWait@HWND@uint16_t@int>

   \ \                                               */
#define NetworkStart() NetworkWait( NULL, 0, 0 )
NETWORK_PROC( LOGICAL, NetworkAlive )( void ); // returns true if network layer still active...
/* Shutdown these network services, stop the network thread, and
   close all sockets open, releasing all internal resources.
   Parameters
   None.                                                         */
NETWORK_PROC( int, NetworkQuit )(void);
// preferred method is to call Idle(); if in doubt.
//NETWORK_PROC( int, ProcessNetworkMessages )( void );

// dwIP would be for 1.2.3.4  (0x01020304 - memory 04 03 02 01) - host order
// VERY RARE!
NETWORK_PROC( SOCKADDR *, CreateAddress_hton )( uint32_t dwIP,uint16_t nHisPort);
// dwIP would be for 1.2.3.4  (0x04030201 - memory 01 02 03 04) - network order
#ifndef WIN32
NETWORK_PROC( SOCKADDR *, CreateUnixAddress )( CTEXTSTR path );
#endif
NETWORK_PROC( SOCKADDR *, CreateAddress )( uint32_t dwIP,uint16_t nHisPort);
NETWORK_PROC( SOCKADDR *, SetAddressPort )( SOCKADDR *pAddr, uint16_t nDefaultPort );
NETWORK_PROC( SOCKADDR *, SetNonDefaultPort )( SOCKADDR *pAddr, uint16_t nDefaultPort );
/*
 * this is the preferred method to create an address
 * name may be "* / *" with a slash, then the address result will be a unix socket (if supported)
 * name may have an options ":port" port number associated, if there is no port, then the default
 * port is used.
 *
 */
NETWORK_PROC( SOCKADDR *, CreateSockAddress )( CTEXTSTR name, uint16_t nDefaultPort );
/*
 * set (*data) and (*datalen) to a binary buffer representation of the sockete address.
 */
NETWORK_PROC( void, GetNetworkAddressBinary )( SOCKADDR *addr, uint8_t **data, size_t *datalen );
/*
 * create a socket address form data and datalen binary buffer representation of the sockete address.
 */
NETWORK_PROC( SOCKADDR *, MakeNetworkAddressFromBinary )( uintptr_t *data, size_t datalen );

NETWORK_PROC( SOCKADDR *, CreateRemote )( CTEXTSTR lpName,uint16_t nHisPort);
NETWORK_PROC( SOCKADDR *, CreateLocal )(uint16_t nMyPort);
NETWORK_PROC( int, GetAddressParts )( SOCKADDR *pAddr, uint32_t *pdwIP, uint16_t *pwPort );
NETWORK_PROC( void, ReleaseAddress )(SOCKADDR *lpsaAddr); // release a socket resource that has been created by an above routine

// result with TRUE if equal, else FALSE
NETWORK_PROC( LOGICAL, CompareAddress )(SOCKADDR *sa1, SOCKADDR *sa2 );
#define SA_COMPARE_FULL 1
#define SA_COMPARE_IP   0
NETWORK_PROC( LOGICAL, CompareAddressEx )(SOCKADDR *sa1, SOCKADDR *sa2, int method );
/*
 * compare this address to see if it is any of my IPv4 interfaces
 */
NETWORK_PROC( LOGICAL, IsThisAddressMe )( SOCKADDR *addr, uint16_t myport );
/*
 *  Get the list of SOCKADDR addresses that are on this box (for this name)
 */
NETWORK_PROC( PLIST, GetLocalAddresses )( void );

/*
 * Return the text of a socket's IP address
 */
NETWORK_PROC( const char *, GetAddrName )( SOCKADDR *addr );
/*
 * Return the numeric form of the address (might have been created by name).
 */
NETWORK_PROC( const char *, GetAddrString )(SOCKADDR *addr);

/*
 * test an address to see if it is v6 (switch connect From behavior at application level)
 */
NETWORK_PROC( LOGICAL, IsAddressV6 )( SOCKADDR *addr );

/*
 *  Duplicate a sockaddr appropriately for the specified network.
 *  SOCKADDR has in(near) it the size of the address block, so this
 * can safely duplicate the the right amount of memory.
 */
NETWORK_PROC( SOCKADDR *, DuplicateAddressEx )( SOCKADDR *pAddr DBG_PASS ); // return a copy of this address...
#define DuplicateAddress(a) DuplicateAddressEx( a DBG_SRC )

NETWORK_PROC( void, SackNetwork_SetSocketSecure )( PCLIENT lpClient );
NETWORK_PROC( void, SackNetwork_AllowSecurityDowngrade )( PCLIENT lpClient );

/* Transmission Control Protocol connection methods. This
   controls opening sockets that are based on TCP.        */
_TCP_NAMESPACE
#ifdef __cplusplus
/* <combine sack::network::tcp::OpenTCPListenerAddrEx@SOCKADDR *@cNotifyCallback>

   \ \                                                                            */
NETWORK_PROC( PCLIENT, CPPOpenTCPListenerAddrExx )( SOCKADDR *, cppNotifyCallback NotifyCallback, uintptr_t psvConnect DBG_PASS );
#define CPPOpenTCPListenerAddrEx(a,b,c)  CPPOpenTCPListenerAddrExx(a,b,c DBG_SRC )

#endif
/* Opens a TCP socket which listens for connections. Other TCP
   sockets may be connected to this one once it has been
   created.
   Parameters
   Address :         address to serve at. See
                     CreateSockAddress().
   Port :            specified the port to listen at. This family
                     that takes just a port FAILS if there are
                     multiple network interfaces and or virtual
                     private networks.
   NotifyCallback :  user callback which will be invoked when a
                     new connection to the TCP server has been
                     made.

   Returns
   NULL if no clients available, or if address bind on listen
   side fails.

   otherwise is a valid network connection to send and receive
   UDP data on.

   The read_complete callback, if specified, will be called,
   with a NULL pointer and 0 size, before the connect complete.   */
NETWORK_PROC( PCLIENT, OpenTCPListenerAddrExx )( SOCKADDR *, cNotifyCallback NotifyCallback DBG_PASS );
#define OpenTCPListenerAddrEx(sa,ca) OpenTCPListenerAddrExx( sa, ca DBG_SRC )
/* <combine sack::network::tcp::OpenTCPListenerAddrEx@SOCKADDR *@cNotifyCallback>

   \ \                                                                            */
#define OpenTCPListenerAddr( pAddr ) OpenTCPListenerAddrEx( paddr, NULL );
#ifdef __cplusplus
/* <combine sack::network::tcp::OpenTCPListenerEx@uint16_t@cNotifyCallback>

   \ \                                                                 */
NETWORK_PROC( PCLIENT, CPPOpenTCPListenerExx )( uint16_t wPort, cppNotifyCallback NotifyCallback, uintptr_t psvConnect DBG_PASS );
#define CPPOpenTCPListenerEx(a,b,c) CPPOpenTCPListenerExx(a,b,c DBG_SRC )
#endif
/* <combine sack::network::tcp::OpenTCPListenerAddrEx@SOCKADDR *@cNotifyCallback>

   \ \                                                                            */
NETWORK_PROC( PCLIENT, OpenTCPListenerExx )( uint16_t wPort, cNotifyCallback NotifyCallback DBG_PASS );
#define OpenTCPListenerEx(a,b) OpenTCPListenerExx(a,b DBG_SRC )
/* <combine sack::network::tcp::OpenTCPListenerEx@uint16_t@cNotifyCallback>

   \ \                                                                 */
#define OpenTCPListener( wPort )    OpenTCPListenerEx( wPort, NULL )

/* <combine sack::network::tcp::OpenTCPListener>

   \ \                                           */
#define OpenTCPServer OpenTCPListener
/* <combine sack::network::tcp::OpenTCPListenerEx@uint16_t@cNotifyCallback>

   \ \                                                                 */
#define OpenTCPServerEx OpenTCPListenerEx
/* <combine sack::network::tcp::OpenTCPListenerAddrEx@SOCKADDR *@cNotifyCallback>

   \ \                                                                            */
#define OpenTCPServerAddr OpenTCPListenerAddr
/* <combine sack::network::tcp::OpenTCPListenerEx@uint16_t@cNotifyCallback>

   \ \                                                                 */
#define OpenTCPServerAddrEx OpenTCPListenerAddrEx

#define OPEN_TCP_FLAG_DELAY_CONNECT 1

#ifdef __cplusplus
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
NETWORK_PROC( PCLIENT, CPPOpenTCPClientAddrExxx )(SOCKADDR *lpAddr,
																  cppReadComplete  pReadComplete, uintptr_t,
																  cppCloseCallback CloseCallback, uintptr_t,
																  cppWriteComplete WriteComplete, uintptr_t,
																  cppConnectCallback pConnectComplete,  uintptr_t, int DBG_PASS );
#define CPPOpenTCPClientAddrExx(a,b,c,d,e,f,g,h,i,j) CPPOpenTCPClientAddrExxx(a,b,c,d,e,f,g,h,i,j DBG_SRC )
#endif

NETWORK_PROC( PCLIENT, OpenTCPClientAddrFromAddrEx )( SOCKADDR *lpAddr, SOCKADDR *pFromAddr
                                                     , cReadComplete     pReadComplete
                                                     , cCloseCallback    CloseCallback
                                                     , cWriteComplete    WriteComplete
                                                     , cConnectCallback  pConnectComplete
                                                     , int flags
                                                     DBG_PASS
                                                     );
#define OpenTCPClientAddrFromAddr( a,f,r,cl,wr,cc ) OpenTCPClientAddrFromAddrEx( a,f,r,cl,wr,cc, 0 DBG_SRC )

NETWORK_PROC( PCLIENT, OpenTCPClientAddrFromEx )( SOCKADDR *lpAddr, int port
                                                , cReadComplete     pReadComplete
                                                , cCloseCallback    CloseCallback
                                                , cWriteComplete    WriteComplete
                                                , cConnectCallback  pConnectComplete
                                                , int flags
                                                DBG_PASS
                                                );
#define OpenTCPClientAddrFrom( a,f,r,cl,wr,cc ) OpenTCPClientAddrFromEx( a,f,r,cl,wr,cc,0 DBG_SRC )
/* Opens a socket which connects to an already existing,
   listening, socket.
   Parameters
   lpAddr :            _nt_
   lpName :            lpName and wPort are passed to
                       CreateSockAddress, and that address is
                       passed as a lpAddr.
   wPort :             lpName and wPort are passed to
                       CreateSockAddress, and that address is
                       passed as a lpAddr.
   pReadComplete :     user callback which is invoked when a
                       buffer now contains data.
   CloseCallback :     user callback when this socket is closed.
   WriteComplete :     user callback which is invoked when a
                       write operation completes.
   pConnectComplete :  user callback which is called when this
                       client connects. The callback gets this
                       network connection as the first parameter.
   Remarks
   WriteComplete is often unused, unless you are using bMsg
   option on do
   Returns
   NULL if no clients available, or if address bind on listen
   side fails.

   otherwise is a valid network connection to send and receive
   UDP data on.

   The read_complete callback, if specified, will be called,
   with a NULL pointer and 0 size, before the connect complete.   */
NETWORK_PROC( PCLIENT, OpenTCPClientAddrExxx )(SOCKADDR *lpAddr,
                                               cReadComplete  pReadComplete,
                                               cCloseCallback CloseCallback,
                                               cWriteComplete WriteComplete,
                                               cConnectCallback pConnectComplete,
                                               int flags
                                               DBG_PASS );
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
#define OpenTCPClientAddrExx(a,r,clo,w,con) OpenTCPClientAddrExxx( a,r,clo,w,con,0 DBG_SRC )
#ifdef __cplusplus
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
NETWORK_PROC( PCLIENT, CPPOpenTCPClientAddrEx )(SOCKADDR *
                                               , cppReadComplete, uintptr_t
                                               , cppCloseCallback, uintptr_t
                                               , cppWriteComplete, uintptr_t
                                               , int flags
                                               );
#endif
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
NETWORK_PROC( PCLIENT, OpenTCPClientAddrExEx )(SOCKADDR *, cReadComplete,
                         cCloseCallback, cWriteComplete DBG_PASS );
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
#define OpenTCPClientAddrEx(a,b,c,d) OpenTCPClientAddrExEx(a,b,c,d DBG_SRC )
#ifdef __cplusplus
/* <combine sack::network::tcp::OpenTCPClientExx@CTEXTSTR@uint16_t@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                      */
NETWORK_PROC( PCLIENT, CPPOpenTCPClientExEx )(CTEXTSTR lpName,uint16_t wPort
                         , cppReadComplete  pReadComplete, uintptr_t
                         , cppCloseCallback CloseCallback, uintptr_t
                         , cppWriteComplete WriteComplete, uintptr_t
															, cppConnectCallback pConnectComplete, uintptr_t, int DBG_PASS );
#define CPPOpenTCPClientExx(name,port,read,rd,close,cd,write,wd,connect,cod,flg) CPPOpenTCPClientExEx(name,port,read,rd,close,cd,write,wd,connect,cod,flg DBG_SRC)
#endif
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
NETWORK_PROC( PCLIENT, OpenTCPClientExxx )(CTEXTSTR lpName,uint16_t wPort
                                           , cReadComplete  pReadComplete
                                           , cCloseCallback CloseCallback
                                           , cWriteComplete WriteComplete
                                           , cConnectCallback pConnectComplete
                                           , int flags
                                           DBG_PASS );
/* <combine sack::network::tcp::OpenTCPClientAddrExx@SOCKADDR *@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                        */
#define OpenTCPClientExx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete, pConnectComplete ) OpenTCPClientExxx( lpName, wPort, pReadComplete, CloseCallback, WriteComplete, pConnectComplete, 0 DBG_SRC )
/* <combine sack::network::tcp::OpenTCPClientExx@CTEXTSTR@uint16_t@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                      */
#define OpenTCPClient( name, port, read ) OpenTCPClientExxx(name,port,read,NULL,NULL,NULL,0 DBG_SRC )
/* <combine sack::network::tcp::OpenTCPClientExx@CTEXTSTR@uint16_t@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                      */
NETWORK_PROC( PCLIENT, OpenTCPClientExEx )( CTEXTSTR, uint16_t, cReadComplete,
													  cCloseCallback, cWriteComplete DBG_PASS );
/* <combine sack::network::tcp::OpenTCPClientExx@CTEXTSTR@uint16_t@cReadComplete@cCloseCallback@cWriteComplete@cConnectCallback>

   \ \                                                                                                                      */
#define OpenTCPClientEx( addr,port,read,close,write ) OpenTCPClientExEx( addr,port,read,close,write DBG_SRC )

/* Do the connect to
*/
int NetworkConnectTCPEx( PCLIENT pc DBG_PASS );
#define NetworkConnectTCP( pc ) NetworkConnectTCPEx( pc DBG_SRC )




/* Drain is an operation on a TCP socket to just drop the next X
   bytes. They are ignored and not stored into any user buffer.
   Drain reads take precedence over any other queued reads.
   Parameters
   pClient :  network connection to drain data from.
   nLength :  how much data to skip.
   bExact :   if TRUE, will consume all of nLength bytes. if
              FALSE, if there are less than nLength bytes
              available right now, the drain will end when no
              further data is available now.                     */
NETWORK_PROC( LOGICAL, TCPDrainEx )( PCLIENT pClient, size_t nLength, int bExact );
/* <combine sack::network::tcp::TCPDrainEx@PCLIENT@int@int>

   \ \                                                      */
#define TCPDrain(c,l) TCPDrainEx( (c), (l), TRUE )

/* TCP sockets have what is called a NAGLE algorithm that helps
   them gather small packets into larger packets. This implies a
   latency on sent communications, but can provide a boost to
   overall speed.
   Parameters
   pClient :  network client to control the nagle algorithm.
   bEnable :  (TRUE)disable NAGLE or (FALSE)enable NAGLE
              (TRUE)nodelay (FALSE)packet gather delay           */
NETWORK_PROC( void, SetTCPNoDelay )( PCLIENT pClient, int bEnable );
/* TCP Connections have a keep-alive option, that data will be
   automatically sent to make sure the connection is still
   alive.
   Parameters
   pClient :  network connection enable or disable the keep alive
              on.
   bEnable :  TRUE to enable keep\-alive else disable keep\-alive. */
NETWORK_PROC( void, SetClientKeepAlive)( PCLIENT pClient, int bEnable );

/* \ \
   Parameters
   lpClient :   network client to read from
   lpBuffer :   buffer to read into
   nBytes :     size of the buffer to read or maximum amount of
                the read desired.
   bIsStream :  if TRUE, any opportunity to return a packet is
                used to pass data to the user's read callback. If
                FALSE, will read to the complete size nBytes
                specified.
   bWait :      if TRUE, will block in the read until there is
                data, or the buffer is filled completely
                depending on the value of bIsStream. If FALSE,
                \returns immediately, the read completion will be
					 notified later by callback.
	user_timeout : user specified timeout to be used if bWait is specified.
                uses internal configurable timeout if 0.
   Returns
   size of the packet read if bWait is TRUE,

   else TRUE for sent, FALSE if the packet could not be sent.

   This buffer needs to continue existing until the socket is
   closed, or the read callback returns.


   Example
   Used in a normal read callback...
   <code lang="c++">
   void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size )
   {
       if( buffer == NULL )
           buffer = malloc( 4096 );
       else
       {
          // size will be non 0, process buffer
       }
       ReadTCP( pc, buffer, 4096 );
   }


   </code>                                                         */
NETWORK_PROC( size_t, doReadExx2)(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream, LOGICAL bWait, int user_timeout DBG_PASS );
#define doReadExx(p,b,n,s,w) DoReadExx2( p,b,n,s,w,0 )
/* \ \
   Parameters
   lpClient :   network client to read from
   lpBuffer :   buffer to read into
   nBytes :     size of the buffer to read or maximum amount of
                the read desired.
   bIsStream :  if TRUE, any opportunity to return a packet is
                used to pass data to the user's read callback. If
                FALSE, will read to the complete size nBytes
                specified.
   bWait :      if TRUE, will block in the read until there is
                data, or the buffer is filled completely
                depending on the value of bIsStream. If FALSE,
                \returns immediately, the read completion will be
                notified later by callback.
   Returns
   size of the packet read if bWait is TRUE,

   else TRUE for sent, FALSE if the packet could not be sent.

   This buffer needs to continue existing until the socket is
   closed, or the read callback returns.


   Example
   Used in a normal read callback...
   <code lang="c++">
   void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size )
   {
       if( buffer == NULL )
           buffer = malloc( 4096 );
       else
       {
          // size will be non 0, process buffer
       }
       ReadTCP( pc, buffer, 4096 );
   }


   </code>                                                         */
//NETWORK_PROC( size_t, doReadExx )(PCLIENT lpClient, POINTER lpBuffer, size_t nBytes
//										, LOGICAL bIsStream, LOGICAL bWait );

/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \
   Remarks
   if bWait is not specifed, it is passed as FALSE.                            */
//NETWORK_PROC( size_t, doReadEx )(PCLIENT lpClient,POINTER lpBuffer,size_t nBytes, LOGICAL bIsStream DBG_PASS );
#define doReadEx( p,b,n,s )  doReadExx2( p,b,n,s,FALSE, 0 DBG_SRC)
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define ReadStream(pc,pBuf,nSize) doReadExx2( pc, pBuf, nSize, TRUE, FALSE, 0 DBG_SRC )
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define doRead(pc,pBuf,nSize)     doReadExx2(pc, pBuf, nSize, FALSE, FALSE, 0 DBG_SRC )
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define ReadTCP ReadStream
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define ReadTCPMsg doRead
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define WaitReadTCP(pc,buf,nSize)    doReadExx2(pc,buf, nSize, TRUE, TRUE, 0 DBG_SRC )
/* <combine sack::network::tcp::doReadExx@PCLIENT@POINTER@int@LOGICAL@LOGICAL>

   \ \                                                                         */
#define WaitReadTCPMsg(pc,buf,nSize) doReadExx2(pc,buf, nSize, FALSE, TRUE, 0  DBG_SRC)


/* \#The buffer will be sent in the order of the writes to the
   socket, and released when empty. If the socket is immediatly
   able to write, the buffer will be sent, and any remai
   Parameters
   lpClient :     network connection to write to
   pInBuffer :    buffer to write
   nInLen :       Length of the buffer to send
   bLongBuffer :  if TRUE, then the buffer written is maintained
                  exactly by the network layer. A WriteComplete
                  callback will be invoked when the buffer has
                  been sent so the application might delete the
                  buffer.
   failpending :  Uhmm... maybe if it goes to pending, fail?

   Remarks
   If bLongBuffer is not set, then if the write cannot
   immediately complete, then a new buffer is allocated
   internally, and unsent data is buffered by the network
   collection. This allows the user to not worry about slowdowns
   due to blocking writes. Often writes complete immediately,
   and are not buffered other than in the user's own buffer
   passed to this write.                                         */
NETWORK_PROC( LOGICAL, doTCPWriteExx )( PCLIENT lpClient
						, CPOINTER pInBuffer
						, size_t nInLen, int bLongBuffer
                                   , int failpending
                                   DBG_PASS
                                  );
/* <combine sack::network::tcp::doTCPWriteExx@PCLIENT@CPOINTER@int@int@int failpending>

   \ \                                                                                  */
#define doTCPWriteEx( c,b,l,f1,f2) doTCPWriteExx( (c),(b),(l),(f1),(f2) DBG_SRC )
/* <combine sack::network::tcp::doTCPWriteExx@PCLIENT@CPOINTER@int@int@int failpending>

   \ \                                                                                  */
#define SendTCPEx( c,b,l,p) doTCPWriteExx( c,b,l,FALSE,p DBG_SRC)
/* <combine sack::network::tcp::doTCPWriteExx@PCLIENT@CPOINTER@int@int@int failpending>

   \ \                                                                                  */
#define SendTCP(c,b,l) doTCPWriteExx(c,b,l, FALSE, FALSE DBG_SRC)
/* <combine sack::network::tcp::doTCPWriteExx@PCLIENT@CPOINTER@int@int@int failpending>

   \ \                                                                                  */
#define SendTCPLong(c,b,l) doTCPWriteExx(c,b,l, TRUE, FALSE DBG_SRC)
_TCP_NAMESPACE_END


NETWORK_PROC( void, SetNetworkLong )(PCLIENT lpClient,int nLong,uintptr_t dwValue);
NETWORK_PROC( void, SetNetworkInt )(PCLIENT lpClient,int nLong, int value);
/* Obsolete. See SetNetworkLong. */
NETWORK_PROC( void, SetNetworkWord )(PCLIENT lpClient,int nLong,uint16_t wValue);
NETWORK_PROC( uintptr_t, GetNetworkLong )(PCLIENT lpClient,int nLong);
NETWORK_PROC( int, GetNetworkInt )(PCLIENT lpClient,int nLong);
NETWORK_PROC( uint16_t, GetNetworkWord )(PCLIENT lpClient,int nLong);

/* Symbols which may be passed to GetNetworkLong to get internal
   parts of the client.                                          */
enum GetNetworkLongAccessInternal{
 GNL_IP      = (-1), /* Gets the IP of the remote side of the connection, if
    applicable. UDP Sockets don't have a bound destination. */

 GNL_PORT    = (-4), /* Gets the port at the remote side of the connection that is
    being sent to.                                             */

 GNL_MYIP    = (-3), /* Gets the 4 byte IPv4 address that is what I am using on my
    side. After a socket has sent, it will have a set source IP
    under windows.                                              */

 GNL_MYPORT  = (-2), /* Gets the 16 bit port of the TCP or UDP connection that you
    are sending from locally.                                  */

 GNL_MAC_LOW = (-5), /* Gets the low 32 bits of a hardware MAC address. */

 GNL_MAC_HIGH= (-6), /* Get MAC address high 16 bits. */
 GNL_REMOTE_ADDRESS = (-7), /* returns SOCKADDR*  of remote side */
 GNL_LOCAL_ADDRESS = (-8), /* returns SOCKADDR* of local side  */


};

NETWORK_PROC( int, GetMacAddress)(PCLIENT pc, uint8_t* buf, size_t *buflen );//int get_mac_addr (char *device, unsigned char *buffer)
//NETWORK_PROC( int, GetMacAddress)(PCLIENT pc );
NETWORK_PROC( PLIST, GetMacAddresses)( void );//int get_mac_addr (char *device, unsigned char *buffer)

NETWORK_PROC( void, RemoveClientExx )(PCLIENT lpClient, LOGICAL bBlockNofity, LOGICAL bLinger DBG_PASS );
/* <combine sack::network::RemoveClientExx@PCLIENT@LOGICAL@LOGICAL bLinger>

   \ \                                                                      */
#define RemoveClientEx(c,b,l) RemoveClientExx(c,b,l DBG_SRC)
/* <combine sack::network::RemoveClientExx@PCLIENT@LOGICAL@LOGICAL bLinger>

   \ \                                                                      */
#define RemoveClient(c) RemoveClientEx(c, FALSE, FALSE )


/* Begin an SSL Connection.  This ends up replacing ReadComplete callback with an inbetween layer*/
NETWORK_PROC( LOGICAL, ssl_BeginClientSession )( PCLIENT pc, CPOINTER keypair, size_t keylen, CPOINTER keypass, size_t keypasslen, CPOINTER rootCert, size_t rootCertLen );
NETWORK_PROC( LOGICAL, ssl_BeginServer )( PCLIENT pc, CPOINTER cert, size_t certlen, CPOINTER keypair, size_t keylen, CPOINTER keypass, size_t keypasslen);
NETWORK_PROC( LOGICAL, ssl_GetPrivateKey )(PCLIENT pc, POINTER *keydata, size_t *keysize);
NETWORK_PROC( LOGICAL, ssl_IsClientSecure )(PCLIENT pc);
NETWORK_PROC( void, ssl_SetIgnoreVerification )(PCLIENT pc);

/* use this to send on SSL Connection instead of SendTCP. */
NETWORK_PROC( LOGICAL, ssl_Send )( PCLIENT pc, CPOINTER buffer, size_t length );




/* User Datagram Packet connection methods. This controls
   opening sockets that are based on UDP.                 */
_UDP_NAMESPACE
/* Open a UDP socket. Since the address to send to is implied on
   each message that is sent, all that is required is to setup
   where the UDP socket is listening.
   Parameters
   pAddr :          Pointer to a string address to listen at. Can
                    be NULL to listen on any interface, (also
                    specified as "0.0.0.0"), see
                    CreateSockAddress notes.
   wPort :          16 bit value for the port to listen at.
   pReadComplete :  user callback which is invoked when a read
                    completes on a UDP socket.
   Close :          close callback which is invoked when the new
                    network connection is closed.

   Returns
   NULL if no clients available, or if address bind on listen
   side fails.

   otherwise is a valid network connection to send and receive
   UDP data on.

   The read_complete callback, if specified, will be called,
	with a NULL pointer and 0 size, before the connect complete.   */

NETWORK_PROC( PCLIENT, CPPServeUDPAddrEx )( SOCKADDR *pAddr
                  , cReadCompleteEx pReadComplete
                  , uintptr_t psvRead
                  , cCloseCallback Close
													 , uintptr_t psvClose
													 , int bCPP DBG_PASS );

NETWORK_PROC( PCLIENT, ServeUDPEx )( CTEXTSTR pAddr, uint16_t wPort,
                  cReadCompleteEx pReadComplete,
                  cCloseCallback Close DBG_PASS );
#define ServeUDP( addr,port,read,close) ServeUDPEx( addr, port, read, close DBG_SRC )

//NETWORK_PROC( PCLIENT, ServeUDP )( CTEXTSTR pAddr, uint16_t wPort,
//                  cReadCompleteEx pReadComplete,
//                  cCloseCallback Close);
//NETWORK_PROC( PCLIENT, ServeUDP )( CTEXTSTR pAddr, uint16_t wPort,
//                  cReadCompleteEx pReadComplete,
//                  cCloseCallback Close);
/* Creates a client to listen for messages or to send UDP
   messages.
   Parameters
   pAddr :          address to listen for UDP messages on.
   pReadComplete :  user callback to received read events.
   Close :          user callback to be invoked when the network
                    connection is closed. (network interface
                    disabled?)

   Returns
   NULL if no sockets are available, or the bind fails. (consult
   log?)

   \returns a network connection which is listening on the
   specified address. The read complete will be called. if it is
	specified, before this function returns.                      */

NETWORK_PROC( PCLIENT, ServeUDPAddrEx )( SOCKADDR *pAddr,
                     cReadCompleteEx pReadComplete,
													 cCloseCallback Close DBG_PASS );
#define ServeUDPAddr(addr,read,close) ServeUDPAddrEx( addr,read,close DBG_SRC )

/* \ \
   Parameters
   address :         Address to listen at (interface
                     specification). Can be NULL to specify ANY
                     address, See notes on CreateSockAddress.
   port :            16 bit port to listen at
   dest_address :    Address to connect to. Can be NULL to
                     specify ANY address, See notes on
                     CreateSockAddress.
   dest_port :       16 bit port to send to. Ignored if
                     dest_address is NULL.
   read_complete :   User event handler which is invoked when
                     data is read from the socket.
   close_callback :  user event handler which is invoked when
                     this socket is closed.

   Returns
   NULL if no clients available, or if address bind on listen
   side fails.

   otherwise is a valid network connection to send and receive
   UDP data on.

   The read_complete callback, if specified, will be called,
   with a NULL pointer and 0 size, before the connect complete. */
NETWORK_PROC( PCLIENT, ConnectUDPEx )( CTEXTSTR , uint16_t ,
                    CTEXTSTR, uint16_t,
                    cReadCompleteEx,
												  cCloseCallback DBG_PASS );
#define ConnectUDP(a,b,c,d,e,f) ConnectUDPEx(a,b,c,d,e,f DBG_SRC )
/* \ \
   Parameters
   sa :             address to listen for UDP messages at.
   saTo :           address to send UDP messages to, if the sa
                    parameter of send is NULL.
   pReadComplete :  user callback which will be invoked when
                    reads complete on the network connection.
   Close :          user callback which will be invoked when the
                    listening socket closes.

   Returns
   NULL if no sockets are available, or the bind fails. (consult
   log?)

   \returns a network connection which is listening on the
   specified address. The read complete will be called. if it is
   specified, before this function returns.                      */
NETWORK_PROC( PCLIENT, ConnectUDPAddrEx )( SOCKADDR *sa,
                        SOCKADDR *saTo,
                    cReadCompleteEx pReadComplete,
													 cCloseCallback Close DBG_PASS );
#define ConnectUDPAddr(a,b,c,d)  ConnectUDPAddrEx(a,b,c,d DBG_SRC )
/* Specify a different default address to send UDP messages to.
   Parameters
   pc :       network connection to change the default target
              address of.
   pToAddr :  text address to connect to. See notes in
              CreateSockAddress.
   wPort :    16 bit port address to connect to.
   Returns
   TRUE if it was a valid address specification.

   FALSE if it could not set the address.                       */
NETWORK_PROC( LOGICAL, ReconnectUDP )( PCLIENT pc, CTEXTSTR pToAddr, uint16_t wPort );
/* Sets the target default address of a UDP connection.
   Parameters
   pc :  network connection to set the target address of.
   sa :  See CreateSockAddress(), this is a network structure that
         is a struct sockaddr{} something.                         */
NETWORK_PROC( LOGICAL, GuaranteeAddr )( PCLIENT pc, SOCKADDR *sa );
/* A UDP message may be sent to a broadcast address or a subnet
   broadcast address, in either case, this must be called to
   enable broadcast communications, else the address must be a
   direct connection.
   Parameters
   pc :       network connection to enable broadcast on.
   bEnable :  TRUE to enable broadcast ability on this socket. FALSE
              to disable broadcast ability.                          */
NETWORK_PROC( void, UDPEnableBroadcast )( PCLIENT pc, int bEnable );

/* Sends to a UDP Network connection.
   Parameters
   pc :     pointer to a network connection to send on.
   pBuf :   buffer to send
   nSize :  size of the buffer to send
   sa :     pointer to a SOCKADDR which this message is destined
            to. Can be NULL, if GuaranteeAddr, or ConnectUDP is
            used.

   Returns
   The number of bytes in the buffer sent? Probably a TRUE if
   success else failure?                                         */
NETWORK_PROC( LOGICAL, SendUDPEx )( PCLIENT pc, CPOINTER pBuf, size_t nSize, SOCKADDR *sa );
/* <combine sack::network::udp::SendUDPEx@PCLIENT@CPOINTER@int@SOCKADDR *>

   \ \                                                                     */
#define SendUDP(pc,pbuf,size) SendUDPEx( pc, pbuf, size, NULL )
/* Queue a read to a UDP socket. A read cannot complete if it
   does not have a buffer to read into. A UDP socket will stall
   if the read callback returns without queuing a read.
   Parameters
   pc :        network connection to read from.
   lpBuffer :  buffer which the next data available on the network
               connection will be read into.
   nBytes :    size of the buffer.                                 */
NETWORK_PROC( int, doUDPRead )( PCLIENT pc, POINTER lpBuffer, int nBytes );
/* <combine sack::network::udp::doUDPRead@PCLIENT@POINTER@int>

   \ \                                                         */
#define ReadUDP doUDPRead

/* Logs to the log file the content of a socket address.
   Parameters
   name :  text leader to print before the address
   sa :    the socket address to dump.                   */
NETWORK_PROC( void, DumpAddrEx )( CTEXTSTR name, SOCKADDR *sa DBG_PASS );
/* <combine sack::network::udp::DumpAddrEx@CTEXTSTR@SOCKADDR *sa>

   \ \                                                            */
#define DumpAddr(n,sa) DumpAddrEx(n,sa DBG_SRC )

NETWORK_PROC( int, SetSocketReuseAddress )( PCLIENT pClient, int32_t enable );
NETWORK_PROC( int, SetSocketReusePort )( PCLIENT pClient, int32_t enable );

_UDP_NAMESPACE_END
USE_UDP_NAMESPACE

struct interfaceAddress {
	SOCKADDR *sa;
	SOCKADDR *saBroadcast;
	SOCKADDR *saMask;
};

NETWORK_PROC( SOCKADDR*, GetBroadcastAddressForInterface )(SOCKADDR *addr);
NETWORK_PROC( SOCKADDR*, GetInterfaceAddressForBroadcast )(SOCKADDR *addr);
NETWORK_PROC( struct interfaceAddress*, GetInterfaceForAddress )( SOCKADDR *addr );
NETWORK_PROC( LOGICAL, IsBroadcastAddressForInterface )( struct interfaceAddress *address, SOCKADDR *addr );
NETWORK_PROC( void, LoadNetworkAddresses )(void);

//----- PING.C ------
NETWORK_PROC( LOGICAL, DoPing )( CTEXTSTR pstrHost,
             int maxTTL,
             uint32_t dwTime,
             int nCount,
             PVARTEXT pResult,
             LOGICAL bRDNS,
             void (*ResultCallback)( uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops ) );
NETWORK_PROC( LOGICAL, DoPingEx )( CTEXTSTR pstrHost,
             int maxTTL,
             uint32_t dwTime,
             int nCount,
             PVARTEXT pResult,
             LOGICAL bRDNS,
											 void (*ResultCallback)( uintptr_t psv, uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops )
											, uintptr_t psv );

//----- WHOIS.C -----
NETWORK_PROC( LOGICAL, DoWhois )( CTEXTSTR pHost, CTEXTSTR pServer, PVARTEXT pvtResult );

#ifdef __cplusplus

typedef class network *PNETWORK;
/* <combine sack::network::network>

   \ \                              */
typedef class network
{
	PCLIENT pc;
	int TCP;
	static void CPROC WrapTCPReadComplete( uintptr_t psv, POINTER buffer, size_t nSize );
	static void CPROC WrapUDPReadComplete( uintptr_t psv, POINTER buffer, size_t nSize, SOCKADDR *sa );
	static void CPROC WrapWriteComplete( uintptr_t psv );
	static void CPROC WrapClientConnectComplete( uintptr_t psv, int nError );
	static void CPROC WrapServerConnectComplete( uintptr_t psv, PCLIENT pcNew );
	static void CPROC WrapCloseCallback( uintptr_t psv );
   // notify == server (listen)
	static void CPROC SetNotify( PCLIENT pc, cppNotifyCallback, uintptr_t psv );
   // connect == client (connect)
   static void CPROC SetConnect( PCLIENT pc, cppConnectCallback, uintptr_t psv );
   static void CPROC SetRead( PCLIENT pc, cppReadComplete, uintptr_t psv );
   static void CPROC SetWrite( PCLIENT pc, cppWriteComplete, uintptr_t psv );
   static void CPROC SetClose( PCLIENT pc, cppCloseCallback, uintptr_t psv );
public:
	network() { NetworkStart(); pc = NULL; TCP = TRUE; };
	network( PCLIENT pc ) { NetworkStart(); this->pc = pc; TCP = TRUE; };
	network( network &cp ) { cp.pc = pc; cp.TCP = TCP; };
	~network() { if( pc ) RemoveClientEx( pc, TRUE, FALSE ); pc = NULL; };
	inline void MakeUDP( void ) { TCP = FALSE; }

	virtual void ReadComplete( POINTER buffer, size_t nSize ) {}
	virtual void ReadComplete( POINTER buffer, size_t nSize, SOCKADDR *sa ) {}
	virtual void WriteComplete( void ) {}
	virtual void ConnectComplete( int nError ) {}
	// received on the server listen object...
	virtual void ConnectComplete( class network &pNewClient ) {}
	virtual void CloseCallback( void ) {}

	inline int Connect( SOCKADDR *sa )
	{
		if( !pc )
		pc = CPPOpenTCPClientAddrExx( sa
									, WrapTCPReadComplete
									, (uintptr_t)this
									, WrapCloseCallback
									, (uintptr_t)this
									, WrapWriteComplete
									, (uintptr_t)this
									, WrapClientConnectComplete
									, (uintptr_t)this
									, 0
									);
		return (int)(pc!=NULL);
	};
	inline int Connect( CTEXTSTR name, uint16_t port )
	{
		if( !pc )
		pc = CPPOpenTCPClientExx( name, port
									, WrapTCPReadComplete
									, (uintptr_t)this
									, WrapCloseCallback
									, (uintptr_t)this
									, WrapWriteComplete
									, (uintptr_t)this
									, WrapClientConnectComplete
									, (uintptr_t)this
									, 0
									);
		return (int)(pc!=NULL);
	};
	inline int Listen( SOCKADDR *sa )
	{
		if( !pc )
		{
			if( ( pc = CPPOpenTCPListenerAddrEx( sa
				                        , (cppNotifyCallback)WrapServerConnectComplete
												, (uintptr_t)this
														)  ) != NULL )
			{
				SetRead( pc, (cppReadComplete)WrapTCPReadComplete, (uintptr_t)this );
				SetWrite( pc, (cppWriteComplete)WrapWriteComplete, (uintptr_t)this );
				SetClose( pc, network::WrapCloseCallback, (uintptr_t)this );
			}
		}
		return (int)(pc!=NULL);
	};
	inline int Listen( uint16_t port )
	{
		if( !pc )
		{
			if( ( pc = CPPOpenTCPListenerEx( port
			                      , (cppNotifyCallback)WrapServerConnectComplete
											 , (uintptr_t)this ) ) )
			{
				SetRead( pc, (cppReadComplete)WrapTCPReadComplete, (uintptr_t)this );
				SetWrite( pc, (cppWriteComplete)WrapWriteComplete, (uintptr_t)this );
				SetClose( pc, network::WrapCloseCallback, (uintptr_t)this );
			}
		}
		return (int)(pc!=NULL);
	};
	inline void Write( POINTER p, int size )
	{
		if( pc ) SendTCP( pc, p, size );
	};
	inline void WriteLong( POINTER p, int size )
	{
		if( pc ) SendTCPLong( pc, p, size );
	};
	inline void Read( POINTER p, int size )
	{
		if( pc ) ReadTCP( pc, p, size );
	};
	inline void ReadBlock( POINTER p, int size )
	{
		if( pc ) ReadTCPMsg( pc, p, size );
	};
	inline void SetLong( int l, uint32_t value )
	{
      if( pc ) SetNetworkLong( pc, l, value );
	}
	inline void SetNoDelay( LOGICAL bTrue )
	{
      if( pc ) SetTCPNoDelay( pc, bTrue );
	}
	inline void SetClientKeepAlive( LOGICAL bTrue )
	{
		if( pc ) sack::network::SetClientKeepAlive( pc, bTrue );
	}
	inline uintptr_t GetLong( int l )
	{
		if( pc )
			return GetNetworkLong( pc, l );
      	return 0;
	}
}NETWORK;

#endif
SACK_NETWORK_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::network;
using namespace sack::network::tcp;
using namespace sack::network::udp;
#endif

#endif
//------------------------------------------------------------------
// $Log: network.h,v $
// Revision 1.36  2005/05/23 19:29:24  jim
// Added definition to support WaitReadTCP...
//
// Revision 1.35  2005/03/15 20:22:32  chrisd
// Declare NotifyCallback with meaningful parameters
//
// Revision 1.34  2005/03/15 20:14:15  panther
// Define a routine to build a PF_UNIX socket for unix... this can be used with TCP_ routines to open a unix socket instead of an IP socket.
//
// Revision 1.33  2004/09/29 00:49:47  d3x0r
// Added fancy wait for PSI frames which allows non-polling sleeping... Extended Idle() to result in meaningful information.
//
// Revision 1.32  2004/08/18 23:52:24  d3x0r
// Cleanups - also enhanced network init to expand if called with larger params.
//
// Revision 1.31  2004/07/28 16:47:18  jim
// added support for get address parts.
//
// Revision 1.31  2004/07/27 18:28:17  d3x0r
// Added definition for getaddressparts
//
// Revision 1.30  2004/01/26 23:47:20  d3x0r
// Misc edits.  Fixed filemon.  Export net startup, added def to edit frame
//
// Revision 1.29  2003/12/03 10:21:34  panther
// Tinkering with C++ networking
//
// Revision 1.28  2003/11/09 03:32:22  panther
// Added some address functions to set port and override default port
//
// Revision 1.27  2003/09/25 08:34:00  panther
// Restore callback defs to proper place
//
// Revision 1.26  2003/09/25 08:29:16  panther
// ...New test
//
// Revision 1.25  2003/09/25 00:22:35  panther
// Move cpp wrapper functions into network library
//
// Revision 1.24  2003/09/25 00:21:49  panther
// Move cpp wrapper functions into network library
//
// Revision 1.23  2003/09/24 15:10:54  panther
// Much mangling to extend C++ network interface...
//
// Revision 1.22  2003/09/24 02:26:02  panther
// Fix C++ methods, extend and correct.
//
// Revision 1.21  2003/07/29 09:27:14  panther
// Add Keep Alive option, enable use on proxy
//
// Revision 1.20  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.19  2003/06/04 11:38:01  panther
// Define PACKED
//
// Revision 1.18  2003/03/25 08:38:11  panther
// Add logging
//
// Revision 1.17  2002/12/22 00:14:11  panther
// Cleanup function declarations and project defines.
//
// Revision 1.16  2002/11/24 21:37:40  panther
// Mods - network - fix server->accepted client method inheritance
// display - fix many things
// types - merge chagnes from verious places
// ping - make function result meaningful yes/no
// controls - fixes to handle lack of image structure
// display - fixes to handle moved image structure.
//
// Revision 1.16  2002/11/21 19:13:11  jim
// Added CreateAddress, CreateAddress_hton
//
// Revision 1.15  2002/07/25 12:59:02  panther
// Added logging, removed logging....
// Network: Added NetworkLock/NetworkUnlock
// Timers: Modified scheduling if the next timer delta was - how do you say -
// to fire again before now.
//
// Revision 1.14  2002/07/23 11:24:26  panther
// Added new function to TCP networking - option on write to disable
// queuing of pending data.
//
// Revision 1.13  2002/07/17 11:33:26  panther
// Added new function to tcp network - dotcpwriteex - allows option to NOT pend
// buffers.
//
// Revision 1.12  2002/07/15 08:34:07  panther
// Include function to set udp broadcast or not.
//
//
// $Log: network.h,v $
// Revision 1.36  2005/05/23 19:29:24  jim
// Added definition to support WaitReadTCP...
//
// Revision 1.35  2005/03/15 20:22:32  chrisd
// Declare NotifyCallback with meaningful parameters
//
// Revision 1.34  2005/03/15 20:14:15  panther
// Define a routine to build a PF_UNIX socket for unix... this can be used with TCP_ routines to open a unix socket instead of an IP socket.
//
// Revision 1.33  2004/09/29 00:49:47  d3x0r
// Added fancy wait for PSI frames which allows non-polling sleeping... Extended Idle() to result in meaningful information.
//
// Revision 1.32  2004/08/18 23:52:24  d3x0r
// Cleanups - also enhanced network init to expand if called with larger params.
//
// Revision 1.31  2004/07/28 16:47:18  jim
// added support for get address parts.
//
// Revision 1.31  2004/07/27 18:28:17  d3x0r
// Added definition for getaddressparts
//
// Revision 1.30  2004/01/26 23:47:20  d3x0r
// Misc edits.  Fixed filemon.  Export net startup, added def to edit frame
//
// Revision 1.29  2003/12/03 10:21:34  panther
// Tinkering with C++ networking
//
// Revision 1.28  2003/11/09 03:32:22  panther
// Added some address functions to set port and override default port
//
// Revision 1.27  2003/09/25 08:34:00  panther
// Restore callback defs to proper place
//
// Revision 1.26  2003/09/25 08:29:16  panther
// ...New test
//
// Revision 1.25  2003/09/25 00:22:35  panther
// Move cpp wrapper functions into network library
//
// Revision 1.24  2003/09/25 00:21:49  panther
// Move cpp wrapper functions into network library
//
// Revision 1.23  2003/09/24 15:10:54  panther
// Much mangling to extend C++ network interface...
//
// Revision 1.22  2003/09/24 02:26:02  panther
// Fix C++ methods, extend and correct.
//
// Revision 1.21  2003/07/29 09:27:14  panther
// Add Keep Alive option, enable use on proxy
//
// Revision 1.20  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.19  2003/06/04 11:38:01  panther
// Define PACKED
//
// Revision 1.18  2003/03/25 08:38:11  panther
// Add logging
//

/* and then we could be really evil

#define send(s,b,x,t,blah)
#define recv
#define socket
#define getsockopt ?
#define heh yeah these have exact equivalents ....

*/
