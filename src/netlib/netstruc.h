#include <stdhdrs.h>
#include <sack_types.h>
#include <loadsock.h>
#include <sharemem.h> // critical section
#include <timers.h>
#include <network.h>

#ifdef __LINUX__
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <wincrypt.h>
#endif

#ifndef OPENSSL_API_COMPAT
#  define OPENSSL_API_COMPAT 10101
#endif

#ifndef NO_SSL
#  define LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING
#  define _LIB
#    if NODE_MAJOR_VERSION >= 17
// this can't work?
#      include <openssl/configuration.h>
#    endif
#  include <openssl/ssl.h>
#  include <openssl/tls1.h>
#  include <openssl/err.h>

#  include <openssl/rsa.h>
#  include <openssl/pem.h>
#  include <openssl/pkcs12.h>
#  if OPENSSL_VERSION_MAJOR >= 3
#    include <openssl/core_names.h>
#  endif
#endif
//#include "../contrib/MatrixSSL/3.7.1/matrixssl/matrixsslApi.h"

// debugging flag for socket creation/closing
//#define LOG_SOCKET_CREATION

// there were some messages regarding the close sequence of sockets
// they were left open... so developers might track why sockets were closing...
// these should be probably be re-enabled and be controlled with a runtime option flag.
//#define LOG_DEBUG_CLOSING

// started using this symbol more in the later days of disabling logging...
//#define VERBOSE_DEBUG
//#define LOG_STARTUP_SHUTDOWN
// Define this symbol to use Log( ghLog, "" ) to log pending
// write status...
//#define LOG_PENDING
// for windows - this will log all FD_XXXX notifications processed...
//#define LOG_NOTICES
//#define LOG_WRITE_AGGREGATION
//#define LOG_WRITE_NOTICES
//#define LOG_PENDING_WRITES // lasst bit of logging around wakeOnUnlock and pending writes.
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

#define MAGIC_SOCKADDR_LENGTH ( sizeof(SOCKADDR_IN)< 256?256:sizeof( SOCKADDR_IN) )

#  define SOCKADDR_NAME(sa) (( ((TEXTSTR*)( ( (uintptr_t)(sa) ) - 1*sizeof(uintptr_t) ))[0] ))

// used by the network thread dispatched network layer messages...
#define SOCKMSG_UDP (WM_USER+1)  // messages for UDP use this window Message
#define SOCKMSG_TCP (WM_USER+2)  // Messages for TCP use this Window Message
#define SOCKMSG_CLOSE (WM_USER+3) // Message for Network layer shutdown.

// not sure if this is used anywhere....
#define HOSTNAME_LEN 50      // maximum length of a host's text name...

typedef struct PendingWrite {
	PCLIENT pc;
	POINTER buffer;
	size_t len;
	int bLong;
	int failpending;
} PendingWrite;

typedef struct PendingBuffer
{
   size_t dwAvail;                // number of bytes to be read yet
   size_t dwUsed;                 // Number of bytes already read.
   size_t dwLastRead;             // number of bytes received on last read.
   struct {
      BIT_FIELD bStream:1;    // is a(read) stream request...
      BIT_FIELD bDynBuffer:1; // lpBuffer was malloced...
	}s;
	union {
		CPOINTER c;              // Buffer Pointer.
		POINTER p;
	} buffer;
   struct PendingBuffer *lpNext; // Next Pending Message to be handled
}PendingBuffer;

