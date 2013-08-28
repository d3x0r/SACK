#ifdef SACK_CORE_BUILD
#define MD5_SOURCE
#endif
#define HTML5_WEBSOCKET_SOURCE

#include <stdhdrs.h>
#include <stdlib.h>
#include <sha1.h>
#include <md5.h>
#include <http.h>


#include <html5.websocket.h>

HTML5_WEBSOCKET_NAMESPACE

struct sockett {

	_8 generated[75];
	
} l;

struct html5_web_socket {
	HTTPState http_state;
	PCLIENT pc;
	struct web_socket_flags
	{
		BIT_FIELD initial_handshake_done : 1;
		BIT_FIELD rfc6455 : 1;
		BIT_FIELD fragment_collecting : 1;
	} flags;
	size_t fragment_collection_avail;
	size_t fragment_collection_length;
	P_8 fragment_collection;
};

const TEXTCHAR *base64 = WIDE("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
	
static void encodeblock( unsigned char in[3], TEXTCHAR out[4], int len )
{
    out[0] = base64[ in[0] >> 2 ];
    out[1] = base64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? base64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? base64[ in[2] & 0x3f ] : '=');
}

static LOGICAL ComputeReplyKey2( PVARTEXT pvt_output, HTML5WebSocket socket, PTEXT key1, PTEXT key2 )
{
	TEXTCHAR buf1[24];
	int buf1_idx = 0;
	_64 number1;
	TEXTCHAR buf2[24];
	int buf2_idx = 0;
	_64 number2;
	int spaces1 = 0;
	int spaces2 = 0;
	int c;
	int len;
	TEXTCHAR *check;
	// test overrides
	//key1 = SegCreateFromText( WIDE("4 @1  46546xW%0l 1 5" ) );
	//key2 = SegCreateFromText( WIDE("12998 5 Y3 1  .P00" ) );

	len = GetTextSize( key1 );
	check = GetText( key1 );
	for( c = 0; c < len; c++ )
	{
		if( check[c] == ' ' )
			spaces1++;
		if( check[c] >= '0' && check[c] <= WIDE( '9' ) )
			buf1[buf1_idx++] = check[c];
	}
	buf1[buf1_idx++] = 0;
	number1 = IntCreateFromText( buf1 );

	len = GetTextSize( key2 );
	check = GetText( key2 );
	for( c = 0; c < len; c++ )
	{
		if( check[c] == ' ' )
			spaces2++;
		if( check[c] >= '0' && check[c] <= WIDE( '9' ) )
			buf2[buf2_idx++] = check[c];
	}
	buf2[buf2_idx++] = 0;
	number2 = IntCreateFromText( buf2 );

	if( !spaces1 || !spaces2 )
		return FALSE;
	{
		int a = number1 % spaces1;
		int b = number2 % spaces2;
		if( a || b )
			return FALSE;
	}
	if( number1 % spaces1 || number2 % spaces2 )
		return FALSE;

	number1 = number1 / spaces1;
	number2 = number2 / spaces2;

	{
		struct replybuf
		{
			_8 result1[4];
			_8 result2[4];
			_8 extra[8];
		} buf;

		buf.result1[0] = ( number1 & 0xFF000000 ) >> 24;
		buf.result1[1] = ( number1 & 0xFF0000 ) >> 16;
		buf.result1[2] = ( number1 & 0xFF00 ) >> 8;
		buf.result1[3] = ( number1 & 0xFF ) >> 0;
		buf.result2[0] = ( number2 & 0xFF000000 ) >> 24;
		buf.result2[1] = ( number2 & 0xFF0000 ) >> 16;
		buf.result2[2] = ( number2 & 0xFF00 ) >> 8;
		buf.result2[3] = ( number2 & 0xFF ) >> 0;
		{
			int n;
			// override text_content with....
			//SegCreateFromText( WIDE("^n:ds[4U" ) );
			PTEXT text_content = GetHttpContent( socket->http_state );
			TEXTCHAR *content = GetText( text_content );
			for( n = 0; n < 8; n++ )
			{
				buf.extra[n] = content[n];
			}
		}
		{
			MD5_CTX ctx;
			_8 result[16];
			int n;
			MD5Init( &ctx );
			if( sizeof( buf ) != 16 )
				xlprintf(LOG_ALWAYS)( WIDE("padding has been injected, and all will be bad.") );
			MD5Update( &ctx, (unsigned char*)&buf, 16 );
			MD5Final(result, &ctx);
			for( n = 0; n < 16; n++ )
				VarTextAddCharacter( pvt_output, result[n] );
		}
		//md5( buf );
		//vtprintf( 
	}

// passes test here
//	Sec-WebSocket-Key1: 18x 6]8vM;54 *(5:  {   U1]8  z [  8
//  Sec-WebSocket-Key2: 1_ tx7X d  <  nw  334J702) 7]o}` 0
//  Tm[K T2u
//  fQJ,fN/4F4!~K~MH

// passes test here
// 4 @1  46546xW%0l 1 5
// 12998 5 Y3 1  .P00
// ^n:ds[4U
// 8jKS'y:G*Co,Wxa-

// not sure all example data lined up.
//  Sec-WebSocket-Key1: 3e6b263  4 17 80
              //Sec-WebSocket-Key2: 17  9 G`ZD9   2 2b 7X 3 /r90
              //WjN}|M(6
//
//result:	 0x09 0x65 0x65 0x0A 0xB9 0x67 0x33 0x57 0x6A 0x4E 0x7D 0x7C 0x4D
//        0x28 0x36.
	return TRUE;
}

