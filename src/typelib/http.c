#include <stdhdrs.h>
#include <network.h>
#include <procreg.h>
#include <sqlgetoption.h>
#include "http.h"


HTTP_NAMESPACE


enum ReadChunkState {
	READ_VALUE, READ_VALUE_CR, READ_VALUE_LF, READ_CR, READ_LF, READ_BYTES
};

struct HttpState {
	// add input into pvt_collector
	PVARTEXT pvt_collector;
	PTEXT partial;  // an accumulator that moves data from collector into whatever we've got leftover
	PTEXT method;
	PTEXT response_status; // the first line of the http responce... (or request)
	PTEXT resource; // the path of the resource - mostly for when this is used to receive requests.
	PLIST fields; // list of struct HttpField *, these other the other meta fields in the header.
	PLIST cgi_fields; // list of HttpField *, taken in from the URL or content (get or post)

	size_t content_length;
	PTEXT content; // content of the message, POST,PUT,PATCH and replies have this.
	LOGICAL returned_status;

	int final; // boolean flag - indicates that the header portion of the http request is finished.

	POINTER buffer; //for handling requests, have to read somewhere

	int numeric_code;
	int response_version;
	TEXTSTR text_code;
	PCLIENT request_socket; // when a request comes in to the server, it is kept in a new http state, this is set for Send Response

	LOGICAL ssl;
	PVARTEXT pvtOut; // this is filled with a request ready to go out; used for HTTPS
	LOGICAL read_chunks;
	size_t read_chunk_byte;
	size_t read_chunk_length;
	size_t read_chunk_total_length;
	enum ReadChunkState read_chunk_state;
	uint32_t last_read_tick;
	PTHREAD waiter;
	PCLIENT *pc;
	struct httpStateFlags {
		BIT_FIELD keep_alive : 1;
		BIT_FIELD close : 1;
		BIT_FIELD upgrade : 1;
		BIT_FIELD h2c_upgrade : 1;
		BIT_FIELD ws_upgrade : 1;
		BIT_FIELD ssl : 1; // prevent issuing network reads... ssl pushes data from internal buffers
		BIT_FIELD success : 1;
	}flags;
};

struct HttpServer {
	PCLIENT server;
	PLIST clients;
	ProcessHttpRequest handle_request;
	uintptr_t psvRequest;
	CTEXTSTR site;
	PCLASSROOT methods;
};

static struct local_http_data
{
	struct http_data_flags {
		BIT_FIELD bLogReceived : 1;
	} flags;
	PLIST pendingConnects;
}local_http_data;
#define l local_http_data

struct pendingConnect {
	PCLIENT pc;
	struct HttpState *state;
};

PRELOAD( loadOption ) {
#ifndef __NO_OPTIONS__
	l.flags.bLogReceived = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/HTTP/Enable Logging Received Data" ), 0, TRUE );
#endif
}

void GatherHttpData( struct HttpState *pHttpState )
{
	if( pHttpState->content_length )
	{
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		//lprintf( "Gathering http data with content length..." );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		if( GetTextSize( pHttpState->partial ) >= pHttpState->content_length )
		{
			//lprintf( "Partial is complete with %d", GetTextSize( pHttpState->partial ) );
			pHttpState->content = SegSplit( &pHttpState->partial, pHttpState->content_length );
			pHttpState->partial = NEXTLINE( pHttpState->partial );		
			SegGrab( pHttpState->partial );
			pHttpState->flags.success = 1;
		}
		//else
		//	lprintf( "Partial is only %d", GetTextSize( pHttpState->partial ) );
	}
	else
	{
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		//lprintf( "Setting content to partial... " );
		pHttpState->content = pHttpState->partial;
	}
}


void ProcessURL_CGI( struct HttpState *pHttpState, PTEXT params )
{
	PTEXT start = TextParse( params, WIDE( "&=" ), NULL, 1, 1 DBG_SRC );
	PTEXT next = start;
	PTEXT tmp;
	//lprintf( "Input was %s", GetText( params ) );
	while( ( tmp = next ) )
	{
		PTEXT name = tmp;
		/*PTEXT equals = */( next = NEXTLINE( tmp ) );
		PTEXT value = ( next = NEXTLINE( next ) );
		/*PTEXT ampersand = */( next = NEXTLINE( next ) );

		struct HttpField *field = New( struct HttpField );
		field->name = SegDuplicate( name );
		field->value = SegDuplicate( value );
		//lprintf( "Added %s=%s", GetText( field->name ), GetText( field->value ) );
		AddLink( &pHttpState->cgi_fields, field );
		next = NEXTLINE( next );
	}
	LineRelease( start );
}

