#include <stdhdrs.h>
#include <network.h>
#include <procreg.h>
#include <sqlgetoption.h>
#include <signed_unsigned_comparisons.h>
#include "http.h"

#ifdef _MSC_VER
// derefecing NULL pointers; the function wouldn't be called with a NULL.
// and partial expressions in lower precision
#  pragma warning( disable:6011 26451)
#endif

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
	PLIST anchor_fields; // parsed anchor (err... doesn't actually get this?)
	int bLine;

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
	LOGICAL closed;
	PCLIENT *pc;
	struct httpStateFlags {
		BIT_FIELD keep_alive : 1;
		BIT_FIELD close : 1;
		BIT_FIELD no_content_length : 1;
		BIT_FIELD upgrade : 1;
		BIT_FIELD h2c_upgrade : 1;
		BIT_FIELD ws_upgrade : 1;
		BIT_FIELD ssl : 1; // prevent issuing network reads... ssl pushes data from internal buffers
		BIT_FIELD success : 1;
	}flags;
	CRITICALSECTION lock;
	struct HTTPRequestOptions* options;
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
	PLIST activeConnects;
}local_http_data;
#define l local_http_data

struct pendingConnect {
	PCLIENT pc;
	struct HttpState *state;
};

PRELOAD( loadOption ) {
#ifndef __NO_OPTIONS__
	l.flags.bLogReceived = SACK_GetProfileIntEx( GetProgramName(), "SACK/HTTP/Enable Logging Received Data", 0, TRUE );
#endif
}

static void lockHttp( struct HttpState *state ) {
	EnterCriticalSec( &state->lock );
	//while( LockedExchange( &state->lock, 1 ) );
}

static void unlockHttp( struct HttpState *state ) {
	LeaveCriticalSec( &state->lock );
	//state->lock = 0;
}

void GatherHttpData( struct HttpState *pHttpState )
{
	lockHttp( pHttpState );
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
		pHttpState->content = pHttpState->partial;
	}
	unlockHttp( pHttpState );
}


static PTEXT  resolvePercents( PTEXT urlword ) {
	PTEXT  url = BuildLine( urlword );
	LineRelease( urlword );
	//while( url = urlword )
	{
		char *_url = GetText(url);
		TEXTRUNE ch;
		char *newUrl = _url;
		int decode = 0;
		while( _url[0] ) {
			if( decode ) {
				ch *= 16;
				if( _url[0] >= '0' && _url[0] <= '9' )
					ch += _url[0] - '0';
				else if( _url[0] >= 'A' && _url[0] <= 'F' )
					ch += (_url[0] - 'A') + 10;
				else if( _url[0] >= 'a' && _url[0] <= 'f' )
					ch += (_url[0] - 'a') + 10;
				else {
					lprintf( "BAD DECODE CHARACTER: %c %d", _url[0], _url[0] );
					//LineRelease( url );
					return url;
				}
				decode--;
				if( !decode ) {
					newUrl[0] = (char)ch;
					newUrl++;
				}
			}
			else if( _url[0] == '%' ) {
				ch = 0;
				decode = 2;
			}
			else {
				newUrl[0] = _url[0];
				newUrl++;
			}
			_url++;
		}
		newUrl[0] = _url[0];
		SetTextSize( url, _url - GetText( url ) );
		urlword = NEXTLINE( url );
	}
	return url;
}

void ProcessURL_CGI( struct HttpState *pHttpState, PLIST *cgi_fields,PTEXT *pparams )
{
	PTEXT params = pparams[0];
	PTEXT start = TextParse( params, "&=", NULL, 1, 1 DBG_SRC );
	PTEXT next = start;
	PTEXT tmp;
	for( tmp = start; tmp; tmp = NEXTLINE( tmp ) ) {
		if( tmp->format.position.offset.spaces ) {
			SegBreak( tmp );
			//LineRelease( tmp );
			if( tmp == start ) // weren't actually any parameters.
				return;
			else{
				pparams[0] = tmp;
				break;  // okay, stripped the end off, use the start...
			}
		}
	}
	//lprintf( "Input was %s", GetText( params ) );
	while( ( tmp = next ) )
	{
		PTEXT name = tmp;
		next = NEXTLINE( tmp );
		while( next && GetText( next )[0] != '=' )
			next = NEXTLINE( next );
		SegBreak( next );
		PTEXT value = ( next = NEXTLINE( next ) );
		while( next && GetText( next )[0] != '&' )
			next = NEXTLINE( next );

		if( next ) SegBreak( next );

		struct HttpField *field = New( struct HttpField );
		field->name = name?resolvePercents( name ):NULL;
		field->value = value?resolvePercents( value ):NULL;
		//lprintf( "Added %s=%s", GetText( field->name ), GetText( field->value ) );
		AddLink( cgi_fields, field );
		next = NEXTLINE( next );
	}
	if( !GetLinkCount( cgi_fields[0] ) ) // otherwise it will have been relesaed with the assignment.
		LineRelease( start );
}

