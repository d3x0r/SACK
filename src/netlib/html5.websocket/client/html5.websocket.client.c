#include <stdhdrs.h>

#include <html5.websocket.client.h>


#include "local.h"

static void SendClose( WebSocketClient websock )
{

}

static void CPROC WebSocketTimer( PTRSZVAL psv )
{
	INDEX idx;
   WebSocketClient websock;
	LIST_FORALL( l.clients, idx, WebSocketClient, websock )
	{
		if( websock->flags.want_close )
		{
         SendClose( websock );
		}
	}
}

static void CPROC WebSocketClientReceive( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	else
	{
      // process buffer?
	}
   ReadTCP( pc, buffer, 4096 );
}

static void CPROC WebSocketClientClosed( PCLIENT pc )
{
}

static void CPROC WebSocketClientConnected( PCLIENT pc, int error )
{
	if( !error )
	{
      // connect succeeded.
		WebSocketClient websock;
		while( !( websock = GetNetworkLong( pc, 0 ) ) )
			Relinquish();
		websock->on_open( websock->psv_on );
	}
}


// create a websocket connection.
//  If web_socket_opened is passed as NULL, this function will wait until the negotiation has passed.
//  since these packets are collected at a lower layer, buffers passed to receive event are allocated for
//  the application, and the application does not need to setup an  initial read.
WebSocketClient WebSocketOpen( CTEXTSTR url_address
									  , int options
									  , web_socket_opened on_open
									  , web_socket_event on_event
									  , web_socket_closed on_closed
									  , web_socket_error on_error
									  , PTRSZVAL psv )
{
	WebSocketClient websock = New( struct web_socket_client );
   MemSet( websock, 0, sizeof( struct web_socket_client ) );
   websock->on_open = on_open;
   websock->on_event = on_event;
   websock->on_close = on_close;
	websock->on_error = on_error;
	websock->psv_on = psv;

	websock->url = SACK_URLParse( url_address );

	{
      int len;
		TEXTSTR namebuf = NewArray( TEXTCHAR
										  , ( len = StrLen( websock->host )
											  + websock->port?StrLen( websock->port ):0 + 2 ) );
		snprintf( namebuf, len, "%s%s%s"
				  , websock->host
				  , websock->port?":":""
				  , websock->port?websock->port:"" );
		websock->pc = OpenTCPClientExx( namebuf, 80
												, WebSocketClientReceive
												, WebSocketClientClose
												, NULL
												, on_open?WebSocketClientConnected:NULL // if there is an on-open event, then register for async open
												);
      SetNetworkLong( websock->pc, 0, (PTRSZVAL)websock );
	}
   return  websock;
}

// end a websocket connection nicely.
void WebSocketClose( WebSocketClient websock )
{
	websock->flags.want_close = 1;
	// wake up the timer processing socket closes... so we can close and result
	// immediatly; also Open is usually an asynch operation; although it's
   // mostly network event driven instead.
   RescheduleTimerEx( wsc_local.timer, 0 );
}

// there is a control bit for whether the content is text or binary or a continuation
void WebSocketSendText( WebSocketClient websock, POINTER buffer, size_t length ) // UTF8 RFC3629
{

}

// literal binary sending; this may happen to be base64 encoded too
void WebSocketSendBinary( WebSocketClient websock, POINTER buffer, size_t length )
{
}

PRELOAD( InitWebSocketServer )
{
   wsc_local.timer = AddTimer( 2000, WebSocketTimer, 0 );
}