//int ProcessHttp( struct HttpState *pHttpState )
int ProcessHttp( PCLIENT pc, struct HttpState *pHttpState )
{
	if( pHttpState->final )
	{
		GatherHttpData( pHttpState );
		if( pHttpState->flags.success && !pHttpState->returned_status ) {
			pHttpState->returned_status = 1;
			return pHttpState->numeric_code;
		}
		return HTTP_STATE_RESULT_NOTHING;
	}
	else
	{
		PTEXT pCurrent;//, pStart;
		PTEXT pLine = NULL;
		TEXTCHAR *c, *line;
		size_t size, pos, len;
		size_t bLine;
		INDEX start = 0;
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		pCurrent = pHttpState->partial;
		//pStart = pCurrent; // at lest is this block....
		//LogBinary( (const uint8_t*)GetText( pInput ), GetTextSize( pInput ) );
		len = 0;

		// we always start without having a line yet, because all input is already merged
		bLine = 0;
		{
			//lprintf( "%s", GetText( pCurrent ) );
			size = GetTextSize( pCurrent );
			c = GetText( pCurrent );
			if( bLine < 4 )
			{
				//start = 0; // new packet and still collecting header....
				for( pos = 0; ( pos < size ) && !pHttpState->final; pos++ )
				{
					if( c[pos] == '\r' )
						bLine++;
					else if( c[pos] == '\n' )
						bLine++;
					else // non end of line character....
					{
	FinalCheck:
						if( bLine >= 2 ) // had an end of line...
						{
							// response status is the data from the fist bit of the packet (on receiving http 1.1/OK ...)
							if( pHttpState->response_status )
							{
								CTEXTSTR field_start;
								CTEXTSTR colon;
								CTEXTSTR field_end;
								CTEXTSTR val_start;
								PTEXT field_name;
								PTEXT value;
								pLine = SegCreate( pos - start - bLine );
								if( (pos-start) < bLine )
								{
									lprintf( WIDE("Failure.") );
								}
								MemCpy( line = GetText( pLine ), c + start, (pos - start - bLine)*sizeof(TEXTCHAR));
								line[pos-start-bLine] = 0;
								field_start = GetText( pLine );

								// this is a  request field.
								colon = StrChr( field_start, ':' );
								if( colon )
								{
									PTEXT trash;
									val_start = colon + 1;
									field_end = colon;
									while( ( field_end > field_start )&& field_end[-1] == ' ' )
										field_end--;
									while( ( val_start[0] && ( val_start[0] == ' ' ) ) )
										val_start++;

									SegSplit( &pLine, val_start - field_start );
									value = NEXTLINE( pLine );
									field_name = SegSplit( &pLine, field_end - field_start );
									trash = NEXTLINE( field_name );
									{
										struct HttpField *field = New( struct HttpField );
										field->name = SegGrab( field_name );
										field->value = SegGrab( value );
										if( TextLike( field->name, "connection" ) )
										{
											if( TextLike( field->value, "keep-alive" ) ) {
												pHttpState->flags.keep_alive = 1;
											}
											if( TextLike( field->value, "close" ) ) {
												pHttpState->flags.close = 1;
											}
										}
										LineRelease( trash );
										AddLink( &pHttpState->fields, field );
									}
								}
								else
								{
									lprintf( WIDE( "Header field [%s] invalid" ), GetText( pLine ) );
									LineRelease( pLine );
								}
							}
							else
							{
								pLine = SegCreate( pos - start - bLine );
								MemCpy( line = GetText( pLine ), c + start, (pos - start - bLine)*sizeof(TEXTCHAR));
								line[pos-start-bLine] = 0;
								pHttpState->response_status = pLine;
								pHttpState->numeric_code = 0; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
								{
									PTEXT request = TextParse( pHttpState->response_status, WIDE( "?#" ), WIDE( " " ), 1, 1 DBG_SRC );
									{
										PTEXT tmp;
										PTEXT resource_path = NULL;
										PTEXT next;
										if( TextSimilar( request, WIDE( "GET" ) ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
										}
										else if( TextSimilar( request, WIDE( "POST" ) ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											lprintf( WIDE("probably shouldn't post final until content length is also received...") );
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
										}
										// this loop is used for both client and server http requests...
										// this will be the first part of a HTTP response (this one will have a result code, the other is just version)
										else if( TextSimilar( request, WIDE( "HTTP/" ) ) )
										{
											TEXTCHAR *tmp2 = (TEXTCHAR*)StrChr( GetText( request ), '.' );
											pHttpState->response_version = (int)((IntCreateFromText( GetText( request ) + 5 ) * 100) + IntCreateFromText( tmp2 + 1 ));
											{
												PTEXT nextword = NEXTLINE( request );
												if( nextword )
												{
													next = NEXTLINE( nextword );
													// cast from int64_t
													pHttpState->numeric_code = (int)IntCreateFromText( GetText( nextword ) );
													nextword = next;
													if( nextword )
													{
														next = NEXTLINE( nextword );
														if( pHttpState->text_code )
															Release( pHttpState->text_code );
														pHttpState->text_code = StrDup( GetText( nextword ) );
													}
												}
												else
												{
													lprintf( WIDE( "failed to find result code in %s" ), line );
												}
											}
										}
										else {
											lprintf( "Unsupported Command:%s", GetText( request ) );
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );

										}
										for( tmp = request; tmp; tmp = next )
										{
											//lprintf( WIDE( "word %s" ), GetText( tmp ) );
											next = NEXTLINE( tmp );
											//lprintf( "Line : %s", GetText( pLine ) );
											if( TextSimilar( tmp, WIDE("HTTP/") ) )
											{
												TEXTCHAR *tmp2 = (TEXTCHAR*)StrChr( GetText( tmp ), '.' );
												pHttpState->response_version = (int)(( IntCreateFromText( GetText( tmp ) + 5 ) * 100 ) + IntCreateFromText( tmp2 + 1 ));
											}
											else if( GetText(tmp)[0] == '?' )
											{
												ProcessURL_CGI( pHttpState, next );
												next = NEXTLINE( next );
											}
											else if( GetText(tmp)[0] == '#' )
											{
												lprintf( WIDE("Page anchor is lost... %s"), GetText( next ) );
												next = NEXTLINE( next );
											}
											else
											{
												if( (resource_path && tmp->format.position.offset.spaces) )
												{
													break;
												}
												else
												{
													resource_path = SegAppend( resource_path, SegGrab( tmp ) );
													request = next;
												}
											}
										}
										pHttpState->resource = resource_path;
									}
									LineRelease( request );
								}
								//else
								//	lprintf( "Not Http header?" );
							}
							// could perhaps append a newline segment block...
							// but probably do not need such a thing....
							// since the return should be assumed as a continuous
							// stream of datas....
							start = pos;
							if( bLine == 2 )
								bLine = 0;
						}
						// may not receive anything other than header information?
						if( bLine == 4 )
						{
							// end of header
							// copy the previous line out...
							//pStart = pCurrent;
							len = size - pos; // remaing size
							break;
						}
					}
					if( bLine == 4 )
					{
						pos++;
						pHttpState->final = 1;
						goto FinalCheck;
					}
				}
				if( pos == size &&
					bLine == 4 &&
					start != pos )
				{
					pHttpState->final = 1;
					goto FinalCheck;
				}
			}
			else
				len += size;
			//pCurrent = NEXTLINE( pCurrent );
			/* Move the remaining data into a single binary data packet...*/
		}
		if( start )
		{
			/*PTEXT tmp = */SegSplit( &pCurrent, start );
			pHttpState->partial = NEXTLINE( pCurrent );
			LineRelease( SegGrab( pCurrent ) );
			start = 0;
		}

		// final is having received the end of the HTTP header, not nessecarily all data
		if( pHttpState->final )
		{
			INDEX idx;
			struct HttpField *field;
			LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
			{
				if( TextLike( field->name, WIDE( "content-length" ) ) )
				{
					// down convert from int64_t
					pHttpState->content_length = (int)IntCreateFromSeg( field->value );
					//lprintf( "content lenght: %d", pHttpState->content_length );
				}
				else if( TextLike( field->name, WIDE( "upgrade" ) ) )
				{
					if( TextLike( field->value, "websocket" ) ) {
						pHttpState->flags.ws_upgrade = 1;
					}
					else if( TextLike( field->value, "h2c" ) ) {
						pHttpState->flags.h2c_upgrade = 1;
					}
				}
				else if( TextLike( field->name, WIDE( "connection" ) ) )
				{
					if( TextLike( field->value, "upgrade" ) ) {
						pHttpState->flags.upgrade = 1;
					}
				}
				else if( TextLike( field->name, WIDE( "Transfer-Encoding" ) ) )
				{
					if( TextLike( field->value, "chunked" ) )
					{
						pHttpState->content_length = 0xFFFFFFF;
						pHttpState->read_chunks = TRUE;
						pHttpState->read_chunk_state = READ_VALUE;
						pHttpState->read_chunk_length = 0;
						pHttpState->read_chunk_total_length = 0;
					}
				}
				else if( TextLike( field->name, WIDE( "Expect" ) ) )
				{
					if( TextLike( field->value, WIDE( "100-continue" ) ) )
					{
						if( l.flags.bLogReceived )
							lprintf( WIDE("Generating 100-continue response...") );
						SendTCP( pc, "HTTP/1.1 100 Continue\r\n\r\n", 25 );
					}
				}
			}
			// do one gather here... with whatever remainder we had.
			GatherHttpData( pHttpState );
		}
	}

	if( pHttpState->final &&
		( ( pHttpState->content_length
			&& ( ( GetTextSize( pHttpState->partial ) >= pHttpState->content_length )
				||( GetTextSize( pHttpState->content ) >= pHttpState->content_length ) ) )
			|| ( !pHttpState->content_length )
			) )
	{
		pHttpState->returned_status = 1;
		if( pHttpState->numeric_code == 500 )
			return HTTP_STATE_INTERNAL_SERVER_ERROR;
		if( pHttpState->content && (pHttpState->numeric_code == 200) ) {
			return HTTP_STATE_RESULT_CONTENT;
		}
		if( pHttpState->numeric_code == 100 )
			return HTTP_STATE_RESULT_CONTINUE;
		if( pHttpState->numeric_code == 404 )
			return HTTP_STATE_RESOURCE_NOT_FOUND;
		if( pHttpState->numeric_code == 400 )
			return HTTP_STATE_BAD_REQUEST;
		return pHttpState->numeric_code;
	}
	return HTTP_STATE_RESULT_NOTHING;
}

LOGICAL AddHttpData( struct HttpState *pHttpState, POINTER buffer, size_t size )
{
	pHttpState->last_read_tick = GetTickCount();
	if( pHttpState->read_chunks )
	{
		uint8_t* buf = (uint8_t*)buffer;
		size_t ofs = 0;
		while( ofs < size )
		{
			switch( pHttpState->read_chunk_state )
			{
			case READ_VALUE:
				if( buf[0] >= '0' && buf[0] <= '9' )
				{
					pHttpState->read_chunk_length *= 16;
					pHttpState->read_chunk_length += buf[0] - '0';
				}
				else if( ( buf[0] | 0x20 ) >= 'a' && (buf[0] | 0x20) <= 'f' )
				{
					pHttpState->read_chunk_length *= 16;
					pHttpState->read_chunk_length += (buf[0] | 0x20) - 'a' + 10;
				}
				else if( buf[0] == '\r' )
				{
					pHttpState->read_chunk_total_length += pHttpState->read_chunk_length;
					if( l.flags.bLogReceived ) {
						lprintf( "Chunck will be %zd", pHttpState->read_chunk_length );
					}
					pHttpState->read_chunk_state = READ_VALUE_LF;
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
					RemoveClient( pHttpState->request_socket );
					return FALSE;
				}
				break;
			case READ_VALUE_CR:
            // didn't actually implement to get into this state... just looks for newlines really.
            break;
			case READ_VALUE_LF:
				if( buf[0] == '\n' )
				{
					if( pHttpState->read_chunk_length == 0 )
						pHttpState->read_chunk_state = READ_CR;
					else
						pHttpState->read_chunk_state = READ_BYTES;
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
					RemoveClient( pHttpState->request_socket );
					return FALSE;
				}
				break;
			case READ_CR:
				if( buf[0] == '\r' )
				{
					pHttpState->read_chunk_state = READ_LF;
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\r, found %d(%c)", buf[0], buf[0] );
					RemoveClient( pHttpState->request_socket );
					return FALSE;
				}
				break;
			case READ_LF:
				if( buf[0] == '\n' )
				{
					if( pHttpState->read_chunk_length )
					{
						pHttpState->read_chunk_length = 0;
						pHttpState->read_chunk_state = READ_VALUE;
					}
					else
					{
						pHttpState->content_length = GetTextSize( VarTextPeek( pHttpState->pvt_collector ) );
						if( pHttpState->waiter ) {
							//lprintf( "Waking waiting to return with result." );
							WakeThread( pHttpState->waiter );
						}
						return TRUE;
					}
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
					RemoveClient( pHttpState->request_socket );
					return FALSE;
				}
				break;
			case READ_BYTES:
				VarTextAddData( pHttpState->pvt_collector, (CTEXTSTR)(buf), 1 );
				pHttpState->read_chunk_byte++;
				if( pHttpState->read_chunk_byte == pHttpState->read_chunk_length )
					pHttpState->read_chunk_state = READ_CR;
				break;
			}
			ofs++;
			buf++;
		}
		if( l.flags.bLogReceived ) {
			lprintf( "chunk read is %zd of %zd", pHttpState->read_chunk_byte, pHttpState->read_chunk_total_length );
		}
		return FALSE;
	}
	else
	{
		VarTextAddData( pHttpState->pvt_collector, (CTEXTSTR)buffer, size );
		return TRUE;
	}
}

struct HttpState *CreateHttpState( void )
{
	struct HttpState *pHttpState;

	pHttpState = New( struct HttpState );
	MemSet( pHttpState, 0, sizeof( struct HttpState ) );
	pHttpState->pvt_collector = VarTextCreate();
	return pHttpState;
}


void EndHttp( struct HttpState *pHttpState )
{
	pHttpState->final = 0;

	pHttpState->content_length = 0;
	LineRelease( pHttpState->method );
	pHttpState->method = NULL;
	LineRelease( pHttpState->content );
	LineRelease( pHttpState->resource );
	pHttpState->resource = NULL;
	if( pHttpState->partial != pHttpState->content )
	{
		LineRelease( pHttpState->partial );
	}
	pHttpState->partial = NULL;
	pHttpState->content = NULL;

	LineRelease( pHttpState->response_status );
	pHttpState->response_status = NULL;

	pHttpState->numeric_code = 0;
	if( pHttpState->text_code )
	{
		Release( pHttpState->text_code );
		pHttpState->text_code = NULL;
	}

	{
		INDEX idx;
		struct HttpField *field;
		LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
		{
			LineRelease( field->name );
			LineRelease( field->value );
			Release( field );
		}
		EmptyList( &pHttpState->fields );
	}
}

PTEXT GetHttpContent( struct HttpState *pHttpState )
{
	if( pHttpState->read_chunks )
	{
		/* did a timeout happen? */
		if( pHttpState->content_length == pHttpState->read_chunk_total_length )
			return pHttpState->content;
		return NULL;
	}

	if( pHttpState->content_length )
		return pHttpState->content;
	return NULL;
}

void ProcessHttpFields( struct HttpState *pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv )
{
	INDEX idx;
	struct HttpField *field;
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		f( psv, field->name, field->value );
	}
}

