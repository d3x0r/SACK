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
	// how many bytes at the front of 'partial' the header scanner has already
	// looked at.  A read can end anywhere - including exactly on a line ending -
	// and everything that has not been recognized as a complete line yet stays in
	// 'partial'; the next ProcessHttp merges the new bytes onto the end of it and
	// has to resume where it stopped instead of re-scanning from 0 (re-scanning
	// re-counts the CR/LF that bLine already counted).
	size_t scanned;

	size_t content_length;
	PTEXT content; // content of the message, POST,PUT,PATCH and replies have this.
	LOGICAL returned_status;

	int final; // boolean flag - indicates that the header portion of the http request is finished.

	POINTER buffer; //for handling requests, have to read somewhere

	int numeric_code;
	int response_version;
	int request_version;
	TEXTSTR text_code;
	PCLIENT request_socket; // when a request comes in to the server, it is kept in a new http state, this is set for Send Response

	LOGICAL ssl;
	PVARTEXT pvtOut; // this is filled with a request ready to go out; used for HTTPS
	LOGICAL read_chunks;
	size_t read_chunk_byte;
	size_t read_chunk_length;
	size_t read_chunk_total_length;
	PVARTEXT pvt_chunk;
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
		// this state is a streaming connection (several requests on one socket)
		// rather than the one-shot GetHttpsQueryEx conversation.
		BIT_FIELD connection_mode : 1;
		// connected (and for TLS, handshaken); queued requests may be written.
		BIT_FIELD connection_ready : 1;
		// the opened callback has already been told; it only fires once.
		BIT_FIELD connection_opened : 1;
		// the opening byte of this message has been validated as plausible HTTP.
		// Cleared by EndHttp so every message on a kept-alive socket is checked.
		BIT_FIELD first_byte_checked : 1;
	}flags;
	CRITICALSECTION lock;
	struct HTTPRequestOptions* options;

	// --- streaming connection bookkeeping; all NULL/0 for a one-shot request ---
	PLINKQUEUE pending;   // struct httpConnectionRequest*, queued but not yet written
	PLINKQUEUE inflight;  // written, awaiting their response; oldest first
	int inflightCount;
	int pipelineDepth;    // how many requests may be on the wire at once
	PTEXT connectionAddress; // host[:port], reused for Host: on every request
	const char *connectionCertChain;
	httpConnectionOpened openedCallback;
	httpConnectionResponse responseCallback;
	httpConnectionClosed closedCallback;
	uintptr_t psvConnection;
	// options of the request currently being written; writeComplete belongs to
	// this one, not to the connection's own options.
	struct HTTPRequestOptions* requestOptions;
};

// One queued request on a streaming connection.  options is the caller's and is
// handed back to the response callback so it can match the reply to the request.
struct httpConnectionRequest {
	PTEXT url;
	struct HTTPRequestOptions *options;
};
// defined with the rest of the connection code, below the client section
static void httpReleaseConnectionRequest( struct httpConnectionRequest *req );

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
	//PLIST pendingConnects;
	//PLIST activeConnects;
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

void LockHttp( struct HttpState *state ) {
	EnterCriticalSec( &state->lock );
	//while( LockedExchange( &state->lock, 1 ) );
}

void UnlockHttp( struct HttpState *state ) {
	LeaveCriticalSec( &state->lock );
	//state->lock = 0;
}


void GatherHttpData( struct HttpState *pHttpState )
{
	if( pHttpState->flags.success ) // already gathered the required data.
		return;
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
			pHttpState->flags.no_content_length = 0;
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
		if( pHttpState->read_chunks )
		{
			PTEXT pMergedLine;
			PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
			PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
			pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
			LineRelease( pNewLine );
			pHttpState->partial = NULL;

			const uint8_t* buf = (const uint8_t*)GetText( pMergedLine );//(const uint8_t*)buffer;
			size_t size = GetTextSize( pMergedLine );
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
						pHttpState->read_chunk_byte = 0;
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
						LineRelease( pMergedLine );
						return;// FALSE;
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
						LineRelease( pMergedLine );
						return;// FALSE;
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
						LogBinary( buf-16, 16 );
						LogBinary( buf, size-ofs );
						TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
						unlockHttp( pHttpState );
						RemoveClient( pHttpState->request_socket );
						LineRelease( pMergedLine );
						return;// FALSE;
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
							// this would be last chunk (0\r\n) val (\r\n) data
							//
							pHttpState->flags.success = 1;
							pHttpState->content_length = GetTextSize( pHttpState->content  = VarTextGet( pHttpState->pvt_chunk ) );
							//lprintf( "This may or may not be the end of content? %d", pHttpState->content_length );
							if( pHttpState->waiter ) {
								WakeThread( pHttpState->waiter );
							}
							LineRelease( pMergedLine );
							unlockHttp( pHttpState );
							return;// TRUE;
						}
					}
					else
					{
						lprintf( "Chunk Processing Error expected \\n, found %d(%c)", buf[0], buf[0] );
						TriggerNetworkErrorCallback( pHttpState->request_socket, SACK_NETWORK_ERROR_HTTP_CHUNK );
						unlockHttp( pHttpState );
						RemoveClient( pHttpState->request_socket );
						LineRelease( pMergedLine );
						return;// FALSE;
					}
					break;
				case READ_BYTES:
					VarTextAddData( pHttpState->pvt_chunk, (CTEXTSTR)(buf), 1 );
					pHttpState->read_chunk_byte++;
					if( pHttpState->read_chunk_byte >= pHttpState->read_chunk_length ) {
						//lprintf( "Gathered the bytes required in the first packet... %d %zd", ofs, GetTextSize( VarTextPeek( pHttpState->pvt_chunk)));
						pHttpState->read_chunk_state = READ_CR;
					}
					break;
				}
				ofs++;
				buf++;
			}
			if( l.flags.bLogReceived ) {
				lprintf( "chunk read is %zd of %zd", pHttpState->read_chunk_byte, pHttpState->read_chunk_total_length );
			}
			// chunked reading is going to read the full block, and have no partial...
			LineRelease( pMergedLine );
			unlockHttp( pHttpState );
			//lprintf( "Gathered some of the chunk? %d %zd", ofs, GetTextSize( VarTextPeek( pHttpState->pvt_chunk)));
			return;// FALSE;
		}
		else{
			PTEXT pMergedLine;
			PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
			PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
			pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
			LineRelease( pNewLine );
			pHttpState->partial = pMergedLine;
			// Reaching here means a length is known and it is zero.  Taking
			// "however much has arrived" as the body is a RESPONSE rule - it is
			// for a peer that never said how long the content was - and applying
			// it to a REQUEST is what broke pipelining: GET and PUT request lines
			// set no_content_length=0 ("GET will never have a body?"), so every
			// GET swallowed whatever was still buffered as its own body.  With one
			// request per read that is an empty no-op, which is why it went
			// unnoticed; with requests coalesced into one read it eats the ones
			// behind it, and EndHttp's LineRelease( content ) then frees them.
			// Responses (including the 101 upgrade, whose trailing bytes belong to
			// the upgraded protocol) keep the old behaviour exactly.
			if( !pHttpState->flags.no_content_length && pHttpState->response_version ) {
				pHttpState->content = pHttpState->partial;
				pHttpState->content_length = GetTextSize( pHttpState->content );
				pHttpState->partial = NULL;
			}
		}
	}
	unlockHttp( pHttpState );
}


