#ifndef MESSAGE_SERVICE_PROTOCOL
#define MESSAGE_SERVICE_PROTOCOL

#include <stddef.h>
#ifdef __cplusplus
using namespace sack;
#endif

#ifdef __cplusplus
#define _MSG_NAMESPACE  namespace msg {
#define _PROTOCOL_NAMESPACE namespace protocol {
#define MSGPROTOCOL_NAMESPACE namespace sack { _MSG_NAMESPACE _PROTOCOL_NAMESPACE
#define MSGPROTOCOL_NAMESPACE_END }} }
#else
#define _MSG_NAMESPACE
#define _PROTOCOL_NAMESPACE 
#define MSGPROTOCOL_NAMESPACE
#define MSGPROTOCOL_NAMESPACE_END 
#endif

SACK_NAMESPACE
	/* This namespace contains an implmentation of inter process
	   communications using a set of message queues which result
	   from 'msgget' 'msgsnd' and 'msgrcv'. This are services
	   available under a linux kernel. Reimplemented a version to
	   service for windows. This is really a client/service
	   registration and message routing system, it is not the
	   message queue itself. See <link sack::containers::message, message>
	   for the queue implementation (again, under linux, does not
	   use this custom queue).
	   
	   
	   See Also
	   RegisterService
	   
	   LoadService                                                         */
	_MSG_NAMESPACE
/* Defines structures and methods for receiving and sending
	   messages. Also defines some utility macros for referencing
		message ID from a user interface structure.                */
	_PROTOCOL_NAMESPACE

#define MSGQ_ID_BASE WIDE("Srvr")

// this is a fun thing, in order to use it,
// undefine MyInterface, and define your own to your
// library's interface structure name (the tag of the structure)
#define MSG_ID(method)  BASE_MESSAGE_ID,( ( offsetof( struct MyInterface, _##method ) / sizeof( void(*)(void) ) ) +  MSG_EventUser )
#define MSG_OFFSET(method)  ( ( offsetof( struct MyInterface, _##method ) / sizeof( void(*)(void) ) ) + MSG_EventUser )
#define INTERFACE_METHOD(type,name) type (CPROC*_##name)

// this is the techincal type of SYSV IPC MSGQueues
#define MSGIDTYPE long

// this will determine the length of parameter list
// based on the first and last parameters.
#define ParamLength( first, last ) ( ((PTRSZVAL)((&(last))+1)) - ((PTRSZVAL)(&(first))) )

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
typedef PREFIX_PACKED struct buffer_len_tag {
	CPOINTER buffer;
	size_t len;
} PACKED BUFFER_LENGTH_PAIR;
#ifdef _MSC_VER
#pragma pack (pop)
#endif


// Dispach Pending - particularly display mouse event messages
//                   needed to be accumulated before being dispatched
//                   this event is generated when no more messages
//                   have been received.
#define MSG_EventDispatchPending   0
#define MSG_DispatchPending   MSG_EventDispatchPending

// these are event message definitions.
// server events come through their function table, clients
// register an event handler... these are low numbered since
// they are guaranteed from the client/server respectively.

// Mate ended - for the client, this means that the server
//              has become defunct.  For the server, this
//              means that a client is no longer present.
//              also issued when a client volentarily leaves
//              which in effect is the same as being discovered gone.
//    param[0] = Process ID of client disconnecting
//  result_length = INVALID_INDEX - NO RESULT DATA, PLEASE!
#define MSG_MateEnded         MSG_ServiceUnload
#define MSG_ServiceUnload     0
//#define MSG_ServiceClose    MSG_ServiceUnload
//#define MSG_ServiceUnload        MSG_MateEnded

// finally - needed to define a way for the service
// to actually know when a client connects... so that
// it may validate commands as being froma good source.
// also, a multiple service server may want this to know which
// service is being loaded.
//     params + 0 = text string of the service to load
//  on return result[1] is the number of messages this routine
//  expects.
//     result[0] is the number of events this service may generate
#define MSG_MateStarted      1
#define MSG_ServiceLoad      MSG_MateStarted

// Service is about to be unloaded - here's a final chance to
// cleanup before being yanked from existance.
// Last reference to the service is now gone, going to do the unload.
#define MSG_UndefinedMessage2      2

// no defined mesasage fo this
#define MSG_UndefinedMessage3       3
// Other messages may be filled in here...

// skip a couple messages so we don't have to recompile everything
// very soon...
#define MSG_EventUser       MSG_UserServiceMessages
#define MSG_UserServiceMessages 4

enum server_event_messages {
	// these messages are sent to client's event channel
	// within the space of core service requests (0-256?)
	// it's on top of client event user - cause the library
	// may also receive client_disconnect/connect messages
   //
	MSG_SERVICE_DATA = MSG_EventUser
      , MSG_SERVICE_NOMORE // end of list - zero or more MSG_SERVICE_DATA mesasges will preceed this.
};

enum server_failure_messages {
	CLIENT_UNKNOWN
									  , MESSAGE_UNKNOWN
									  , MESSAGE_INVALID // sending server(sourced) messages to server
									  , SERVICE_UNKNOWN // could not find a service for the message.
									  , UNABLE_TO_LOAD
};