void ProcessCGIFields( struct HttpState *pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv )
{
	INDEX idx;
	struct HttpField *field;
	LIST_FORALL( pHttpState->cgi_fields, idx, struct HttpField *, field )
	{
		f( psv, field->name, field->value );
	}
}

PTEXT GetHttpField( struct HttpState *pHttpState, CTEXTSTR name )
{
	INDEX idx;
	struct HttpField *field;
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		if( StrCaseCmp( GetText( field->name ), name ) == 0 )
			return field->value;
	}
	return NULL;
}

PTEXT GetHttpResponce( struct HttpState *pHttpState )
{
	return pHttpState->response_status;
}

PTEXT GetHttpRequest( struct HttpState *pHttpState )
{
	return pHttpState->resource;
}

PTEXT GetHttpResource( struct HttpState *pHttpState )
{
	return pHttpState->resource;
}

PTEXT GetHttpMethod( struct HttpState *pHttpState )
{
	return pHttpState->method;
}


void DestroyHttpStateEx( struct HttpState *pHttpState DBG_PASS )
{
   //_lprintf(DBG_RELAY)( "Destroy http state... (should clear content too?" );
	EndHttp( pHttpState ); // empties variables
	DeleteList( &pHttpState->fields );
	VarTextDestroy( &pHttpState->pvt_collector );
	if( pHttpState->buffer )
		Release( pHttpState->buffer );
	Release( pHttpState );
}

