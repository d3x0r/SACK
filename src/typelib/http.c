#include <stdhdrs.h>
#include <network.h>
#include <procreg.h>
#include <sqlgetoption.h>
#include "http.h"

HTTP_NAMESPACE


struct HttpField {
	PTEXT name;
	PTEXT value;
};

struct HttpState {
   // add input into pvt_collector
	PVARTEXT pvt_collector;
	PTEXT partial;  // an accumulator that moves data from collector into whatever we've got leftover

	PTEXT response_status; // the first line of the http responce... (or request)
	PTEXT resource; // the path of the resource - mostly for when this is used to receive requests.
	PLIST fields; // list of struct HttpField *, these other the other meta fields in the header.
	PLIST cgi_fields; // list of HttpField *, taken in from the URL or content (get or post)

	_32 content_length;
	PTEXT content; // content of the message, POST,PUT,PATCH and replies have this.

	int final; // boolean flag - indicates that the header portion of the http request is finished.

	POINTER buffer; //for handling requests, have to read somewhere

	int numeric_code;
	int response_version;
	TEXTSTR text_code;
   PCLIENT request_socket; // when a request comes in to the server, it is kept in a new http state, this is set for Send Response
};

struct HttpServer {
	PCLIENT server;
	PLIST clients;
	ProcessHttpRequest handle_request;
	PTRSZVAL psvRequest;
	CTEXTSTR site;
	PCLASSROOT methods;
};

static struct local_http_data
{
	struct http_data_flags {
		BIT_FIELD bLogReceived : 1;
	} flags;
}local_http_data;
#define l local_http_data

