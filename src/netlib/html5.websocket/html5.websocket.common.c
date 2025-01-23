#include <stdhdrs.h>
#include <network.h>
#include <idle.h>
#define SACK_WEBSOCKET_CLIENT_SOURCE
#define WEBSOCKET_COMMON_SOURCE
#include "html5.websocket.common.h"

#ifdef __ANDROID_OLD_PLATFORM_SUPPORT__
#define rand lrand48
#endif

int WebSocketSendTCP( uintptr_t psv, CPOINTER buffer, size_t length ){
	return SendTCP( (PCLIENT)psv, buffer, length );
}

int WebSocketSendSSL( uintptr_t psv, CPOINTER buffer, size_t length ) {
	return ssl_Send( (PCLIENT)psv, buffer, length );
}

void WebSocketSetAggregration( PCLIENT pc, LOGICAL enable ) {
	SetTCPWriteAggregation( pc, enable );
}

static void _SendWebSocketMessage( int (*sender)( uintptr_t target, CPOINTER buffer, size_t length )
		, uintptr_t target
		, int opcode, int final, int do_mask, const uint8_t* payload, size_t length )
{
	uint8_t* msgout;
	uint8_t* use_mask;
	uint32_t zero = 0;
	size_t length_out = length + 2; // minimum additional is the opcode and tiny payload length (2 bytes)
	if( ( opcode & 8 ) && ( length > 125 ) )
	{
		lprintf( "Invalid send, control packet with large payload. (opcode %d  length %" _size_f ")", opcode, length );
		return;
	}
	if( length > 125 )
	{
		if( length > 32767 )
		{
			length_out += 8; // need 8 more bytes for a really long length
		}
		else
			length_out += 2; // need 2 more bytes for a longer length
	}
	if( do_mask )
	{
		static uint32_t newmask;
		newmask ^= rand() ^ ( rand() << 13 ) ^ ( rand() << 26 );
		use_mask = (uint8_t*)&newmask;
		length_out += 4; // need 4 more bytes for the mask
	}
	else
	{
		use_mask = (uint8_t*)&zero;
	}

	msgout = NewArray( uint8_t, length_out );
	msgout[0] = opcode;
	if( length > 125 )
	{
		if( length > 32767 )
		{
			msgout[1] = 127;
#if __64__
			// size_t will be 32 bits on non 64 bit builds
			msgout[2] = (uint8_t)(length >> 56);
			msgout[3] = (uint8_t)(length >> 48);
			msgout[4] = (uint8_t)(length >> 40);
			msgout[5] = (uint8_t)(length >> 32);
#else
			msgout[2] = 0;
			msgout[3] = 0;
			msgout[4] = 0;
			msgout[5] = 0;
#endif
			msgout[6] = (uint8_t)(length >> 24);
			msgout[7] = (uint8_t)(length >> 16);
			msgout[8] = (uint8_t)(length >> 8);
			msgout[9] = (uint8_t)length;
		}
		else
		{
			msgout[1] = 126;
			msgout[2] = (uint8_t)(length >> 8);
			msgout[3] = (uint8_t)(length);
		}
	}
	else
		msgout[1] = (uint8_t)length;

	if( do_mask && length )
	{
		int mask_offset = (int)(length_out-length) - 4;
		msgout[1] |= 0x80;
		msgout[mask_offset+0] = (uint8_t)(use_mask[3]);
		msgout[mask_offset+1] = (uint8_t)(use_mask[2]);
		msgout[mask_offset+2] = (uint8_t)(use_mask[1]);
		msgout[mask_offset+3] = (uint8_t)(use_mask[0]);
		use_mask = msgout + mask_offset;
	}
	{
		size_t n;
		uint8_t* data_out = msgout + (length_out-length);
		size_t mlen = length / 4;
		for( n = 0; n < mlen; n++ )
		{
			(*data_out++) = (*payload++) ^ use_mask[0];
			(*data_out++) = (*payload++) ^ use_mask[1];
			(*data_out++) = (*payload++) ^ use_mask[2];
			(*data_out++) = (*payload++) ^ use_mask[3];
		}
		n <<= 2;
		if( n < length ) {
			(*data_out++) = (*payload++) ^ use_mask[0];
			n++;
		}
		if( n < length ) {
			(*data_out++) = (*payload++) ^ use_mask[1];
			n++;
		}
		if( n < length ) {
			(*data_out++) = (*payload++) ^ use_mask[2];
			n++;
		}
	}
	sender( target, msgout, length_out );
	Deallocate( uint8_t*, msgout );
}