void DestroyHttpState( struct HttpState *pHttpState ) {
   DestroyHttpStateEx( pHttpState DBG_SRC );
}

#define DestroyHttpState(state) DestroyHttpStateEx(state DBG_SRC )



void SendHttpResponse ( struct HttpState *pHttpState, PCLIENT pc, int numeric, CTEXTSTR text, CTEXTSTR content_type, PTEXT body )
{
	//int offset = 0;
	PVARTEXT pvt_message = VarTextCreate();
	PTEXT header;
	PTEXT tmp_content;
	//TEXTCHAR message[500];

	vtprintf( pvt_message, WIDE( "HTTP/1.1 %d %s\r\n" ), numeric, text );
	if( content_type && body )
	{
		vtprintf( pvt_message, WIDE( "Content-Length: %d\r\n" ), GetTextSize(body));
		vtprintf( pvt_message, WIDE( "Content-Type: %s\r\n" )
				  , content_type?content_type
					:(tmp_content=GetHttpField( pHttpState, WIDE("Accept") ))?GetText(tmp_content)
					:WIDE("text/plain; charset=utf-8")  );
	}
	//else
	//	vtprintf( pvt_message, WIDE( "%s\r\n" ), GetText( body ) );
	vtprintf( pvt_message, WIDE( "Server: SACK Core Library 2.x\r\n" )  );

	if( body )
		vtprintf( pvt_message, WIDE( "\r\n" )  );

	header = VarTextPeek( pvt_message );
	//offset += snprintf( message + offset, sizeof( message ) - offset, WIDE( "%s" ),  "Body");
	if( l.flags.bLogReceived )
	{
		lprintf( WIDE("Sending response...") );
		LogBinary( (uint8_t*)GetText( header ), GetTextSize( header ) );
		if( content_type )
			LogBinary( (uint8_t*)GetText( body ), GetTextSize( body ) );
	}
	if( !pc )
		pc = pHttpState->request_socket;
	SendTCP( pc, GetText( header ), GetTextSize( header ) );
	if( content_type )
		SendTCP( pc, GetText( body ), GetTextSize( body ) );
	VarTextDestroy( &pvt_message );
}