//int ProcessHttp( struct HttpState *pHttpState )
int ProcessHttp( struct HttpState *pHttpState, int ( *send )( uintptr_t psv, CPOINTER buf, size_t len ), uintptr_t psv )
{
	lockHttp( pHttpState );
	if( pHttpState->final )
	{
		//if( !pHttpState->method )
		{
			//lprintf( "Reading more, after returning a packet before... %d", pHttpState->response_version );
			if( pHttpState->response_version ) {
				GatherHttpData( pHttpState );
				//lprintf( "return http nothing  %d %d %d", pHttpState->content_length, pHttpState->flags.success, pHttpState->returned_status );

				if( pHttpState->flags.success && !pHttpState->returned_status ) {
					unlockHttp( pHttpState );
					pHttpState->returned_status = 1;
					return pHttpState->numeric_code;
				}
			}
			else {
				if( pHttpState->content_length ) {
					GatherHttpData( pHttpState );
					if( ((GetTextSize( pHttpState->partial ) >= pHttpState->content_length)
						|| (GetTextSize( pHttpState->content ) >= pHttpState->content_length))
						) {
						unlockHttp( pHttpState );
						// prorbably a POST with a body?
						// had to gather the body...
						return HTTP_STATE_RESULT_CONTENT;
					}
				}
			}
		}
		unlockHttp( pHttpState );

		return HTTP_STATE_RESULT_NOTHING;
	}
	else
	{
		PTEXT pCurrent;//, pStart;
		PTEXT pLine = NULL;
		TEXTCHAR *c, *line;
		size_t size, pos, len;
		INDEX start = 0;
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		pCurrent = pHttpState->partial;
		//pStart = pCurrent; // at lest is this block....
		//lprintf( "ND THIS IS WHAT WE PROCESSL:" );
		//LogBinary( (const uint8_t*)GetText( pInput ), GetTextSize( pInput ) );
		len = 0;

		// we always start without having a line yet, because all input is already merged
		{
			//lprintf( "process HTTP: %s %d", GetText( pCurrent ), pHttpState->bLine );
			size = GetTextSize( pCurrent );
			c = GetText( pCurrent );
			if( pHttpState->bLine < 4 )
			{
				//start = 0; // new packet and still collecting header....
				for( pos = 0; ( pos < size ) && !pHttpState->final; pos++ )
				{
					if( ((int)pos - (int)start - (int)pHttpState->bLine) < 0 )
						continue;
					if( c[pos] == '\r' )
						pHttpState->bLine++;
					else if( c[pos] == '\n' )
						pHttpState->bLine++;
					else // non end of line character....
					{
	FinalCheck:
						if( pHttpState->bLine >= 2 ) // had an end of line...
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
								pLine = SegCreate( pos - start - pHttpState->bLine );
								if( USS_LT( (pos-start), INDEX, pHttpState->bLine, int ) )
								{
									lprintf( "Failure." );
								}
								MemCpy( line = GetText( pLine ), c + start, (pos - start - pHttpState->bLine)*sizeof(TEXTCHAR));
								line[pos-start- pHttpState->bLine] = 0;
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
									lprintf( "Header field [%s] invalid", GetText( pLine ) );
									LineRelease( pLine );
								}
							}
							else
							{
			//lprintf( "Parsing http state content for something.." );
								pLine = SegCreate( pos - start - pHttpState->bLine );
								MemCpy( line = GetText( pLine ), c + start, (pos - start - pHttpState->bLine)*sizeof(TEXTCHAR));
								line[pos-start- pHttpState->bLine] = 0;
								pHttpState->response_status = pLine;
								pHttpState->numeric_code = 0; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
								{
									PTEXT request = TextParse( pHttpState->response_status, "?#", " ", 1, 1 DBG_SRC );
									{
										PTEXT tmp;
										PTEXT resource_path = NULL;
										PTEXT next;
										if( TextSimilar( request, "GET" ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
                                                                                        pHttpState->method = SegBreak( request );
                                                                                        pHttpState->flags.no_content_length = 0;
										}
										else if( TextSimilar( request, "POST" ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
											pHttpState->flags.no_content_length = 0;
										}
										// this loop is used for both client and server http requests...
										// this will be the first part of a HTTP response (this one will have a result code, the other is just version)
										else if( TextSimilar( request, "HTTP/" ) )
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
														next = NULL;// NEXTLINE( nextword );
														if( pHttpState->text_code )
															Release( pHttpState->text_code );
														{
															PTEXT words = BuildLine( nextword );
															pHttpState->text_code = StrDup( GetText( words ) );
															LineRelease( words );
														}
													}
													if( pHttpState->numeric_code == 101 )
														pHttpState->flags.no_content_length = 0;
												}
												else
												{
													lprintf( "failed to find result code in %s", line );
												}
											}
										}
										else {
											//lprintf( "Unsupported Command:%s", GetText( request ) );
											if( pHttpState->pc )
												TriggerNetworkErrorCallback( *pHttpState->pc, SACK_NETWORK_ERROR_HTTP_UNSUPPORTED );
											LineRelease( request );
											unlockHttp( pHttpState );
											if( pHttpState->pc )
												RemoveClient( *pHttpState->pc );
											return HTTP_STATE_RESULT_NOTHING;
										}
										for( tmp = request; tmp; tmp = next )
										{
											//lprintf( "word %s", GetText( tmp ) );
											next = NEXTLINE( tmp );
											//lprintf( "Line : %s", GetText( pLine ) );
											if( TextSimilar( tmp, "HTTP/" ) )
											{
												TEXTCHAR *tmp2 = (TEXTCHAR*)StrChr( GetText( tmp ), '.' );
												if( tmp2 )
													pHttpState->response_version = (int)(( IntCreateFromText( GetText( tmp ) + 5 ) * 100 ) + IntCreateFromText( tmp2 + 1 ));
												else if( GetTextSize( tmp ) > 5 )
													pHttpState->response_version = (int)( IntCreateFromText( GetText( tmp ) + 5 ) * 100 );
												else
													pHttpState->response_version = 0;

												if( pHttpState->response_version >= 101 ) {
													pHttpState->flags.close = 0;
													pHttpState->flags.keep_alive = 1;
												}
												else if( pHttpState->response_version == 100 ) {
													pHttpState->flags.close = 1;
													pHttpState->flags.keep_alive = 0;
												}
												else {
													pHttpState->flags.close = 1;
													pHttpState->flags.keep_alive = 0;
												}
											}
											else if( GetText(tmp)[0] == '?' )
											{
												ProcessURL_CGI( pHttpState, &pHttpState->cgi_fields, &next );
												//next = NEXTLINE( next );
											}
											else if( GetText(tmp)[0] == '#' )
											{
												// anchor is stripped by the client before requesting
												ProcessURL_CGI( pHttpState, &pHttpState->anchor_fields, &next );
												lprintf( "Page anchor of URL is lost(not saved)...%s %s"
													, GetText( tmp )
													, GetText( next ) );
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
										if( resource_path ) resource_path->format.position.offset.spaces = 0;
										pHttpState->resource = BuildLine( resource_path );
										LineRelease( resource_path );
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
							if( pHttpState->bLine == 2 )
								pHttpState->bLine = 0;
						}
						// may not receive anything other than header information?
						if( pHttpState->bLine == 4 )
						{
							// end of header
							// copy the previous line out...
							//pStart = pCurrent;
							len = size - pos; // remaing size
							break;
						}
					}
					if( pHttpState->bLine == 4 )
					{
						pos++;
						pHttpState->final = 1;
						goto FinalCheck;
					}
				}
				if( pos == size &&
					pHttpState->bLine == 4 &&
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
				if( TextLike( field->name, "content-length" ) )
				{
					// down convert from int64_t
				    pHttpState->content_length = (int)IntCreateFromSeg( field->value );
				    pHttpState->flags.no_content_length = 0;
					//lprintf( "content length: %d", pHttpState->content_length );
				}
				else if( TextLike( field->name, "upgrade" ) )
				{
					if( TextLike( field->value, "websocket" ) ) {
						pHttpState->flags.ws_upgrade = 1;
					}
					else if( TextLike( field->value, "h2c" ) ) {
						pHttpState->flags.h2c_upgrade = 1;
					}
				}
				else if( TextLike( field->name, "connection" ) )
				{
					if( StrCaseStr( GetText( field->value ), "upgrade" ) ) {
						pHttpState->flags.upgrade = 1;
					}
				}
				else if( TextLike( field->name, "Transfer-Encoding" ) )
				{
					if( TextLike( field->value, "chunked" ) )
					{
						pHttpState->content_length = 0xFFFFFFF;
						pHttpState->flags.no_content_length = 0;
						pHttpState->read_chunks = TRUE;
						pHttpState->read_chunk_state = READ_VALUE;
						pHttpState->read_chunk_length = 0;
						pHttpState->read_chunk_total_length = 0;
					}
				}
				else if( TextLike( field->name, "Expect" ) )
				{
					if( TextLike( field->value, "100-continue" ) )
					{
						if( l.flags.bLogReceived )
							lprintf( "Generating 100-continue response..." );
						send( psv, "HTTP/1.1 100 Continue\r\n\r\n", 25 );
					}
				}
			}
			// do one gather here... with whatever remainder we had.
			GatherHttpData( pHttpState );
		}
	}
	unlockHttp( pHttpState );

	if( pHttpState->final &&
		( ( pHttpState->content_length
			&& ( ( GetTextSize( pHttpState->partial ) >= pHttpState->content_length )
				||( GetTextSize( pHttpState->content ) >= pHttpState->content_length ) ) )
			|| ( !pHttpState->content_length && !pHttpState->flags.no_content_length )
			) )
	{
		pHttpState->returned_status = 1;
		//lprintf( "return http %d",pHttpState->numeric_code );
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
	//lprintf( "return http nothing" );
	return HTTP_STATE_RESULT_NOTHING;
}

LOGICAL AddHttpData( struct HttpState *pHttpState, POINTER buffer, size_t size )
{
	lockHttp( pHttpState );
	pHttpState->last_read_tick = timeGetTime();
	if( pHttpState->read_chunks )
	{
		const uint8_t* buf = (const uint8_t*)buffer;
		size_t ofs = 0;
      //lprintf( "Add Chunk HTTP Data:%d", size );

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
#ifdef _DEBUG
					if( l.flags.bLogReceived ) {
						lprintf( "Chunck will be %zd", pHttpState->read_chunk_length );
					}
#endif
					pHttpState->read_chunk_state = READ_VALUE_LF;
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
					TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
					unlockHttp( pHttpState );
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
					TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
					unlockHttp( pHttpState );
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
					TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
					unlockHttp( pHttpState );
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
						//lprintf( "This may or may not be the end of content? %d", pHttpState->content_length );
						if( pHttpState->waiter ) {
							//lprintf( "Waking waiting to return with result." );
							WakeThread( pHttpState->waiter );
						}
						unlockHttp( pHttpState );
						return TRUE;
					}
				}
				else
				{
					lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
					TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
					unlockHttp( pHttpState );
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
		unlockHttp( pHttpState );
		return FALSE;
	}
	else
	{
		//lprintf( "Add HTTP Data:%p %d", pHttpState->pc[0], size );
		//LogBinary( (uint8_t*)buffer, 256>size?size:256 );
		if( size )
			VarTextAddData( pHttpState->pvt_collector, (CTEXTSTR)buffer, size );
		unlockHttp( pHttpState );
		if( pHttpState->final ) {
			// this will cause it to wait until 'endhttp' to process next block.
			//lprintf( "still handling a previous requet in add data", pHttpState->pc[0] );
			return FALSE;
		}
		return TRUE;
	}
}

struct HttpState *CreateHttpState( PCLIENT *pc )
{
	struct HttpState *pHttpState;

	pHttpState = New( struct HttpState );
	MemSet( pHttpState, 0, sizeof( struct HttpState ) );
	InitializeCriticalSec( &pHttpState->lock );
	pHttpState->flags.no_content_length = 1;
	pHttpState->pvt_collector = VarTextCreate();
	pHttpState->pc = pc;
	return pHttpState;
}


void EndHttp( struct HttpState *pHttpState )
{
	//lprintf( "Ending HTTP %p", pHttpState );
	lockHttp( pHttpState );
	pHttpState->bLine = 0;
	pHttpState->final = 0;
	pHttpState->response_version = 0;
	pHttpState->flags.no_content_length = 0;
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
		LIST_FORALL( pHttpState->cgi_fields, idx, struct HttpField *, field )
		{
			LineRelease( field->name );
			LineRelease( field->value );
			Release( field );
		}
		EmptyList( &pHttpState->cgi_fields );
	}
	unlockHttp( pHttpState );
}