enum NetworkConnectionFlags {
	CF_UDP               = 0x00000001
	// no flag... is NOT UDP....
	, CF_TCP             = 0x00000000
	, CF_LISTEN          = 0x00000002
	// some write is left hanging to output
	, CF_WRITEPENDING    = 0x00000004
	// set if buffers have been set by a read
	, CF_READPENDING     = 0x00000008
	// set if next read to pend should recv also
	, CF_READREADY       = 0x00000010
	// set if reading application is waiting in-line for result.
	, CF_READWAITING     = 0x00008000
	// set when FD_CONNECT is issued...
	, CF_CONNECTED       = 0x00000020
	, CF_CONNECTERROR    = 0x00000040
	, CF_CONNECTING      = 0x00000080
	, CF_CONNECT_WAITING = 0x00400000
	, CF_CONNECT_CLOSED  = 0x00100000
	, CF_CONNECT_ISSUED  = 0x00800000  // connection to socket callback for server connect was issued.
	, CF_TOCLOSE         = 0x00000100  // wants to close at the next opportunity.
	, CF_WANTCLOSE       = 0x00000200  // this flag is unused at this time
	, CF_CLOSING         = 0x00000400
	, CF_DRAINING        = 0x00000800
	// closed, handled everything except releasing the socket.
	, CF_CLOSED          = 0x00001000
	, CF_ACTIVE          = 0x00002000
	, CF_AVAILABLE       = 0x00004000
	, CF_CPPCONNECT      = 0x00010000
	// server/client is implied in usage....
	// much like Read, ReadEX are implied in TCP/UDP usage...
	//#define CF_CPPSERVERCONNECT 0x010000
	//#define CF_CPPCLIENTCONNECT 0x020000
	, CF_CPPREAD         = 0x00020000
	, CF_CPPCLOSE        = 0x00040000
	, CF_CPPWRITE        = 0x00080000
	, CF_CALLBACKTYPES   = 0x00010000
                        | 0x00020000
                        | 0x00040000
                        | 0x00080000//(CF_CPPCONNECT | CF_CPPREAD | CF_CPPCLOSE | CF_CPPWRITE)
	, CF_STATEFLAGS      = 0x1000 | 0x2000 | 0x4000  //( CF_ACTIVE | CF_AVAILABLE | CF_CLOSED)
	//, CF_WANTS_GLOBAL_LOCK = 0x10000000
	, CF_PROCESSING      = 0x20000000
	, CF_WRITEREADY 	= 0x00200000
};

#ifdef __cplusplus
#  ifndef DEFINE_ENUM_FLAG_OPERATORS
#    ifdef __GNUC__
// used as an approximation of std::underlying_type<T>
template <size_t S>
struct _ENUM_FLAG_INTEGER_FOR_SIZE;

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<1>
{
	typedef int8_t type;
};

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<2>
{
	typedef int16_t type;
};

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<4>
{
	typedef int32_t type;
};

template <class T>
struct _ENUM_FLAG_SIZED_INTEGER
{
	typedef typename _ENUM_FLAG_INTEGER_FOR_SIZE<sizeof( T )>::type type;
};
#    endif
#    define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
        extern "C++" { \
        inline ENUMTYPE operator | ( ENUMTYPE a, ENUMTYPE b ) { return ENUMTYPE( ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) | ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b) ); } \
        inline ENUMTYPE &operator |= ( ENUMTYPE &a, ENUMTYPE b ) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) |= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
        inline ENUMTYPE operator & ( ENUMTYPE a, ENUMTYPE b ) { return ENUMTYPE( ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) & ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b) ); } \
        inline ENUMTYPE &operator &= ( ENUMTYPE &a, ENUMTYPE b ) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) &= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
        inline ENUMTYPE operator ~ ( ENUMTYPE a ) { return ENUMTYPE( ~((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) ); } \
        inline ENUMTYPE operator ^ ( ENUMTYPE a, ENUMTYPE b ) { return ENUMTYPE( ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) ^ ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b) ); } \
        inline ENUMTYPE &operator ^= ( ENUMTYPE &a, ENUMTYPE b ) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) ^= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
        }
#  endif
DEFINE_ENUM_FLAG_OPERATORS( NetworkConnectionFlags )
#endif