void SendHttpMessage ( struct HttpState *pHttpState, PCLIENT pc, PTEXT body )
{	
	PTEXT message;
	PVARTEXT pvt_message = VarTextCreate();
	PTEXT content_type;

	vtprintf( pvt_message, WIDE( "%s" ),  WIDE("HTTP/1.1 200 OK\r\n") );
	vtprintf( pvt_message, WIDE( "Content-Length: %d\r\n" ), GetTextSize( body ));	
	vtprintf( pvt_message, WIDE( "Content-Type: %s\r\n" )
		, (content_type = GetHttpField( pHttpState, WIDE("Accept") ))?GetText(content_type):WIDE("text/plain" ));	
	vtprintf( pvt_message, WIDE( "\r\n" )  );	
	vtprintf( pvt_message, WIDE( "%s" ), GetText( body ));	
	message = VarTextGet( pvt_message );
	if( l.flags.bLogReceived )
	{
		lprintf( WIDE(" Response Message:" ));
		LogBinary( (uint8_t*)GetText( message ), GetTextSize( message ));
	}
	SendTCP( pc, GetText( message ), GetTextSize( message ));		
}

//---------- CLIENT --------------------------------------------

static void CPROC HttpReader( PCLIENT pc, POINTER buffer, size_t size )
{
	struct HttpState *state = (struct HttpState *)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
		//lprintf( "Initial read on HTTP requestor" );
		if( state && state->ssl )
		{
			PTEXT send = VarTextGet( state->pvtOut );
			if( l.flags.bLogReceived )
			{
				lprintf( WIDE("Sending Request...") );
				LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
			}
#ifndef NO_SSL
			// had to wait for handshake, so NULL event
			// on secure has already had time to build the send
			// but had to wait until now to do that.
			ssl_Send( pc, GetText( send ), GetTextSize( send ) );
#endif
			LineRelease( send );
		}
		else
		{
			state->buffer = Allocate( 4096 );
			ReadTCP( pc, state->buffer, 4096 );
		}
	}
	else
	{
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Received web request... %zu"), size );
			//LogBinary( buffer, size );
		}
		if( AddHttpData( state, buffer, size ) )
			if( ProcessHttp( pc, state ) )
			{
				RemoveClient( pc );
				return;
			}
	}

	// read is handled by the SSL layer instead of here.  Just trust that someone will give us data later
	if( buffer && ( !state || !state->ssl ) )
	{
		ReadTCP( pc, state->buffer, 4096 );
	}
}

