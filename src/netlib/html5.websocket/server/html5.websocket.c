#define NO_UNICODE_C
#ifdef SACK_CORE_BUILD
#define MD5_SOURCE
#endif
#define HTML5_WEBSOCKET_SOURCE
#define SACK_WEBSOCKET_CLIENT_SOURCE // websocketclose is a common...

#include <stdhdrs.h>
#include <stdlib.h>
#include <sha1.h>
#include <md5.h>
#include <http.h>

#include "../html5.websocket.common.h"
#include <html5.websocket.h>

HTML5_WEBSOCKET_NAMESPACE


typedef struct html5_web_socket *HTML5WebSocket;


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
	uint64_t number1;
	TEXTCHAR buf2[24];
	int buf2_idx = 0;
	uint64_t number2;
	int spaces1 = 0;
	int spaces2 = 0;
	size_t c;
	size_t len;
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
			uint8_t result1[4];
			uint8_t result2[4];
			uint8_t extra[8];
		} buf;

		buf.result1[0] = (uint8_t)(( number1 & 0xFF000000 ) >> 24);
		buf.result1[1] = (uint8_t)(( number1 & 0xFF0000 ) >> 16);
		buf.result1[2] = (uint8_t)(( number1 & 0xFF00 ) >> 8);
		buf.result1[3] = (uint8_t)(( number1 & 0xFF ) >> 0);
		buf.result2[0] = (uint8_t)(( number2 & 0xFF000000 ) >> 24);
		buf.result2[1] = (uint8_t)(( number2 & 0xFF0000 ) >> 16);
		buf.result2[2] = (uint8_t)(( number2 & 0xFF00 ) >> 8);
		buf.result2[3] = (uint8_t)(( number2 & 0xFF ) >> 0);
		{
			int n;
			// override text_content with....
			//SegCreateFromText( WIDE("^n:ds[4U" ) );
			PTEXT text_content = GetHttpContent( socket->http_state );
			TEXTCHAR *content = GetText( text_content );
			for( n = 0; n < 8; n++ )
			{
				buf.extra[n] = ((char*)content)[n];
			}
		}
		{
			MD5_CTX ctx;
			uint8_t result[16];
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
	//int randNum;
	//TEXTCHAR output[25];
	uint8_t* bytes = (uint8_t*)buffer;
	for( n = 0; n < length; n++ )
	{
		if( bytes[n] == 0 )
		{
			if( socket->input_state.fragment_collection_length )
			{
				lprintf( WIDE("Message start with a message outstanding, double null chars?!") );
				socket->input_state.fragment_collection_length = 0;
			}
		}
		if( bytes[n] == 0xFF )
		{
			lprintf( WIDE("Completed message...") );
			LogBinary( socket->input_state.fragment_collection, socket->input_state.fragment_collection_length );
			socket->input_state.fragment_collection_length = 0;
		}
		{
			if( socket->input_state.fragment_collection_avail == socket->input_state.fragment_collection_length )
			{
				uint8_t* new_fragment = NewArray( uint8_t, socket->input_state.fragment_collection_avail + 64 );
				socket->input_state.fragment_collection_avail += 64;
				MemCpy( new_fragment, socket->input_state.fragment_collection, socket->input_state.fragment_collection_length );
				Deallocate( POINTER, socket->input_state.fragment_collection );
				socket->input_state.fragment_collection = new_fragment;
			}
			socket->input_state.fragment_collection[socket->input_state.fragment_collection_length++] = bytes[n];
		}
	}
}