struct peer_thread_info
{
	struct peer_thread_info *parent_peer;
	struct peer_thread_info * volatile child_peer;
	PLIST monitor_list;   // list of PCLIENT which are waiting on
	PDATALIST event_list; // list of HANDLE which is waited on
	PTHREAD thread;
#ifdef _WIN32
	WSAEVENT hThread;
	volatile int nEvents;
	LOGICAL counting;
	int nWaitEvents; // updated with count thread is waiting on
#endif
#ifdef __LINUX__
#  ifdef __MAC__
	int kqueue;
   PDATALIST kevents;
#  else
	int epoll_fd;
#  endif
	uint32_t nEvents;
#endif
	struct {
		BIT_FIELD bProcessing : 1;
		BIT_FIELD bBuildingList : 1;
	} flags;
};

typedef struct {
  int verbose_mode;
  int verify_depth;
  int always_continue;
} verify_mydata_t;
#define verify_mydata_index 0

#ifndef NO_SSL
struct ssl_session {
	PLIST hosts;
	SSL_CTX        *ctx;
	struct internalCert* cert;
	LOGICAL ignoreVerification;
	LOGICAL firstPacket;
	LOGICAL noHost;
	LOGICAL closed;
	BIO *rbio;
	BIO *wbio;
	//EVP_PKEY *privkey;
	PLIST          accepting; // sockets being accepted so we can find the proper SSL*
	TEXTSTR	       hostname;  // accepted client's hostname request (NULL if none)
	PLIST          protocols; // protocol select from TLS
	SSL*           ssl;
	uint32_t       dwOriginalFlags; // CF_CPPREAD

	cppReadComplete  recv_callback;
	cppWriteComplete  send_callback;
	uintptr_t 	psvSendRecv;
	cReadComplete  user_read;
	cppReadComplete  cpp_user_read;
	uintptr_t psvRead;

	cNotifyCallback user_connected;
	cppNotifyCallback cpp_user_connected;
	// ssl uses C callbacks which get socket, and can save psvConnect in original socket.

	cCloseCallback user_close;
	cppCloseCallback cpp_user_close;
	// ssl uses C callbacks which get socket, and can save psvClose in original socket.

	cErrorCallback errorCallback;
	uintptr_t psvErrorCallback;

	uint8_t *obuffer;
	size_t obuflen;
	uint8_t *ibuffer;
	size_t ibuflen;
	uint8_t *dbuffer;
	size_t dbuflen;
	CRITICALSECTION csReadWrite;
	verify_mydata_t verify_data;
	uint32_t inUse;
	uint32_t deleteInUse;
	PCLIENT pc; // for the rare case that we only have session and no socket.
	//CRITICALSECTION csReadWrite;
	//CRITICALSECTION csWrite;
};
#endif

struct NetworkClient
{
	SOCKADDR *saClient;  //Dest Address
	SOCKADDR *saSource;  //Local Address of this port ...
	SOCKADDR *saLastClient; // use this for UDP recvfrom
	//uint8_t     hwClient[6];
	//uint8_t     hwSource[6];
	//  ServeUDP( "SourceIP", SourcePort );
	// 		saSource w/ no Dest - read is a connect...
	//  ConnectUDP( "DestIP", DestPort );
	//     saClient is DestIP
	//		saSource and implied source...
	//     USE TCP to locate MY Address?
	//     bind(UDP) results in?
	//     connect(UDP) results in?

	SOCKET      Socket;
	SOCKET      SocketBroadcast; // okay keep both...
	struct interfaceAddress* interfaceAddress;
	#ifdef MSVC
	[Flags]
	#endif
	volatile int/*enum NetworkConnectionFlags*/ dwFlags; // CF_
	uintptr_t        *lpUserData;

	union {
		void (CPROC*ClientConnected)( struct NetworkClient *old, struct NetworkClient *newclient ); // new incoming client.
		void (CPROC*ThisConnected)(struct NetworkClient *me, int nStatus );
		void (CPROC*CPPClientConnected)( uintptr_t psv, struct NetworkClient *newclient ); // new incoming client.
		void (CPROC*CPPThisConnected)( uintptr_t psv, int nStatus );
	}connect;
	uintptr_t psvConnect;
	union {
		void (CPROC*CloseCallback)(struct NetworkClient *);
		void (CPROC*CPPCloseCallback)(uintptr_t psv);
	} close;
	uintptr_t psvClose;
	union {
		cReadComplete ReadComplete;
		cppReadComplete CPPReadComplete;
		cReadCompleteEx ReadCompleteEx;
		cppReadCompleteEx CPPReadCompleteEx;
	}read;
	uintptr_t psvRead;