static void CPROC HttpReaderClose( PCLIENT pc )
{
	struct HttpState *data = (struct HttpState *)GetNetworkLong( pc, 0 );
	PCLIENT *ppc = data->pc;// (PCLIENT*)GetNetworkLong( pc, 0 );
	if( ppc )
		ppc[0] = NULL;
	if( data->waiter ) {
		//lprintf( "(on close) Waking waiting to return with result." );
		WakeThread( data->waiter );
	}
	if( !data->flags.success )
		DestroyHttpState( data );
}

static void CPROC HttpConnected( PCLIENT pc, int error ) {
	INDEX idx;
	struct pendingConnect *connect;
	while( 1 ) {
		LIST_FORALL( l.pendingConnects, idx, struct pendingConnect *, connect ) {
			if( connect->pc == pc ) {
				SetLink( &l.pendingConnects, idx, NULL );
				break;
			}
		}
		if( connect )
			break;
		Relinquish();
	}
	SetNetworkLong( pc, 0, (uintptr_t)connect->state );
	Release( connect );
}

HTTPState PostHttpQuery( PTEXT address, PTEXT url, PTEXT content )
{
	struct HttpState *state = CreateHttpState();
	PCLIENT pc;
	struct pendingConnect *connect = New( struct pendingConnect );
	connect->state = state;
	AddLink( &l.pendingConnects, connect );
	pc = OpenTCPClientExx( GetText( address ), 80, HttpReader, NULL, NULL, HttpConnected );
	connect->pc = pc;
	PVARTEXT pvtOut = VarTextCreate();
	vtprintf( pvtOut, WIDE( "POST %s HTTP/1.1\r\n" ), url );
	vtprintf( pvtOut, WIDE( "content-length:%d\r\n" ), GetTextSize( content ) );
	vtprintf( pvtOut, WIDE( "\r\n\r\n" ) );
	VarTextAddData( pvtOut, GetText( content ), GetTextSize( content ) );
	if( pc )
	{
		PTEXT send = VarTextGet( pvtOut );
		state->pc = &pc;
		state->waiter = MakeThread();
		SetNetworkLong( pc, 0, (uintptr_t)state );
		SetNetworkCloseCallback( pc, HttpReaderClose );
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Sending POST...") );
			LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
		}
		SendTCP( pc, GetText( send ), GetTextSize( send ) );
		LineRelease( send );
		while( pc )
		{
			WakeableSleep( 100 );
		}
	}
	VarTextDestroy( &pvtOut );
	return state;
}

PTEXT PostHttp( PTEXT address, PTEXT url, PTEXT content )
{
	HTTPState state = PostHttpQuery( address, url, content );
	if( state )
	{
		PTEXT result = GetHttpContent( state );
		if( result )
			Hold( result );
		DestroyHttpState( state );
		return result;
	}
	return NULL;
}

static void httpConnected( PCLIENT pc, int error ) {
	if( error ) {
		struct HttpState *state = (struct HttpState *)GetNetworkLong( pc, 0 );
		RemoveClient( pc );
		return;
	}
	{
		INDEX idx;
		struct pendingConnect *connect;
		while( 1 ) {
			LIST_FORALL( l.pendingConnects, idx, struct pendingConnect *, connect ) {
				if( connect->pc == pc ) {
					SetLink( &l.pendingConnects, idx, NULL );
					break;
				}
			}
			if( connect )
				break;
			Relinquish();
		}
		SetNetworkLong( pc, 0, (uintptr_t)connect->state );
		Release( connect );
	}
}

HTTPState GetHttpQuery( PTEXT address, PTEXT url )
{
	if( !address )
		return NULL;
	{
		PCLIENT pc;
		SOCKADDR *addr = CreateSockAddress( GetText( address ), 443 );
		struct HttpState *state = CreateHttpState();
		struct pendingConnect *connect = New( struct pendingConnect );
		connect->state = state;
		AddLink( &l.pendingConnects, connect );
		pc = OpenTCPClientAddrExxx( addr, HttpReader, HttpReaderClose, NULL, httpConnected, 0 DBG_SRC );
		connect->pc = pc;
		ReleaseAddress( addr );
		if( pc ) {
			PVARTEXT pvtOut = VarTextCreate();
			SetTCPNoDelay( pc, TRUE );
			vtprintf( pvtOut, WIDE( "GET %s HTTP/1.1\r\n" ), GetText( url ) );
			vtprintf( pvtOut, WIDE( "Host: %s\r\n" ), GetText( address ) );
			vtprintf( pvtOut, WIDE( "\r\n" ) );
			if( pc )
			{
				PTEXT send = VarTextGet( pvtOut );
				state->waiter = MakeThread();
				state->pc = &pc;
				SetNetworkLong( pc, 0, (uintptr_t)state );
				SetNetworkCloseCallback( pc, HttpReaderClose );
				if( l.flags.bLogReceived )
				{
					lprintf( WIDE( "Sending POST..." ) );
					LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
				}
				SendTCP( pc, GetText( send ), GetTextSize( send ) );
				LineRelease( send );
				while( pc )
				{
					WakeableSleep( 100 );
				}
			}
			VarTextDestroy( &pvtOut );
			return state;
		}
	}
	return NULL;
}