static void AddFragment( HTML5WebSocket socket, POINTER fragment, size_t frag_length )
{
	P_8 new_fragbuf;
	socket->fragment_collection_length += frag_length;
	new_fragbuf = (P_8)Allocate( socket->fragment_collection_length + frag_length );
	if( socket->fragment_collection_length )
		MemCpy( new_fragbuf, socket->fragment_collection, socket->fragment_collection_length );
	MemCpy( new_fragbuf + socket->fragment_collection_length, fragment, frag_length );
	Deallocate( P_8, socket->fragment_collection );
	socket->fragment_collection = new_fragbuf;
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
static void HandleData( HTML5WebSocket socket, PCLIENT pc, POINTER buffer, size_t length )
{
	size_t n;
	_8 okay = 0;
	int randNum;
	TEXTCHAR output[25];
	P_8 bytes = (P_8)buffer;
	for( n = 0; n < length; n++ )
	{
		if( bytes[n] == 0 )
		{
			if( socket->fragment_collection_length )
			{
				lprintf( WIDE("Message start with a message outstanding, double null chars?!") );
				socket->fragment_collection_length = 0;
			}
		}
		if( bytes[n] == 0xFF )
		{
			lprintf( WIDE("Completed message...") );
			LogBinary( socket->fragment_collection, socket->fragment_collection_length );
			socket->fragment_collection_length = 0;
		}
		if( bytes[n] == 'n' && bytes[n + 1] == 'u' && bytes[n + 2] == 'm' )
		{			
			while( !okay )
			{
				//srand();
				randNum = ( rand() % 75 );
				if( randNum != 0 && l.generated[randNum - 1] != 1 )
				{
					l.generated[randNum - 1] = 1;
					okay = 1;
					lprintf( WIDE(" Random Number: %d"), randNum );
					snprintf( output, 10, WIDE("%d"), randNum );
					SendTCP( pc, output, sizeof( output ));
				}
			}
		}
		else
		{
			if( socket->fragment_collection_avail == socket->fragment_collection_length )
			{
				P_8 new_fragment = NewArray( _8, socket->fragment_collection_avail + 64 );
				socket->fragment_collection_avail += 64;
				MemCpy( new_fragment, socket->fragment_collection, socket->fragment_collection_length );
				Deallocate( POINTER, socket->fragment_collection );
				socket->fragment_collection = new_fragment;
			}
			socket->fragment_collection[socket->fragment_collection_length++] = bytes[n];
		}
	}
}

static void HandleData_RFC6455( HTML5WebSocket socket, POINTER buffer, size_t length )
{
	LOGICAL final = 0;
	LOGICAL mask = 0;
	_8 mask_key[4];
	int opcode;
	_64 frame_length;
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
		AddFragment( socket, msg + offset+2, frame_length );
		if( final )
		{
			LogBinary( socket->fragment_collection, socket->fragment_collection_length );
		}
		break;
	case 0x01: //text
      if( !final )
			AddFragment( socket, msg + offset+2, frame_length );
      else
         LogBinary( msg + offset+2, frame_length );
		break;
	case 0x02: //binary
		if( !final )
			AddFragment( socket, msg + offset+2, frame_length );
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

static void CPROC read_complete( PCLIENT pc, POINTER buffer, size_t length )
{
	if( buffer )
	{
		HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
		enum ProcessHttpResult result;
		TEXTSTR tmp = DupCharToText( (const char*)buffer );
		LogBinary( buffer, length );
		if( !socket->flags.initial_handshake_done )
		{
			lprintf( WIDE("Initial handshake is not done...") );
			((char*)buffer)[length] = 0; // make sure nul terminated.
			AddHttpData( socket->http_state, tmp, length );
			while( ( result = ProcessHttp( pc, socket->http_state ) ) )
			{
				lprintf( WIDE("result is %d"), result );
				switch( result )
				{
				case HTTP_STATE_RESULT_CONTENT:
					{
						PVARTEXT pvt_output = VarTextCreate();
						PTEXT value;
						PTEXT value2;
						PTEXT key1, key2;
						char *output;
						key1 = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Key1" ) );
						key2 = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Key2" ) );
						if( key1 && key2 )
							socket->flags.rfc6455 = 0;
                  else
							socket->flags.rfc6455 = 1;

						if( key1 && key2 )
							vtprintf( pvt_output, WIDE("HTTP/1.1 101 WebSocket Protocol Handshake\r\n") );
						else
							vtprintf( pvt_output, WIDE("HTTP/1.1 101 Switching Protocols\r\n") );

						vtprintf( pvt_output, WIDE("Upgrade: WebSocket\r\n") );
						vtprintf( pvt_output, WIDE("Connection: Upgrade\r\n") );


						value = GetHTTPField( socket->http_state, WIDE( "Origin" ) );
						if( value )
						{
							if( key1 && key2 )
								vtprintf( pvt_output, WIDE("Sec-WebSocket-Origin: %s\r\n"), GetText( value ) );
							else
								vtprintf( pvt_output, WIDE("WebSocket-Origin: %s\r\n"), GetText( value ) );
						}
						if( key1 && key2 )
						{
							vtprintf( pvt_output, WIDE("Sec-WebSocket-Location: ws://%s%s\r\n" )
								, GetText( GetHTTPField( socket->http_state, WIDE("Host") ) )
								, GetText( GetHttpRequest( socket->http_state ) )
								);
						}
						value = GetHTTPField( socket->http_state, WIDE( "Sec-webSocket-Key" ) );
						if( value )
						{
							{
								const TEXTCHAR guid[] = WIDE("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
								size_t len;

								TEXTCHAR *resultval = NewArray( TEXTCHAR, len = ( GetTextSize( value ) + sizeof(guid)/sizeof(TEXTCHAR)));
								snprintf( resultval, len, WIDE("%s%s")
									, GetText(value)
									, guid );
								{
									TEXTCHAR output[32];
									char *tmpval = DupTextToChar( resultval );
									SHA1Context context;
									int n;
									struct convert_64 {
										BIT_FIELD filler : 6;
										BIT_FIELD char1 : 6;
										BIT_FIELD char2a : 2;
										BIT_FIELD char2b : 4;
										BIT_FIELD char3b : 4;
										BIT_FIELD char3c : 2;
										BIT_FIELD char4 : 6;
										BIT_FIELD junk2 : 2;
									};

									uint8_t Message_Digest[SHA1HashSize + 2];
									SHA1Reset( &context );
									SHA1Input( &context, (uint8_t*)tmpval, len - 1 );
									SHA1Result( &context, Message_Digest );
									Message_Digest[SHA1HashSize] = 0;
									Message_Digest[SHA1HashSize+1] = 0;
									for( n = 0; n < (SHA1HashSize+2)/3; n++ )
									{
										int blocklen;
										blocklen = SHA1HashSize - n*3;
										if( blocklen > 3 )
											blocklen = 3;
										encodeblock( Message_Digest + n * 3, output + n*4, blocklen );
									}
									output[n*4+0] = 0;
									// s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
									vtprintf( pvt_output, WIDE("Sec-WebSocket-Accept: %s\r\n"), output );
									Deallocate( char*, tmpval );
								}
							}
						}


						value = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Protocol" ) );
						if( value )
							vtprintf( pvt_output, WIDE("Sec-WebSocket-Protocol: %s\r\n"), GetText( value ) );

						vtprintf( pvt_output, WIDE("\r\n" ) );

						if( key1 && key2 )
						{
							ComputeReplyKey2( pvt_output, socket, key1, key2 );
						}

						value = VarTextGet( pvt_output );
						output = DupTextToChar( GetText( value ) );
						LogBinary( output, GetTextSize( value ) );
						SendTCP( pc, output, GetTextSize( value ) );
					}
					EndHttp( socket->http_state );
					socket->flags.initial_handshake_done = 1;
					break;
				case HTTP_STATE_RESULT_CONTINUE:
					break;
				}
			}
		}
		else
		{
			lprintf( WIDE("Okay then hand this as data to process... within protocol") );
			if( socket->flags.rfc6455 )
				HandleData_RFC6455( socket, buffer, length );
			else
				HandleData( socket, pc, buffer, length );
		}
	}
	else
	{
		buffer = Allocate( 4096 );
      //SendTCP( "
	}
	ReadTCP( pc, buffer, 4096 );
}

static void CPROC connected( PCLIENT pc_server, PCLIENT pc_new )
{
	//HTML5WebSocket server_socket = (HTML5WebSocket)GetNetworkLong( pc_server, 0 );
	HTML5WebSocket socket = New( struct html5_web_socket );
	MemSet( socket, 0, sizeof( struct html5_web_socket ) );
	socket->pc = pc_new;
	socket->http_state = CreateHttpState();

	SetNetworkLong( pc_new, 0, (PTRSZVAL)socket );
	SetNetworkReadComplete( pc_new, read_complete );
}

static LOGICAL CPROC HandleRequest( PTRSZVAL psv, HTTPState pHttpState )
{

}

HTML5WebSocket CreateWebSocket( CTEXTSTR hosturl )
{
	HTML5WebSocket socket = New( struct html5_web_socket );
	NetworkStart();
	MemSet( socket, 0, sizeof( struct html5_web_socket ) );

	socket->pc = 0;
	socket->pc = //CreateHttpServer( "localhost:9998", "WebSockets", "url", HandleRequest, 0 );
		OpenTCPListenerEx( 9998, connected );
	socket->http_state = CreateHttpState();
	SetNetworkLong( socket->pc, 0, (PTRSZVAL)socket );
	return socket;
}

HTML5_WEBSOCKET_NAMESPACE_END

