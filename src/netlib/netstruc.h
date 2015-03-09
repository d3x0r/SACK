#include <stdhdrs.h>
#include <sack_types.h>
#include <loadsock.h>
#include <sharemem.h> // critical section
#include <timers.h>
#include <network.h>

#include "../contrib/MatrixSSL/3.7.1/matrixssl/matrixsslApi.h"

// debugging flag for socket creation/closing
//#define LOG_SOCKET_CREATION

// there were some messages regarding the close sequence of sockets
// they were left open... so developers might track why sockets were closing...
// these should be probably be re-enabled and be controlled with a runtime option flag.
//#define LOG_DEBUG_CLOSING

// started using this symbol more in the later days of disabling logging...
//#define VERBOSE_DEBUG
//#define LOG_STARTUP_SHUTDOWN
// Define this symbol to use Log( ghLog, WIDE("") ) to log pending
// write status...
//#define LOG_PENDING
// for windows - this will log all FD_XXXX notifications processed...
//#define LOG_NOTICES
//#define LOG_CLIENT_LISTS
//#define LOG_NETWORK_LOCKING
/// for windows - this logs detailed info about the new threaded events
//#define LOG_NETWORK_EVENT_THREAD
//TODO: modify the client struct to contain the MAC addr
#ifndef __LINUX__
#define USE_WSA_EVENTS
#endif

#ifndef CLIENT_DEFINED
#define CLIENT_DEFINED

SACK_NETWORK_NAMESPACE

#define MAGIC_SOCKADDR_LENGTH (sizeof(SOCKADDR_IN)< 256?256:sizeof( SOCKADDR_IN))

// this might have to be like sock_addr_len_t
#define SOCKADDR_LENGTH(sa) ( (int)*(PTRSZVAL*)( ( (PTRSZVAL)(sa) ) - sizeof(PTRSZVAL) ) )
#define SET_SOCKADDR_LENGTH(sa,size) ( ( *(PTRSZVAL*)( ( (PTRSZVAL)(sa) ) - sizeof(PTRSZVAL) ) ) = size )

// used by the network thread dispatched network layer messages...
#define SOCKMSG_UDP (WM_USER+1)  // messages for UDP use this window Message
#define SOCKMSG_TCP (WM_USER+2)  // Messages for TCP use this Window Message
#define SOCKMSG_CLOSE (WM_USER+3) // Message for Network layer shutdown.

// not sure if this is used anywhere....
#define HOSTNAME_LEN 50      // maximum length of a host's text name...

typedef struct PendingBuffer
{
   size_t dwAvail;                // number of bytes to be read yet
   size_t dwUsed;                 // Number of bytes already read.
   size_t dwLastRead;             // number of bytes received on last read.
   struct {
      int  bStream:1;    // is a stream request...
      int  bDynBuffer:1; // lpBuffer was malloced...
	}s;
	union {
		CPOINTER c;              // Buffer Pointer.
		POINTER p;
	} buffer;
   struct PendingBuffer *lpNext; // Next Pending Message to be handled
}PendingBuffer;

#define CF_UDP           0x0001
#define CF_TCP           0x0000  // no flag... is NOT UDP....

#define CF_LISTEN        0x0002
#define CF_CONNECT       0x0000

#define CF_WRITEPENDING  0x0004 // some write is left hanging to output

#define CF_READPENDING   0x0008 // set if buffers have been set by a read
#define CF_READREADY     0x0010 // set if next read to pend should recv also
#define CF_READWAITING   0x8000 // set if reading application is waiting in-line for result.

#define CF_CONNECTED     0x0020 // set when FD_CONNECT is issued...
#define CF_CONNECTERROR  0x0040
#define CF_CONNECTING    0x0080
#define CF_CONNECT_WAITING 0x8000
#define CF_CONNECT_CLOSED 0x100000

#define CF_TOCLOSE       0x0100

#define CF_WRITEISPENDED 0x0200

#define CF_CLOSING       0x0400
#define CF_DRAINING      0x0800

#define CF_CLOSED        0x1000 // closed, handled everything except releasing the socket.
#define CF_ACTIVE        0x2000
#define CF_AVAILABLE     0x4000


