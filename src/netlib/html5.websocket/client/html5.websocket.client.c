#include <stdhdrs.h>
#include <idle.h>
#define SACK_WEBSOCKET_CLIENT_SOURCE
#include <html5.websocket.client.h>
#include "../html5.websocket.common.h"

#include "local.h"


static void SendRequestHeader( WebSocketClient websock )
{
	PVARTEXT pvtHeader = VarTextCreate();
	vtprintf( pvtHeader, "GET /%s/%s%s%s HTTP/1.1\r\n"
			  , websock->url->resource_path
			  , websock->url->resource_file
			  , websock->url->resource_extension?".":""
			  , websock->url->resource_extension?websock->url->resource_extension:""

			  );
	vtprintf( pvtHeader, "Host: %s:%d\r\n"
			  , websock->url->host
			  , websock->url->port?websock->url->port:websock->url->default_port );
	vtprintf( pvtHeader, "Upgrade: websocket\r\n");
	vtprintf( pvtHeader, "Connection: Upgrade\r\n");
	vtprintf( pvtHeader, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n" );
	vtprintf( pvtHeader, "Sec-WebSocket-Version: 13\r\n" );
	vtprintf( pvtHeader, "\r\n" );
	{
		PTEXT text = VarTextPeek( pvtHeader ); // just leave the buffer in-place
		SendTCP( websock->pc, GetText( text ), GetTextSize( text ) );
	}
   VarTextDestroy( &pvtHeader );
}


static void CPROC WebSocketTimer( PTRSZVAL psv )
{
   _32 now;
	INDEX idx;
   WebSocketClient websock;
	LIST_FORALL( wsc_local.clients, idx, WebSocketClient, websock )
	{
		now = timeGetTime();

      // close is delay notified
		if( websock->flags.want_close )
		{
			struct {
				_16 reason;
			} msg;
         msg.reason = 1000; // normal
			websock->input_state.flags.closed = 1;
         SendWebSocketMessage( websock->pc, 8, 1, 0, (P_8)&msg, 2 );
		}

      // do auto ping...
		if( !websock->input_state.flags.closed )
		{
			if( websock->ping_delay )
				if( !websock->input_state.flags.sent_ping )
				{
					if( ( now - websock->input_state.last_reception ) > websock->ping_delay )
					{
						SendWebSocketMessage( websock->pc, 0x09, 1, 0, NULL, 0 );
					}
				}
				else
				{
					if( ( now - websock->input_state.last_reception ) > ( websock->ping_delay * 2 ) )
					{
						websock->flags.want_close = 1;
                  // send close immediately
                  RescheduleTimerEx( wsc_local.timer, 0 );
					}
				}
		}
	}
}

static void CPROC WebSocketClientReceive( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
      SetTCPNoDelay( pc, TRUE );
		wsc_local.opening_client->buffer = Allocate( 4096 );
		SetNetworkLong( pc, 0, (PTRSZVAL)wsc_local.opening_client );
      SetNetworkLong( pc, 1, (PTRSZVAL)&wsc_local.opening_client->output_state );
      wsc_local.opening_client = NULL; // clear this to allow open to return.
	}
	else
	{
		WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
		if( !websock->flags.connected )
		{
         enum ProcessHttpResult result;
			// this is HTTP state...
			AddHttpData( websock->pHttpState, buffer, len );
			result = ProcessHttp( pc, websock->pHttpState );
			if( (int)result >= 200 && (int)result < 300 )
			{
				websock->flags.connected = 1;
				{
					PTEXT content = GetHttpContent( websock->pHttpState );
					if( websock->input_state.on_open )
                  websock->input_state.on_open( websock->input_state.psv_on );
					if( content )
                  ProcessWebSockProtocol( &websock->input_state, websock->pc, (P_8)GetText( content ), GetTextSize( content ) );
				}
			}
			else if( (int)result >= 300 && (int)result < 400 )
			{
            // redirect, disconnect, reconnect to new address offered.
			}
			else if( (int)result )
			{
				lprintf( "Some other error: %d", result );
			}
			else
			{
            // not a full header yet. (something about no content-length?)
			}
		}
		else
		{
			ProcessWebSockProtocol( &websock->input_state, websock->pc, (P_8)buffer, len );
		}
		// process buffer?

	}
   ReadTCP( pc, buffer, 4096 );
}

static void CPROC WebSocketClientClosed( PCLIENT pc )
{
	WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
   if( websock )
	{
		Release( websock->buffer );
		DestroyHttpState( websock->pHttpState );
      SACK_ReleaseURL( websock->url );
      Release( websock );
	}
}

static void CPROC WebSocketClientConnected( PCLIENT pc, int error )
{
	if( !error )
	{
      // connect succeeded.
		WebSocketClient websock;
		while( !( websock = (WebSocketClient)GetNetworkLong( pc, 0 ) ) )
			Relinquish();
      SendRequestHeader( websock );
	}
}


// create a websocket connection.
//  If web_socket_opened is passed as NULL, this function will wait until the negotiation has passed.
//  since these packets are collected at a lower layer, buffers passed to receive event are allocated for
//  the application, and the application does not need to setup an  initial read.
PCLIENT WebSocketOpen( CTEXTSTR url_address
							, int options
							, web_socket_opened on_open
							, web_socket_event on_event
							, web_socket_closed on_closed
							, web_socket_error on_error
							, PTRSZVAL psv )
{
	WebSocketClient websock = New( struct web_socket_client );
	MemSet( websock, 0, sizeof( struct web_socket_client ) );
	websock->input_state.on_open = on_open;
	websock->input_state.on_event = on_event;
	websock->input_state.on_close = on_closed;
	websock->input_state.on_error = on_error;
	websock->input_state.psv_on = psv;

	websock->url = SACK_URLParse( url_address );

	EnterCriticalSec( &wsc_local.cs_opening );
	wsc_local.opening_client = websock;
	{
		websock->pc = OpenTCPClientExx( websock->url->host
												, websock->url->port?websock->url->port:websock->url->default_port
												, WebSocketClientReceive
												, WebSocketClientClosed
												, NULL
												, on_open?WebSocketClientConnected:NULL // if there is an on-open event, then register for async open
												);
		if( websock->pc && !on_open )
		{
			// send request if we got connected, if there is a on_open callback, then we're delay waiting
			// so this will be sent in the socket on-open event.
			SendRequestHeader( websock );
			while( !websock->flags.connected && !websock->input_state.flags.closed )
				Idle();
		}
		while( wsc_local.opening_client )
			Idle();
	}
	LeaveCriticalSec( &wsc_local.cs_opening );
	return  websock->pc;
}

// end a websocket connection nicely.
void WebSocketClose( PCLIENT pc )
{
   RemoveClient( pc );
}

void WebSocketEnableAutoPing( PCLIENT pc, _32 delay )
{
	WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
	if( websock->Magic == 0x20130911 )
	{
		websock->ping_delay = delay;
	}
}


PRELOAD( InitWebSocketServer )
{
   wsc_local.timer = AddTimer( 2000, WebSocketTimer, 0 );
}

