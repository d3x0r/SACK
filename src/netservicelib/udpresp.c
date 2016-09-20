#ifndef NETSERVICE_SOURCE
#define NETSERVICE_SOURCE
#endif

#include <stdhdrs.h>
#include <network.h>
#include <sharemem.h>
#define DO_LOGGING
#include <logging.h>
#include <netservice.h>

struct responder_info {
	PCLIENT responder;
   int port;
};

static struct {
   PLIST responders;
} l;

void CPROC UDPMessage( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *sa )
{
   if( !buffer )
   {
      buffer = Allocate( 16 );
   }
   else
   {
   	//Log( WIDE("UDP Message received!") );
      if( *(uint64_t*)buffer == *(uint64_t*)"WHERE RU" )
      {
      	Log( WIDE("Informing I'm here...") );
         SendUDPEx( pc, (void*)WIDE("IM HERE!"), 8, sa );
      }
   }
   ReadUDP( pc, buffer, 16 );
}


/* shared with udpdiscover... */
PCLIENT NegotiateResponderClient( int port, PCLIENT pc )
{
		INDEX idx;
		struct responder_info *resp;
		LIST_FORALL( l.responders, idx, struct responder_info *, resp )
		{
			if( resp->port == port )
				break;
		}
		if( !resp )
		{
			resp = New( struct responder_info );
			resp->port = port;
			AddLink( &l.responders, resp );
			if( pc )
			{
				resp->responder = pc;
				return NULL;
			}
			AddLink( &l.responders, resp );
		}
		else if( !pc )
		{
			return resp->responder;
		}
		else
		{
			lprintf( "probably both PCLIENT pc(%p) passed and client %p internal pc=%p", pc, resp, resp?resp->responder:NULL );
		}
	return NULL;
}

NETSERVICE_PROC( int, ServiceRespond )( int port )
{
	if( !NetworkStart() )
		return 0;
	{
		INDEX idx;
		struct responder_info *resp;
		LIST_FORALL( l.responders, idx, struct responder_info *, resp )
		{
			if( resp->port == port )
				break;
		}
		if( !resp )
		{
			resp = New( struct responder_info );
			if( !(resp->responder = ServeUDP( NULL, port, UDPMessage, NULL )) )
			{
				Release( resp );
				return 0;
			}
			resp->port = port;
			AddLink( &l.responders, resp );
		}
	}
	return 1;	
}

NETSERVICE_PROC( void, EndServiceRespondEx )( int port )
{
	{
		INDEX idx;
		struct responder_info *resp;
		LIST_FORALL( l.responders, idx, struct responder_info *, resp )
		{
			if( !port || resp->port == port )
			{
				RemoveClient( resp->responder );
				DeleteLink( &l.responders, resp );
				Release( resp );
            if( port )
					break;
			}
		}
		if( !resp )
		{
         lprintf( "Failed to find the desired port to close..." );
		}
	}
}

NETSERVICE_PROC( void, EndServiceRespond )( void )
{
   EndServiceRespondEx( 0 );
}