void SendWebSocketMessage( struct web_socket_input_state *input
			, int opcode, int final, int do_mask, const uint8_t* payload, size_t length ) {
	//struct web_socket_input_state *input = (struct web_socket_input_state *)GetNetworkLong( pc, 1 );

#ifndef __NO_WEBSOCK_COMPRESSION__
	if( (!input->flags.do_not_deflate) && input->flags.deflate && opcode < 3 ) {
		int r;
		if( opcode ) opcode |= 0x40;
		input->deflater.next_in = (Bytef*)payload;
		input->deflater.avail_in = (uInt)length;
		input->deflater.next_out = (Bytef*)input->deflateBuf;
		input->deflater.avail_out = (uInt)input->deflateBufLen;
		do {
			r = deflate( &input->deflater, Z_FINISH );
			if( r == Z_STREAM_END )
				break;
			if( r == Z_BUF_ERROR )	 {
				lprintf( "Zlib error: buffer error" );
				//Z_BUF_ERROR( )
			}
			else if( r ) {
				lprintf( "Unhandled ZLIB error: %d", r );
			}
			if( input->deflater.avail_out == 0 ) {
				input->deflateBuf = Reallocate( input->deflateBuf, input->deflateBufLen + 4096 );
				input->deflater.next_out = (Bytef*)(((uintptr_t)input->deflateBuf) + input->deflateBufLen);
				input->deflateBufLen += 4096;
				input->deflater.avail_out += 4096;
				continue;
			}
			else
				break;
		} while( 1 );
		opcode = (final ? 0x80 : 0x00) | opcode;
		_SendWebSocketMessage( input->on_send, input->psvSender, opcode, final, do_mask, (uint8_t*)input->deflateBuf, input->deflater.total_out );
		deflateReset( &input->deflater );
	}
	else 
#endif
	{
		opcode = (final ? 0x80 : 0x00) | opcode;
		_SendWebSocketMessage( input->on_send, input->psvSender, opcode, final, do_mask, payload, length );
	}
}


static void ResetInputState( WebSocketInputState websock )
{
	//lprintf( "Reset input state?" );
	websock->input_msg_state = 0;
	websock->final = 0;
	websock->mask = 0;
	websock->fragment_collection_avail = 0; 
	websock->fragment_collection_index = 0; // mask index counter
	websock->input_type = 0; // assume text input; binary will set flag opposite

	// just always process the mask key, so set it to 0 for a no-op on XOR
	websock->mask_key[0] = 0;
	websock->mask_key[1] = 0;
	websock->mask_key[2] = 0;
	websock->mask_key[3] = 0;
}


#ifndef __NO_WEBSOCK_COMPRESSION__
//typedef unsigned( *in_func ) OF( (void FAR *,
//		z_const unsigned char FAR * FAR *) );
//typedef int( *out_func ) OF( (void FAR *, unsigned char FAR *, unsigned) );
static unsigned CPROC inflateBackInput( void* state, unsigned char **output ) {
	WebSocketInputState websock = (WebSocketInputState)state;
	(*output) = (unsigned char *)websock->fragment_collection;
	return (unsigned)websock->fragment_collection_avail;
}

