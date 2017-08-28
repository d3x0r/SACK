#include <stdhdrs.h>
#include <idle.h>
#define SACK_WEBSOCKET_CLIENT_SOURCE
#include <html5.websocket.client.h>
#include "../html5.websocket.common.h"

#include "local.h"


static void SendRequestHeader( WebSocketClient websock )
{
	PVARTEXT pvtHeader = VarTextCreate();
	vtprintf( pvtHeader, WIDE("GET /%s%s%s%s%s HTTP/1.1\r\n")
			  , websock->url->resource_path?websock->url->resource_path:WIDE("")
			  , websock->url->resource_path?WIDE("/"):WIDE("")
			  , websock->url->resource_file
			  , websock->url->resource_extension?WIDE("."):WIDE("")
			  , websock->url->resource_extension?websock->url->resource_extension:WIDE("")

			  );
	vtprintf( pvtHeader, WIDE("Host: %s:%d\r\n")
			  , websock->url->host
			  , websock->url->port?websock->url->port:websock->url->default_port );
	vtprintf( pvtHeader, WIDE("Upgrade: websocket\r\n"));
	vtprintf( pvtHeader, WIDE("Connection: Upgrade\r\n"));
	if( websock->protocols )
		vtprintf( pvtHeader, WIDE("Sec-WebSocket-Protocol: %s\r\n"), websock->protocols );
	vtprintf( pvtHeader, WIDE("Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n") );
	vtprintf( pvtHeader, WIDE("Sec-WebSocket-Version: 13\r\n") );
	vtprintf( pvtHeader, WIDE("\r\n") );
	{
		PTEXT text = VarTextPeek( pvtHeader ); // just leave the buffer in-place
		if( websock->output_state.flags.use_ssl )
			ssl_Send( websock->pc, GetText( text ), GetTextSize( text ) );
		else
			SendTCP( websock->pc, GetText( text ), GetTextSize( text ) );
	}
	VarTextDestroy( &pvtHeader );
}


static void CPROC WebSocketTimer( uintptr_t psv )
{
	uint32_t now;
	INDEX idx;
	WebSocketClient websock;
	LIST_FORALL( wsc_local.clients, idx, WebSocketClient, websock )
	{
		now = timeGetTime();

		// close is delay notified
		if( websock->flags.want_close )
		{
			struct {
				uint16_t reason;
			} msg;
			msg.reason = 1000; // normal
			websock->input_state.flags.closed = 1;
			SendWebSocketMessage( websock->pc, 8, 1, 0, (uint8_t*)&msg, 2, websock->output_state.flags.use_ssl );
		}

		// do auto ping...
		if( !websock->input_state.flags.closed )
		{
			if( websock->ping_delay ) {
				if( !websock->input_state.flags.sent_ping )
				{
					if( ( now - websock->input_state.last_reception ) > websock->ping_delay )
					{
						SendWebSocketMessage( websock->pc, 0x09, 1, 0, NULL, 0, websock->output_state.flags.use_ssl );
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
}

static void CPROC WebSocketClientReceive( PCLIENT pc, POINTER buffer, size_t len )
{
	WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
		if( !websock )
		{
			if( wsc_local.opening_client )
			{
				//SetTCPNoDelay( pc, TRUE );

				//SetNetworkLong( pc, 0, (uintptr_t)wsc_local.opening_client );
				//SetNetworkLong( pc, 1, (uintptr_t)&wsc_local.opening_client->output_state );
				wsc_local.opening_client = NULL; // clear this to allow open to return.
			}
			else
			{
				lprintf( WIDE("Fatality; didn't have a related structure, and no client opening") );
			}
		}
		else
			buffer = websock->buffer;
		SendRequestHeader( websock );
	}
	else
	{
		WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
		if( !websock->flags.connected )
		{
			int result;
			// this is HTTP state...
			AddHttpData( websock->pHttpState, buffer, len );
			result = ProcessHttp( pc, websock->pHttpState );
			//lprintf( WIDE("reply is %d"), result );
			if( (int)result == 101 )
			{
				websock->flags.connected = 1;
				{
					PTEXT content = GetHttpContent( websock->pHttpState );
					if( websock->input_state.on_open )
						websock->input_state.on_open( pc, websock->input_state.psv_on );
					if( content )
						ProcessWebSockProtocol( &websock->input_state, websock->pc, (uint8_t*)GetText( content ), GetTextSize( content ) );
				}
			}
			else if( (int)result >= 300 && (int)result < 400 )
			{
				lprintf( WIDE("Redirection of some sort") );
				// redirect, disconnect, reconnect to new address offered.
			}
			else if( (int)result )
			{
				lprintf( WIDE("Some other error: %d"), result );
			}
			else
			{
				// not a full header yet. (something about no content-length?)
			}
		}
		else
		{
			ProcessWebSockProtocol( &websock->input_state, websock->pc, (uint8_t*)buffer, len );
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
	WebSocketClient websock;
	while( !( websock = (WebSocketClient)GetNetworkLong( pc, 0 ) ) )
		Relinquish();

	if( !error )
	{
		if( websock->output_state.flags.use_ssl )
			ssl_BeginClientSession( websock->pc, NULL, 0 );
	}
	else
	{
		wsc_local.opening_client = NULL;
		if( websock->input_state.on_close )
			websock->input_state.on_close( NULL, websock->input_state.psv_on );
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
							, uintptr_t psv
	, const char *protocols )
{
	WebSocketClient websock = New( struct web_socket_client );
	//va_arg args;
	//va_start( args, psv );
	if( !wsc_local.timer )
		wsc_local.timer = AddTimer( 2000, WebSocketTimer, 0 );

	websock->buffer = Allocate( 4096 );
	websock->pHttpState = CreateHttpState();
	websock->input_state.on_open = on_open;
	websock->input_state.on_event = on_event;
	websock->input_state.on_close = on_closed;
	websock->input_state.on_error = on_error;
	websock->input_state.psv_on = psv;
	websock->protocols = protocols;
	websock->output_state.flags.expect_masking = 1; // client to server is MUST mask because of proxy handling in that direction

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
		if( websock->pc )
		{
			SetNetworkLong( websock->pc, 0, (uintptr_t)websock );
			SetNetworkLong( websock->pc, 1, (uintptr_t)&websock->output_state );
#ifndef NO_SSL
			if( StrCaseCmp( websock->url->protocol, "wss" ) == 0 )
				websock->output_state.flags.use_ssl = 1;
#endif
			if( !on_open )
			{	
				while( !websock->flags.connected && !websock->input_state.flags.closed )
					Idle();
			}
		}
		else
			wsc_local.opening_client = NULL;	
	}
	LeaveCriticalSec( &wsc_local.cs_opening );
	return  websock->pc;
}

// end a websocket connection nicely.
void WebSocketClose( PCLIENT pc )
{
   RemoveClientEx( pc, 0, 1 );
}

void WebSocketEnableAutoPing( PCLIENT pc, uint32_t delay )
{
	WebSocketClient websock = (WebSocketClient)GetNetworkLong( pc, 0 );
	if( websock->Magic == 0x20130911 )
	{
		websock->ping_delay = delay;
	}
}


PRELOAD( InitWebSocketServer )
{
//   wsc_local.timer = AddTimer( 2000, WebSocketTimer, 0 );
	InitializeCriticalSec( &wsc_local.cs_opening );
}