PTEXT GetHttpContent( struct HttpState *pHttpState )
{
	lockHttp( pHttpState );
	if( pHttpState->read_chunks )
	{
		/* did a timeout happen? */
		if( pHttpState->content_length == pHttpState->read_chunk_total_length )
			return pHttpState->content;
		return NULL;
	}
	unlockHttp( pHttpState );

	if( pHttpState->content_length )
		return pHttpState->content;
	return NULL;
}

void ProcessHttpFields( struct HttpState *pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv )
{
	INDEX idx;
	struct HttpField *field;
	lockHttp( pHttpState );
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		f( psv, field->name, field->value );
	}
	unlockHttp( pHttpState );
}

void ProcessCGIFields( struct HttpState *pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv )
{
	INDEX idx;
	struct HttpField *field;
	lockHttp( pHttpState );
	LIST_FORALL( pHttpState->cgi_fields, idx, struct HttpField *, field )
	{
		f( psv, field->name, field->value );
	}
	unlockHttp( pHttpState );
}

PTEXT GetHttpField( struct HttpState *pHttpState, CTEXTSTR name )
{
	INDEX idx;
	struct HttpField *field;
	lockHttp( pHttpState );
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		if( StrCaseCmp( GetText( field->name ), name ) == 0 )
			return field->value;
	}
	unlockHttp( pHttpState );
	return NULL;
}