static int CPROC inflateBackOutput( void* state, unsigned char *output, unsigned outlen ) {
	WebSocketInputState websock = (WebSocketInputState)state;
	memcpy( websock->inflateBuf, output, outlen );
	websock->inflateBufUsed += outlen;
	return Z_OK;
}
#endif

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
void ProcessWebSockProtocol( WebSocketInputState websock, const uint8_t* msg, size_t length )
{
	size_t n;
	//int oldstate = websock->input_msg_state;
	for( n = 0; n < length; n++ )
	{
		//if( oldstate != websock->input_msg_state ) {
		//	lprintf( "Process state: %d %d %d %d", websock->input_msg_state, n, length, websock->frame_length );
		//	oldstate = websock->input_msg_state;
		//}
		switch( websock->input_msg_state )
		{
		case 0: // opcode/final
			if( msg[n] & 0x80 )
				websock->final = 1;
			websock->opcode = ( msg[n] & 0xF );
			websock->_RSV1 = (msg[n] & 0x40);
			if( websock->opcode == 1 ) websock->input_type = 0;
			else if( websock->opcode == 2 ) websock->input_type = 1;
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
					lprintf( "Bad length of control packet: %" _size_f, length );
					// RemoveClient( websock->pc );
					ResetInputState( websock );
					// drop the rest of the data, maybe the beginning of the next packet will make us happy
					return;
				}
			}
			if( !websock->frame_length ) {
				websock->input_msg_state = 17;
				// re-process this byte but in the new state.
				n--;
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

		case 2: // byte 1, extended payload uint16_t
			websock->frame_length = msg[n] << 8;
			websock->input_msg_state++;
			break;
		case 3: // byte 1, extended payload uint16_t
			websock->frame_length |= msg[n];

			if( websock->mask )
				websock->input_msg_state = 12;
			else
				websock->input_msg_state = 16;
			break;

		case 4: // byte 1, extended payload uint64_t
		case 5: // byte 2, extended payload uint64_t
		case 6: // byte 3, extended payload uint64_t
		case 7: // byte 4, extended payload uint64_t
		case 8: // byte 5, extended payload uint64_t
		case 9: // byte 6, extended payload uint64_t
		case 10: // byte 7, extended payload uint64_t
			websock->frame_length |= msg[n] << ( ( 11 - websock->input_msg_state ) * 8 );
			websock->input_msg_state++;
			break;
		case 11: // byte 8, extended payload uint64_t
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
			//lprintf( "Received... and need %d  %d  %d", websock->fragment_collection_avail, websock->fragment_collection_length, websock->frame_length, websock->fragment_collection_length + websock->frame_length );
			if( websock->fragment_collection_avail < ( websock->fragment_collection_length + websock->frame_length ) )
			{
				uint8_t* new_fragbuf;
				websock->fragment_collection_avail += websock->frame_length;
				if( websock->fragment_collection_avail > websock->fragment_collection_buffer_size ) {
					new_fragbuf = (uint8_t*)Allocate( websock->fragment_collection_avail * 2 );
					if( websock->fragment_collection_length )
						MemCpy( new_fragbuf, websock->fragment_collection, websock->fragment_collection_length );
					Deallocate( uint8_t*, websock->fragment_collection );
					websock->fragment_collection = new_fragbuf;
					websock->fragment_collection_buffer_size = websock->fragment_collection_avail * 2;
				}
				websock->fragment_collection_index = 0; // start with mask byte 0 on this new packet
			}
			websock->input_msg_state++;
			// fall through, no break statement; add the byte to the buffer
		case 17:
			// if there was no data, then there's nothing to demask
			if( websock->fragment_collection && (websock->fragment_collection_length < websock->fragment_collection_avail) )
			{
				websock->fragment_collection[websock->fragment_collection_length++]
					= msg[n] ^ websock->mask_key[(websock->fragment_collection_index++) % 4];
			}

			// if final packet, and we have all the bytes for this packet
			// dispatch the opcode.
			if( websock->final && ( websock->fragment_collection_length == websock->fragment_collection_avail) )
			{
				//lprintf( "Final: %d  opcode %d  length %d ", websock->final, websock->opcode, websock->fragment_collection_length );
				websock->last_reception = timeGetTime();

				switch( websock->opcode )
				{
				case 0x02: //binary
				case 0x01: //text
					websock->RSV1 = websock->_RSV1;
				case 0x00: // continuation
					/// single packet, final...
					//LogBinary( websock->fragment_collection, websock->fragment_collection_length );
					if( websock->on_event ) {
						while( !websock->flags.initial_handshake_done || websock->flags.in_open_event ) Relinquish();
#ifndef __NO_WEBSOCK_COMPRESSION__
						if( websock->flags.deflate && ( websock->RSV1 & 0x40 ) ) {
							int r;
							websock->inflateBufUsed = 0;
							websock->inflater.next_in = websock->fragment_collection;
							websock->inflater.avail_in = (uInt)websock->fragment_collection_length;
							websock->inflater.next_out = (Bytef*)websock->inflateBuf;
							websock->inflater.avail_out = (uInt)websock->inflateBufLen;
							do {
								//r = inflateBack( &websock->inflater, inflateBackInput, websock, inflateBackOutput, websock );
								r = inflate( &websock->inflater, Z_FINISH );
								if( r == Z_DATA_ERROR ) {
									lprintf( "zlib Data Error..." );
								}
								else if( r != Z_OK && r != Z_BUF_ERROR )
									lprintf( "unhandle zlib inflate error... %d", r );
								if( websock->inflater.avail_out == 0 ) {
									websock->inflateBuf = Reallocate( websock->inflateBuf, websock->inflateBufLen + 4096 );
									websock->inflater.next_out = (Bytef*)(((uintptr_t)websock->inflateBuf) + websock->inflateBufLen);
									websock->inflateBufLen += 4096;
									websock->inflater.avail_out += 4096;
									continue;
								}
								else
									break;
							} while( r != Z_STREAM_END || r != Z_BUF_ERROR );
							websock->inflateBufUsed = websock->inflater.total_out;
							websock->on_event( webock->socket->pc, websock->psv_open, websock->input_type
								, websock->inflateBuf, websock->inflateBufUsed );
							inflateReset( &websock->inflater );
						}
						else
#endif
						{
							//lprintf( "Completed packet; %d %d", websock->input_type, websock->fragment_collection_length );
							websock->on_event( (PCLIENT)websock->psvSender, websock->psv_open, websock->input_type, websock->fragment_collection, websock->fragment_collection_length );
						}
					}
					websock->fragment_collection_length = 0;
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
					//lprintf( "Got close...%d", websock->flags.closed );
					if( !websock->flags.closed )
					{
						//lprintf( "reply close with same payload." );
						SendWebSocketMessage( websock, 0x08, 1, websock->flags.expect_masking, websock->fragment_collection, websock->frame_length );
						websock->flags.closed = 1;
					}
					if( websock->on_close ) {
						int code;
						char buf[128];
						if( websock->frame_length > 2 ) {
							// assume the buffer is longer by 1 so the NUL terminator gets put after the string
							StrCpyEx( buf, (char*)(websock->fragment_collection + 2), websock->frame_length - 1 );
							//buf[websock->frame_length - 2] = 0;
							code = ((int)websock->fragment_collection[0] << 8) + websock->fragment_collection[1];
						}
						else if( websock->frame_length ) {
							code = ((int)websock->fragment_collection[0] << 8) + websock->fragment_collection[1];
							buf[0] = 0;
						}
						else {
							code = 1000;
							buf[0] = 0;
						}
						websock->close_code = code;
						websock->close_reason = DupCStrLen( buf, websock->frame_length - 2 );
						websock->on_close( (PCLIENT)websock->psvSender, websock->psv_open, code, buf );
						websock->on_close = NULL;
					}
					websock->fragment_collection_length = 0;
					if( !websock->flags.pipe )
						RemoveClientEx( (PCLIENT)websock->psvSender, 0, 0 ); // this should not linger; client already sent closed, nothing more to receive.
					else {
						if( websock->do_close)
							websock->do_close( websock->psvCloser );
						else
							lprintf( "Pipe handle close: This should probably release weboscket and all other stuff" );
					}
					// resetInputstate after this would squash next memory....
					return;

					break;
				case 0x09: // ping
					{
						SendWebSocketMessage( websock, 0x0a, 1, websock->flags.expect_masking, websock->fragment_collection, websock->frame_length );
						websock->fragment_collection_length = 0;
					}
					break;
				case 0x0A: // pong
					{
						// this is for the ping routine to wait (or rather to end wait)
						websock->flags.received_pong = 1;
					}
					websock->fragment_collection_length = 0;
					break;
				default:
					lprintf( "Bad WebSocket opcode: %d", websock->opcode );
					if( !websock->flags.pipe )
						RemoveClient( (PCLIENT)websock->psvSender );
					else {
						lprintf( "pipe handle bad opcode: again should probably destroy websocket object" );
					}
					return;
				}
				// after processing any opcode (this is IN final, and length match) we're done, start next message
				ResetInputState( websock );
			} else if( websock->fragment_collection_length == websock->fragment_collection_avail ) {
				//lprintf( "Completed packet; still not final fragment though.... %d  %d", websock->fragment_collection_avail );
				websock->input_msg_state = 0;
				if( websock->on_fragment_done )
					websock->on_fragment_done( (PCLIENT)websock->psvSender, websock->psv_open, websock->input_type, (int)websock->fragment_collection_length );
				//lprintf( "came back" );
			}
			break;
		}
	}
}