static void CPROC read_complete( PCLIENT pc, POINTER buffer, size_t length )
{
	if( buffer )
	{
		HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
		int result;
#ifdef _UNICODE
		TEXTSTR tmp = CharWConvertExx( (const char*)buffer, length DBG_SRC );
#else
		TEXTSTR tmp = (TEXTSTR)buffer;
#endif
		//LogBinary( buffer, length );
		if( !socket->flags.initial_handshake_done )
		{
			//lprintf( WIDE("Initial handshake is not done...") );
			((char*)buffer)[length] = 0; // make sure nul terminated.
			AddHttpData( socket->http_state, tmp, length );
			while( ( result = ProcessHttp( pc, socket->http_state ) ) )
			{
				switch( result )
				{
				default:
					lprintf( WIDE("unexpected result is %d"), result );
					break;
				case HTTP_STATE_RESULT_CONTENT:
				{
					PVARTEXT pvt_output = VarTextCreate();
					PTEXT value, value2;
					PTEXT key1, key2;
					value = GetHTTPField( socket->http_state, WIDE( "Connection" ) );
					value2 = GetHTTPField( socket->http_state, WIDE( "Upgrade" ) );
					if( !value || !value2
						|| !TextLike( value, "upgrade" )
						|| !TextLike( value2, "websocket" ) ) {
						lprintf( "request is not an upgrade for websocket." );
						socket->flags.initial_handshake_done = 1;
						socket->flags.http_request_only = 1;
						if( socket->input_state.on_request )
							socket->input_state.on_request( pc, socket->input_state.psv_on );
						else {
							RemoveClient( pc );
							return;
						}
						break;
					}

					value = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Extensions" ) );
					if( value )
					{
						PTEXT options = TextParse( value, "=", "; ", 0, 0 DBG_SRC );
						PTEXT opt = options;
						while( opt ) {
							// "server_no_context_takeover"
							// "client_no_context_takeover"
							// "server_max_window_bits"
							// "client_max_window_bits"
							if( TextLike( opt, "permessage-deflate" ) ) {
								socket->input_state.flags.deflate = socket->input_state.flags.deflate & 1;
								if( socket->input_state.flags.deflate ) {
									socket->input_state.server_max_bits = 15;
									socket->input_state.client_max_bits = 15;
								}
							}
							else if( TextLike( opt, "client_max_window_bits" ) ) {
								opt = NEXTLINE( opt );
								if( opt ) {
									if( GetText( opt )[0] == '=' ) {
										opt = NEXTLINE( opt );
										socket->input_state.client_max_bits = (int)IntCreateFromSeg( opt );
									}
									else
										opt = PRIORLINE( opt );
								}
								//socket->flags.max_window_bits = 1;
							}
							else if( TextLike( opt, "server_max_window_bits" ) ) {
								opt = NEXTLINE( opt );
								if( opt ) {
									if( GetText( opt )[0] == '=' ) {
										opt = NEXTLINE( opt );
										socket->input_state.server_max_bits = (int)IntCreateFromSeg( opt );
									}
								}
								else {
									lprintf( "required server_max_window_bits value is missing" );
								}

								//socket->flags.max_window_bits = 1;
							}
							else if( TextLike( opt, "client_no_context_takeover" ) ) {
								//socket->flags.max_window_bits = 1;
							}
							else if( TextLike( opt, "server_no_context_takeover" ) ) {
								//socket->flags.max_window_bits = 1;
							}
							opt = NEXTLINE( opt );
						}
						LineRelease( options );
						if( socket->input_state.flags.deflate && !socket->input_state.flags.do_not_deflate ) {
							if( deflateInit2( &socket->input_state.deflater
								, Z_BEST_SPEED, Z_DEFLATED
								, -socket->input_state.server_max_bits
								, 8
								, Z_DEFAULT_STRATEGY ) != Z_OK )
								socket->input_state.flags.deflate = 0;
						}
						if( socket->input_state.flags.deflate ) {
							//socket->inflateWindow = NewArray( uint8_t, (size_t)(1 << (socket->client_max_bits&0x1f)) );
							if( inflateInit2( &socket->input_state.inflater, -socket->input_state.client_max_bits ) != Z_OK ) {
							//if( inflateBackInit( &socket->input_state.inflater, socket->client_max_bits, socket->inflateWindow ) != Z_OK ) {
								deflateEnd( &socket->input_state.deflater );
								socket->input_state.flags.deflate = 0;
							}
							else {
								socket->input_state.inflateBuf = NewArray( uint8_t, 4096 );
								socket->input_state.inflateBufLen = 4096;
								socket->input_state.deflateBuf = NewArray( uint8_t, 4096 );
								socket->input_state.deflateBufLen = 4096;
							}
						}
					}
					else {
						socket->input_state.flags.deflate = 0;
					}

					value = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Protocol" ) );

					if( value ) {
						socket->protocols = GetText( value );
					}
					else socket->protocols = NULL;

					{
						PTEXT protocols = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Protocol" ) );
						PTEXT resource = GetHttpResource( socket->http_state );
						if( socket->input_state.on_accept ) {
							socket->flags.accepted = socket->input_state.on_accept( pc, socket->input_state.psv_on, GetText( protocols ), GetText( resource ), &socket->protocols );
						}
						else
							socket->flags.accepted = 1;
					}

					key1 = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Key1" ) );
					key2 = GetHTTPField( socket->http_state, WIDE( "Sec-WebSocket-Key2" ) );
					if( key1 && key2 )
						socket->flags.rfc6455 = 0;
					else
						socket->flags.rfc6455 = 1;

					if( !socket->flags.accepted ) {
						vtprintf( pvt_output, WIDE( "HTTP/1.1 403 Connection refused\r\n" ) );
					}
					else
					{
						if( key1 && key2 )
							vtprintf( pvt_output, WIDE( "HTTP/1.1 101 WebSocket Protocol Handshake\r\n" ) );
						else
							vtprintf( pvt_output, WIDE( "HTTP/1.1 101 Switching Protocols\r\n" ) );
						vtprintf( pvt_output, WIDE( "Upgrade: WebSocket\r\n" ) );
						vtprintf( pvt_output, WIDE( "Connection: Upgrade\r\n" ) );
					}

					value = GetHTTPField( socket->http_state, WIDE( "Origin" ) );
					if( value )
					{
						if( key1 && key2 )
							vtprintf( pvt_output, WIDE( "Sec-WebSocket-Origin: %s\r\n" ), GetText( value ) );
						else
							vtprintf( pvt_output, WIDE( "WebSocket-Origin: %s\r\n" ), GetText( value ) );
					}

					if( key1 && key2 )
					{
						vtprintf( pvt_output, WIDE( "Sec-WebSocket-Location: ws://%s%s\r\n" )
							, GetText( GetHTTPField( socket->http_state, WIDE( "Host" ) ) )
							, GetText( GetHttpRequest( socket->http_state ) )
						);
					}
					if( socket->flags.accepted ) {
						value = GetHTTPField( socket->http_state, WIDE( "Sec-webSocket-Key" ) );
						if( value )
						{
							{
								const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
								size_t len;

								char *resultval = NewArray( char, len = (GetTextSize( value ) + sizeof( guid ) ) );
								snprintf( resultval, len, WIDE( "%s%s" )
									, GetText( value )
									, guid );
								{
									TEXTCHAR output[32];
									SHA1Context context;
									int n;
									uint8_t Message_Digest[SHA1HashSize + 2];
									SHA1Reset( &context );
									SHA1Input( &context, (uint8_t*)resultval, len - 1 );
									SHA1Result( &context, Message_Digest );
									Message_Digest[SHA1HashSize] = 0;
									Message_Digest[SHA1HashSize + 1] = 0;
									for( n = 0; n < (SHA1HashSize + 2) / 3; n++ )
									{
										int blocklen;
										blocklen = SHA1HashSize - n * 3;
										if( blocklen > 3 )
											blocklen = 3;
										encodeblock( Message_Digest + n * 3, output + n * 4, blocklen );
									}
									output[n * 4 + 0] = 0;
									// s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
									vtprintf( pvt_output, WIDE( "Sec-WebSocket-Accept: %s\r\n" ), output );
								}
								Deallocate( char *, resultval );
							}
						}
						if( socket->input_state.flags.deflate ) {
							vtprintf( pvt_output, WIDE( "Sec-WebSocket-Extensions: permessage-deflate; client_no_context_takeover; server_max_window_bits=%d\r\n" ), socket->input_state.server_max_bits );
						}
						if( socket->protocols )
							vtprintf( pvt_output, WIDE( "Sec-WebSocket-Protocol: %s\r\n" ), socket->protocols );
						vtprintf( pvt_output, WIDE( "WebSocket-Server: sack\r\n" ) );

						vtprintf( pvt_output, WIDE( "\r\n" ) );

						if( key1 && key2 )
						{
							ComputeReplyKey2( pvt_output, socket, key1, key2 );
						}

						value = VarTextPeek( pvt_output );
#ifdef _UNICODE
						{
							char *output;
							output = DupTextToChar( GetText( value ) );
							//LogBinary( output, GetTextSize( value ) );
							if( socket->input_state.flags.use_ssl )
								ssl_Send( pc, output, GetTextSize( value ) );
							else
								SendTCP( pc, output, GetTextSize( value ) );
						}
#else
						if( socket->input_state.flags.use_ssl )
							ssl_Send( pc, GetText( value ), GetTextSize( value ) );
						else
							SendTCP( pc, GetText( value ), GetTextSize( value ) );
#endif
						//lprintf( "Sent http reply." );
						VarTextDestroy( &pvt_output );
						if( socket->input_state.on_open )
							socket->input_state.psv_open = socket->input_state.on_open( pc, socket->input_state.psv_on );
					}
					else {
						WebSocketClose( pc, 0, NULL );
						return;
					}
					// keep this until close, application might want resource and/or headers from this.
					//EndHttp( socket->http_state );
					socket->flags.initial_handshake_done = 1;
					break;
				}
				case HTTP_STATE_RESULT_CONTINUE:
					break;
				}
			}
		}
		else
		{
			//lprintf( WIDE("Okay then hand this as data to process... within protocol") );
			if( socket->flags.rfc6455 )
			{
				ProcessWebSockProtocol( &socket->input_state, pc, (uint8_t*)buffer, length );
			}
			else
				HandleData( socket, pc, buffer, length );
		}
	}
	else
	{
		HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
		buffer = socket->buffer = Allocate( 4096 );
	}
	ReadTCP( pc, buffer, 4096 );
}