PTEXT GetHttpResponse( struct HttpState *pHttpState )
{
	if( pHttpState )
		return pHttpState->response_status;
	return NULL;
}

const char* GetHttpResponseStatus( HTTPState  pHttpState ) {
	if( pHttpState )
		return pHttpState->text_code;
	return NULL;
}

PTEXT GetHttpRequest( struct HttpState *pHttpState )
{
	if( pHttpState )
		return pHttpState->resource;
	return NULL;
}

PTEXT GetHttpResource( struct HttpState *pHttpState )
{
	if( pHttpState )
		return pHttpState->resource;
	return NULL;
}

PTEXT GetHttpMethod( struct HttpState *pHttpState )
{
	if( pHttpState )
		return pHttpState->method;
	return NULL;
}


void DestroyHttpStateEx( struct HttpState *pHttpState DBG_PASS )
{
	lockHttp( pHttpState );
	//_lprintf(DBG_RELAY)( "Destroy http state... (should clear content too? %p", pHttpState );
	EndHttp( pHttpState ); // empties variables
	DeleteList( &pHttpState->fields );
	DeleteList( &pHttpState->cgi_fields );
	VarTextDestroy( &pHttpState->pvtOut );
	VarTextDestroy( &pHttpState->pvt_collector );
	if( pHttpState->buffer )
		Release( pHttpState->buffer );
	unlockHttp( pHttpState );
	DeleteCriticalSec( &pHttpState->lock );
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

	vtprintf( pvt_message, "HTTP/1.1 %d %s\r\n", numeric, text );
	if( content_type && body )
	{
		vtprintf( pvt_message, "Content-Length: %d\r\n", GetTextSize(body));
		vtprintf( pvt_message, "Content-Type: %s\r\n"
				  , content_type?content_type
					:(tmp_content=GetHttpField( pHttpState, "Accept" ))?GetText(tmp_content)
					:"text/plain; charset=utf-8"  );
	}
	//else
	//	vtprintf( pvt_message, "%s\r\n", GetText( body ) );
	vtprintf( pvt_message, "Server: SACK Core Library 2.x\r\n"  );

	if( body )
		vtprintf( pvt_message, "\r\n"  );

	header = VarTextPeek( pvt_message );
	//offset += snprintf( message + offset, sizeof( message ) - offset, "%s",  "Body");
	if( l.flags.bLogReceived )
	{
		lprintf( "Sending response..." );
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

	vtprintf( pvt_message, "%s",  "HTTP/1.1 200 OK\r\n" );
	vtprintf( pvt_message, "Content-Length: %d\r\n", GetTextSize( body ));
	vtprintf( pvt_message, "Content-Type: %s\r\n"
		, (content_type = GetHttpField( pHttpState, "Accept" ))?GetText(content_type):"text/plain");
	vtprintf( pvt_message, "\r\n"  );
	vtprintf( pvt_message, "%s", GetText( body ));
	message = VarTextGet( pvt_message );
	if( l.flags.bLogReceived )
	{
		lprintf( " Response Message:");
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
#ifndef NO_SSL
		if( state && state->ssl )
		{
			PTEXT send = VarTextGet( state->pvtOut );
			if( l.flags.bLogReceived )
			{
				lprintf( "Sending Request..." );
				LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
			}
			// had to wait for handshake, so NULL event
			// on secure has already had time to build the send
			// but had to wait until now to do that.
			ssl_Send( pc, GetText( send ), GetTextSize( send ) );
			if( state->options && state->options->content && state->options->contentLen ) {
				ssl_Send( pc, state->options->content, state->options->contentLen );
				if( state->options->writeComplete ) {
					state->options->writeComplete( state->options->userData );
					state->options->writeComplete = NULL;
				}
			}

			LineRelease( send );
		}
		else
#endif
		{
			state->buffer = Allocate( 4096 );
			ReadTCP( pc, state->buffer, 4096 );
		}
	}
	else
	{
#ifdef _DEBUG
		if( l.flags.bLogReceived )
		{
			lprintf( "Received web request... %zu", size );
			LogBinary( (const uint8_t*) buffer, size );
		}
#endif
		//lprintf( "adding data:%d", size );
		if( AddHttpData( state, buffer, size ) )
			if( ProcessHttp( state, NULL, 0 ) ) // this shouldn't cause any auto send?
			{
				//lprintf( "this is where we should close and not end... %d %d",state->flags.close , !state->flags.keep_alive );
				if( state->flags.close || !state->flags.keep_alive) {
					RemoveClient( state->pc[0]);
					return;
				} else
					EndHttp( state );
			}
	}

	// read is handled by the SSL layer instead of here.  Just trust that someone will give us data later
	if( buffer && ( !state || ( state && !state->ssl ) ) )
	{
		ReadTCP( pc, state->buffer, 4096 );
	}
}

static void CPROC HttpReaderClose( PCLIENT pc )
{
	struct HttpState *data = (struct HttpState *)GetNetworkLong( pc, 0 );
	if( !data ) return;
	PCLIENT *ppc = data->pc;// (PCLIENT*)GetNetworkLong( pc, 0 );
	if( data->flags.no_content_length ) { // data is collected into 'partial' until close
		GatherHttpData( data );
		data->content_length = GetTextSize( data->partial );
		//lprintf( "at close what is content length? %d", data->content_length );
	}
	if( data->content_length ) {
		// should do one further gather; will set resulting status better.
		ProcessHttp( data, NULL, 0 );
	}
	//lprintf( "Closing http: %p ", pc );
	if( ppc[0] == pc ) {
		if( ppc )
			ppc[0] = NULL;
		//lprintf( "So now i's null?" );
		if( data->waiter ) {
			//lprintf( "(on close) Waking waiting to return with result." );
			data->closed = TRUE;
			WakeThread( data->waiter );
		}
	}
	else {
		lprintf( "Close resulting on a socket using the same state, but that state is now already busy." );
	}
	//if( !data->flags.success )
	//	DestroyHttpState( data );
}

static void CPROC HttpConnected( PCLIENT pc, int error ) {
	INDEX idx;
	struct pendingConnect *connect;
	//lprintf( "Connection for Http: %p", pc );
	while( 1 ) {
		LIST_FORALL( l.pendingConnects, idx, struct pendingConnect *, connect ) {
			if( connect->pc == pc ) {
				//lprintf( "Found pending connect(Http): %p %d", connect, idx );
				SetLink( &l.pendingConnects, idx, NULL );
				break;
			}
		}
		if( connect )
			break;
		else {
			AddLink( &l.activeConnects, pc );
			break;
		}
		Relinquish();
	}
	if( connect ) {
		SetNetworkLong( pc, 0, (uintptr_t)connect->state );
		Release( connect );
	}
	//lprintf( "Got connected... so connect gets released?");
}

HTTPState PostHttpQuery( PTEXT address, PTEXT url, PTEXT content )
{
	PCLIENT pc;
	struct pendingConnect *connect = New( struct pendingConnect );
	struct HttpState *state = CreateHttpState(&connect->pc);
	connect->pc = NULL;
	connect->state = state;
	state->closed = FALSE;
	//lprintf( "adding pending: %p", connect->pc );
	AddLink( &l.pendingConnects, connect );
	pc = OpenTCPClientExx( GetText( address ), 80, HttpReader, NULL, NULL, HttpConnected );
	connect->pc = pc;
	PVARTEXT pvtOut = VarTextCreate();
	vtprintf( pvtOut, "POST %s HTTP/1.1\r\n", url );
	vtprintf( pvtOut, "content-length:%d\r\n", GetTextSize( content ) );
	vtprintf( pvtOut, "\r\n\r\n" );
	VarTextAddData( pvtOut, GetText( content ), GetTextSize( content ) );
	if( pc )
	{
		PTEXT send = VarTextGet( pvtOut );
		state->request_socket = pc;
		state->pc = &state->request_socket;
		state->waiter = MakeThread();
		SetNetworkLong( pc, 0, (uintptr_t)state );
		SetNetworkCloseCallback( pc, HttpReaderClose );
		if( l.flags.bLogReceived )
		{
			lprintf( "Sending POST..." );
			LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
		}
		SendTCP( state->request_socket, GetText( send ), GetTextSize( send ) );
		LineRelease( send );
		while( state->request_socket )
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
		RemoveClient( pc );
	}
	if(0)
	{
		INDEX idx;
		struct pendingConnect *connect;
		//lprintf( "Connection for http: %p", pc );
		while( 1 ) {
			LIST_FORALL( l.pendingConnects, idx, struct pendingConnect *, connect ) {
				if( connect->pc == pc ) {
					//lprintf( "Found pending connect(http): %p %d", connect, idx );
					SetLink( &l.pendingConnects, idx, NULL );
					break;
				}
			}
			if( connect )
				break;
			else {
				AddLink( &l.activeConnects, pc );
				break;
			}
			Relinquish();
		}
		if( connect ){
			SetNetworkLong( pc, 0, (uintptr_t)connect->state );
			Release( connect );
		}
	}
}

HTTPState GetHttpQuery( PTEXT address, PTEXT url )
{
	int retries = 0;
	if( !address )
		return NULL;
	for( retries = 0; retries < 3; retries++ )
	{
		PCLIENT pc;
		SOCKADDR *addr = CreateSockAddress( GetText( address ), 443 );
		struct pendingConnect *connect = New( struct pendingConnect );
		struct HttpState *state = CreateHttpState( &connect->pc );
		connect->pc = NULL;
		connect->state = state;
		state->closed = FALSE;
		//lprintf( "adding pending2: %p", connect->pc );
		//AddLink( &l.pendingConnects, connect );
		pc = OpenTCPClientAddrExxx( addr, HttpReader, HttpReaderClose, NULL, httpConnected, 0 DBG_SRC );
		connect->pc = pc;
		ReleaseAddress( addr );
		if( pc ) {
			PVARTEXT pvtOut = VarTextCreate();
			const char* resource = GetText( url );
			if( !resource ) resource = "/";
			SetTCPNoDelay( pc, TRUE );
			vtprintf( pvtOut, "GET %s HTTP/1.0\r\n", resource );
			vtprintf( pvtOut, "Host: %s\r\n", GetText( address ) );
			//vtprintf( pvtOut, "connection: close\r\n" );
			vtprintf( pvtOut, "\r\n" );
			if( pc )
			{
				PTEXT send = VarTextGet( pvtOut );
				state->waiter = MakeThread();
				state->request_socket = pc;
				state->pc = &state->request_socket;
				SetNetworkLong( pc, 0, (uintptr_t)state );
				//SetNetworkCloseCallback( connect->pc, HttpReaderClose );
				if( l.flags.bLogReceived )
				{
					lprintf( "Sending GET..." );
					LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
				}
				SendTCP( pc, GetText( send ), GetTextSize( send ) );
				LineRelease( send );
				while( state->request_socket )
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

HTTPState GetHttpsQuery( PTEXT address, PTEXT url, const char* certChain )
{
	static struct HTTPRequestOptions defaultOpts = {
		"GET",
		NULL,
		NULL,
		NULL, NULL, 0,
		TRUE
	};
	return GetHttpsQueryEx( address, url, certChain, &defaultOpts );
}

static void writeComplete( PCLIENT pc, CPOINTER buffer, size_t length ) {
	struct HttpState* data = (struct HttpState*)GetNetworkLong( pc, 0 );
	if( data && data->options && data->options->writeComplete )
		data->options->writeComplete( data->options->userData );
}

HTTPState GetHttpsQueryEx( PTEXT address, PTEXT url, const char* certChain, struct HTTPRequestOptions* options )
{
	static struct HTTPRequestOptions defaultOpts = {
		"GET",  // method
		NULL,  // url 
		NULL,  // address (IP:PORT)
		NULL,  // headers
		NULL,  // content
		0,     // content length
		FALSE, // SSL
		"1.1", // HTTP Version ("1.1" default)
		3000, // timeout (3000 default)
		3, // retries (3 default)
	};
	if( !options ) options = &defaultOpts;
	int retries;
	if( !address )
		return NULL;
	if( !options->timeout ) options->timeout = 3000;
	if( !options->retries ) options->retries = 3;
	for( retries = 0; retries < options->retries; retries++ )
	{
		PCLIENT pc;
		SOCKADDR *addr = CreateSockAddress( GetText( address ), 443 );
		struct pendingConnect *connect = New( struct pendingConnect );
		struct HttpState *state = CreateHttpState( &connect->pc );
		state->options = options;
		state->closed = FALSE;
		connect->pc = NULL;
		connect->state = state;
		//lprintf( "adding pending3: %p", connect );
		//AddLink( &l.pendingConnects, connect );
		//lprintf( "added pending3" );
		if( retries ) {
			//lprintf( "HTTP(S) Query (retry):%s", GetText( url ) );
			//lprintf( "PC of connect:%p  %d", pc, retries );
		}
		//DumpAddr( "Http Address:", addr );
		pc = OpenTCPClientAddrExxx( addr, HttpReader, HttpReaderClose
				, writeComplete, httpConnected, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
		connect->pc = pc;
		//lprintf( "setting pending3: %p", connect->pc );
		ReleaseAddress( addr );
		if( pc )
		{
			char* header;
			INDEX idx;
			LOGICAL hadUserAgent = FALSE;
			const char* resource = GetText( url );
			if( !resource ) resource = "/";
			state->last_read_tick = timeGetTime();
			state->waiter = MakeThread();
			state->request_socket = connect->pc;
			state->pc = &state->request_socket;
			SetNetworkLong( pc, 0, (uintptr_t)state );

			//SetNetworkConn
			state->ssl = options->ssl;
			state->pvtOut = VarTextCreate();
			// 1.0 expects close after request - this is a one shot synchronous process so...
			vtprintf( state->pvtOut, "%s %s HTTP/%s\r\n", options->method, resource, options->httpVersion?options->httpVersion:"1.0" );
			// 1.1 would need this sort of header....
			//vtprintf( state->pvtOut, "connection: close\r\n" );
			if( options->content && options->contentLen ) {
				vtprintf( state->pvtOut, "Content-Length:%d\r\n", options->contentLen);
			}
			vtprintf( state->pvtOut, "Host:%s\r\n", GetText( address ) );
			
			LIST_FORALL( options->headers, idx, char*, header ) {
				if( !hadUserAgent && ( StrCaseCmpEx( header, "user-agent", 10 ) == 0 ) ) hadUserAgent = TRUE;
				vtprintf( state->pvtOut, "%s\r\n", header );
			}
			if( !hadUserAgent )
				vtprintf( state->pvtOut, "User-Agent:%s\r\n", options->agent?options->agent:"SACK(System)" );
			vtprintf( state->pvtOut, "\r\n" ); // send blank header
#ifndef NO_SSL
			if( options->ssl ) {
				if( ssl_BeginClientSession( pc, NULL, 0, NULL, 0, options->certChain?options->certChain:certChain, certChain
							? strlen( options->certChain ? options->certChain:certChain ) : 0 ) ) {
					state->waiter = MakeThread();
					if( !options->rejectUnauthorized )
						ssl_SetIgnoreVerification( pc );
					if( NetworkConnectTCP( pc ) < 0 ) {
						DestroyHttpState( state );
						return NULL;
					}
				} else
					RemoveClient( pc );
			} else
#endif

			if( pc ) {
				state->waiter = MakeThread();
				PTEXT send = VarTextPeek( state->pvtOut );

				if( NetworkConnectTCP( pc ) < 0 ) {
					DestroyHttpState( state );
					return NULL;
				}

				if( l.flags.bLogReceived ) {
					lprintf( "Sending %s...", options->method );
					LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
				}
				SendTCP( pc, GetText( send ), GetTextSize( send ) );
				if( options->content && options->contentLen )
					SendTCPLong( pc, options->content, options->contentLen );
			}

			// wait for response.
			while( state->request_socket && !state->closed
				&& ( state->last_read_tick > ( timeGetTime() - options->timeout ) ) ) {
				WakeableSleep( 1000 );
			}
			//lprintf( "Request has completed.... %p %p %d", pc, state->content, state->closed );
			if( state->request_socket && !state->closed ) {
				RemoveClient( state->request_socket ); // this shouldn't happen... it should have ben closed already.
			}

			VarTextDestroy( &state->pvtOut );
			if( !state->request_socket || state->closed )
				return state;
		}
		else
		{
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
	//PTEXT request = TextParse( pHttpState->response_status, "?#", " ", 1, 1 DBG_SRC );
	if( TextLike( method, "get" ) || TextLike( method, "post" ) )
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
			DECLTEXT( body, "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD><BODY>Resource handler not found" );
			SendHttpResponse( pHttpState, NULL, 404, "NOT FOUND", "text/html", (PTEXT)&body );
		}
		return 1;
	}
	else
		lprintf( "not a get or a post?" );

	//LineRelease( request );
	return 0;
}

static void CPROC HandleRequest( PCLIENT pc, POINTER buffer, size_t length )
{
	if( !buffer )
	{
		struct HttpState *pHttpStateServer = (struct HttpState *)GetNetworkLong( pc, 0 );
		struct HttpState *pHttpState = CreateHttpState( NULL );
		pHttpState->ssl = pHttpStateServer->ssl;
		buffer = pHttpState->buffer = Allocate( 4096 );
		pHttpState->request_socket = pc;
		//lprintf( "update pc here?" );
		pHttpState->pc = &pHttpState->request_socket;
		SetNetworkLong( pc, 1, (uintptr_t)pHttpState );
	}
	else
	{
		int result;
		struct HttpState *pHttpState = (struct HttpState *)GetNetworkLong( pc, 1 );
#ifdef _DEBUG
		if( l.flags.bLogReceived )
		{
			lprintf( "Received web request..." );
			LogBinary( (uint8_t*)buffer, length );
		}
#endif
		//lprintf( "RECEVED HTTP FROM NETWORK." );
		//LogBinary( buffer, length );
		AddHttpData( pHttpState, buffer, length );
		while( ( result = ProcessHttp( pHttpState, NULL, 0 ) ) )
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
	tnprintf( class_name, sizeof( class_name ), "SACK/Http/Methods/%s%s%s"
		, TargetName ? TargetName : ""
		, (TargetName && site) ? "/" : ""
		, site ? site : "" );
	//lprintf( "Server root = %s", class_name );
	server->methods = GetClassRoot( class_name );
	NetworkStart();
	server->server = OpenTCPListenerAddrEx( tmp = CreateSockAddress( interface_address ? interface_address : "0.0.0.0", 80 )
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
	tnprintf( class_name, sizeof( class_name ), "SACK/Http/Methods/%s%s%s"
			  , TargetName?TargetName:""
			  , ( TargetName && site )?"/":""
			  , site?site:"" );
	//lprintf( "Server root = %s", class_name );
	server->methods = GetClassRoot( class_name );
	NetworkStart();
	server->server = OpenTCPListenerAddrEx( tmp = CreateSockAddress( interface_address?interface_address:"0.0.0.0", 80 )
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
	if( pHttpState )
		return pHttpState->fields;
	return NULL;
}

int GetHttpVersion( HTTPState pHttpState ) {
	if( pHttpState )
		return pHttpState->response_version;
	return -1;
}

int GetHttpResponseCode( HTTPState pHttpState ) {
	if( pHttpState )
		return pHttpState->numeric_code;
	return -1;
}

HTTPState GetHttpState( PCLIENT pc ) {
	return (struct HttpState *)GetNetworkLong( pc, 1 );
}


HTTP_NAMESPACE_END
#undef l
#ifdef _MSC_VER
#  pragma warning( default:6011 26451)
#endif