static void WebSocketPing_( struct web_socket_input_state *input_state, uint32_t timeout )
{
	SendWebSocketMessage( input_state, 9, 1, input_state->flags.expect_masking, NULL, 0 );
	if( timeout ) {
		uint32_t start_at = timeGetTime();
		uint32_t target = start_at + timeout;
		uint32_t now;
		while( !input_state->flags.received_pong
			&& (((now = timeGetTime()) - start_at) < timeout) )
			IdleFor( target - now );
		input_state->flags.received_pong = 0;
	}
}

void WebSocketPing( PCLIENT pc, uint32_t timeout )
{
	struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
	WebSocketPing_( input_state, timeout );
}

void WebSocketPipePing( struct html5_web_socket* ws, uint32_t timeout )
{
	WebSocketPing_( &ws->input_state, timeout );
}

// there is a control bit for whether the content is text or binary or a continuation
static void WebSocketBeginSendText_( struct web_socket_input_state* output, const char* buffer, size_t length ) // UTF8 RFC3629
{
	if( length > 8100 ) {
		size_t sentLen;
		size_t maxLen = length - 8100;
		for( sentLen = 0; sentLen < maxLen; sentLen += 8100 )
			WebSocketBeginSendText_( output, buffer + sentLen, 8100 );
		length = length - ( sentLen );
		buffer = buffer + ( sentLen );
	}
	SendWebSocketMessage( output, output->flags.sent_type ? 0 : 1, 0, output->flags.expect_masking, (const uint8_t*)buffer, length );
	output->flags.sent_type = 1;
}

