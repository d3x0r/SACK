#include <stdhdrs.h>
#include <idle.h>
#define SACK_WEBSOCKET_CLIENT_SOURCE
#include <html5.websocket.client.h>


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


static void SendClose( WebSocketClient websock )
{

}

static void AddFragment( WebSocketClient websock, POINTER fragment, size_t frag_length )
{
	P_8 new_fragbuf;
	websock->fragment_collection_length += frag_length;
	new_fragbuf = (P_8)Allocate( websock->fragment_collection_length + frag_length );
	if( websock->fragment_collection_length )
		MemCpy( new_fragbuf, websock->fragment_collection, websock->fragment_collection_length );
	MemCpy( new_fragbuf + websock->fragment_collection_length, fragment, frag_length );
	Deallocate( P_8, websock->fragment_collection );
	websock->fragment_collection = new_fragbuf;
}

/* opcodes
      *  %x0 denotes a continuation frame
      *  %x1 denotes a text frame
      *  %x2 denotes a binary frame
      *  %x3-7 are reserved for further non-control frames
      *  %x8 denotes a connection close
      *  %x9 denotes a ping
      *  %xA denotes a pong
      *  %xB-F are reserved for further control frames
*/
static void ProcessWebSockProtocol( WebSocketClient websock, POINTER buffer, size_t length )
{
	LOGICAL final = 0;
	LOGICAL mask = 0;
	_8 mask_key[4];
	int opcode;
	size_t frame_length;
	int offset = 0;
	P_8 msg = (P_8)buffer;

	if( msg[0] & 0x80 )
		final = 1;

	opcode = ( msg[0] & 0xF );
	mask = (msg[1] & 0x80) != 0;
	frame_length = (msg[1] & 0x7f );
	if( frame_length == 126 )
	{
		offset = 2;
		frame_length = ( msg[2] << 8 ) | msg[3];
	}
	lprintf( WIDE("Final: %d  opcode %d  mask %d length %Ld "), final, opcode, mask, frame_length );

	if( frame_length == 127 )
	{
		frame_length = ( (_64)msg[2] << 56 ) 
			| ( (_64)msg[3] << 48 ) 
			| ( (_64)msg[4] << 32 ) 
			| ( (_64)msg[5] << 24 ) 
			| ( (_64)msg[6] << 16 ) 
			| ( (_64)msg[7] << 8 ) 
			| msg[8];
		offset = 8;
	}
	if( mask )
	{
		mask_key[0] = msg[offset + 2];
		mask_key[1] = msg[offset + 3];
		mask_key[2] = msg[offset + 4];
		mask_key[3] = msg[offset + 5];
		offset += 4;
	}
	{
		size_t n;
		for( n = 0; n < frame_length; n++ )
		{
			msg[offset + 2+n] = msg[offset+2+n] ^ mask_key[n&3];
		}
	}

   // control opcodes are limited to the one byte size limit
	if( opcode & 0x8 )
	{
		if( length > 125 )
		{
			lprintf( WIDE("Bad length of control packet: %d"), length );
			return;
		}

	}
	switch( opcode )
	{
	case 0x00: // continuation
		AddFragment( websock, msg + offset+2, frame_length );
		if( final )
		{
			LogBinary( websock->fragment_collection, websock->fragment_collection_length );
		}
		break;
	case 0x01: //text
      if( !final )
			AddFragment( websock, msg + offset+2, frame_length );
		else if( websock->fragment_collection_length )
		{
			AddFragment( websock, msg + offset+2, frame_length );
         LogBinary( websock->fragment_collection, websock->fragment_collection_length );
		}
		else
         LogBinary( msg + offset+2, frame_length );
		break;
	case 0x02: //binary
		if( !final )
			AddFragment( websock, msg + offset+2, frame_length );
		else if( websock->fragment_collection_length )
		{
			AddFragment( websock, msg + offset+2, frame_length );
         LogBinary( websock->fragment_collection, websock->fragment_collection_length );
		}
		else
			LogBinary( msg + offset+2, frame_length );

		break;
	case 0x08: // close
		// close may have app data with a reason.
		// if it has a reason, then the first two bytes are a code
		//  1000 - normal
		//  1001 - end point going away (page close, server shutdown)
		//  1002 - termination from protocol error
		//  1003 - binary/text mismatch (if supported)
		// 1004 - reserved
		// 1005 - No status code (reserved, must not send in close)
		// 1006 - reserved for not in clude; connection terminated unexpected ( guess there are local-only, not sent)
		// 1007 - inconsistant data for data in message
		// 1008 - ereceived a message that violates policy (generic message for nothing better)
		// 1009 - message too big
		// 1010 - server did not negotiate extension
		// 1011 - excpetion in handling message.
		// 1015 - reservd, must not send in close; failure to perform TLS handshake (or bad server verification)
		//  0-999 = not used;
		// 1000-2999 - reserved for this protocol, reserved for specification
		// 3000-3999 - libarry/framework/application.  Registered with IANA.  Defined by protocol.
      // 4000-4999 - reserved for private use; cannot be registerd;
		break;
	case 0x09: // ping
		break;
	case 0x0A: // pong

		break;
	default:
		lprintf( WIDE("Bad WebSocket opcode: %d"), opcode );
      return;
	}
}


static void CPROC WebSocketTimer( PTRSZVAL psv )
{
	INDEX idx;
   WebSocketClient websock;
	LIST_FORALL( wsc_local.clients, idx, WebSocketClient, websock )
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
      SetTCPNoDelay( pc, TRUE );
		wsc_local.opening_client->buffer = Allocate( 4096 );
		SetNetworkLong( pc, 0, (PTRSZVAL)wsc_local.opening_client );
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
					if( websock->on_open )
                  websock->on_open( websock->psv_on );
					if( content )
                  ProcessWebSockProtocol( websock, GetText( content ), GetTextSize( content ) );
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
			ProcessWebSockProtocol( websock, buffer, len );
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
   websock->on_close = on_closed;
	websock->on_error = on_error;
	websock->psv_on = psv;

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
			while( !websock->flags.connected && !websock->flags.closed )
            Idle();
		}
		while( wsc_local.opening_client )
         Idle();
	}
   LeaveCriticalSec( &wsc_local.cs_opening );
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