#define CF_CPPCONNECT       0x010000
// server/client is implied in usage....
// much like Read, ReadEX are implied in TCP/UDP usage...
//#define CF_CPPSERVERCONNECT 0x010000
//#define CF_CPPCLIENTCONNECT 0x020000
#define CF_CPPREAD          0x020000
#define CF_CPPCLOSE         0x040000
#define CF_CPPWRITE         0x080000
#define CF_CALLBACKTYPES ( CF_CPPCONNECT|CF_CPPREAD|CF_CPPCLOSE|CF_CPPWRITE )
#define CF_STATEFLAGS (CF_ACTIVE|CF_AVAILABLE|CF_CLOSED)

#define CF_WANTS_GLOBAL_LOCK 0x10000000
#define CF_PROCESSING        0x20000000

struct peer_thread_info
{
	struct peer_thread_info *parent_peer;
	struct peer_thread_info *child_peer;
	PLIST monitor_list;   // list of PCLIENT which are waiting on
	PDATALIST event_list; // list of HANDLE which is waited on
#ifdef USE_WSA_EVENTS
	WSAEVENT hThread;
#endif
	int nEvents;
	int nWaitEvents; // updated with count thread is waiting on
	struct {
		BIT_FIELD bProcessing : 1;
		BIT_FIELD bBuildingList : 1;
	} flags;
	PTHREAD thread;
};

struct NetworkClient
{
	SOCKADDR *saClient;  //Dest Address
	SOCKADDR *saSource;  //Local Address of this port ...
	SOCKADDR *saLastClient; // use this for UDP recvfrom
	_8     hwClient[6];
	_8     hwSource[6];
	//  ServeUDP( WIDE("SourceIP"), SourcePort );
	// 		saSource w/ no Dest - read is a connect...
	//  ConnectUDP( WIDE("DestIP"), DestPort );
	//     saClient is DestIP
	//		saSource and implied source...
	//     USE TCP to locate MY Address?
	//     bind(UDP) results in?
	//     connect(UDP) results in?

	SOCKET      Socket;
	_32         dwFlags; // CF_
	_8        *lpUserData;

	union {
		void (CPROC*ClientConnected)( struct NetworkClient *old, struct NetworkClient *newclient ); // new incoming client.
		void (CPROC*ThisConnected)(struct NetworkClient *me, int nStatus );
		void (CPROC*CPPClientConnected)( PTRSZVAL psv, struct NetworkClient *newclient ); // new incoming client.
		void (CPROC*CPPThisConnected)( PTRSZVAL psv, int nStatus );
	}connect;
	PTRSZVAL psvConnect;
	union {
		void (CPROC*CloseCallback)(struct NetworkClient *);
		void (CPROC*CPPCloseCallback)(PTRSZVAL psv);
	} close;
	PTRSZVAL psvClose;
	union {
		cReadComplete ReadComplete;
		cppReadComplete CPPReadComplete;
		cReadCompleteEx ReadCompleteEx;
		cppReadCompleteEx CPPReadCompleteEx;
	}read;
	PTRSZVAL psvRead;
	union {
		void (CPROC*WriteComplete)( struct NetworkClient * );
		void (CPROC*CPPWriteComplete)( PTRSZVAL psv );
	}write;
	PTRSZVAL psvWrite;

	LOGICAL 	      bWriteComplete; // set during bWriteComplete Notify...

	LOGICAL        bDraining;    // byte sink functions.... JAB:980202
	LOGICAL        bDrainExact;  // length does not matter - read until one empty read.
	size_t         nDrainLength;
#if defined( USE_WSA_EVENTS )
	WSAEVENT event;
#endif
	CRITICALSECTION csLock;      // per client lock.
	PTHREAD pWaiting; // Thread which is waiting for a result...
	PendingBuffer RecvPending, FirstWritePending; // current incoming buffer
	PendingBuffer *lpFirstPending,*lpLastPending; // outgoing buffers
	_32    LastEvent; // GetTickCount() of last event...
	DeclareLink( struct NetworkClient );
	struct network_client_flags {
		BIT_FIELD bAddedToEvents : 1;
		BIT_FIELD bRemoveFromEvents : 1;
		BIT_FIELD bSecure : 1;
		BIT_FIELD bAllowDowngrade : 1;
	} flags;
   // this is set to what the thread that's waiting for this event is.
	struct peer_thread_info *this_thread;
	int tcp_delay_count;
	struct {
      ssl_t *ssl_session;
	} ssl;
};
typedef struct NetworkClient CLIENT;