void WebSocketBeginSendText( PCLIENT pc, const char* buffer, size_t length ) // UTF8 RFC3629
{
	if( pc ) {
		struct web_socket_input_state* output = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		WebSocketBeginSendText_( output, buffer, length );
	}
}

void WebSocketPipeBeginSendText( struct html5_web_socket* ws, const char* buffer, size_t length ) // UTF8 RFC3629
{
	WebSocketBeginSendText_( &ws->input_state, buffer, length );
}


// there is a control bit for whether the content is text or binary or a continuation
static void WebSocketSendText_( struct web_socket_input_state *input, const char *buffer, size_t length ) // UTF8 RFC3629
{
		if( !input ) return;
		if( length > 8100 ) {
			size_t sentLen;
			size_t maxLen = length - 8100;
			for( sentLen = 0; sentLen < maxLen; sentLen += 8100 )
				WebSocketBeginSendText_( input, buffer + sentLen, 8100 );
			length = length - ( sentLen );
			buffer = buffer + ( sentLen );
		}
		SendWebSocketMessage( input, input->flags.sent_type?0:1, 1, input->flags.expect_masking, (uint8_t*)buffer, length );
		input->flags.sent_type = 0;
}

void WebSocketSendText( PCLIENT pc, const char *buffer, size_t length ) // UTF8 RFC3629
{
	if( pc ) {
		struct web_socket_input_state *input = (struct web_socket_input_state *)GetNetworkLong( pc, 1 );
		WebSocketSendText_( input, buffer, length );
	}
}

void WebSocketPipeSendText( struct html5_web_socket *ws, const char *buffer, size_t length ) // UTF8 RFC3629
{
	WebSocketSendText_( &ws->input_state, buffer, length );
}

// literal binary sending; this may happen to be base64 encoded too
static void WebSocketBeginSendBinary_( struct web_socket_input_state* output, const uint8_t* buffer, size_t length ) {
	//struct web_socket_input_state *output = (struct web_socket_input_state *)GetNetworkLong(pc, 1);
	if( length > 8100 ) {
		size_t sentLen;
		size_t maxLen = length - 8100;
		for( sentLen = 0; sentLen < maxLen; sentLen += 8100 )
			WebSocketBeginSendBinary_( output, buffer + sentLen, 8100 );
		length = length - ( sentLen );
		buffer = buffer + ( sentLen );
	}
	SendWebSocketMessage( output, output->flags.sent_type ? 0 : 2, 0, output->flags.expect_masking, (const uint8_t*)buffer, length );
	output->flags.sent_type = 1;
}

void WebSocketBeginSendBinary( PCLIENT pc, const uint8_t* buffer, size_t length ) {
	if( pc ) {
		struct web_socket_input_state* output = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		WebSocketBeginSendBinary_( output, buffer, length );
	}
}

