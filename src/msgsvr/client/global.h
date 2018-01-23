#ifdef __QNX__
#define USE_SACK_MSGQ
#endif

//#define ENABLE_GENERAL_USEFUL_DEBUGGING

#ifdef ENABLE_GENERAL_USEFUL_DEBUGGING
//#define DEBUG_THREAD
#define LOG_LOCAL_EVENT
//#define DEBUG_RU_ALIVE_CHECK
//#define DEBUG_HANDLER_LOCATE
#define LOG_SENT_MESSAGES
// event messages need to be enabled to log event message data...
#define DEBUG_DATA_XFER
//#define NO_LOGGING
/// show event messages...
#define DEBUG_EVENTS
#define DEBUG_OUTEVENTS
/// attempt to show the friendly name for messages handled
#define LOG_HANDLED_MESSAGES
//#define DEBUG_MESSAGE_BASE_ID
#define _DEBUG_RECEIVE_DISPATCH_
//#define DEBUG_THREADS
//#define DEBUG_MSGQ_OPEN
#endif

#include <stdhdrs.h>
#ifdef __LINUX__
#include <time.h>
// ipc sysv msgque (msgget,msgsnd,msgrcv)
#ifdef __ANDROID__
#  include <linux/ipc.h>
#  include <linux/msg.h>
#else
#  ifndef USE_SACK_MSGQ
#    include <sys/ipc.h>
#    include <sys/msg.h>
#  endif
#endif
#include <signal.h>
#define MSGTYPE (struct msgbuf*)
#else
#define MSGTYPE
#endif
#include <msgclient.h>
#include <msgserver.h>
#include <procreg.h>
#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include <timers.h>
#include <idle.h>


MSGCLIENT_NAMESPACE

#define MSG_DEFAULT_RESULT_BUFFER_MAX (sizeof( uint32_t ) * 16384)

#ifdef  _DEBUG
#define DEFAULT_TIMEOUT 300000 // standard transaction timout
#else
#define DEFAULT_TIMEOUT 500 // standard transaction timout
#endif


#if defined( _WIN32 ) || defined(USE_SACK_MSGQ)
#ifdef _MSC_VER
#undef errno
#define errno GetLastError()
#endif
#define MSGQ_TYPE PMSGHANDLE
#else
#define MSGQ_TYPE int
#endif

typedef PREFIX_PACKED struct message_header_tag
{
// internal message information
	// transportable across messsage queues or networks
   SERVICE_ENDPOINT source;
	uint32_t msgid;
// #define MSGDATA( msghdr ) ( (&msghdr.sourceid)+1 )
} PACKED MSGHDR;

typedef PREFIX_PACKED struct full_message_header_tag
{
	SERVICE_ENDPOINT dest;
	MSGHDR hdr;
#define QMSGDATA( qmsghdr ) ((uint32_t*)( (qmsghdr)+1 ))
	//uint32_t data[];
} PACKED QMSG;

// these are client structures
// services which are loaded result in the creation
// of a handler device.
typedef struct ServiceEventHandler_tag EVENTHANDLER, *PEVENTHANDLER;
struct ServiceEventHandler_tag
{
	DeclareLink( EVENTHANDLER );
	struct {
		BIT_FIELD dispatched : 1;
		BIT_FIELD notify_if_dispatched : 1;
		BIT_FIELD destroyed : 1;
		BIT_FIELD local_service : 1;
	} flags;
	// when receiving messages from the application...
	// this is the event base which it will give me...
	// it is also the unique routing ID
	// my list will be built from the sum of all services
	// connected to, and the number of MsgCount they claim to have..
	SERVICE_ROUTE RouteID;
	THREAD_ID EventID;
	// destination address of this service's
	// messages...

	EventHandlerFunction Handler;
	EventHandlerFunctionEx HandlerEx;
	EventHandlerFunctionExx HandlerExx;
	uintptr_t psv;
	// this handler's events come from this queue.
	MSGQ_TYPE msgq_events;
	////CRITICALSECTION csMsgTransact;
	// timeout on this local handler's reception of a responce.

	TEXTSTR servicename;
};

typedef struct ServiceTransactionHandler_tag TRANSACTIONHANDLER, *PTRANSACTIONHANDLER;
struct ServiceTransactionHandler_tag
{
	DeclareLink( TRANSACTIONHANDLER );

	struct {
		BIT_FIELD waiting_for_responce : 1;
		BIT_FIELD responce_received : 1;
		BIT_FIELD bCheckedResponce : 1;
	} flags;
	PEVENTHANDLER transaction_for_handler;  // the handler this transaction belongs to.
	PSERVICE_ROUTE route;

	CRITICALSECTION csMsgTransact;
	// timeout on this local handler's reception of a responce.
	uint32_t wait_for_responce;
	MSGIDTYPE LastMsgID; // last outbound resquest - which is waiting for a responce
	MSGIDTYPE *MessageID; // address of the reply message ID
	POINTER msg;
	size_t *len;
	uint32_t last_check_tick;
	PQMSG MessageIn;
	size_t MessageLen;
	
};