	union {
		void (CPROC*WriteComplete)( struct NetworkClient *, CPOINTER buffer, size_t len );
		void (CPROC*CPPWriteComplete)( uintptr_t psv, CPOINTER buffer, size_t len );
	}write;
	uintptr_t psvWrite;

	cErrorCallback errorCallback;
	uintptr_t psvErrorCallback;

	LOGICAL        bWriteComplete; // set during bWriteComplete Notify...

	LOGICAL        bDraining;    // byte sink functions.... JAB:980202
	LOGICAL        bDrainExact;  // length does not matter - read until one empty read.
	size_t         nDrainLength;
#if defined( USE_WSA_EVENTS )
	WSAEVENT event;
#endif
	CRITICALSECTION csLockRead;    // per client lock.
	CRITICALSECTION csLockWrite;   // per client lock.
	PTHREAD pWaiting; // Thread which is waiting for a result...
	PendingBuffer RecvPending, FirstWritePending; // current incoming buffer
	PendingBuffer *volatile lpFirstPending,*volatile lpLastPending; // outgoing buffers
	uint32_t    LastEvent; // GetTickCount() of last event...
	DeclareLink( struct NetworkClient );
	PCLIENT pcServer; // server this listen socket came from - because connect callback has to be delayed until after handshake of TLS.
	PCLIENT pcOther; // listeners opened(was deprecated since most connections to v4 can be seen on v6) with port only have two connections, one IPV4 one IPV6
	volatile struct network_client_flags {
		BIT_FIELD bAddedToEvents : 1;
		BIT_FIELD bRemoveFromEvents : 1;
		BIT_FIELD bSecure : 1;
		BIT_FIELD bAllowDowngrade : 1;
		BIT_FIELD bWaiting : 1; // waiting is a accept() flag to prevent accepting sockets before really setup.
		BIT_FIELD bWriteOnUnlock : 1; // write event failed to get lock, so if the locked holder would please write...
		BIT_FIELD bInUse : 1; // has work outstanding; wait for close until release
		BIT_FIELD bAggregateOutput : 1;
	} flags;
	PTHREAD wakeOnUnlock;
	volatile uint32_t writeTimer;
	PLIST psvInUse; // we have the ability to save outstatnding UID locks...

	// this is set to what the thread that's waiting for this event is.
	struct peer_thread_info * volatile this_thread;
	//int tcp_delay_count;
#ifndef NO_SSL
	struct ssl_session *ssl_session;
#endif
};
typedef struct NetworkClient CLIENT;

#ifdef MAIN_PROGRAM
#define LOCATION
#else
#define LOCATION extern
#endif

//LOCATION CRITICALSECTION csNetwork;

#define MAX_NETCLIENTS  globalNetworkData.nMaxClients

typedef struct client_slab_tag {
	uint32_t count;
	uintptr_t* pUserData;
	CLIENT client[1];
} CLIENT_SLAB, *PCLIENT_SLAB;

// global network data goes here...
#if !defined( MAIN_PROGRAM ) && defined( __STATIC_GLOBALS__ )
extern
#endif
struct network_global_data{
	uint32_t     nMaxClients;
	int     nUserData;     // number of longs.
	//uint8_t*     pUserData;
	PLIST   ClientSlabs;
	LOGICAL bLog;
	LOGICAL bQuit;
	PLIST   pThreads; // list of all threads - needed because of limit of 64 sockets per multiplewait
	PCLIENT AvailableClients;
	PCLIENT ActiveClients;
	PCLIENT ClosedClients;
	CRITICALSECTION csNetwork;
	volatile uint32_t uNetworkPauseTimer;
	uint32_t uPendingTimer;
#ifndef __LINUX__
	HWND ghWndNetwork;
#endif
	CTEXTSTR system_name;
#ifdef WIN32
	int nProtos;
	WSAPROTOCOL_INFOW *pProtos;

