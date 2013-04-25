#ifndef NETWORK_SERVICE_RESPOND_DISCOVER
#define NETWORK_SERVICE_RESPOND_DISCOVER

#ifdef BCC16
#ifdef NETSERVICE_SOURCE
#define NETSERVICE_PROC(type,name) type STDPROC _export name
#else
#define NETSERVICE_PROC(type,name) type STDPROC name
#endif
#else
#if !defined(__STATIC__) && !defined(__UNIX__)
#ifdef NETSERVICE_SOURCE
#define NETSERVICE_PROC(type,name) EXPORT_METHOD type name
#else
#define NETSERVICE_PROC(type,name) IMPORT_METHOD type name
#endif
#else
#ifdef NETSERVICE_SOURCE
#define NETSERVICE_PROC(type,name) type name
#else
#define NETSERVICE_PROC(type,name) extern type name
#endif
#endif
#endif

#include <sack_types.h>
#include <network.h> // SOCKADDR
//---------------------------------------------------------------------
// Discover service opens a udp port, and optionally broadcasts or asks
// the local box only for a service at the specified port.  The service
// should be monitoring the port with RespondService.
//
// Intended use is to use a TCP/UDP pairing, the UDP is used to discover
// if the service exits, and then a TCP connection to said service
// would be created.

NETSERVICE_PROC( int, DiscoverService )( int netwide
													, int port
													, int (CPROC *Handler)( PTRSZVAL psvUser, SOCKADDR *responder )
													, PTRSZVAL psvUser );
NETSERVICE_PROC( void, EndDiscoverService )( int port );


//---------------------------------------------------------------------
// At the moment only one port per application may be used
// for service responce...
// there is no indication to the application that a request for the service
// has been received... however one may consider implementing a callback
// such that the appliction may determine whether the responce is given,
// of if it wants to play dead.

NETSERVICE_PROC( int, ServiceRespond )( int port );

NETSERVICE_PROC( void, EndServiceRespondEx )( int port );
NETSERVICE_PROC( void, EndServiceRespond )( void );

#endif