// registration of a service results in a new
// CLIENT_SERVICE structure.  Messages are received
// from clients, and dispatched.. ..
// Just like clients, services also support events...
typedef struct service_tag
{
	struct {
		BIT_FIELD bRegistered : 1;
		BIT_FIELD bFailed : 1;
		BIT_FIELD connected : 1;
		BIT_FIELD bMasterServer : 1;
		BIT_FIELD bWaitingInReceive : 1;
		BIT_FIELD bClosed : 1;
	} flags;

	GetServiceFunctionTable GetFunctionTable;
	server_function_table functions;
	server_message_handler handler;
	server_message_handler_ex handler_ex;
	uintptr_t psv;
	uint32_t entries;
	uint32_t references;
	SERVICE_ROUTE route;  // this is a static route that goes to the message server.
	MSGIDTYPE ServiceID;  // when registered this is our ID for later message handler resolution
	TEXTCHAR *name;
	PTHREAD thread;
	//uint32_t pid_me; // these pids should be phased out, and given an ID from msgsvr.
	PLIST service_routes; // these are clients that are connected to this service

	PQMSG recv;
	PQMSG result;
	DeclareLink( struct service_tag );
} CLIENT_SERVICE, *PCLIENT_SERVICE;


// PEVENTHANDLER creation also results in the creation of one of these...
// this allows the ProbeClientAlive to work correctly...
// then on the server side, when the service is loaded, one of these is
// created... allowing the server to monitor client connectivity.
typedef struct service_client_tag
{
   SERVICE_ROUTE route;
	//uint32_t pid; // process only? no thread?
	uint32_t last_time_received;
	struct {
		BIT_FIELD valid : 1;
		BIT_FIELD error : 1; // uhmm - something like got a message from this but it wasn't known
		BIT_FIELD status_queried : 1;
		BIT_FIELD is_service : 1; // created to track connection from client to service...
	} flags;
	PLIST services;
	PEVENTHANDLER handler;
	DeclareLink( struct service_client_tag );
} SERVICE_CLIENT, *PSERVICE_CLIENT;

typedef  struct sleeper_tag
{
	PTHREAD thread;
   PTRANSACTIONHANDLER handler;
} SLEEPER, *PSLEEPER;


#define MAXBUFFER_LENGTH_PAIRSPERSET 256
/* use a small local pool of these instead of allocate/release */
/* this high-reuse area just causes fragmentation */
DeclareSet( BUFFER_LENGTH_PAIR );

#if defined( _WIN32 ) || defined( USE_SACK_MSGQ )
#define IPC_NOWAIT MSGQUE_NOWAIT

#ifndef ENOMSG
#define ENOMSG MSGQUE_ERROR_NOMSG
#endif

#ifndef EINTR
#define EINTR -1
#endif

#ifndef EIDRM
#define EIDRM -2
#endif

#ifndef EINVAL
#define EINVAL -3
#endif

#ifndef E2BIG
#define E2BIG -4
#endif

#ifndef EABORT
#define EABORT MSGQUE_ERROR_EABORT
#endif

#ifdef DEBUG_DATA_XFER
#  define msgsnd( q,msg,len,opt) ( _lprintf(DBG_RELAY)( WIDE("Send Message...%d %d"), len, len+4 ), LogBinary( (uint8_t*)msg, (len)+4  ), EnqueMsg( q,(msg),(len),(opt)) )
#else
#  define msgsnd(a,b,c,d) EnqueMsg((PMSGHANDLE)a,b,c,d)
#endif
	#define msgrcv(q,m,sz,id,o) DequeMsg((PMSGHANDLE)q,&id,m,sz,o)
	#define MSGQSIZE 32768
	#define IPC_CREAT  1
	#define IPC_EXCL	2
#define msgget(name,n,opts) ( (opts) & IPC_CREAT )		\
	? ( ( (opts) & IPC_EXCL)									  \
	  ? ( SackOpenMsgQueue( name, NULL, 0 )						\
		 ? (MSGQ_TYPE)MSGFAIL													 \
		 : SackCreateMsgQueue( name, MSGQSIZE, NULL, 0 ))	  \
	  : SackCreateMsgQueue( name, MSGQSIZE, NULL, 0 ))		 \
	: SackOpenMsgQueue( name, NULL, 0 )
	//#define msgget(name,n,opts ) CreateMsgQueue( name, MSGQSIZE, NULL, 0 )
	#define msgctl(n,o,f) 
	#define MSGFAIL NULL
#else
#ifdef DEBUG_DATA_XFER
	//#define msgsnd( q,msg,len,opt) ( _xlprintf(1 DBG_RELAY)( WIDE("Send Message...") ), LogBinary( (POINTER)msg, (len)+4  ), msgsnd( q,(msg),(len),(opt)) )
	#define msgsnd( q,msg,len,opt) ( lprintf( WIDE("Send Message...") ), LogBinary( (POINTER)msg, (len)+4  ), msgsnd( q,(msg),(len),(opt)) )