	INDEX tcp_protocol;
	INDEX udp_protocol;
	INDEX tcp_protocolv6;
	INDEX udp_protocolv6;
#endif
#if defined( USE_WSA_EVENTS )
   HANDLE hMonitorThreadControlEvent;
   PLINKQUEUE client_schedule;  // shorter list of new sockets to monitor than the full list
#endif
	uint32_t dwReadTimeout;
	uint32_t dwConnectTimeout;
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
		BIT_FIELD bOptionsRead : 1;
	} flags;
	int nPeers; // how many peer threads do we have
	struct peer_thread_info *root_thread;
	int lastAddressError;
#if !defined( USE_WSA_EVENTS ) && defined( WIN32 )
	WNDCLASS wc;
#endif
}
#ifdef __STATIC_GLOBALS__
    global_network_data__
#endif
;

LOCATION struct network_global_data *global_network_data
#if defined( __STATIC_GLOBALS__ ) && defined( MAIN_PROGRAM )
   = &global_network_data__;
#endif
; // aka 'globalNetworkData'

#define globalNetworkData (*global_network_data)

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
int FinishUDPRead( PCLIENT pc, int broadcastEvent );
_UDP_NAMESPACE_END

#ifdef WIN32
	// errors started arrising because of faulty driver stacks.
	// spontaneous 10106 errors in socket require migration to winsock2.
   // socket is opened specifically by protocol descriptor...
SOCKET OpenSocket( LOGICAL v4, LOGICAL bStream, LOGICAL bRaw, int another_offset );
int SystemCheck( void );
#endif

// internal functions
const char * GetAddrName( SOCKADDR *addr );

void TerminateClosedClientEx( PCLIENT pc DBG_PASS );
#define TerminateClosedClient(pc) TerminateClosedClientEx(pc DBG_SRC)
void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNofity, LOGICAL bLinger DBG_PASS );
#define InternalRemoveClientEx(c,b,l) InternalRemoveClientExx(c,b,l DBG_SRC)
#define InternalRemoveClient(c) InternalRemoveClientEx(c, FALSE, FALSE )
struct peer_thread_info *IsNetworkThread( void );

PCLIENT AddActive( PCLIENT pClient );

void RemoveThreadEvent( PCLIENT pc );
void AddThreadEvent( PCLIENT pc, int broadcast );
LOGICAL TryNetworkGlobalLock( DBG_VOIDPASS );

//------------- 
// TCP Interface 

_TCP_NAMESPACE
void AcceptClient( PCLIENT pc );
int TCPWriteEx( PCLIENT pc DBG_PASS );
#define TCPWrite(pc) TCPWriteEx(pc DBG_SRC)

int FinishPendingRead( PCLIENT lpClient DBG_PASS );
LOGICAL TCPDrainRead( PCLIENT pClient );
void clearPending( PCLIENT pc );

_TCP_NAMESPACE_END

//----------------------------
// System Handling Event Interface

#ifdef _WIN32
void CPROC NetworkPauseTimer( uintptr_t psv );
#endif

int CPROC ProcessNetworkMessages( struct peer_thread_info* thread, uintptr_t unused );
uintptr_t CPROC NetworkThreadProc( PTHREAD thread );
int CPROC IdleProcessNetworkMessages( uintptr_t quick_check );

char *NetworkExpandFlags( PCLIENT pc );

//----------------------------
// ssl_close - redirected removeClient from network..
void ssl_CloseSession( PCLIENT pc );
LOGICAL ssl_IsClosed( PCLIENT pc );

//---------------------------
// some utility macros

#define IsValid(S)   ((S)!=INVALID_SOCKET)
#define IsInvalid(S) ((S)==INVALID_SOCKET)

#define CLIENT_DEFINED


SACK_NETWORK_NAMESPACE_END

#endif