HTTPState GetHttpsQuery( PTEXT address, PTEXT url, const char *certChain )
{
	if( !address )
		return NULL;
	{
		struct HttpState *state = CreateHttpState();
		static PCLIENT pc;
		SOCKADDR *addr = CreateSockAddress( GetText( address ), 443 );
		struct pendingConnect *connect = New( struct pendingConnect );
		connect->state = state;
		AddLink( &l.pendingConnects, connect );
		//DumpAddr( "Http Address:", addr );
		pc = OpenTCPClientAddrExxx( addr, HttpReader, HttpReaderClose, NULL, httpConnected, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
		connect->pc = pc;
		ReleaseAddress( addr );
		if( pc )
		{
			state->last_read_tick = GetTickCount();
			state->waiter = MakeThread();
			state->pc = &pc;
			SetNetworkLong( pc, 0, (uintptr_t)state );

			//SetNetworkConn
			state->ssl = TRUE;
			state->pvtOut = VarTextCreate();
			vtprintf( state->pvtOut, WIDE( "GET %s HTTP/1.1\r\n" ), GetText( url ) );
			vtprintf( state->pvtOut, WIDE( "Host: %s\r\n" ), GetText( address ) );
			vtprintf( state->pvtOut, "User-Agent: SACK(%s)\r\n", "System" );
			vtprintf( state->pvtOut, WIDE( "\r\n" ) );
#ifndef NO_SSL
			if( ssl_BeginClientSession( pc, NULL, 0, NULL, 0, certChain, certChain ? strlen( certChain ) : 0 ) )
			{
				if( !certChain )
					ssl_SetIgnoreVerification( pc );
				if( NetworkConnectTCP( pc ) < 0 ) {
					DestroyHttpState( state );
					return NULL;
				}
				state->waiter = MakeThread();
				while( pc && ( state->last_read_tick > ( GetTickCount() - 20000 ) ) )
				{
					WakeableSleep( 1000 );
				}
				//lprintf( "Request has completed.... %p %p", pc, state->content );
				if( pc )
					RemoveClient( pc );
			}
			else
				RemoveClient( pc );
#endif
			VarTextDestroy( &state->pvtOut );
			if( !pc )
				return state;
		}
		else
		{
			RemoveClient( pc );
			DestroyHttpState( state );
		}
	}
	return NULL;
}

PTEXT GetHttp( PTEXT address, PTEXT url, LOGICAL secure )
{
	if( secure )
		return GetHttps( address, url, NULL );
	else

	{
	HTTPState state = GetHttpQuery( address, url );
	if( state )
	{
		PTEXT result = GetHttpContent( state );
		if( result )
			Hold( result );
		DestroyHttpState( state );
		return result;
	}}
	return NULL;
}
PTEXT GetHttps( PTEXT address, PTEXT url, const char *ca )
{
	HTTPState state = GetHttpsQuery( address, url, ca );
	if( state )
	{
		PTEXT result = GetHttpContent( state );
		if( result )
			Hold( result );
		DestroyHttpState( state );
		return result;
	}
	return NULL;
}

//---------- SERVER --------------------------------------------

static LOGICAL InvokeMethod( PCLIENT pc, struct HttpServer *server, struct HttpState *pHttpState )
{
	PTEXT method = GetHttpMethod( pHttpState );
	//PTEXT request = TextParse( pHttpState->response_status, WIDE( "?#" ), WIDE( " " ), 1, 1 DBG_SRC );
	if( TextLike( method, WIDE( "get" ) ) || TextLike( method, WIDE( "post" ) ) )
	{
		LOGICAL (CPROC *f)(uintptr_t, PCLIENT, struct HttpState *, PTEXT);
		LOGICAL status = FALSE;
		f = (LOGICAL (CPROC*)(uintptr_t, PCLIENT, struct HttpState *, PTEXT))GetRegisteredProcedureExxx( server->methods, (PCLASSROOT)(GetText( pHttpState->resource ) + 1), "LOGICAL", GetText(method), "(uintptr_t, PCLIENT, struct HttpState *, PTEXT)" );
		//lprintf( "got for %s %s", (PCLASSROOT)(GetText( pHttpState->resource ) + 1),  GetText( request ) );
		if( f )
			status = f( server->psvRequest, pc, pHttpState, pHttpState->content );

		if( !status )
		{
			if( server->handle_request )
				status = server->handle_request( server->psvRequest, pHttpState );
		}
		if( !status )
		{
			DECLTEXT( body, WIDE( "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD><BODY>Resource handler not found" ) );
			SendHttpResponse( pHttpState, NULL, 404, WIDE("NOT FOUND"), WIDE("text/html"), (PTEXT)&body );
		}
		return 1;
	}
	else
		lprintf( WIDE("not a get or a post?") );

	//LineRelease( request );
	return 0;
}

static void CPROC HandleRequest( PCLIENT pc, POINTER buffer, size_t length )
{
	if( !buffer )
	{
		struct HttpState *pHttpStateServer = (struct HttpState *)GetNetworkLong( pc, 0 );
		struct HttpState *pHttpState = CreateHttpState();
		pHttpState->ssl = pHttpStateServer->ssl;
		buffer = pHttpState->buffer = Allocate( 4096 );
		pHttpState->request_socket = pc;
		SetNetworkLong( pc, 1, (uintptr_t)pHttpState );
	}
	else
	{
		int result;
		struct HttpState *pHttpState = (struct HttpState *)GetNetworkLong( pc, 1 );
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Received web request...") );
			LogBinary( (uint8_t*)buffer, length );
		}

		AddHttpData( pHttpState, buffer, length );
		while( ( result = ProcessHttp( pc, pHttpState ) ) )
		{
			int status;
			struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
			//lprintf( "result = %d", result );
			switch( result )
			{
			case HTTP_STATE_RESULT_CONTENT:
				status = InvokeMethod( pc, server, pHttpState );
				if( status
					&& ( ( pHttpState->response_version == 9 )
					|| (pHttpState->response_version == 100 && !pHttpState->flags.keep_alive)
					||( pHttpState->response_version == 101 && pHttpState->flags.close ) ) ) {
					RemoveClientEx( pc, 0, 1 );
					return;
				}
				else
					EndHttp( pHttpState );
				break;
			case HTTP_STATE_RESULT_CONTINUE:
				break;
			}
		}
		if( !pHttpState->ssl )
			ReadTCP( pc, buffer, 4096 );
	}
}