static void CPROC closed( PCLIENT pc_client ) {
	HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc_client, 0 );
	if( socket->input_state.flags.deflate ) {
		deflateEnd( &socket->input_state.deflater );
		inflateEnd( &socket->input_state.inflater );
		Deallocate( POINTER, socket->input_state.inflateBuf );
		Deallocate( POINTER, socket->input_state.deflateBuf );
	}
	Deallocate( uint8_t*, socket->input_state.fragment_collection );
	DestroyHttpState( socket->http_state );
	Deallocate( POINTER, socket->buffer );
	Deallocate( HTML5WebSocket, socket );
	SetNetworkLong( pc_client, 0, 0 );
}

static void CPROC connected( PCLIENT pc_server, PCLIENT pc_new )
{
	HTML5WebSocket server_socket = (HTML5WebSocket)GetNetworkLong( pc_server, 0 );
	HTML5WebSocket socket = New( struct html5_web_socket );
	MemSet( socket, 0, sizeof( struct html5_web_socket ) );
	socket->Magic = 0x20130912;
	socket->pc = pc_new;
	socket->input_state = server_socket->input_state; // clone callback methods and config flags
	if( ssl_IsClientSecure( pc_new ) )
		socket->input_state.flags.use_ssl = 1;
	socket->http_state = CreateHttpState(); // start a new http state collector

	SetNetworkLong( pc_new, 0, (uintptr_t)socket );
	SetNetworkLong( pc_new, 1, (uintptr_t)&socket->input_state );
	SetNetworkReadComplete( pc_new, read_complete );
	SetNetworkCloseCallback( pc_new, closed );
}