void GatherHttpData( struct HttpState *pHttpState )
{
	if( pHttpState->content_length )
	{
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		if( GetTextSize( pHttpState->partial ) >= pHttpState->content_length ) 
		{
			pHttpState->content = SegSplit( &pHttpState->partial, pHttpState->content_length );
			pHttpState->partial = NEXTLINE( pHttpState->partial );		
			SegGrab( pHttpState->partial );
		}
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
}


void ProcessURL_CGI( struct HttpState *pHttpState, PTEXT params )
{
	PTEXT start = TextParse( params, WIDE( "&=" ), NULL, 1, 1 DBG_SRC );
	PTEXT next = start;
	PTEXT tmp;
	//lprintf( "Input was %s", GetText( params ) );
	while( tmp = next )
	{
		PTEXT name = tmp;
		PTEXT equals = ( next = NEXTLINE( tmp ) );
		PTEXT value = ( next = NEXTLINE( next ) );
		PTEXT ampersand = ( next = NEXTLINE( next ) );

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
enum ProcessHttpResult ProcessHttp( PCLIENT pc, struct HttpState *pHttpState )
{
	if( pHttpState->final )
	{
		GatherHttpData( pHttpState );
		if( pHttpState->content )
			return HTTP_STATE_RESULT_CONTENT;
	}
	else
	{
		PTEXT pCurrent, pStart;
		PTEXT pLine = NULL,
			 pHeader = NULL;
		TEXTCHAR *c, *line;
		size_t size, pos, len;
		int bLine;
		INDEX start = 0;
		PTEXT pMergedLine;
		PTEXT pInput = VarTextGet( pHttpState->pvt_collector );
		PTEXT pNewLine = SegAppend( pHttpState->partial, pInput );
		pMergedLine = SegConcat( NULL, pNewLine, 0, GetTextSize( pHttpState->partial ) + GetTextSize( pInput ) );
		LineRelease( pNewLine );
		pHttpState->partial = pMergedLine;
		pCurrent = pHttpState->partial;
		pStart = pCurrent; // at lest is this block....
		len = 0;

		// we always start without having a line yet, because all input is already merged
		bLine = 0;
		{
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
								pHttpState->numeric_code = 200; // initialize to assume it's OK.  (requests should be OK)
								{
									PTEXT request = TextParse( pHttpState->response_status, WIDE( "?#" ), WIDE( " " ), 1, 1 DBG_SRC );
									{
										PTEXT tmp;
										PTEXT resource_path = NULL;
										PTEXT next;
										for( tmp = NEXTLINE( request ); tmp; tmp = next )
										{
											//lprintf( WIDE( "word %s" ), GetText( tmp ) );
											next = NEXTLINE( tmp );
											if( GetText(tmp)[0] == '?' )
											{
												ProcessURL_CGI( pHttpState, next );
												next = NEXTLINE( next );
											}
											if( GetText(tmp)[0] == '#' )
											{
												lprintf( WIDE("Page anchor is lost... %s"), GetText( next ) );
												next = NEXTLINE( next );
											}
											if( ( resource_path && tmp->format.position.offset.spaces ) )
											{
												break;
											}
											else
											{
												resource_path = SegAppend( resource_path, SegGrab( tmp ) );
											}
										}



										pHttpState->resource = resource_path;
									}
									LineRelease( request );
								}
								//lprintf( "Line : %s", GetText( pLine ) );
								if( TextSimilar( pLine, WIDE("HTTP/") ) )
								{
									TEXTCHAR *tmp = (TEXTCHAR*)StrChr( GetText( pLine ), '.' );
									pHttpState->response_version = ( atoi( GetText( pLine ) + 5 ) * 100 ) + atoi( tmp + 1 );
									{
										TEXTCHAR *space = (TEXTCHAR*)StrChr( line, ' ' );
										if( space )
										{
											space++;
											// cast from S_64
											pHttpState->numeric_code = (int)IntCreateFromText( space );
											{
												TEXTCHAR *code = (TEXTCHAR*)StrChr( space, ' ' );
												if( pHttpState->text_code )
													Release( pHttpState->text_code );
												pHttpState->text_code = StrDup( code );
											}
										}
										else
										{
											lprintf( WIDE( "failed to find result code in %s" ), line );
										}
									}
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
							pStart = pCurrent;
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
			PTEXT tmp = SegSplit( &pCurrent, start );
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
               // down convert from S_64
					pHttpState->content_length = (int)IntCreateFromSeg( field->value );
				}
				if( TextLike( field->name, WIDE( "Expect" ) ) )
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
	if( pHttpState->final )
	{
		if( pHttpState->numeric_code == 500 )
			return HTTP_STATE_INTERNAL_SERVER_ERROR;
		if( pHttpState->content && ( pHttpState->numeric_code == 200 ) )
			return HTTP_STATE_RESULT_CONTENT;
		if( pHttpState->numeric_code == 100 )
			return HTTP_STATE_RESULT_CONTINUE;
		if( pHttpState->numeric_code == 404 )
			return HTTP_STATE_RESOURCE_NOT_FOUND;
	}
	return HTTP_STATE_RESULT_NOTHING;
}

void AddHttpData( struct HttpState *pHttpState, POINTER buffer, size_t size )
{
	VarTextAddData( pHttpState->pvt_collector, (CTEXTSTR)buffer, size );
}

struct HttpState *CreateHttpState( void )
{
	struct HttpState *pHttpState;
#ifndef __NO_OPTIONS__
	l.flags.bLogReceived = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/HTTP/Enable Logging Received Data"), 0, TRUE );
#endif

	pHttpState = New( struct HttpState );
	MemSet( pHttpState, 0, sizeof( struct HttpState ) );
	pHttpState->pvt_collector = VarTextCreate();
	return pHttpState;
}


void EndHttp( struct HttpState *pHttpState )
{
	pHttpState->final = 0;

	pHttpState->content_length = 0;
	LineRelease( pHttpState->content );
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
	if( pHttpState->final )
		return pHttpState->content;
	return NULL;
}

void ProcessHttpFields( struct HttpState *pHttpState, void (CPROC*f)( PTRSZVAL psv, PTEXT name, PTEXT value ), PTRSZVAL psv )
{
	INDEX idx;
	struct HttpField *field;
	LIST_FORALL( pHttpState->fields, idx, struct HttpField *, field )
	{
		f( psv, field->name, field->value );
	}
}

void ProcessCGIFields( struct HttpState *pHttpState, void (CPROC*f)( PTRSZVAL psv, PTEXT name, PTEXT value ), PTRSZVAL psv )
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

void DestroyHttpState( struct HttpState *pHttpState )
{
	EndHttp( pHttpState ); // empties variables
	DeleteList( &pHttpState->fields );
	VarTextDestroy( &pHttpState->pvt_collector );
	if( pHttpState->buffer )
		Release( pHttpState->buffer );
   Release( pHttpState );
}

void SendHttpResponse ( struct HttpState *pHttpState, PCLIENT pc, int numeric, CTEXTSTR text, CTEXTSTR content_type, PTEXT body )
{
	int offset = 0;
	PVARTEXT pvt_message = VarTextCreate();
	PTEXT header;
	PTEXT tmp_content;
	//TEXTCHAR message[500];

	vtprintf( pvt_message, WIDE( "HTTP/1.1 %d %s\r\n" ), numeric, text );
	if( content_type )
	{
		vtprintf( pvt_message, WIDE( "Content-Length: %d\r\n" ), GetTextSize(body));
		vtprintf( pvt_message, WIDE( "Content-Type: %s\r\n" )
				  , content_type?content_type
					:(tmp_content=GetHttpField( pHttpState, WIDE("Accept") ))?GetText(tmp_content)
					:WIDE("text/plain; charset=utf-8")  );
	}
	else
      vtprintf( pvt_message, WIDE( "%s\r\n" ), GetText( body ) );
	vtprintf( pvt_message, WIDE( "Server: SACK Core Library 2.x\r\n" )  );


	vtprintf( pvt_message, WIDE( "\r\n" )  );

	header = VarTextGet( pvt_message );
	//offset += snprintf( message + offset, sizeof( message ) - offset, WIDE( "%s" ),  "Body");
	if( l.flags.bLogReceived )
	{
      lprintf( WIDE("Sending response...") );
		LogBinary( GetText( header ), GetTextSize( header ) );
      if( content_type )
			LogBinary( GetText( body ), GetTextSize( body ) );
	}
	if( !pc )
      pc = pHttpState->request_socket;
	SendTCP( pc, GetText( header ), GetTextSize( header ) );
   if( content_type )
		SendTCP( pc, GetText( body ), GetTextSize( body ) );
}

void SendHttpMessage ( struct HttpState *pHttpState, PCLIENT pc, PTEXT body )
{	
	PTEXT message;
	size_t offset = 0;
	PVARTEXT pvt_message = VarTextCreate();
	PTEXT content_type;

	offset += vtprintf( pvt_message, WIDE( "%s" ),  WIDE("HTTP/1.1 200 OK\r\n") );
	offset += vtprintf( pvt_message, WIDE( "Content-Length: %d\r\n" ), GetTextSize( body ));	
	offset += vtprintf( pvt_message, WIDE( "Content-Type: %s\r\n" )
		, (content_type = GetHttpField( pHttpState, WIDE("Accept") ))?GetText(content_type):WIDE("text/plain" ));	
	offset += vtprintf( pvt_message, WIDE( "\r\n" )  );	
	offset += vtprintf( pvt_message, WIDE( "%s" ), GetText( body ));	
	message = VarTextGet( pvt_message );
	if( l.flags.bLogReceived )
	{
		lprintf( WIDE(" Response Message:" ));
		LogBinary( GetText( message ), GetTextSize( message ));
	}
	SendTCP( pc, GetText( message ), GetTextSize( message ));		
}

//---------- CLIENT --------------------------------------------

static void CPROC HttpReader( PCLIENT pc, POINTER buffer, size_t size )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
		SetNetworkLong( pc, 1, (PTRSZVAL)buffer );
	}
	else
	{
		struct HttpState *state = (struct HttpState *)GetNetworkLong( pc, 2 );
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Received web request...") );
			LogBinary( buffer, size );
		}
		AddHttpData( state, buffer, size );
		if( ProcessHttp( pc, state ) )
		{
			RemoveClient( pc );
			return;
		}
	}
	ReadTCP( pc, buffer, 4096 );
}

static void CPROC HttpReaderClose( PCLIENT pc )
{
	Release( (POINTER)GetNetworkLong( pc, 1 ) );
   ((PCLIENT*)GetNetworkLong( pc, 0 ))[0] = 0;
}

HTTPState PostHttpQuery( PTEXT address, PTEXT url, PTEXT content )
{
	PCLIENT pc = OpenTCPClient( GetText( address ), 80, HttpReader );
	struct HttpState *state = CreateHttpState();
	PVARTEXT pvtOut = VarTextCreate();
	vtprintf( pvtOut, WIDE( "POST %s HTTP/1.1\r\n" ), url );
	vtprintf( pvtOut, WIDE( "content-length:%d\r\n" ), GetTextSize( content ) );
	vtprintf( pvtOut, WIDE( "\r\n\r\n" ) );
	VarTextAddData( pvtOut, GetText( content ), GetTextSize( content ) );
	if( pc )
	{
		PTEXT send = VarTextGet( pvtOut );
		SetNetworkCloseCallback( pc, HttpReaderClose );
		SetNetworkLong( pc, 0, (PTRSZVAL)&pc );
		SetNetworkLong( pc, 2, (PTRSZVAL)state );
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Sending POST...") );
         LogBinary( GetText( send ), GetTextSize( send ) );
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

HTTPState GetHttpQuery( PTEXT address, PTEXT url )
{
	if( !address )
		return NULL;
	{
		PCLIENT pc = OpenTCPClient( GetText( address ), 80, HttpReader );
		struct HttpState *state = CreateHttpState();
		PVARTEXT pvtOut = VarTextCreate();
		vtprintf( pvtOut, WIDE( "GET %s HTTP/1.1\r\n" ), GetText( url ) );
		vtprintf( pvtOut, WIDE( "host: %s\r\n" ), GetText( address ) );
		vtprintf( pvtOut, WIDE( "\r\n\r\n" ) );
		if( pc )
		{
			PTEXT send = VarTextGet( pvtOut );
			SetNetworkCloseCallback( pc, HttpReaderClose );
			SetNetworkLong( pc, 0, (PTRSZVAL)&pc );
			SetNetworkLong( pc, 2, (PTRSZVAL)state );
			if( l.flags.bLogReceived )
			{
				lprintf( WIDE("Sending POST...") );
				LogBinary( GetText( send ), GetTextSize( send ) );
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
   return NULL;
}

PTEXT GetHttp( PTEXT address, PTEXT url )
{
	HTTPState state = GetHttpQuery( address, url );
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
	PTEXT request = TextParse( pHttpState->response_status, WIDE( "?#" ), WIDE( " " ), 1, 1 DBG_SRC );
	if( TextLike( request, WIDE( "get" ) ) || TextLike( request, WIDE( "post" ) ) )
	{
		CTEXTSTR name;
		PCLASSROOT data = NULL;
		//lprintf( "is a get or post? ");
		{
			LOGICAL (CPROC *f)(PTRSZVAL, PCLIENT, struct HttpState *, PTEXT);
			LOGICAL status = FALSE;
			//lprintf( "is %s==%s?", name, GetText( pHttpState->resource ) + 1 );
			//if( CompareMask( name, GetText( pHttpState->resource ) + 1, FALSE ) )
			{
				f = GetRegisteredProcedureExx( server->methods, (PCLASSROOT)(GetText( pHttpState->resource ) + 1), LOGICAL, GetText(request), (PTRSZVAL, PCLIENT, struct HttpState *, PTEXT) );
				//lprintf( "got for %s %s", (PCLASSROOT)(GetText( pHttpState->resource ) + 1),  GetText( request ) );
				//if( !f )
				//   DumpRegisteredNames();
				if( f )
					status = f( server->psvRequest, pc, pHttpState, pHttpState->content );
			}
			if( !status )
			{
				if( server->handle_request )
					status = server->handle_request( server->psvRequest, pHttpState );
			}
			if( !status )
			{
				PTEXT body = SegCreateFromText( WIDE( "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD><BODY>Resource handler not found" ) );
				SendHttpResponse( pHttpState, NULL, 404, WIDE("NOT FOUND"), WIDE("text/html"), body );
				LineRelease( body );
			}
		}
		LineRelease( request );
		return 1;
	}
	else
		lprintf( WIDE("not a get or a post?") );

	LineRelease( request );
	return 0;
}

static void CPROC HandleRequest( PCLIENT pc, POINTER buffer, size_t length )
{
	if( !buffer )
	{
		struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
		struct HttpState *pHttpState = CreateHttpState();
		buffer = pHttpState->buffer = Allocate( 4096 );
		pHttpState->request_socket = pc;
		SetNetworkLong( pc, 1, (PTRSZVAL)pHttpState );
	}
	else
	{
		enum ProcessHttpResult result;
		//struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
		struct HttpState *pHttpState = (struct HttpState *)GetNetworkLong( pc, 1 );
		if( l.flags.bLogReceived )
		{
			lprintf( WIDE("Received web request...") );
			LogBinary( buffer, length );
		}

		AddHttpData( pHttpState, buffer, length );
		while( ( result = ProcessHttp( pc, pHttpState ) ) )
		{
			struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
			//lprintf( "result = %d", result );
			switch( result )
			{
			case HTTP_STATE_RESULT_CONTENT:
				InvokeMethod( pc, server, pHttpState );
				EndHttp( pHttpState );
				break;
			case HTTP_STATE_RESULT_CONTINUE:
				break;
			}
		}
	}
	ReadTCP( pc, buffer, 4096 );
}

static void CPROC RequestorClosed( PCLIENT pc )
{
	struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc, 0 );
	struct HttpState *pHttpState = (struct HttpState *)GetNetworkLong( pc, 1 );
	DeleteLink( &server->clients, pc );
	DestroyHttpState( pHttpState );
}

static void CPROC AcceptHttpClient( PCLIENT pc_server, PCLIENT pc_new )
{
	struct HttpServer *server = (struct HttpServer *)GetNetworkLong( pc_server, 0 );
	AddLink( &server->clients, pc_new );
	SetNetworkLong( pc_new, 0, (PTRSZVAL)server );
	SetNetworkReadComplete( pc_new, HandleRequest );
	SetNetworkCloseCallback( pc_new, RequestorClosed );
}

struct HttpServer *CreateHttpServerEx( CTEXTSTR interface_address, CTEXTSTR TargetName, CTEXTSTR site, ProcessHttpRequest handle_request, PTRSZVAL psv )
{
	struct HttpServer *server = New( struct HttpServer );
	SOCKADDR *tmp;
	TEXTCHAR class_name[256];
#ifndef __NO_OPTIONS__
	l.flags.bLogReceived = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/HTTP/Enable Logging Received Data"), 0, TRUE );
#endif
	server->clients = NULL;
	server->handle_request = handle_request;
	server->psvRequest = psv;
	server->site = StrDup( site );
	snprintf( class_name, sizeof( class_name ), WIDE( "SACK/Http/Methods/%s%s%s" )
			  , TargetName?TargetName:WIDE("")
			  , ( TargetName && site )?WIDE( "/" ):WIDE( "" )
			  , site?site:WIDE( "" ) );
   //lprintf( "Server root = %s", class_name );
	server->methods = GetClassRoot( class_name );
	//DumpRegisteredNamesFrom( server->methods );
   //lprintf( "---------------------" );
   //DumpRegisteredNames();
	NetworkStart();
	server->server = OpenTCPListenerAddrEx( tmp = CreateSockAddress( interface_address?interface_address:WIDE( "0.0.0.0" ), 80 )
													  , AcceptHttpClient );
	ReleaseAddress( tmp );
	if( !server->server )
	{
		Release( server );
		return NULL;
	}
	SetNetworkLong( server->server, 0, (PTRSZVAL)server );

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

PTEXT GetHttpResource( struct HttpState *pHttpState )
{
   return pHttpState->resource;
}


HTTP_NAMESPACE_END