enum service_messages {
	INVALID_MESSAGE  = 0 // no message ID 0 ever.
							 , SERVER_FAILURE   = 0x80000000 // server responce to clients - failure
							 // failure may result for the above reasons.
							 , SERVER_SUCCESS   = 0x40000000 // server responce to clients - success
							 , SERVER_NEED_TIME = 0x20000000 // server needs more time to complete...
							 , SERVER_UNHANDLED = 0x10000000 // server had no method to process the message
							 , CLIENT_LOAD_SERVICE = 1 // client requests a service (load by name)
							 , CLIENT_UNLOAD_SERVICE // client no longer needs a service (unload msgbase)
							 , CLIENT_CONNECT       // new client wants to connect
							 , CLIENT_DISCONNECT    // client disconnects (no responce)
							 , RU_ALIVE             // server/client message to other requesting status
							 , IM_ALIVE             // server/client message to other responding status
							 , CLIENT_REGISTER_SERVICE // client register service (name, serivces, callback table.)
                      , CLIENT_LIST_SERVICES // client requests a list of services (optional param partial filter?)
                      , IM_TARDY   // Service needs more time, and passes back a millisecond delay-reset
};

#define LOWEST_BASE_MESSAGE 0x100

typedef struct ServiceRoute_tag SERVICE_ROUTE;
typedef struct ServiceRoute_tag *PSERVICE_ROUTE;
typedef struct ServiceEndPoint_tag SERVICE_ENDPOINT, *PSERVICE_ENDPOINT;
// this is part of the message structure
//
// this structure is avaialble at ((PSERVICE_ROUTE)(((_32*)params)-1)[-1])
// (to explain that, the first _32 back is the MsgID... to get JUST the route tag
//  have to go back one Dword then back a service_route struct...
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
PREFIX_PACKED struct ServiceEndPoint_tag
{
	MSGIDTYPE process_id;   // remote process ID
	MSGIDTYPE service_id;   // service (either served or connected as client) remote id
}PACKED;

PREFIX_PACKED struct ServiceRoute_tag
{
   SERVICE_ENDPOINT dest;
	//MSGIDTYPE process_id;   // remote process ID
   //MSGIDTYPE service_id;   // service (either served or connected as client) remote id
   SERVICE_ENDPOINT source;
   //_32 source_process_id; // need this defined here anyway; so this can be used in receivers
	//_32 source_service_id;  // the service this is connected to, or is a connection for local ID

}PACKED;
#ifdef _MSC_VER
#pragma pack (pop)
#endif
#define GetServiceRoute(data)   ((PSERVICE_ROUTE)(((_32*)data)-1)-1)

// server functions will return TRUE if no failure
// server functions will return FALSE on failure
// FAILURE or SUCCESS will be returned to the client,
//   along with any result data set.
// native mode (unspecified... one would assume
// stack passing, but the world is bizarre and these are
// probably passed by registers.

typedef int (CPROC *server_message_handler)( PSERVICE_ROUTE SourceRouteID, _32 MsgID
														 , _32 *params, size_t param_length
														 , _32 *result, size_t *result_length );
typedef int (CPROC *server_message_handler_ex)( PTRSZVAL psv
															 , PSERVICE_ROUTE SourceRouteID, _32 MsgID
															 , _32 *params, size_t param_length
															 , _32 *result, size_t *result_length );

// params[-1] == Source Process ID
// params[-2] == Source Route ID
typedef int (CPROC *server_function)( PSERVICE_ROUTE route, _32 *params, size_t param_length
										 , _32 *result, size_t *result_length );

typedef struct server_function_entry_tag{
#ifdef _DEBUG
	CTEXTSTR name;
#endif
	server_function function;
} SERVER_FUNCTION;

#ifdef _DEBUG
#define ServerFunctionEntry(name) { _WIDE(#name), name }
#else
#define ServerFunctionEntry(name) { name }
#endif

typedef SERVER_FUNCTION *server_function_table;

// MsgID will be < MSG_EventUser if it's a system message...
// MsgID will be service msgBase + Remote ID...
//    so the remote needs to specify a unique base... so ...
//    entries must still be used...
typedef int (CPROC*EventHandlerFunction)( _32 MsgID, _32*params, size_t paramlen);
typedef int (CPROC*EventHandlerFunctionEx)( PSERVICE_ROUTE SourceID, _32 MsgID, _32*params, size_t paramlen);
typedef int (CPROC*EventHandlerFunctionExx)( PTRSZVAL psv, PSERVICE_ROUTE SourceID, _32 MsgID
														 , _32*params, size_t paramlen);

// result of EventHandlerFunction shall be one fo the following values...
//   EVENT_HANDLED
// 0 - no futher action required
//   EVENT_WAIT_DISPATCH
// 1 - when no further events are available, please send event_dispatched.
//     this Event was handled by an internal queuing for later processing.
enum EventResult {
	EVENT_HANDLED = 0,
	EVENT_WAIT_DISPATCH = 1
};

//------------------- Begin Server Message Structs ----------------

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
typedef struct MsgSvr_RegisterRequest_msg MsgSvr_RegisterRequest;
PREFIX_PACKED struct MsgSvr_RegisterRequest_msg
{
	MSGIDTYPE RouteID;
   MSGIDTYPE ClientID;  // service_id...
}PACKED;

typedef struct MsgSrv_ReplyServiceLoad_msg MsgSrv_ReplyServiceLoad;
PREFIX_PACKED struct MsgSrv_ReplyServiceLoad_msg
{
	MSGIDTYPE ServiceID;
	THREAD_ID thread; // if this is a local service, it's dispatched this way?
}PACKED;


#ifdef _MSC_VER
#pragma pack (pop)
#endif

MSGPROTOCOL_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::msg::protocol;
#endif

#endif