#endif
	#define msgget( name,n,opts) msgget( n,opts )
	#define MSGFAIL -1
#endif


typedef struct global_message_service_tag
{
	struct {
		BIT_FIELD message_responce_handler_ready : 1;
		BIT_FIELD message_handler_ready : 1;
		BIT_FIELD failed : 1;
		BIT_FIELD events_ready : 1;
		BIT_FIELD local_events_ready : 1;
		BIT_FIELD disconnected : 1;
		//BIT_FIELD connected : 1;
		BIT_FIELD handling_client_message : 1;
		BIT_FIELD bAliveThreadStarted : 1;
		// enabled when my_message_id is resolved...
		BIT_FIELD connected : 1;
		BIT_FIELD bMasterServer : 1;
		BIT_FIELD bServiceHandlerStarted : 1; // thread for service handling...
		BIT_FIELD bCoreServiceHandlerStarted : 1; // thread for service handling...
		BIT_FIELD found_server : 1; // set this when a valid server was found - dont' reset master id
		BIT_FIELD bWaitingInReceive : 1; // receive is waiting to get service input messages
		BIT_FIELD bCoreServiceInputHandlerStarted : 1;
	} flags;

   // idenfitier for this process.  Messages from me, and to me reference this ID.
	MSGIDTYPE my_message_id;

	PLIST pSleepers;

	PTHREAD pThread;			// handles messages on msgq_in (responce from service)
	PTHREAD pMessageThread;  // handles messages on msgq_out (request for service)
	PTHREAD pEventThread;	 // handles messages to msgq_event (client side events)
	PTHREAD pLocalEventThread; // handles messages on msgq_local_event (client only internal events)

	// these are client side connection managers.
   // for each service connected to, there is a handler.
	PEVENTHANDLER pHandlers;

	// these are for handling send/receive matched response things
   // maybe these are temporary based on PEVENTHANDLERs
   PTRANSACTIONHANDLER pTransactions;

	// this list is only populated in the master server.  This is all registered servers
   // Actually, this service list is also maintained in the registerant of the service.
	PCLIENT_SERVICE services;

	// these are tracked from...
   // clients have a pHandler in them.
	PSERVICE_CLIENT clients;

	EVENTHANDLER _handler; // a static handler to cover communication with this library itself.

	// this is the one waiting for a register service ack...
	// I won't have the ServiceID
	CRITICALSECTION csLoading;

	CRITICALSECTION csMsgTransact;

	SERVICE_ROUTE master_service_route;  // this is a static route that goes to the message server.

	MSGQ_TYPE msgq_in;	// id + 1 (thread 0)
	MSGQ_TYPE msgq_out;  // id + 0 (thread 0)
	MSGQ_TYPE msgq_event;// id + 2 (thread 1)
	MSGQ_TYPE msgq_local;// id + 3 (thread 2 (or 1 if only local)

	PLINKQUEUE Messages;
	int pending;
} GLOBAL;
#ifdef g
#   undef g
#endif
#define g (*global_msgclient)
#ifndef DEFINE_MESSAGE_SERVER_GLOBAL
extern
#endif
GLOBAL *global_msgclient;

//-----  client_common.c -------
// register with amster is called as part of _InitMessageService...
void RegisterWithMasterService( void );
int _InitMessageService( int local );
void CPROC MonitorClientActive( uintptr_t psv );
void CloseMessageQueues( void );
void DropMessageBuffer( PQMSG msg );
PQMSG GetMessageBuffer( void );


//------ client_input.c -------
PTRANSACTIONHANDLER GetTransactionHandler( PSERVICE_ROUTE route );
//int GetAMessageEx( MSGQ_TYPE msgq, CTEXTSTR q, int flags DBG_PASS );
uintptr_t CPROC HandleMessages( PTHREAD thread );
int WaitReceiveServerMsg ( PSLEEPER sleeper
				, uint32_t MsgOut
					DBG_PASS );
int QueueWaitReceiveServerMsg ( PSLEEPER sleeper, PTRANSACTIONHANDLER handler
										  , MSGIDTYPE *MsgIn
										  , POINTER BufferIn
										  , size_t *LengthIn
											DBG_PASS );

//------ client_client.c ------
PSERVICE_CLIENT FindClient( PSERVICE_ROUTE pid );


//-----  client_event.c ---------
uintptr_t CPROC HandleEventMessages( PTHREAD thread );
int HandleEvents( MSGQ_TYPE msgq, PQMSG MessageEvent, int initial_flags );

//----- client_local.c -------
uintptr_t CPROC HandleLocalEventMessages( PTHREAD thread );

//----- client_service.c --------
int ReceiveServerMessageEx( PTRANSACTIONHANDLER handler, PQMSG MessageIn, size_t MessageLen DBG_PASS );
uintptr_t CPROC HandleServiceMessages( PTHREAD thread );
LOGICAL HandleCoreMessage( PQMSG msg, size_t msglen DBG_PASS );


MSGCLIENT_NAMESPACE_END
