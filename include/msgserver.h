#ifndef SERVER_MESSAGE_INTERFACE
#define SERVER_MESSAGE_INTERFACE


#ifdef BCC16
# ifdef SERVERMSG_SOURCE
# define SERVERMSG_PROC(type,name) type STDPROC _export name
# else
# define SERVERMSG_PROC(type,name) type STDPROC name
# endif
#else
# ifdef SERVERMSG_SOURCE
# define SERVERMSG_PROC(type,name) EXPORT_METHOD type CPROC name
# else
# define SERVERMSG_PROC(type,name) IMPORT_METHOD type CPROC name
# endif
#endif

#include <msgprotocol.h>

#ifdef __cplusplus
#define _SERVER_NAMESPACE namespace server {
#define MSGSERVER_NAMESPACE namespace sack { namespace msg { namespace server {
#define MSGSERVER_NAMESPACE_END }} }
#else
#define _SERVER_NAMESPACE 
#define MSGSERVER_NAMESPACE 
#define MSGSERVER_NAMESPACE_END
#endif

SACK_NAMESPACE
   _MSG_NAMESPACE
/* Defines methods and events that the service side might want
   to use.                                                     */
	_SERVER_NAMESPACE

#ifdef _DEBUG
#define CLIENT_TIMEOUT   120000 // 2 seconds
#else
#define CLIENT_TIMEOUT   2000
#endif


/* User callback signature to return the function callback table
   to the server for event dispatch to a service (?) (INTERNAL?) */
typedef int (CPROC *GetServiceFunctionTable)(server_function_table *ppTable
												  ,_32 *nEntries
												  ,_32 MsgBase);

#ifndef CLIENT_MESSAGE_INTERFACE
#ifndef CLIENTMSG_SOURCE
// now - is there some magic to allow libraries to link to
// the core application?? - this is in the server's core
// and is access by the services it loads.
SERVERMSG_PROC(int, SendMultiServiceEvent)( _32 pid, _32 event
								 , _32 parts
								 , ... );
/* <combine sack::msg::client::SendMultiServiceEvent@_32@_32@_32@...>
   
   \ \                                                                */
#define SendServiceEvent(pid,event,data,len) SendMultiServiceEvent(pid,event,1,data,len)
//void SendServiceEvent( _32 pid, _32 event, _32 *data, _32 len );
#endif
#endif

MSGSERVER_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::msg::server;
#endif

#endif
// $Log: msgserver.h,v $
// Revision 1.11  2005/05/12 21:12:50  jim
// Merge process_service_branch into trunk development.
//
// Revision 1.10.2.1  2005/05/02 17:01:08  jim
// Nearly works... time to move over to linux... still need some cleanup on exits... and dead clients.
//
// Revision 1.10  2003/10/28 01:14:34  panther
// many changes to implement msgsvr on windows.  Even to get displaylib service to build, there's all sorts of errors in inconsistant definitions...
//
// Revision 1.9  2003/09/21 11:49:04  panther
// Fix service refernce counting for unload.  Fix Making sub images hidden.
// Fix Linking of services on server side....
// cleanup some comments...
//
// Revision 1.8  2003/03/25 08:38:11  panther
// Add logging
//