#ifdef MAIN_PROGRAM
#define LOCATION
#else
#define LOCATION extern
#endif

//LOCATION CRITICALSECTION csNetwork;

#define MAX_NETCLIENTS  g.nMaxClients

typedef struct client_slab_tag {
	_32 count;
   CLIENT client[1];
} CLIENT_SLAB, *PCLIENT_SLAB;

// global network data goes here...
LOCATION struct network_global_data{
	int     nMaxClients;
	int     nUserData;     // number of longs.
	P_8     pUserData;
	PLIST   ClientSlabs;
	LOGICAL bLog;
	LOGICAL bQuit;
	PTHREAD pThread;
	PLIST   pThreads; // list of all threads - needed because of limit of 64 sockets per multiplewait
	PCLIENT AvailableClients;
	PCLIENT ActiveClients;
	PLINKQUEUE event_schedule;
	PCLIENT ClosedClients;
	CRITICALSECTION csNetwork;
	_32 uNetworkPauseTimer;
   _32 uPendingTimer;
#ifndef __LINUX__
	HWND ghWndNetwork;
#endif
	CTEXTSTR system_name;
#ifdef WIN32
   int nProtos;
	WSAPROTOCOL_INFO *pProtos;

	INDEX tcp_protocol;
	INDEX udp_protocol;
	INDEX tcp_protocolv6;
	INDEX udp_protocolv6;
#endif
#if defined( USE_WSA_EVENTS )
   HANDLE hMonitorThreadControlEvent;
#endif
	_32 dwReadTimeout;
	_32 dwConnectTimeout;
	PLIST addresses;
	struct {
		BIT_FIELD bLogNotices : 1;
		BIT_FIELD bShortLogReceivedData : 1;
		BIT_FIELD bLogReceivedData : 1;
		BIT_FIELD bLogSentData : 1;
		BIT_FIELD bThreadInitComplete : 1;
		BIT_FIELD bThreadExit : 1;
		BIT_FIELD bNetworkReady : 1;
		BIT_FIELD bThreadInitOkay : 1;
		BIT_FIELD bLogProtocols : 1;
	} flags;
	struct peer_thread_info *root_thread;
#if !defined( USE_WSA_EVENTS ) && !( defined( __LINUX__ ) || defined( __LINUX64__ ) )
	WNDCLASS wc;
#endif
}
*global_network_data; // aka 'g'

#define g (*global_network_data)

#ifdef _WIN32
#ifndef errno
#define errno WSAGetLastError()
#endif
#else
#endif

//---------------------------------------------------------------------
// routines exported from the core for use in external modules
PCLIENT GetFreeNetworkClientEx( DBG_VOIDPASS );
#define GetFreeNetworkClient() GetFreeNetworkClientEx( DBG_VOIDSRC )

_UDP_NAMESPACE
int FinishUDPRead( PCLIENT pc );
_UDP_NAMESPACE_END

#ifdef WIN32
	// errors started arrising because of faulty driver stacks.
	// spontaneous 10106 errors in socket require migration to winsock2.
   // socket is opened specifically by protocol descriptor...
SOCKET OpenSocket( LOGICAL v4, LOGICAL bStream, LOGICAL bRaw, int another_offset );
int SystemCheck( void );
#endif


void TerminateClosedClientEx( PCLIENT pc DBG_PASS );
#define TerminateClosedClient(pc) TerminateClosedClientEx(pc DBG_SRC)
void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNofity, LOGICAL bLinger DBG_PASS );
#define InternalRemoveClientEx(c,b,l) InternalRemoveClientExx(c,b,l DBG_SRC)
#define InternalRemoveClient(c) InternalRemoveClientEx(c, FALSE, FALSE )
struct peer_thread_info *IsNetworkThread( void );

SOCKADDR *AllocAddrEx( DBG_VOIDPASS );
#define AllocAddr() AllocAddrEx( DBG_VOIDSRC )
PCLIENT AddActive( PCLIENT pClient );


#define IsValid(S)   ((S)!=INVALID_SOCKET)  
#define IsInvalid(S) ((S)==INVALID_SOCKET)  

#define CLIENT_DEFINED

//----------------  ssl_layer.c --------------------
void ssl_BeginClientSession( PCLIENT pc );


SACK_NETWORK_NAMESPACE_END

#endif