static PTEXT  resolvePercents( PTEXT urlword ) {
	PTEXT  url = BuildLine( urlword );
	LineRelease( urlword );
	//while( url = urlword )
	{
		char *_url = GetText(url);
		char *start = _url;
		TEXTRUNE ch;
		char *newUrl = start;
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
		SetTextSize( url, newUrl - start );
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
enum ProcessHttpResult ProcessHttp( struct HttpState *pHttpState, int ( *send )( uintptr_t psv, CPOINTER buf, size_t len ), uintptr_t psv )
{
	if( pHttpState->final )
	{
		{
			//lprintf( "Reading more, after returning a packet before...%d %d", pHttpState->response_version, pHttpState->request_version );
			if( pHttpState->response_version ) {
				lockHttp( pHttpState );
				GatherHttpData( pHttpState );
				//lprintf( "return http nothing  %d %d %d", pHttpState->content_length, pHttpState->flags.success, pHttpState->returned_status );
				if( pHttpState->flags.success && !pHttpState->returned_status ) {
					unlockHttp( pHttpState );
					pHttpState->returned_status = 1;
					return (enum ProcessHttpResult)pHttpState->numeric_code;
				}
				unlockHttp( pHttpState );
			}
			else /* if( httpState->request_version ) */ {
				// this is a request not a response we are processing...
				if( pHttpState->content_length ) {
					lockHttp( pHttpState );
					GatherHttpData( pHttpState );
					if( ((GetTextSize( pHttpState->partial ) >= pHttpState->content_length)
						|| (GetTextSize( pHttpState->content ) >= pHttpState->content_length))
						) {
						unlockHttp( pHttpState );
						// prorbably a POST with a body?
						// had to gather the body...
						return HTTP_STATE_RESULT_CONTENT;
					}
					unlockHttp( pHttpState );
				}
			}
		}
		//lprintf( "return http nothing  %d %d %d", pHttpState->content_length, pHttpState->flags.success, pHttpState->returned_status );
		return HTTP_STATE_RESULT_NOTHING;
	}
	else
	{
		PTEXT pCurrent;//, pStart;
		PTEXT pLine = NULL;
		TEXTCHAR *c, *line;
		size_t size, pos;
		INDEX start = 0;
		PTEXT pMergedLine;
		// lock across the whole merge+parse: the prologue below consumes
		// pvt_collector and replaces pHttpState->partial, and a concurrent
		// EndHttp / second ProcessHttp on the same state (JS end() thread)
		// frees the old partial - pCurrent would dangle (use-after-free).
		lockHttp( pHttpState );
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		pCurrent = pHttpState->partial;
		//pStart = pCurrent; // at lest is this block....
		//lprintf( "ND THIS IS WHAT WE PROCESSL:" );
		//LogBinary( (const uint8_t*)GetText( pInput ), GetTextSize( pInput ) );

		// we always start without having a line yet, because all input is already merged
		{
			//lprintf( "process HTTP: %s %d", GetText( pCurrent ), pHttpState->bLine );
			size = GetTextSize( pCurrent );
			if( !size ) { unlockHttp( pHttpState ); return HTTP_STATE_RESULT_NOTHING; }
			c = GetText( pCurrent );
			if( pHttpState->bLine < 4 )
			{
				//start = 0; // new packet and still collecting header....
				// Resume where the previous read stopped.  Everything before 'scanned'
				// has already been counted into bLine (and any complete line before it
				// was split off), so re-examining it would count its CR/LF twice - which
				// is what used to swallow the status line whenever a read ended exactly
				// after its CRLF (sqlite.org's 503 arrives that way).
				pos = ( pHttpState->scanned <= size ) ? pHttpState->scanned : size;
				for( ; ( pos < size ) && !pHttpState->final; pos++ )
				{
					if( c[pos] == '\r' )
						if( !(pHttpState->bLine & 1 ) )
							pHttpState->bLine++;
						else
							pHttpState->bLine=0;
					else if( c[pos] == '\n' )
						if( pHttpState->bLine & 1 )
							pHttpState->bLine++;
						else
							pHttpState->bLine=0;
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
									// these fields are kept for some things like websockets, until they are closed...
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
									lprintf( "Header field [%.*s] invalid(%.*s)", (int)GetTextSize(pLine),GetText( pLine ), (int)GetTextSize(pCurrent), GetText( pCurrent ) );
									LineRelease( pLine );
								}
							}
							else
							{
								//lprintf( "Parsing http state request for something.." );
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
										/* Methods with no request body.  These used to fall off the end of this
						 * chain leaving numeric_code 0, so the request never completed and the
						 * connection sat there - a plain OPTIONS preflight was indistinguishable
						 * from junk.  They only need to reach the app; what it does with CONNECT
						 * or TRACE is the app's decision, not the parser's. */
						if( TextSimilar( request, "GET" )
						 || TextSimilar( request, "HEAD" )
						 || TextSimilar( request, "OPTIONS" )
						 || TextSimilar( request, "DELETE" )
						 || TextSimilar( request, "TRACE" )
						 || TextSimilar( request, "CONNECT" ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
											//GET will never have a body?
											pHttpState->flags.no_content_length = 0;
										}
										else if( TextSimilar( request, "PUT" ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
											//GET will never have a body?
											pHttpState->flags.no_content_length = 0;
										}
										/* may carry a body - same handling POST already had */
						else if( TextSimilar( request, "POST" )
						      || TextSimilar( request, "PATCH" ) )
										{
											pHttpState->numeric_code = HTTP_STATE_RESULT_CONTENT; // initialize to assume it's incomplete; NOT OK.  (requests should be OK)
											request = NEXTLINE( request );
											pHttpState->method = SegBreak( request );
											// a post could have a body, and we should wait for it?
											//pHttpState->flags.no_content_length = 0;
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
											// this describes a GET or POST request; an HTTP response would be handled above, and would be before or equal to 'request'
											//lprintf( "word %s", GetText( tmp ) );
											next = NEXTLINE( tmp );
											//lprintf( "Line : %s", GetText( pLine ) );
											if( !pHttpState->response_version && TextSimilar( tmp, "HTTP/" ) )
											{
												TEXTCHAR *tmp2 = (TEXTCHAR*)StrChr( GetText( tmp ), '.' );
												if( tmp2 )
													pHttpState->request_version = (int)(( IntCreateFromText( GetText( tmp ) + 5 ) * 100 ) + IntCreateFromText( tmp2 + 1 ));
												else if( GetTextSize( tmp ) > 5 )
													pHttpState->request_version = (int)( IntCreateFromText( GetText( tmp ) + 5 ) * 100 );
												else
													pHttpState->request_version = 0;

												if( pHttpState->request_version >= 101 ) {
													pHttpState->flags.close = 0;
													pHttpState->flags.keep_alive = 1;
												}
												else if( pHttpState->request_version == 100 ) {
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
						else // 0 or 1
							pHttpState->bLine = 0;
						// may not receive anything other than header information?
						if( pHttpState->bLine == 4 )
						{
							// end of header (after finalcheck:)
							// copy the previous line out...
							//pStart = pCurrent;
							//len = size - pos; // remaing size
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
				// what is left in 'partial' after the split below is [start,pos); that
				// much has been scanned, and the next read continues from its end.
				// (when the header ended, start==pos and this is 0 - what follows is
				// body, which this loop does not scan.)
				pHttpState->scanned = pos - start;
			}
			//else
			//	len += size;
			//pCurrent = NEXTLINE( pCurrent );
			/* Move the remaining data into a single binary data packet...*/
		}
		if( start )
		{
			/*PTEXT tmp = */SegSplit( &pCurrent, start );
			pHttpState->partial = NEXTLINE( pCurrent );
			//lprintf( "Partial HTTP data put back...");
			//LogBinary( (const uint8_t*)GetText( pHttpState->partial ), GetTextSize( pHttpState->partial ) );
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
					else if( StrCaseStr( GetText( field->value ), "close" ) ) {
						// the close defines the length of content...
						// ... but only when reading a RESPONSE.  On a request this
						// clobbers the no_content_length=0 that the GET/PUT request-line
						// parse just set, so the dispatch gate below never fires and the
						// request is never delivered - the server sits waiting for a body
						// that a GET/PUT is never going to send.  (node's http client sends
						// 'Connection: close' whenever it isn't using a keep-alive agent.)
						if( pHttpState->response_version )
							if( !pHttpState->content_length ) // might have length already specified...
								pHttpState->flags.no_content_length = 1;
					}
				}
				else if( TextLike( field->name, "Transfer-Encoding" ) )
				{
					if( TextLike( field->value, "chunked" ) )
					{
						//lprintf( "Setup state to read chunked data - final should still be set? %d"
						//		, pHttpState->final );
						pHttpState->content_length = 0;
						pHttpState->flags.no_content_length = 0;
						pHttpState->read_chunks = TRUE;
						pHttpState->read_chunk_state = READ_VALUE;
						pHttpState->read_chunk_length = 0;
						pHttpState->read_chunk_total_length = 0;
						pHttpState->pvt_chunk = VarTextCreate();
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
		( ( ( !pHttpState->read_chunks ) &&
			( ( pHttpState->content_length
				&& ( ( GetTextSize( pHttpState->partial ) >= pHttpState->content_length )
					||( GetTextSize( pHttpState->content ) >= pHttpState->content_length ) ) )
				|| ( !pHttpState->content_length && !pHttpState->flags.no_content_length )
				) )
		// A chunked response never satisfied the length tests above (its length is
		// only known after de-chunking), so this gate could not fire for it and
		// returned_status was never set - the blocking client in GetHttpsQueryEx then
		// waited out its ENTIRE timeout budget on every chunked response, and the
		// reader never woke it.  flags.success is set exactly when the terminating
		// zero-length chunk is consumed, so that is the chunked completion signal.
		|| ( pHttpState->read_chunks && pHttpState->flags.success )
		) )
	{
		pHttpState->returned_status = 1;
		//lprintf( "return http %d l:%d nl:%d",pHttpState->numeric_code, pHttpState->content_length, pHttpState->flags.no_content_length );
		if( pHttpState->numeric_code == 500 )
			return HTTP_STATE_INTERNAL_SERVER_ERROR;
		if( pHttpState->content && ( (pHttpState->numeric_code == 201) || (pHttpState->numeric_code == 200) ) ) {
			pHttpState->final = FALSE; // this returned the correct status for this; no new data, 
			                           // next should be a NEW request... (or end of stream)
			return HTTP_STATE_RESULT_CONTENT;
		}
		if( pHttpState->numeric_code == 100 )
			return HTTP_STATE_RESULT_CONTINUE;
		if( pHttpState->numeric_code == 404 )
			return HTTP_STATE_RESOURCE_NOT_FOUND;
		if( pHttpState->numeric_code == 400 )
			return HTTP_STATE_BAD_REQUEST;
		return (enum ProcessHttpResult)pHttpState->numeric_code;
	}
	//lprintf( "return http nothing" );
	return HTTP_STATE_RESULT_NOTHING;
}

LOGICAL AddHttpData( struct HttpState *pHttpState, CPOINTER buffer, size_t size )
{
	lockHttp( pHttpState );
	//lprintf( "AddHttpData:%d", size );
	pHttpState->last_read_tick = timeGetTime();
	/* Is this even HTTP?  Nothing downstream asks: ProcessHttp scans for CR/LF and
	 * nothing else, so a stream that never produces one - a TLS ClientHello arriving
	 * on a plain listener is the ordinary case - accumulates in 'partial' forever and
	 * the connection just hangs until the peer gives up.  A TLS client will not give
	 * up; it is waiting for a ServerHello.
	 *
	 * Every method's first letter, plus 'H' for the HTTP/x.y response line this same
	 * parser reads on the client side:
	 *   Connect Delete Get Head Options Post Put Patch Trace  ->  C D G H O P T
	 * A TLS record starts 0x16, an SSH banner 'S' - both refused.  Returning FALSE
	 * means "not HTTP, drop the socket"; the callers close without answering, because
	 * writing a TLS alert back would claim a security layer this socket never had. */
	if( !pHttpState->flags.first_byte_checked && size ) {
		const unsigned char *scan = (const unsigned char *)buffer;
		size_t n;
		/* RFC 7230 3.5 - tolerate leading CRLF before a request line.  If this read is
		 * nothing but line endings, stay undecided and check the next one. */
		for( n = 0; n < size && ( scan[n] == 13 || scan[n] == 10 ); n++ );  /* CR, LF */
		if( n < size ) {
			unsigned char c0 = scan[n];
			pHttpState->flags.first_byte_checked = 1;
			if( c0 != 'C' && c0 != 'D' && c0 != 'G' && c0 != 'H'
			 && c0 != 'O' && c0 != 'P' && c0 != 'T' ) {
				unlockHttp( pHttpState );
				return FALSE;
			}
		}
	}
	{
		//lprintf( "Add HTTP Data:%p %d", pHttpState->pc[0], size );
		//LogBinary( (uint8_t*)buffer, 256>size?size:256 );
		if( size )
			VarTextAddData( pHttpState->pvt_collector, (CTEXTSTR)buffer, size );
		unlockHttp( pHttpState );
		if(0)
		if( pHttpState->final ) {
			// this will cause it to wait until 'endhttp' to process next block.
			//lprintf( "still handling a previous request in add data", pHttpState->pc[0] );
			return FALSE;
		}
		return TRUE;
	}
}

struct HttpState *CreateHttpState( PCLIENT *ppc )
{
	struct HttpState *pHttpState;

	pHttpState = New( struct HttpState );
	MemSet( pHttpState, 0, sizeof( struct HttpState ) );
	InitializeCriticalSec( &pHttpState->lock );
	pHttpState->flags.no_content_length = 1;
	if (ppc)
		pHttpState->pc = ppc;
	else
		pHttpState->pc = &pHttpState->request_socket;

	pHttpState->pvt_collector = VarTextCreate();
	//pHttpState->pc = pc;
	return pHttpState;
}


void EndHttp( struct HttpState *pHttpState )
{
	//lprintf( "Ending HTTP %p", pHttpState );
	lockHttp( pHttpState );
	pHttpState->bLine = 0;
	// whatever is left in 'partial' is the start of the NEXT message (see below),
	// and none of it has been scanned as header yet.
	pHttpState->scanned = 0;
	pHttpState->final = 0;
	pHttpState->response_version = 0;
	pHttpState->request_version = 0;
	pHttpState->flags.no_content_length = 1;
	pHttpState->content_length = 0;
	pHttpState->flags.success = 0;
	// next message on this socket gets its own opening-byte check
	pHttpState->flags.first_byte_checked = 0;
	LineRelease( pHttpState->method );
	pHttpState->method = NULL;
	LineRelease( pHttpState->content );
	LineRelease( pHttpState->resource );
	pHttpState->resource = NULL;
	// Whatever is left in 'partial' once the content has been split out of it is
	// the beginning of the NEXT message - the peer coalesced it into the same
	// read.  This used to be released here, which is what made pipelining lose
	// requests: a client that packs several requests into one segment got only
	// the first one answered and the rest silently dropped (measurable as the
	// server jumping from request 0 straight to request 4).  Keeping it is what
	// makes the ProcessHttp re-parse loops after EndHttp able to find anything;
	// the next ProcessHttp merges new input onto the end of it.
	// GatherHttpData SegGrab'd it off the content chain, so it is independent of
	// the LineRelease( content ) above.
	// DestroyHttpStateEx releases it explicitly, since there is no 'next message'
	// at teardown.
	if( pHttpState->partial == pHttpState->content )
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
			SetLink(&pHttpState->fields, idx, NULL);
			LineRelease( field->name );
			LineRelease( field->value );
			Release( field );
		}
		EmptyList( &pHttpState->fields );
		LIST_FORALL( pHttpState->cgi_fields, idx, struct HttpField *, field )
		{
			SetLink(&pHttpState->cgi_fields, idx, NULL);
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
	PTEXT result = NULL;
	lockHttp( pHttpState );
	if( pHttpState->read_chunks )
	{
		/* did a timeout happen? */
		if( pHttpState->content_length == pHttpState->read_chunk_total_length )
			result = pHttpState->content;
	}
	else if( pHttpState->content_length )
		result = pHttpState->content;
	unlockHttp( pHttpState );
	return result;
}

void ProcessHttpFields( struct HttpState *pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv )
{
	INDEX idx;
	struct HttpField *field;
	if( !pHttpState ) return;
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
	if( !pHttpState ) return;
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
	PTEXT result = NULL;
	lockHttp( pHttpState );
	if( pHttpState->fields )
		LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
		{
			if( StrCaseCmp( GetText( field->name ), name ) == 0 ) {
				result = field->value;
				break;
			}
		}
	unlockHttp( pHttpState );
	return result;
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

void ShutdownHttpStateEx( struct HttpState *pHttpState DBG_PASS ) {
	lockHttp( pHttpState );
	RemoveClient( pHttpState->pc[0] );
	unlockHttp(pHttpState );	
}

void DestroyHttpStateEx( struct HttpState *pHttpState DBG_PASS )
{
	lockHttp( pHttpState );
	if( pHttpState->pc ) {
		if( ((uintptr_t)pHttpState) == GetNetworkLong( pHttpState->pc[0], 0 ) )
			SetNetworkLong( pHttpState->pc[0], 0, 0 );
	}

	//_lprintf(DBG_RELAY)( "Destroy http state... (should clear content too? %p", pHttpState );
	EndHttp( pHttpState ); // empties variables
	// EndHttp deliberately keeps any bytes of a following message; at teardown
	// there is no following message.
	LineRelease( pHttpState->partial );
	pHttpState->partial = NULL;
	//lprintf( "Fields should have been emptied already?" );
	DeleteList( &pHttpState->fields );
	DeleteList( &pHttpState->cgi_fields );
	VarTextDestroy( &pHttpState->pvtOut );
	VarTextDestroy( &pHttpState->pvt_collector );
	VarTextDestroy( &pHttpState->pvt_chunk );
	if( pHttpState->buffer )
		Release( pHttpState->buffer );
	if( pHttpState->flags.connection_mode ) {
		struct httpConnectionRequest *req;
		while( ( req = (struct httpConnectionRequest*)DequeLink( &pHttpState->inflight ) ) )
			httpReleaseConnectionRequest( req );
		while( ( req = (struct httpConnectionRequest*)DequeLink( &pHttpState->pending ) ) )
			httpReleaseConnectionRequest( req );
		DeleteLinkQueue( &pHttpState->inflight );
		DeleteLinkQueue( &pHttpState->pending );
		LineRelease( pHttpState->connectionAddress );
		pHttpState->connectionAddress = NULL;
	}
	unlockHttp( pHttpState );
	DeleteCriticalSec( &pHttpState->lock );
	Release( pHttpState );
}

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

// Write the request already formatted into state->pvtOut, plus any content, and
// consume the vartext.  There are two moments this can happen and they used to be
// two copies of this code: plain sockets send inline right after NetworkConnectTCP,
// while TLS has to wait for the handshake and sends from HttpReader's initial-read
// callback (which is why pvtOut has to outlive the plain-path send).  Both call this
// now, which is also what lets a second request go out on a connection already up.
// The content paths differ deliberately: SendTCPLong hands the buffer to the network
// layer (writeComplete fires when it drains) while ssl_Send copies, so the TLS path
// has to signal writeComplete itself.
// httpOpenSocket wires up all four socket callbacks, and they are all defined
// further down this file, so they need declaring here.
static void CPROC HttpReader( uintptr_t psv, POINTER buffer, size_t size );
static void CPROC HttpReaderClose( uintptr_t psv );
static void httpConnected( uintptr_t psv, int error );
static void writeComplete( uintptr_t psv, CPOINTER buffer, size_t length );
// defined with the request formatting, down by GetHttpsQueryEx; the connection
// code up here needs it to format requests 2..N.
static void httpBuildRequest( struct HttpState *state, PTEXT address, PTEXT url
                            , struct HTTPRequestOptions *options );

// Create the socket for an HTTP conversation and attach the state to it.  The
// connect is deliberately deferred (OPEN_TCP_FLAG_DELAY_CONNECT) so the caller can
// format the first request - and for TLS begin the session - before the handshake
// starts; HttpReader's initial-read callback is what sends it on the TLS path.
// Returns NULL if the socket could not be created; the caller still owns state.
static PCLIENT httpOpenSocket( PTEXT address, struct HttpState *state, struct HTTPRequestOptions *options ) {
	PCLIENT pc;
	SOCKADDR *addr = CreateSockAddressV2( GetText( address ), options->ssl?443:80, options->addrFlags );
	options->connectError = 0; // clear any previous error.
	pc = CPPOpenTCPClientAddrExxx( addr, HttpReader, (uintptr_t)state, HttpReaderClose, (uintptr_t)state
			, writeComplete, (uintptr_t)state, httpConnected, (uintptr_t)state, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	SetTCPNoDelay( pc, TRUE );
	state->request_socket = pc;
	ReleaseAddress( addr );
	if( pc ) {
		state->last_read_tick = timeGetTime();
		state->waiter = MakeThread();
		SetNetworkLong( pc, 0, (uintptr_t)state );
		state->ssl = options->ssl;
	}
	return pc;
}

static LOGICAL httpSendRequest( struct HttpState *state, struct HTTPRequestOptions *options ) {
	PCLIENT pc = state->request_socket;
	PTEXT send;
	if( !pc || !state->pvtOut ) return FALSE;
	send = VarTextGet( state->pvtOut );  // consumes the accumulated text
	if( !send ) return FALSE;
	if( l.flags.bLogReceived ) {
		lprintf( "Sending %s...", options ? options->method : "request" );
		LogBinary( (uint8_t*)GetText( send ), GetTextSize( send ) );
	}
	if( state->ssl ) {
		ssl_Send( pc, GetText( send ), GetTextSize( send ) );
		if( options && options->content && options->contentLen ) {
			ssl_Send( pc, options->content, options->contentLen );
			if( options->writeComplete ) {
				options->writeComplete( options->userData );
				options->writeComplete = NULL;
			}
		}
	} else {
		SendTCP( pc, GetText( send ), GetTextSize( send ) );
		if( options && options->content && options->contentLen )
			SendTCPLong( pc, options->content, options->contentLen );
	}
	LineRelease( send );
	return TRUE;
}

//---------- streaming connections ------------------------------
// A streaming connection is one HttpState whose parse state is reused for reply
// after reply, with the requests it is answering kept in two queues: 'pending'
// (queued, not written) and 'inflight' (written, waiting).  HTTP/1.1 gives no
// way to correlate a reply with a request other than order, so the head of
// 'inflight' is by definition whose reply just arrived.

static void httpReleaseConnectionRequest( struct httpConnectionRequest *req ) {
	if( !req ) return;
	LineRelease( req->url );
	Release( req );
}

// Write as many queued requests as the pipeline depth allows.  Called when the
// connection comes up, when a new request is queued, and after each reply.
static void httpFlushRequests( struct HttpState *state ) {
	if( !state->flags.connection_ready ) return;
	while( state->request_socket && !state->closed
	     && state->inflightCount < state->pipelineDepth ) {
		struct httpConnectionRequest *req = (struct httpConnectionRequest*)DequeLink( &state->pending );
		if( !req ) break;
		state->requestOptions = req->options;
		httpBuildRequest( state, state->connectionAddress, req->url, req->options );
		if( !httpSendRequest( state, req->options ) ) {
			// socket went away between the check and the write; put it back so
			// the close path reports it like any other outstanding request.
			VarTextDestroy( &state->pvtOut );
			PrequeLink( &state->inflight, req );
			state->inflightCount++;
			break;
		}
		VarTextDestroy( &state->pvtOut );
		EnqueLink( &state->inflight, req );
		state->inflightCount++;
	}
}

// The connection is up (TLS: handshake done) - tell the owner once, then let
// anything queued while we were connecting go out.
static void httpConnectionReady( struct HttpState *state ) {
	state->flags.connection_ready = 1;
	if( !state->flags.connection_opened ) {
		state->flags.connection_opened = 1;
		if( state->openedCallback )
			state->openedCallback( state->psvConnection, state, 0 );
	}
	httpFlushRequests( state );
}

// Hand the reply sitting in the parse state to the owner, then reset the state
// for the next one.  EndHttp keeps any bytes of a following reply that arrived
// in the same read, which is what lets the caller loop for another one.
static void httpDeliverResponse( struct HttpState *state ) {
	struct httpConnectionRequest *req = (struct httpConnectionRequest*)DequeLink( &state->inflight );
	if( state->inflightCount ) state->inflightCount--;
	state->requestOptions = NULL;
	if( state->responseCallback )
		state->responseCallback( state->psvConnection, state, req ? req->options : NULL );
	httpReleaseConnectionRequest( req );

	EndHttp( state );
	// EndHttp resets the parse, but not the 'a reply was returned' latch that
	// ProcessHttp checks before returning another one.
	state->returned_status = 0;
}

// Everything queued or unanswered is reported with the parse state empty (so
// GetHttpResponseCode reads 0), which is how the owner learns those requests
// will never be answered.
static void httpFailOutstanding( struct HttpState *state ) {
	struct httpConnectionRequest *req;
	while( ( req = (struct httpConnectionRequest*)DequeLink( &state->inflight ) ) ) {
		if( state->responseCallback )
			state->responseCallback( state->psvConnection, state, req->options );
		httpReleaseConnectionRequest( req );
	}
	while( ( req = (struct httpConnectionRequest*)DequeLink( &state->pending ) ) ) {
		if( state->responseCallback )
			state->responseCallback( state->psvConnection, state, req->options );
		httpReleaseConnectionRequest( req );
	}
	state->inflightCount = 0;
}

static void CPROC HttpReader( uintptr_t psv, POINTER buffer, size_t size )
{
	struct HttpState *state = (struct HttpState *)psv;
	PCLIENT pc = state->pc[0];
	if( !buffer )
	{
		//lprintf( "Initial read on HTTP requestor" );
#ifndef NO_SSL
		if( state && state->ssl )
		{
			// had to wait for handshake, so NULL event
			// on secure has already had time to build the send
			// but had to wait until now to do that.
			if( state->flags.connection_mode )
				httpConnectionReady( state );
			else
				httpSendRequest( state, state->options );
		}
		else
#endif
		if( state ) {
			state->buffer = Allocate( 4096 );
			ReadTCP( pc, state->buffer, 4096 );
			// a plain socket is writable as soon as it is connected, but this
			// is the point the read side is armed, so it is the same signal for
			// both transports and keeps the connection paths symmetric.
			if( state->flags.connection_mode )
				httpConnectionReady( state );
		} else {
			lprintf( "Initial read on http with no state set?" );
		}
	}
	else
	{
#ifdef _DEBUG
		if( l.flags.bLogReceived )
		{
			lprintf( "Received web data... %zu", size );
			LogBinary( (const uint8_t*) buffer, size );
		}
#endif
		if( !state ) {
			lprintf( "Http state was stolen before the read into it?" );
			
		} else if( AddHttpData( state, buffer, size ) ) {
			enum ProcessHttpResult r;
			if( state->flags.connection_mode ) {
				// A single read can carry more than one reply, so keep parsing
				// until the buffered bytes stop completing one.
				while( ( r = ProcessHttp( state, NULL, 0 ) ) ) {
					LOGICAL closing = state->flags.close || !state->flags.keep_alive;
					httpDeliverResponse( state );
					if( closing ) {
						// peer is done with this connection; whatever is still
						// queued gets failed by the close callback.
						RemoveClient( state->pc[0] );
						return;
					}
				}
				httpFlushRequests( state );
			}
			else if( ( r = ProcessHttp( state, NULL, 0 ) ) ) // this shouldn't cause any auto send?
			{
				//lprintf( "this is where we should close and not end...%d %d %d",r, state->flags.close , !state->flags.keep_alive );
				if( state->flags.close || !state->flags.keep_alive) {
					RemoveClient( state->pc[0]);
					if( state->waiter )
						WakeThread( state->waiter );
					return;
				} else {
					//lprintf( "Waking up on response received?", state->flags.success, state->returned_status );
					if( state->waiter )
						WakeThread( state->waiter );
					//EndHttp( state );
					if( state->flags.keep_alive ) {

					}
				}
			}
		}
	}

	// read is handled by the SSL layer instead of here.  Just trust that someone will give us data later
	if( buffer && state && !state->ssl )
	{
		ReadTCP( pc, state->buffer, 4096 );
	}
}

static void CPROC HttpReaderClose( uintptr_t psv )
{
	struct HttpState *data = (struct HttpState *)psv;
	if( !data ) return;
	PCLIENT *ppc = data->pc;// (PCLIENT*)GetNetworkLong( pc, 0 );
	//lprintf( "HTTP Socket Close Event" );
	if( data->flags.no_content_length ) { // data is collected into 'partial' until close
		//lprintf( "HTTP Close event... collecting data...");
		GatherHttpData( data );
		data->content_length = GetTextSize( data->partial );
		//lprintf( "at close what is content length? %p %d", data, data->content_length );
	}
	if( data->content_length ) {
		// should do one further gather; will set resulting status better.
		ProcessHttp( data, NULL, 0 );
	}
	//lprintf( "Closing http: %p ", pc );
	if( ppc )
		ppc[0] = NULL;
	data->closed = TRUE;
	data->flags.connection_ready = 0;
	if( data->flags.connection_mode ) {
		// a reply with no content-length is only complete at the close; the
		// gather above finished it, so hand it over before failing the rest.
		if( data->returned_status && data->inflightCount )
			httpDeliverResponse( data );
		if( !data->flags.connection_opened ) {
			// never got up in the first place - report the connect failure
			// through the same callback a successful open would have used.
			data->flags.connection_opened = 1;
			if( data->openedCallback )
				data->openedCallback( data->psvConnection, data
				                    , data->options && data->options->connectError
				                      ? data->options->connectError : -1 );
		}
		httpFailOutstanding( data );
		if( data->closedCallback )
			data->closedCallback( data->psvConnection, data );
		return;
	}
	if( data->waiter ) {
		//lprintf( "(on close) Waking waiting to return with result." );
		WakeThread( data->waiter );
	}
}

static void CPROC HttpConnected( uintptr_t psv, int error ) {
//	struct HttpState *state = (struct HttpState*)psv;
//	PCLIENT pc = state->requestSocket;
#if 0
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
	} else {
		lprintf( "Pending connect didn't have a connection; so we didn't set a http State" );
	}
#endif
	//lprintf( "Got connected... so connect gets released?");
}

HTTPState PostHttpQuery( PTEXT address, PTEXT url, PTEXT content )
{
	PCLIENT pc;
	//struct pendingConnect *connect = New( struct pendingConnect );
	struct HttpState *state = CreateHttpState(NULL);
	//connect->pc = NULL;
	//connect->state = state;
	state->closed = FALSE;
	//lprintf( "adding pending: %p", connect->pc );
	//AddLink( &l.pendingConnects, connect );
	pc = CPPOpenTCPClientExx( GetText( address ), 80, HttpReader, (uintptr_t)state
					, NULL, 0, NULL, 0, HttpConnected, (uintptr_t)state
					, 0 );
	//connect->pc = pc;
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
		SetCPPNetworkCloseCallback( pc, HttpReaderClose, (uintptr_t)state );
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

static void httpConnected( uintptr_t psv, int error ) {
	struct HttpState *pHttpState = (struct HttpState *)psv;//GetNetworkLong( pc, 0 );
	if( pHttpState ) {
		if( error ) {
			pHttpState->options->connectError = error;
			lprintf( "This is a request, and it failed with error %d", error );
			RemoveClient( pHttpState->pc[0] );
		}
		else {
			pHttpState->options->connected = TRUE;
		}	
	} else {
		lprintf( "Client in connected should already have a state set too...." );
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
		//struct pendingConnect *connect = New( struct pendingConnect );
		struct HttpState *state = CreateHttpState( NULL);
		//connect->pc = NULL;
		//connect->state = state;
		state->closed = FALSE;
		//lprintf( "adding pending2: %p", connect->pc );
		//AddLink( &l.pendingConnects, connect );
		pc = CPPOpenTCPClientAddrExxx( addr, HttpReader, (uintptr_t)state
					, HttpReaderClose, (uintptr_t)state
					, NULL, 0
					, httpConnected, (uintptr_t)state
					, 0 DBG_SRC );
		state->request_socket = pc;
		//connect->pc = pc;
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

void httpSSLError( uintptr_t psv, PCLIENT pc, SackNetworkError error, ... ) {
	lprintf( "SSL Level Error: %d (unhandled)", error );
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

static void writeComplete( uintptr_t psv, CPOINTER buffer, size_t length ) {
	struct HttpState* data = (struct HttpState*)psv;//GetNetworkLong( pc, 0 );
	// on a streaming connection the content that just drained belongs to the
	// request being written, not to the connection's own options.
	struct HTTPRequestOptions *options = data ? ( data->requestOptions ? data->requestOptions : data->options ) : NULL;
	if( options && options->writeComplete )
		options->writeComplete( options->userData );
}

// Format one request into state->pvtOut: request line, Host, the caller's headers
// (noting Connection/User-Agent/Content-Length as they go by so the defaults below
// do not duplicate them), then the defaults and the terminating blank line.
// This is the only part of issuing a request that is not connection state, which is
// what makes it reusable for sending several requests on one connection.
static void httpBuildRequest( struct HttpState *state, PTEXT address, PTEXT url
                            , struct HTTPRequestOptions *options ) {
	char* header;
	LOGICAL skipLength = FALSE;
	INDEX idx;
	LOGICAL hadUserAgent = FALSE;
	LOGICAL hadConnection = FALSE;
	const char* resource = GetText( url );
	// An origin-form request target has to start with '/' (RFC 9112 3.2.1), and a
	// caller that passes a bare "download.html" would otherwise put
	// `GET download.html HTTP/1.1` on the wire.  Servers do not merely refuse that:
	// althttpd (which serves sqlite.org) counts a target that does not start with
	// '/' as a hack attempt and shuns the source IP - 300 seconds per offense, and
	// every retry of the request adds another one.  Prepend the slash instead.
	// The two legal targets that do not begin with '/' are absolute-form
	// ("http://host/path", used when talking to a proxy) and asterisk-form
	// ("OPTIONS *"), so leave those alone.
	const char* leadin = "";
	if( !resource || !resource[0] )
		resource = "/";
	else if( resource[0] != '/' && resource[0] != '*'
	      && StrCaseCmpEx( resource, "http://", 7 ) != 0
	      && StrCaseCmpEx( resource, "https://", 8 ) != 0 )
		leadin = "/";
	if( !state->pvtOut ) state->pvtOut = VarTextCreate();

	vtprintf( state->pvtOut, "%s %s%s HTTP/%s\r\n", options->method, leadin, resource, options->httpVersion?options->httpVersion:"1.1" );

	// Host must carry a nonstandard port; the caller decides that by setting
	// options->hostname (NULL falls back to the "host:port" address text).
	// The space after the colon is optional per RFC 9110 (field-line is
	// name ":" OWS value OWS) but not everyone parses that way: althttpd splits
	// header lines on whitespace and compares the first token against "Host:",
	// so "Host:example.com" is one token that matches nothing, the host is lost,
	// and the request 404s with "Missing HOST: parameter".  Every other client
	// sends the space; so do we.
	{
		const char* targetHost = options->hostname ? options->hostname : GetText( address );
		vtprintf( state->pvtOut, "Host: %s\r\n", targetHost );
	}

	LIST_FORALL( options->headers, idx, char*, header ) {
		if( !hadConnection && ( StrCaseCmpEx( header, "connection", 10 ) == 0 ) ) {
			hadConnection = TRUE;
			int spaces = 0;
			while( header[11+spaces] == ' ' || header[11+spaces] == ':' ) spaces++;
			if( StrCaseCmpEx( header+11+spaces, "keep-alive", 9 ) == 0 ) {
				state->flags.keep_alive = 1;
			} else if( StrCaseCmpEx( header+11+spaces, "close", 5 ) == 0 ) {
				state->flags.close = 1;
			}
		}
		if( !hadUserAgent && ( StrCaseCmpEx( header, "user-agent", 10 ) == 0 ) ) hadUserAgent = TRUE;
		if( !skipLength   && ( StrCaseCmpEx( header, "Content-Length", 15 ) == 0 ) ) {
			skipLength = TRUE;
			if( header[15] == '~' ) // force content length to get hidden; should be ':' to be valid
				continue;
		}
		vtprintf( state->pvtOut, "%s\r\n", header );
	}

	if( !hadConnection ) {
		if( !options->httpVersion || strcmp( options->httpVersion, "1.1" ) == 0 || strcmp( options->httpVersion, "2.0" ) == 0) {
			vtprintf( state->pvtOut, "Connection: Keep-Alive\r\n" );
			state->flags.keep_alive = 1;
		}
	}

	if( !skipLength ) {
		vtprintf( state->pvtOut, "Content-Length: %d\r\n", options->contentLen);
	}
	if( !hadUserAgent )
		vtprintf( state->pvtOut, "User-Agent: %s\r\n", options->agent?options->agent:"SACK/1.3" );
	vtprintf( state->pvtOut, "\r\n" ); // send blank header
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
		NETWORK_ADDRESS_FLAG_PREFER_NONE
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
		struct HttpState *state = CreateHttpState(NULL);
		state->options = options;
		state->closed = FALSE;
		pc = httpOpenSocket( address, state, options );
		if( pc )
		{
			state->pvtOut = VarTextCreate();
			httpBuildRequest( state, address, url, options );
#ifndef NO_SSL
			if( options->ssl ) {
				if( ssl_BeginClientSession( pc, NULL, 0, NULL, 0, options->certChain?options->certChain:certChain, certChain
							? strlen( options->certChain ? options->certChain:certChain ) : 0 ) ) {
					SetNetworkErrorCallback( pc, httpSSLError, (uintptr_t)state );
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
				if( NetworkConnectTCP( pc ) < 0 ) {
					DestroyHttpState( state );
					return NULL;
				}
				// Plain sockets can send as soon as connect returns.  The TLS branch
				// above deliberately does not: pvtOut is left for HttpReader to send
				// once the handshake completes.
				httpSendRequest( state, options );
				VarTextDestroy( &state->pvtOut );
			}


			// response timeout budget starts now; time spent connecting and
			// sending (which stalls under TIME_WAIT port pressure) is not
			// response-wait time.
			state->last_read_tick = timeGetTime();
			// wait for response.
			LOGICAL timeout = FALSE;
			while ((timeout = FALSE), state->request_socket && !state->closed && !state->returned_status
				&& ((timeout = TRUE), (state->last_read_tick > (timeGetTime() - options->timeout)))) {
				//lprintf( "waiting for response 1000 second %d", options->timeout );
				WakeableSleep(1000);
			}
			state->waiter = NULL;
			if (!timeout && !state->returned_status) {
				// this becomes the caller's generic 'Bad Parsing State'; say why,
				// and say where the request bytes are being held if they never left.
				lprintf( "HTTP request ended without a response: pc:%p closed:%d sinceRead:%" _32f " budget:%" _32f " pendingSend:%d deferred:%" _32f " flags:%08x evstate:%08x"
				       , state->request_socket, state->closed
				       , timeGetTime() - state->last_read_tick
				       , options->timeout
				       , state->request_socket ? NetworkClientHasPendingSend( state->request_socket ) : 0
				       , state->request_socket ? NetworkClientWritesPended( state->request_socket ) : 0
				       , state->request_socket ? NetworkClientFlags( state->request_socket ) : 0
				       , state->request_socket ? NetworkClientEventState( state->request_socket ) : 0 );
			}
			//lprintf( "Request has completed.... %p %p %d", pc, state->content, state->closed );
			if( state->request_socket && !state->closed ) {
				//lprintf( "Closing in got response?" );
				// the state is returned, so the close shouldn't do anything to it...
				SetNetworkLong( state->request_socket, 0, 0 );
				RemoveClient( state->request_socket ); // this shouldn't happen... it should have ben closed already.
				//state->request_socket = NULL;
				return state;
			}

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

//---------- streaming connection API ---------------------------
// Same five phases GetHttpsQueryEx runs through, minus its wait loop, and with
// the two per-request phases (format, write) moved out to
// SendHttpConnectionRequest so they can happen more than once.

HTTPState OpenHttpConnection( PTEXT address, const char *certChain
                            , struct HTTPRequestOptions *options
                            , httpConnectionOpened opened
                            , httpConnectionResponse response
                            , httpConnectionClosed closed
                            , uintptr_t psv ) {
	PCLIENT pc;
	struct HttpState *state;
	if( !address || !options ) return NULL;
	state = CreateHttpState( NULL );
	state->options = options;
	state->closed = FALSE;
	state->flags.connection_mode = 1;
	state->pipelineDepth = 1;
	state->connectionAddress = SegDuplicate( address );
	state->connectionCertChain = options->certChain ? options->certChain : certChain;
	state->openedCallback = opened;
	state->responseCallback = response;
	state->closedCallback = closed;
	state->psvConnection = psv;

	pc = httpOpenSocket( address, state, options );
	if( !pc ) {
		DestroyHttpState( state );
		return NULL;
	}
	// no waiter thread: nothing about this API blocks.
	state->waiter = NULL;
#ifndef NO_SSL
	if( options->ssl ) {
		if( !ssl_BeginClientSession( pc, NULL, 0, NULL, 0, state->connectionCertChain, state->connectionCertChain
		                           ? strlen( state->connectionCertChain ) : 0 ) ) {
			RemoveClient( pc );
			DestroyHttpState( state );
			return NULL;
		}
		SetNetworkErrorCallback( pc, httpSSLError, (uintptr_t)state );
		if( !options->rejectUnauthorized )
			ssl_SetIgnoreVerification( pc );
	}
#endif
	if( NetworkConnectTCP( pc ) < 0 ) {
		DestroyHttpState( state );
		return NULL;
	}
	state->last_read_tick = timeGetTime();
	return state;
}

LOGICAL SendHttpConnectionRequest( HTTPState connection, PTEXT url, struct HTTPRequestOptions *options ) {
	struct httpConnectionRequest *req;
	if( !connection || !connection->flags.connection_mode ) return FALSE;
	if( connection->closed || !connection->request_socket ) return FALSE;
	req = New( struct httpConnectionRequest );
	req->url = url ? SegDuplicate( url ) : NULL;
	req->options = options;
	lockHttp( connection );
	EnqueLink( &connection->pending, req );
	unlockHttp( connection );
	// before the connection is up this just queues; httpConnectionReady flushes.
	httpFlushRequests( connection );
	return TRUE;
}

void SetHttpConnectionPipeline( HTTPState connection, int depth ) {
	if( !connection ) return;
	connection->pipelineDepth = depth > 0 ? depth : 1;
	httpFlushRequests( connection );
}

int GetHttpConnectionPending( HTTPState connection ) {
	if( !connection ) return 0;
	return connection->inflightCount + (int)GetQueueLength( connection->pending );
}

void CloseHttpConnection( HTTPState connection ) {
	if( !connection ) return;
	connection->flags.connection_ready = 0;
	if( connection->request_socket )
		RemoveClient( connection->request_socket );
	else if( connection->closedCallback && !connection->closed ) {
		// never had a socket to close; still owes the owner a close.
		connection->closed = TRUE;
		httpFailOutstanding( connection );
		connection->closedCallback( connection->psvConnection, connection );
	}
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
		struct HttpState *pHttpState = CreateHttpState(NULL);
		pHttpState->ssl = pHttpStateServer->ssl;
		buffer = pHttpState->buffer = Allocate( 4096 );
		pHttpState->request_socket = pc;
		//lprintf( "update pc here?" );
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
		if( !AddHttpData( pHttpState, buffer, length ) ) {
			// not HTTP - drop it without answering.
			RemoveClientEx( pc, 0, 1 );
			return;
		}
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
					|| (pHttpState->request_version == 100 && !pHttpState->flags.keep_alive)
					||( pHttpState->request_version == 101 && pHttpState->flags.close ) ) ) {
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
	SetTCPNoDelay( pc_new, TRUE );
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
	PTEXT result = NULL;
	// the field list is emptied by EndHttp on the JS end() thread; walking it
	// unlocked can trip over LineRelease'd names/values mid-iteration.
	lockHttp( pHttpState );
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		if( TextLike( field->name, name ) ) {
			result = field->value;
			break;
		}
	}
	unlockHttp( pHttpState );
	return result;
}

PNVLIST GetHttpHeaderFields( HTTPState pHttpState )
{
	if( pHttpState )
		return pHttpState->fields;
	return NULL;
}

int GetHttpReplyVersion( HTTPState pHttpState ) {
	if( pHttpState )
		return pHttpState->response_version;
	return -1;
}

int GetHttpRequestVersion( HTTPState pHttpState ) {
	if( pHttpState )
		return pHttpState->request_version;
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