void WebSocketPipeBeginSendBinary( struct html5_web_socket* ws, const uint8_t* buffer, size_t length ) {
	WebSocketBeginSendBinary_( &ws->input_state, buffer, length );
}


// literal binary sending; this may happen to be base64 encoded too
static void WebSocketSendBinary_( struct web_socket_input_state *output, const uint8_t *buffer, size_t length )
{
		if( length > 8100 ) {
			size_t sentLen;
			size_t maxLen = length - 8100;
			for( sentLen = 0; sentLen < maxLen; sentLen += 8100 )
				WebSocketBeginSendBinary_( output, buffer + sentLen, 8100 );
			length = length - ( sentLen );
			buffer = buffer + ( sentLen );
		}
		SendWebSocketMessage( output, output->flags.sent_type?0:2, 1, output->flags.expect_masking, (const uint8_t*)buffer, length );
		output->flags.sent_type = 0;
}

void WebSocketSendBinary( PCLIENT pc, const uint8_t *buffer, size_t length )
{
	if( pc ) {
		struct web_socket_input_state *output = (struct web_socket_input_state *)GetNetworkLong(pc, 1);
		WebSocketSendBinary_( output, buffer, length );
	}
}

void WebSocketPipeSendBinary( struct html5_web_socket *ws, const uint8_t *buffer, size_t length )
{
	WebSocketSendBinary_( &ws->input_state, buffer, length );
}

//------------------ Setup Callbacks -------------------


void SetWebSocketAcceptCallback( PCLIENT pc, web_socket_accept callback )
{
	if( pc ) {
 		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_accept = callback;
	}
}

void SetWebSocketPipeAcceptAsyncCallback( struct html5_web_socket* socket, web_socket_accept_async callback )
{
	if( socket ) {
		socket->input_state.on_accept_async = callback;
	}
}


void SetWebSocketAcceptAsyncCallback( PCLIENT pc, web_socket_accept_async callback )
{
	if( pc ) {
 		struct html5_web_socket* socket = (struct html5_web_socket*)GetNetworkLong( pc, 0 );
		SetWebSocketPipeAcceptAsyncCallback( socket, callback );
	}
}


void SetWebSocketReadCallback( PCLIENT pc, web_socket_event callback )
{
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_event = callback;
	}
}

void SetWebSocketCloseCallback( PCLIENT pc, web_socket_closed callback )
{
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_close = callback;
	}
}

void SetWebSocketHttpCallback( PCLIENT pc, web_socket_http_request callback )
{
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_request = callback;
	}
}

void SetWebSocketPipeHttpCallback( struct html5_web_socket* ws, web_socket_http_request callback )
{
	ws->input_state.on_request = callback;
	
}

void SetWebSocketHttpCloseCallback( PCLIENT pc, web_socket_http_close callback )
{
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_http_close = callback;
	}
}

void SetWebSocketPipeHttpCloseCallback( struct html5_web_socket *ws, web_socket_http_close callback )
{
	ws->input_state.on_http_close = callback;
}

void SetWebSocketErrorCallback( PCLIENT pc, web_socket_error callback )
{
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_error = callback;
	}
}

void SetWebSocketPipeErrorCallback( struct html5_web_socket *ws, web_socket_error callback )
{
	ws->input_state.on_error = callback;
}

void SetWebSocketDeflate( PCLIENT pc, int enable ) {
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->flags.deflate = enable?1:0;
		input_state->flags.do_not_deflate = enable==2?1:0;
	}
}

void SetWebSocketPipeDeflate( struct html5_web_socket *ws, int enable ) {
	ws->input_state.flags.deflate = enable?1:0;
	ws->input_state.flags.do_not_deflate = enable==2?1:0;
}

void SetWebSocketMasking( PCLIENT pc, int enable ) {
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->flags.expect_masking = enable;
	}
}

void SetWebSocketPipeMasking( struct html5_web_socket *ws, int enable ) {
	ws->input_state.flags.expect_masking = enable;
}

void SetWebSocketDataCompletion( PCLIENT pc, web_socket_completion callback ) {
	if( pc ) {
		struct web_socket_input_state *input_state = (struct web_socket_input_state*)GetNetworkLong( pc, 1 );
		input_state->on_fragment_done = callback;
	}
}

void SetWebSocketPipeDataCompletion( struct html5_web_socket *ws, web_socket_completion callback ) {
	ws->input_state.on_fragment_done = callback;
}
