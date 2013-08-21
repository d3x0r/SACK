#ifndef CLIENT_MESSAGE_INTERFACE
#define CLIENT_MESSAGE_INTERFACE

#include <sack_types.h>

#  ifdef CLIENTMSG_SOURCE
#   define CLIENTMSG_PROC(type,name) EXPORT_METHOD type CPROC name
#  else
#   define CLIENTMSG_PROC(type,name) IMPORT_METHOD type CPROC name
#  endif

#include <msgprotocol.h>

#ifdef __cplusplus
#define _CLIENT_NAMESPACE namespace client {
#define MSGCLIENT_NAMESPACE SACK_NAMESPACE namespace msg { namespace client {
#define MSGCLIENT_NAMESPACE_END }} SACK_NAMESPACE_END
#else
#define _CLIENT_NAMESPACE
#define MSGCLIENT_NAMESPACE 
#define MSGCLIENT_NAMESPACE_END

#endif
SACK_NAMESPACE
   _MSG_NAMESPACE
	/* Defines methods and macros for use as a client of a service. */
	_CLIENT_NAMESPACE


#define MSG_DEFAULT_RESULT_BUFFER_MAX (sizeof( _32 ) * 16384)

	// not realy sure where this doc goes
   //
// result is TRUE/FALSE.  Successful registration will result TRUE.
// a timeout to the master message service
// only [functions, entries] or [event_handler]
// need to be specified.  If both are specified....
// then both methods for invoking functions will be used.
// Also if event_handler is used, then the result of this is MsgBase
//  and needs to be subtracted from the MsgID to...
//  --- above comments are somewhat invalid --- events have bee biased
//   to have event 0 be the first user event... system events have the
// MSB (most sig bit) set.


// Use this to make sure message subsystem is active
CLIENTMSG_PROC( int, InitMessageService )( void );

// allow messaage service opportunity to shutdown.?
CLIENTMSG_PROC( void, CloseMessageService )( void );


// the result is a RouteID.  (ie. or MsgBase)...
CLIENTMSG_PROC( LOGICAL, RegisterServiceExx )( CTEXTSTR name
																	 , server_function_table functions
																	 , int entries
																	 , server_message_handler event_handler
																	 , server_message_handler_ex handler_ex
																	 , PTRSZVAL psv
																	 );
#define RegisterServiceEx( n,f,psv ) RegisterServiceExx( n,NULL,16,NULL,f,psv)
#define RegisterService(n,f,e)        RegisterServiceExx(n,f,e,NULL,NULL,0)
#define RegisterServiceHandler(n,f)   RegisterServiceExx(n,NULL,16,f,NULL,0)
#define RegisterServiceHandlerEx(n,f,psv)   RegisterServiceExx(n,NULL,16,NULL,f,psv)
CLIENTMSG_PROC( void, UnregisterService )( CTEXTSTR name );

CLIENTMSG_PROC( int, ProcessClientMessages )( PTRSZVAL unused );

// returns INVALID_INDEX on failure - else is message base to servic.
// the core message services may be opened with ID 0 - which serves
// to dispatch local event queue messages... transactions between client
// and server hrm - suppose someone could send to this service ID...
// LoadService( NULL ) will return 0 on success
#define INVALID_MESSAGE_BASE ((_32)-1)
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadService )( CTEXTSTR service, EventHandlerFunction );
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceEx )( CTEXTSTR service, EventHandlerFunctionEx );
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceExx )( CTEXTSTR service, EventHandlerFunctionExx, PTRSZVAL psv );
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceAsServer )( CTEXTSTR service, EventHandlerFunction, server_message_handler );
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceAsServerEx )( CTEXTSTR service, EventHandlerFunctionEx, server_message_handler );
CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceAsServerExx )( CTEXTSTR service, EventHandlerFunctionExx, server_message_handler_ex, PTRSZVAL psv );
CLIENTMSG_PROC( void, UnloadService )( CTEXTSTR service );

/*
 When a service connects, this registered callback is called.
 static LOGICAL OnServiceConnect( "RegisterService_name" )( PSERVICE_ROUTE route_to_client );
 */
#define OnServiceConnect(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY  \
	,"SACK/Message Service",_OnServiceConnect,WIDE("server"),name,WIDE("on_connect")  \
	,LOGICAL,(PSERVICE_ROUTE), __LINE__)


// this extended feature does not look up the routing ID
// from the message ID, and if you happen to somehow know
// the destionation process, you can send any message directed to it
// with this.
CLIENTMSG_PROC( int, TransactRoutedServerMultiMessageEx )( PSERVICE_ROUTE RouteID
																			, _32 MsgOut, _32 buffers
																			, _32 *MsgIn
																			, POINTER BufferIn, size_t *LengthIn
																			, _32 timeout // non zero overrides default timeout
																			// buffer starts arg list, length is
																			// not used, but is here for demonstration
																	, ... );