static void CPROC RequestorClosed( PCLIENT pc )
{
	struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
	struct HttpState *pHttpState = (struct HttpState *)GetNetworkLong( pc, 1 );
	DeleteLink( &server->clients, pc );
	if( pHttpState )
		DestroyHttpState( pHttpState );
}

static void CPROC AcceptHttpClient( PCLIENT pc_server, PCLIENT pc_new )
{
	struct HttpServer *server;

	while( !(server = (struct HttpServer *)GetNetworkLong( pc_server, 0 )) ) {
		Relinquish();
	}
	AddLink( &server->clients, pc_new );
	SetNetworkLong( pc_new, 0, (uintptr_t)server );
	SetNetworkReadComplete( pc_new, HandleRequest );
	SetNetworkCloseCallback( pc_new, RequestorClosed );
}

#ifndef NO_SSL
struct HttpServer *CreateHttpsServerEx( CTEXTSTR interface_address, CTEXTSTR TargetName, CTEXTSTR site, ProcessHttpRequest handle_request, uintptr_t psv ) {
	struct HttpServer *server = New( struct HttpServer );
	SOCKADDR *tmp;
	TEXTCHAR class_name[256];
	server->clients = NULL;
	server->handle_request = handle_request;
	server->psvRequest = psv;
	server->site = StrDup( site );
	tnprintf( class_name, sizeof( class_name ), WIDE( "SACK/Http/Methods/%s%s%s" )
		, TargetName ? TargetName : WIDE( "" )
		, (TargetName && site) ? WIDE( "/" ) : WIDE( "" )
		, site ? site : WIDE( "" ) );
	//lprintf( "Server root = %s", class_name );
	server->methods = GetClassRoot( class_name );
	NetworkStart();
	server->server = OpenTCPListenerAddrEx( tmp = CreateSockAddress( interface_address ? interface_address : WIDE( "0.0.0.0" ), 80 )
		, AcceptHttpClient );
	SetNetworkLong( server->server, 0, (uintptr_t)server );
	ssl_BeginServer( server->server, NULL, 0, NULL, 0, NULL, 0 );
	ReleaseAddress( tmp );
	if( !server->server )
	{
		Release( server );
		return NULL;
	}

	return server;

}
#endif

struct HttpServer *CreateHttpServerEx( CTEXTSTR interface_address, CTEXTSTR TargetName, CTEXTSTR site, ProcessHttpRequest handle_request, uintptr_t psv )
{
	struct HttpServer *server = New( struct HttpServer );
	SOCKADDR *tmp;
	TEXTCHAR class_name[256];
	server->clients = NULL;
	server->handle_request = handle_request;
	server->psvRequest = psv;
	server->site = StrDup( site );
	tnprintf( class_name, sizeof( class_name ), WIDE( "SACK/Http/Methods/%s%s%s" )
			  , TargetName?TargetName:WIDE("")
			  , ( TargetName && site )?WIDE( "/" ):WIDE( "" )
			  , site?site:WIDE( "" ) );
	//lprintf( "Server root = %s", class_name );
	server->methods = GetClassRoot( class_name );
	NetworkStart();
	server->server = OpenTCPListenerAddrEx( tmp = CreateSockAddress( interface_address?interface_address:WIDE( "0.0.0.0" ), 80 )
													  , AcceptHttpClient );
	ReleaseAddress( tmp );
	if( !server->server )
	{
		Release( server );
		return NULL;
	}
	SetNetworkLong( server->server, 0, (uintptr_t)server );

	return server;
}

PTEXT GetHTTPField( struct HttpState *pHttpState, CTEXTSTR name )
{
	INDEX idx;
	struct HttpField *field;
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		if( TextLike( field->name, name ) )
			return field->value;
	}
	return NULL;
}

PLIST GetHttpHeaderFields( HTTPState pHttpState )
{
	return pHttpState->fields;
}

int GetHttpVersion( HTTPState pHttpState ) {
	return pHttpState->response_version;
}

int GetHttpResponseCode( HTTPState pHttpState ) {
	return pHttpState->numeric_code;
}

HTTP_NAMESPACE_END
#undef l