static LOGICAL CPROC HandleWebsockRequest( uintptr_t psv, HTTPState pHttpState )
{
   return 0;
}

PCLIENT WebSocketCreate( CTEXTSTR hosturl
							, web_socket_opened on_open
							, web_socket_event on_event
							, web_socket_closed on_closed
							, web_socket_error on_error
							, uintptr_t psv )
{
	struct url_data *url;
	HTML5WebSocket socket = New( struct html5_web_socket );
	NetworkStart();
	MemSet( socket, 0, sizeof( struct html5_web_socket ) );
	socket->Magic = 0x20130912;
	socket->input_state.flags.deflate = 1;
	socket->input_state.on_open = on_open;
	socket->input_state.on_event = on_event;
	socket->input_state.on_close = on_closed;
	socket->input_state.on_error = on_error;
	socket->input_state.psv_on = psv;
	url = SACK_URLParse( hosturl );
	socket->pc = OpenTCPListenerAddrEx( CreateSockAddress( url->host, url->port?url->port:url->default_port ), connected );
	SACK_ReleaseURL( url );
	socket->http_state = CreateHttpState();
	SetNetworkLong( socket->pc, 0, (uintptr_t)socket );
	SetNetworkLong( socket->pc, 1, (uintptr_t)&socket->input_state );
	return socket->pc;
}

PLIST GetWebSocketHeaders( PCLIENT pc ) {
	HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
	if( socket && socket->Magic == 0x20130912 ) {
		return GetHttpHeaderFields( socket->http_state );
	}
	return NULL;
}

PTEXT GetWebSocketResource( PCLIENT pc ) {
	HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
	if( socket && socket->Magic == 0x20130912 ) {
		return GetHttpResource( socket->http_state );
	}
	return NULL;
}

HTTPState GetWebSocketHttpState( PCLIENT pc ) {
	if( pc ) {
		HTML5WebSocket socket = (HTML5WebSocket)GetNetworkLong( pc, 0 );
		if( socket && socket->Magic == 0x20130912 ) {
			return socket->http_state;
		}
	}
	return NULL;
}


HTML5_WEBSOCKET_NAMESPACE_END