// again if you happen to know something special, you can use the routeID
// to probe if a client is alive - it's a 10ms check.
CLIENTMSG_PROC( int, ProbeClientAlive )( PSERVICE_ENDPOINT RouteID );

// this is the message which should NORMALLY be used.
// timeout willreturn FALSE else a reponce (or non-responce) will result
// in non-zero.
										 // buffer starts arg list, length is
                               // not used, but is here for demonstration
typedef  int (CPROC *TSMMProto)(PSERVICE_ROUTE,_32, _32, _32 *, POINTER , size_t *,...);
CLIENTMSG_PROC( TSMMProto, TransactServerMultiMessageExEx )( DBG_VOIDPASS );

CLIENTMSG_PROC( int, TransactServerMultiMessage )( PSERVICE_ROUTE RouteID
																 , _32 MsgOut, _32 buffers
																 , _32 *MsgIn, POINTER BufferIn, size_t *LengthIn
																  // buffer starts arg list, length is
																  // not used, but is here for demonstration
																 , ... );
//#define TransactServerMultiMessage TransactServerMultiMessageExEx(DBG_VOIDSRC)

CLIENTMSG_PROC( int, TransactServerMessageExx)( PSERVICE_ROUTE RouteID
															 , _32 MsgOut, CPOINTER BufferOut, size_t LengthOut
															 , _32 *MsgIn, POINTER BufferIn, size_t *LengthIn
															  , _32 timeout DBG_PASS );
CLIENTMSG_PROC( int, TransactServerMessageEx )( PSERVICE_ROUTE RouteID
															 , _32 MsgOut, CPOINTER BufferOut, size_t LengthOut
															 , _32 *MsgIn, POINTER BufferIn, size_t *LengthIn DBG_PASS);
CLIENTMSG_PROC( int, TransactServerMultiMessageEx )( PSERVICE_ROUTE RouteID
																	, _32 MsgOut, _32 buffers
																	, _32 *MsgIn, POINTER BufferIn, size_t *LengthIn
                                                   , _32 timeout
																	 // buffer starts arg list, length is
																	 // not used, but is here for demonstration
																	, ... );
//#define TransactServerMessageEx(mo,bo,lo,mi,bi,li DBG_RELAY) TransactServer
#define TransactServerMessage(rid,mo,bo,lo,mi,bi,li) TransactServerMessageEx(rid,mo,bo,lo,mi,bi,li DBG_SRC )
#define TransactRoutedServerMessage(ro,mo,bo,lo,mi,bi,li) TransactRoutedServerMultiMessageEx(ro,mo,1,mi,bi,li,0,bo,lo DBG_SRC )
//#define TransactRoutedMessage(ro,mo,bo,lo,mi,bi,li) TransactMultiServerMessageEx(ro,mo,bi,li,1,&bo DBG_SRC )

// these are provided but are storngly discouraged from use.
CLIENTMSG_PROC( int, SendRoutedServerMultiMessage )( PSERVICE_ROUTE RouteID, _32 MessageID, _32 buffers, ... );
CLIENTMSG_PROC( int, SendRoutedServerMessage )( PSERVICE_ROUTE RouteID, _32 MessageID, POINTER msg, size_t len );
CLIENTMSG_PROC( int, SendServerMultiMessage )( PSERVICE_ROUTE RouteID
															, _32 MessageID, _32 buffers, ... );
CLIENTMSG_PROC( int, SendServerMessage )( PSERVICE_ROUTE RouteID
													 , _32 MessageID, POINTER msg, size_t len );

// really I guess the integeration of all message handles [as a msg_base_id] allows some unique opportunities....
// this message is one of those that does not take a base message ID...
#define TellClientTardy( client_source_id, new_responce_timeout )  { _32 timeout = new_responce_timeout; lprintf( WIDE("TELL TARDY") ); SendInMessage( (client_source_id), IM_TARDY, &timeout, sizeof( timeout ) ); }
// TellClientTardy( 5000000 ); // tell client to wait a LONG time.
// TellClientTardy( 3000 ); // reset timeout to 3 seconds

// this may result FALSE if it satisfied no message
// otherwise it will return true...
// the waiter then check his PEVENTHANDLER to see if ti was complete.
//CLIENTMSG_PROC( int, ReceiveServerMessageEx )( _32 *MessageID, POINTER msg, _32 *len DBG_PASS);
//#define ReceiveServerMessage(mid,m,l) ReceiveServerMessageEx( mid,m,l DBG_SRC )
CLIENTMSG_PROC( void, DumpServiceList )( void );
CLIENTMSG_PROC( void, GetServiceList )( PLIST *ppList );

CLIENTMSG_PROC( int, SendInMultiMessage )( PSERVICE_ROUTE routeID, _32 MsgID, _32 parts, BUFFER_LENGTH_PAIR *pairs );
CLIENTMSG_PROC( int, SendInMessage )( PSERVICE_ROUTE routeID, _32 MsgID, POINTER buffer, size_t length );

