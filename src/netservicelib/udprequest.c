#ifndef NETSERVICE_SOURCE
#define NETSERVICE_SOURCE
#endif
#include <stdhdrs.h>
#include <network.h>
#include <sharemem.h>
#include <timers.h>
#define DO_LOGGING
#include <logging.h>
#include <netservice.h>

typedef struct responce_handlers{
	SOCKADDR *saAsk;
	uint16_t port;
	int timer;
	int (CPROC*HandleResponce)( uintptr_t psv, SOCKADDR *responder );
   uintptr_t psvUser;
	struct responce_handlers *next, **me;
} RESPONCEHANDLER, *PRESPONCEHANDLER;

static PCLIENT requestor;
static PRESPONCEHANDLER responders;

#define LOCALADDR "127.0.0.1" // local host discovery only....
#define BROADCASTADDR "255.255.255.255" // ask everyone where the service is.
// this broadcast is appropriate for things like watchdog....

static void CPROC RequestTimer( PRESPONCEHANDLER prh )
{
  	if( prh && prh->saAsk )
  	{
		Log1( "Sending request for WHERE RU service %d", prh->port );
      Log8( " %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d",
       				*(((unsigned char *)prh->saAsk)+0),
       				*(((unsigned char *)prh->saAsk)+1),
       				*(((unsigned char *)prh->saAsk)+2),
       				*(((unsigned char *)prh->saAsk)+3),
       				*(((unsigned char *)prh->saAsk)+4),
       				*(((unsigned char *)prh->saAsk)+5),
       				*(((unsigned char *)prh->saAsk)+6),
       				*(((unsigned char *)prh->saAsk)+7) );
		SendUDPEx( requestor, (void*)"WHERE RU", 8, prh->saAsk );
	}
	else
		Log( "Failed to find who to ask them..." );
}

struct handler_params {
	int received;
	PRESPONCEHANDLER prh;
	SOCKADDR *sa;
} param;

uintptr_t CPROC InvokeHandler( PTHREAD thread )
{
	struct handler_params *params = (struct handler_params*)GetThreadParam( thread );
	params->received = 1;
	params->prh->HandleResponce( params->prh->psvUser, params->sa );
	return 0;
}

static void CPROC UDPMessage( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *sa )
{
	if( !buffer )
	{
		Log( "Allocate Buffer for UDP Discovery messages..." );
		buffer = Allocate( 1024 );
	}
	else
	{
		if( *(uint64_t*)buffer == *(uint64_t*)"IM HERE!" )
		{
			PRESPONCEHANDLER prh = responders;
			uint16_t port;// = GetNetworkLong( pc, GNL_PORT );
			GetAddressParts( sa, NULL, &port );
			//Log( "Received a responce from service!" );
			while( prh && (prh->port != port ) )
			{
				prh = prh->next;
			}
			if( prh && prh->HandleResponce )
			{
				struct handler_params param;
				param.sa = sa;
				param.prh = prh;
				param.received =0;
				//Log( "Found a service handler, calling it..." );
				ThreadTo( InvokeHandler, (uintptr_t)&param );
				while( !param.received )
					Relinquish();
			}
			//else
			//   Log( "Could not find service handler" );
		}
	}
	ReadUDP( pc, buffer, 1024 );
}


NETSERVICE_PROC( int, DiscoverServiceEx )( int netwide
													, int port
													, int (CPROC*Handler)( uintptr_t psv, SOCKADDR *responder )
													, uintptr_t psvUser 
													, int bRespond )
{
	PRESPONCEHANDLER handler = New( RESPONCEHANDLER );
	int sendport = 16660;

	while( !requestor && ( sendport < 17000 ) )
	 	requestor = ServeUDP( NULL, sendport++, UDPMessage, NULL );

	if( !requestor )
	{
		Log( "Failed to open service requestor port." );
		return 0;
	}
	if( netwide )
      UDPEnableBroadcast( requestor, TRUE );
	handler->port = port;
	handler->HandleResponce = Handler;
   handler->psvUser = psvUser;
   if( netwide )
		handler->saAsk = CreateRemote( BROADCASTADDR, port );
   else
		handler->saAsk = CreateRemote( LOCALADDR, port );
	handler->me = &responders;
	if( ( handler->next = responders ) )
		responders->me = &handler->next;
	responders = handler;
	handler->timer = AddTimerEx( 1000, 10000, (void (CPROC*)(uintptr_t))RequestTimer, (uintptr_t)handler );
        
   return 1;
}

int DiscoverService ( int netwide
													, int port
													, int (CPROC*Handler)( uintptr_t psv, SOCKADDR *responder )
													, uintptr_t psvUser )
{
	return DiscoverServiceEx( netwide, port, Handler, psvUser, 0 );
}

NETSERVICE_PROC( void, EndDiscoverService )( int port )
{
	PRESPONCEHANDLER handler;
	handler = responders;
	Log1( "Ending discover %d", port );
	while( handler )
	{
		if( handler->port == port )
		{
			if( ( (*handler->me) = handler->next ) )
				handler->next->me = handler->me;

			ReleaseAddress( handler->saAsk );
			RemoveTimer( handler->timer );
			Release( handler );
			return;
		}
		handler = handler->next;
	}
}

// $Revision: 1.2 $
// $Log: udprequest.c,v $
// Revision 1.2  2003/07/30 11:28:25  panther
// Minor name changes.  Enable broadcast on requestor
//
// Revision 1.1  2003/07/29 14:06:09  panther
// Added library for network service discovery/responce
//
// Revision 1.10  2003/07/25 15:57:44  jim
// Update to make watcom compile happy
//
// Revision 1.9  2002/07/25 15:41:32  panther
// Removed unused code.
// Added NetworkLock() and NetworkUnlock() code.  This should be the final
// instability issue gone.  Timers within this would use clients which were
// able to be deleted.
//
// Revision 1.8  2002/07/16 20:42:40  panther
// Added logging message when we failed to open the service requestor port.
//
// Revision 1.7  2002/07/15 14:42:22  panther
// *** empty log message ***
//
// Revision 1.6  2002/04/29 22:25:59  panther
// Removed some exessive logging for UDP discoveries...
//
// Revision 1.5  2002/04/23 17:13:20  panther
// *** empty log message ***
//
// Revision 1.4  2002/04/18 16:29:44  panther
// Removed useless messages from UDPReqeust...(comments)
// windows fiels - added loggig, remove logging, net no change
//
// Revision 1.3  2002/04/15 22:12:51  panther
// Updated Accounts.Data
// remove Author tag... log is sufficent.
//
// Revision 1.2  2002/04/15 16:25:03  panther
// Added Revision Tags...
//
