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

static void MaskMessageOut( P_8 bufout, size_t length )
{
	size_t n;
	wsc_local.newmask ^= rand() ^ ( rand() << 13 ) ^ ( rand() << 26 );
	for( n = 0; n < length; n++ )
	{
		bufout[n] = bufout[n] ^ ((P_8)&wsc_local.newmask)[n&3];
	}
}

static void ResetInputState( WebSocketClient websock )
{
	websock->input_msg_state = 0;
	websock->final = 0;
	websock->mask = 0;
	websock->fragment_collection_index = 0; // mask index counter
	websock->input_type = 0; // assume text input; binary will set flag opposite

	// just always process the mask key, so set it to 0 for a no-op on XOR
	websock->mask_key[0] = 0;
	websock->mask_key[1] = 0;
	websock->mask_key[2] = 0;
	websock->mask_key[3] = 0;
}

// literal binary sending; this may happen to be base64 encoded too
static void SendWebSocketMessage( WebSocketClient websock, int opcode, int final, int no_mask, P_8 payload, size_t length )
{
	P_8 msgout;
	P_8 use_mask;
   _32 zero = 0;
   size_t length_out = length + 2; // minimum additional is the opcode and tiny payload length (2 bytes)
	if( ( opcode & 8 ) && ( length > 125 ) )
	{
      lprintf( "Invalid send, control packet with large payload. (opcode %d  length %d)", opcode, length );
		return;
	}

	if( length > 125 )
	{
      if( length > 32767 )
			length_out += 8; // need 8 more bytes for a really long length
		else
         length_out += 2; // need 2 more bytes for a longer length
	}
	if( !no_mask )
	{
      wsc_local.newmask ^= rand() ^ ( rand() << 13 ) ^ ( rand() << 26 );
		use_mask = (P_8)&wsc_local.newmask;
      length_out += 4; // need 4 more bytes for the mask
	}
	else
	{
		use_mask = (P_8)&zero;
	}

	msgout = NewArray( _8, length_out );
	msgout[0] = (final?0x80:0x00) | opcode;
	if( length > 125 )
	{
		if( length > 32767 )
		{
			msgout[1] = 127;
#if __64__
			msgout[2] = (_8)(length >> 56);
			msgout[3] = (_8)(length >> 48);
			msgout[4] = (_8)(length >> 40);
			msgout[5] = (_8)(length >> 32);
#else
			msgout[2] = 0;
			msgout[3] = 0;
			msgout[4] = 0;
			msgout[5] = 0;
#endif
			msgout[6] = (_8)(length >> 24);
			msgout[7] = (_8)(length >> 16);
			msgout[8] = (_8)(length >> 8);
			msgout[9] = (_8)length;
		}
		else
		{
			msgout[1] = 126;
			msgout[2] = (_8)(length >> 8);
         msgout[3] = (_8)(length);
		}
	}
	else
		msgout[1] = length;
	if( !no_mask )
	{
      int mask_offset = (length_out-length) - 4;
      msgout[mask_offset+0] = (_8)(wsc_local.newmask >> 24);
      msgout[mask_offset+1] = (_8)(wsc_local.newmask >> 16);
      msgout[mask_offset+2] = (_8)(wsc_local.newmask >> 8);
      msgout[mask_offset+3] = (_8)(wsc_local.newmask);
	}
	{
		size_t n;
      P_8 data_out = msgout + (length_out-length);
		for( n = 0; n < length; n++ )
		{
         (*data_out++) = payload[n] ^ use_mask[n&3];
		}
	}
	SendTCP( websock->pc, msgout, length_out );
   Deallocate( P_8, msgout );
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
static void ProcessWebSockProtocol( WebSocketClient websock, P_8 msg, size_t length )
{
	size_t n;

	for( n = 0; n < length; n++ )
	{
		switch( websock->input_msg_state )
		{
		case 0: // opcode/final
			if( msg[n] & 0x80 )
				websock->final = 1;
			websock->opcode = ( msg[n] & 0xF );
			websock->input_msg_state++;
			break;
		case 1: // mask bit, and 7 bits of frame_length(payload)
			websock->mask = (msg[n] & 0x80) != 0;
			websock->frame_length = (msg[n] & 0x7f );

			// control opcodes are limited to the one byte size limit; they can never be encoded with extended payload length
			if( websock->opcode & 0x8 )
			{
				if( websock->frame_length > 125 )
				{
					lprintf( WIDE("Bad length of control packet: %d"), length );
					// RemoveClient( websock->pc );
					ResetInputState( websock );
					// drop the rest of the data, maybe the beginning of the next packet will make us happy
					return;
				}
			}

			if( websock->frame_length == 126 )
			{
				websock->frame_length = 0;
				websock->input_msg_state = 2;
			}
			else if( websock->frame_length == 127 )
			{
				websock->frame_length = 0;
				websock->input_msg_state = 4;
			}
			else
			{
				if( websock->mask )
					websock->input_msg_state = 12;
				else
					websock->input_msg_state = 16;
			}
			break;

		case 2: // byte 1, extended payload _16
			websock->frame_length = msg[n] << 8;
			websock->input_msg_state++;
			break;
		case 3: // byte 1, extended payload _16
			websock->frame_length |= msg[3];

			if( websock->mask )
				websock->input_msg_state = 12;
			else
				websock->input_msg_state = 16;
			break;

		case 4: // byte 1, extended payload _64
		case 5: // byte 2, extended payload _64
		case 6: // byte 3, extended payload _64
		case 7: // byte 4, extended payload _64
		case 8: // byte 5, extended payload _64
		case 9: // byte 6, extended payload _64
		case 10: // byte 7, extended payload _64
			websock->frame_length |= msg[n] << ( ( 11 - websock->input_msg_state ) * 8 );
			websock->input_msg_state++;
			break;
		case 11: // byte 8, extended payload _64
			websock->frame_length |= msg[n];

			if( websock->mask )
				websock->input_msg_state++;
			else
				websock->input_msg_state = 16;
			break;

		case 12: // mask data byte 1
		case 13: // mask data byte 2
		case 14: // mask data byte 3
		case 15: // mask data byte 4
			websock->mask_key[websock->input_msg_state-12] = msg[n];
			websock->input_msg_state++;
			break;

		case 16: // extended data or application data byte 1.
			// might have already collected fragments (non final packets, so increase the full buffer )
			// first byte of data, check we have enough room for the remaining bytes; the frame_length is valid now.
			if( websock->fragment_collection_avail < ( websock->fragment_collection_length + websock->frame_length ) )
			{
				P_8 new_fragbuf;
				websock->fragment_collection_avail += websock->frame_length;
				new_fragbuf = (P_8)Allocate( websock->fragment_collection_avail );
				if( websock->fragment_collection_length )
					MemCpy( new_fragbuf, websock->fragment_collection, websock->fragment_collection_length );
				Deallocate( P_8, websock->fragment_collection );
				websock->fragment_collection = new_fragbuf;
            websock->fragment_collection_index = 0; // start with mask byte 0 on this new packet
			}
			websock->input_msg_state++;
			// fall through, no break statement; add the byte to the buffer
		case 17:
			websock->fragment_collection[websock->fragment_collection_length++]
				= msg[n] ^ websock->mask_key[(websock->fragment_collection_index++) % 4];

			// if final packet, and we have all the bytes for this packet
			// dispatch the opcode.
			if( websock->final && ( websock->fragment_collection_length == websock->frame_length ) )
			{

				lprintf( WIDE("Final: %d  opcode %d  mask %d length %Ld ")
						 , websock->final, websock->opcode, websock->mask, websock->frame_length );
				websock->last_reception = timeGetTime();

				switch( websock->opcode )
				{
				case 0x02: //binary
					websock->input_type = 1;
				case 0x01: //text
				case 0x00: // continuation
					if( websock->final )
					{
						/// single packet, final...
						LogBinary( websock->fragment_collection, websock->fragment_collection_length );
						if( websock->on_event )
							websock->on_event( websock->psv_on, websock->fragment_collection, websock->fragment_collection_length );
						websock->fragment_collection_length = 0;
					}
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
					if( !websock->flags.closed )
					{
						SendWebSocketMessage( websock, 8, 1, 0, websock->fragment_collection, websock->frame_length );
                  websock->flags.closed = 1;
					}
					if( websock->on_close )
                  websock->on_close( websock->psv_on );
					websock->fragment_collection_length = 0;
					break;
				case 0x09: // ping
					SendWebSocketMessage( websock, 0x0a, 1, 0, websock->fragment_collection, websock->frame_length );
					websock->fragment_collection_length = 0;
					break;
				case 0x0A: // pong
					{
                  // this is for the ping routine to wait (or rather to end wait)
                  websock->flags.received_pong = 1;
					}
					websock->fragment_collection_length = 0;
					break;
				default:
					lprintf( WIDE("Bad WebSocket opcode: %d"), websock->opcode );
					return;
				}

				ResetInputState( websock );
			}
			break;
		}
	}
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
			websock->flags.closed = 1;
         SendWebSocketMessage( websock, 8, 1, 0, (P_8)&msg, 2 );
		}

      // do auto ping...
		if( !websock->flags.closed )
		{
			if( websock->ping_delay )
				if( !websock->flags.sent_ping )
				{
					if( ( now - websock->last_reception ) > websock->ping_delay )
					{
						SendWebSocketMessage( websock, 0x09, 1, 0, NULL, 0 );
					}
				}
				else
				{
					if( ( now - websock->last_reception ) > ( websock->ping_delay * 2 ) )
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
                  ProcessWebSockProtocol( websock, (P_8)GetText( content ), GetTextSize( content ) );
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
			ProcessWebSockProtocol( websock, (P_8)buffer, len );
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

void WebSocketPing( WebSocketClient websock, _32 timeout )
{
	_32 start_at = timeGetTime();
	_32 target = start_at + timeout;
   _32 now;
	SendWebSocketMessage( websock, 9, 1, 0, NULL, 0 );

	while( !websock->flags.received_pong
			&& ( ( ( now=timeGetTime() ) - start_at ) < timeout ) )
		IdleFor( target-now );
   websock->flags.received_pong = 0;
}

void WebSocketEnableAutoPing( WebSocketClient websock, _32 delay )
{
	if( websock )
	{
      websock->ping_delay = delay;
	}
}

// there is a control bit for whether the content is text or binary or a continuation
void WebSocketSendText( WebSocketClient websock, POINTER buffer, size_t length ) // UTF8 RFC3629
{
	SendWebSocketMessage( websock, websock->flags.sent_type?0:1, 1, 0, (P_8)buffer, length );
   websock->flags.sent_type = 0;
}

// there is a control bit for whether the content is text or binary or a continuation
void WebSocketBeginSendText( WebSocketClient websock, POINTER buffer, size_t length ) // UTF8 RFC3629
{
   SendWebSocketMessage( websock, 1, 0, 0, (P_8)buffer, length );
   websock->flags.sent_type = 1;

}

// literal binary sending; this may happen to be base64 encoded too
void WebSocketSendBinary( WebSocketClient websock, POINTER buffer, size_t length )
{
	SendWebSocketMessage( websock, websock->flags.sent_type?0:2, 1, 0, (P_8)buffer, length );
}

// literal binary sending; this may happen to be base64 encoded too
void WebSocketBeginSendBinary( WebSocketClient websock, POINTER buffer, size_t length )
{
   SendWebSocketMessage( websock, 2, 0, 0, (P_8)buffer, length );
   websock->flags.sent_type = 1;
}



PRELOAD( InitWebSocketServer )
{
   wsc_local.timer = AddTimer( 2000, WebSocketTimer, 0 );
}