// now - is there some magic to allow libraries to link to
// the core application?? - this is in the server's core
// and is access by the services it loads.
// add event base - determines which loaded service this
// event is destined to...
typedef int (CPROC* SendMultiServiceEventProto)( PSERVICE_ROUTE pid, _32 event
															  , _32 parts
															  ,... );

CLIENTMSG_PROC(int, SendMultiServiceEventPairs)( PSERVICE_ROUTE pid, _32 event
															  , _32 parts
															  , BUFFER_LENGTH_PAIR *pairs );
// SendMultiServiceEvent calls ...Pairs with all information and the address
// of the var-args...
CLIENTMSG_PROC(int, SendMultiServiceEvent)( PSERVICE_ROUTE pid, _32 event
								 , _32 parts
								 , ... );
CLIENTMSG_PROC(SendMultiServiceEventProto, SendMultiServiceEventEx)( DBG_VOIDPASS );
#define SendMultiServiceEvent SendMultiServiceEventEx(DBG_VOIDSRC)
#define SendServiceEvent(pid,event,data,len) SendMultiServiceEvent(pid,event,1,data,len)
//void SendServiceEvent( _32 pid, _32 event, _32 *data, _32 len );

// sends a message on the out queue.  Normally one should use SendServerMessage et al.
typedef struct message_header_tag *PMSGHDR;
typedef struct full_message_header_tag *PQMSG;
CLIENTMSG_PROC( int, SendOutMessage )( PQMSG buffer, size_t len );

CLIENTMSG_PROC( LOGICAL, IsSameMsgEndPoint )( PSERVICE_ENDPOINT a, PSERVICE_ENDPOINT b );
CLIENTMSG_PROC( LOGICAL, IsSameMsgSource )( PSERVICE_ROUTE a, PSERVICE_ROUTE b );
CLIENTMSG_PROC( LOGICAL, IsSameMsgDest )( PSERVICE_ROUTE a, PSERVICE_ROUTE b );
CLIENTMSG_PROC( LOGICAL, IsMsgSourceSameAsMsgDest )( PSERVICE_ROUTE a, PSERVICE_ROUTE b );



MSGCLIENT_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::msg::client;
#endif


#endif
// $Log: msgclient.h,v $
// Revision 1.26  2005/06/30 18:31:32  jim
// Deifne extended methods for communication.
//
// Revision 1.22  2005/06/30 13:22:44  d3x0r
// Attempt to define preload, atexit methods for msvc.  Fix deadstart loading to be more protected.
//
// Revision 1.21  2005/05/30 11:56:20  d3x0r
// various fixes... working on psilib update optimization... various stabilitizations... also extending msgsvr functionality.
//
// Revision 1.20  2005/05/25 16:50:09  d3x0r
// Synch with working repository.
//
// Revision 1.23  2005/05/23 19:32:03  jim
// Remove extra definition of eventhandlerfunction typedef
//
// Revision 1.22  2005/05/23 19:26:07  jim
// Improved registeration of services to allow a specification of a message-proc style handler.... Also worked on straightening out MSG_ and CLIENT_ message IDs...
//
// Revision 1.21  2005/05/13 00:32:06  jim
// Added some missing functions for clean windows build
//
// Revision 1.19.2.3  2005/05/12 21:02:30  jim
// Add extended transaction method that takes a RoutingID
//
// Revision 1.19.2.2  2005/05/06 21:38:51  jim
// Fix sendservice event definition.
//
// Revision 1.19.2.1  2005/05/02 17:01:08  jim
// Nearly works... time to move over to linux... still need some cleanup on exits... and dead clients.
//
// Revision 1.19  2004/12/19 15:44:57  panther
// Extended set methods to interact with raw index numbers
//
// Revision 1.18  2004/09/30 01:14:42  d3x0r
// Cleaned up consistancy of PID and thread ID... extended message service a bit to supply event PID both ways.
//
// Revision 1.17  2004/09/29 00:49:47  d3x0r
// Added fancy wait for PSI frames which allows non-polling sleeping... Extended Idle() to result in meaningful information.
//
// Revision 1.16  2004/09/22 20:26:13  d3x0r
// Begin implementation of message queues to handle events from video to application
//
// Revision 1.15  2003/10/28 01:14:34  panther
// many changes to implement msgsvr on windows.  Even to get displaylib service to build, there's all sorts of errors in inconsistant definitions...
//
// Revision 1.14  2003/09/19 14:52:40  panther
// Added new procedure name registry
//
// Revision 1.13  2003/03/27 15:36:38  panther
// Changes were done to limit client messages to server - but all SERVER-CLIENT messages were filtered too... Define LOWEST_BASE_MESSAGE
//
// Revision 1.12  2003/03/25 08:38:11  panther
// Add logging
//
