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


// create a websocket connection.
//  If web_socket_opened is passed as NULL, this function will wait until the negotiation has passed.
//  since these packets are collected at a lower layer, buffers passed to receive event are allocated for
//  the application, and the application does not need to setup an  initial read.
WebSocketClient WebSocketOpen( CTEXTSTR address
															 , int options
															 , web_socket_opened
									  , web_socket_event, web_socket_closed, web_socket_error, psv )
{
   WebSocketClient websock = New( struct web_socket_client );
   return  websock;
}

// end a websocket connection nicely.
void WebSocketClose( WebSocketClient websock )
{

}

// there is a control bit for whether the content is text or binary or a continuation
void WebSocketSendText( WebSocketClient websock, POINTER buffer, size_t length ) // UTF8 RFC3629
{

}

// literal binary sending; this may happen to be base64 encoded too
void WebSocketSendBinary( WebSocketClient websock, POINTER buffer, size_t length )
{
}

