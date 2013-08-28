#include <stdhdrs.h>

#include <http.h>

struct default_port
{
	CTEXTSTR name;
	int number;
};
#define num_defaults (sizeof(default_ports)/sizeof(default_ports[0]))
static struct default_port default_ports[] = { { "http", 80 }
															, { "ftp", 21 }
															, { "ssh", 22 }
															, { "telnet", 23 }
															, { "https", 443 }
															, { "ws", 80 }
															, { "wss", 443 }
															, { "file", 0 }
															};


// TEXTSTR result = ConvertURIText( addr, length )

// SACK_ParseURL takes a URL string and gets the peices it can identify
// if a peice is not specified, the result will be NULL.



enum URLParseState
{
	PARSE_STATE_COLLECT_PROTOCOL = 0  // find ':', store characters in buffer
	, PARSE_STATE_COLLECT_PROTOCOL_1  // find '/', eat /
	, PARSE_STATE_COLLECT_PROTOCOL_2  // eat '/', eat /
		, PARSE_STATE_COLLECT_USER
		, PARSE_STATE_COLLECT_PASSWORD
		, PARSE_STATE_COLLECT_ADDRESS
		, PARSE_STATE_COLLECT_PORT
		, PARSE_STATE_COLLECT_RESOURCE_PATH
		, PARSE_STATE_COLLECT_RESOURCE_NAME
		, PARSE_STATE_COLLECT_RESOURCE_EXTENSION
		, PARSE_STATE_COLLECT_RESOURCE_ANCHOR
		, PARSE_STATE_COLLECT_CGI_NAME
		, PARSE_STATE_COLLECT_CGI_VALUE
};

static void AppendBuffer( CTEXTSTR *output, CTEXTSTR seperator, CTEXTSTR input )
{
	CTEXTSTR tmpbuf = ConvertURIText( input, StrLen( input ) + 1 );
	TEXTSTR newout;
	if( *output )
	{
		int len;
		len = StrLen( *output ) + StrLen( tmpbuf ) + 1;
		if( seperator )
			len += StrLen( seperator );
		newout = NewArray( TEXTCHAR, len );
		snprintf( newout, len, "%s%s%s", (*output), seperator?seperator:"", tmpbuf );
		Release( (POINTER)*output );
		(*output) = newout;
		Release( (POINTER)tmpbuf );
	}
	else
	{
		(*output) = tmpbuf;
	}
}

struct url_data * SACK_URLParse( CTEXTSTR url )
{
	struct url_data *data = New( struct url_data );
	struct url_cgi_data *cgi_data;

	int inchar = 0;
	int outchar = 0;
	TEXTSTR outbuf = NewArray( TEXTCHAR, StrLen( url ) + 1 );
	int _state, state;
	_state = -1;
	state = PARSE_STATE_COLLECT_PROTOCOL;
	MemSet( data, 0, sizeof( struct url_data ) );
	while( url[inchar] )
	{
		int use_char;
		use_char = 0;
		switch( url[inchar] )
		{
		case '&':
			if( state == PARSE_STATE_COLLECT_CGI_NAME )
			{
				cgi_data = New( struct url_cgi_data );
				cgi_data->name = NULL;
				cgi_data->value = NULL;
				AddLink( &data->cgi_parameters, cgi_data );
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &cgi_data->name, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_NAME;  // same state, just a blank cgi param
			}
			if( state == PARSE_STATE_COLLECT_CGI_VALUE )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &cgi_data->value, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_NAME;
			}
			break;
		case '=':
			if( state == PARSE_STATE_COLLECT_CGI_NAME )
			{
				cgi_data = New( struct url_cgi_data );
				cgi_data->name = NULL;
				cgi_data->value = NULL;
				AddLink( &data->cgi_parameters, cgi_data );
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &cgi_data->name, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_VALUE;
			}
			break;
		case '?':
			if( state == PARSE_STATE_COLLECT_RESOURCE_EXTENSION )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_extension, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_NAME;
			}
			if( state == PARSE_STATE_COLLECT_RESOURCE_NAME )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_file, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_NAME;
			}
			if( state == PARSE_STATE_COLLECT_RESOURCE_ANCHOR )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_anchor, NULL, outbuf );
				state = PARSE_STATE_COLLECT_CGI_NAME;
			}
			break;
		case '#':
			if( state == PARSE_STATE_COLLECT_RESOURCE_EXTENSION )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_extension, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_ANCHOR;
			}
			else if( ( state == PARSE_STATE_COLLECT_RESOURCE_NAME )
				|| ( state == PARSE_STATE_COLLECT_RESOURCE_PATH ) )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_file, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_ANCHOR;
			}
			break;
		case '.':
			// I just want to process the '.' when finding the extension.
			// just because we will always want to know it for other reasons later
			if( state == PARSE_STATE_COLLECT_RESOURCE_PATH
				|| state == PARSE_STATE_COLLECT_RESOURCE_NAME )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_file, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_EXTENSION;
			}
			else
				use_char = 1;
			break;
		case '/':
			if( state == PARSE_STATE_COLLECT_PROTOCOL_1 )
			{
				if( outchar > 0 )
					lprintf( "Characters between protocol ':' and first slash" );
				state = PARSE_STATE_COLLECT_PROTOCOL_2;
			}
			else if( state == PARSE_STATE_COLLECT_PROTOCOL_2 )
			{
				if( outchar > 0 )
					lprintf( "Characters between protocol first and second slash" );
				state = PARSE_STATE_COLLECT_USER;
			}
			else if( state == PARSE_STATE_COLLECT_USER )
			{
				// what was collected was really
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->host, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_PATH;
			}
			else if( state == PARSE_STATE_COLLECT_PASSWORD )
			{
				// what was collected was really the ip:port not user:password
				data->host = data->user;
				data->user = NULL;
				outbuf[outchar] = 0;
				outchar = 0;
				// should validate port is in numeric.
				data->port = atoi( outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_PATH;
			}
			else if( state == PARSE_STATE_COLLECT_ADDRESS )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->host, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_PATH;
			}
			else if( state == PARSE_STATE_COLLECT_PORT )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				data->port = atoi( outbuf );
				//AppendBuffer( &data->port, NULL, outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_PATH;
			}
			else if( state == PARSE_STATE_COLLECT_RESOURCE_PATH )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_path, "/", outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_NAME;
			}
			else if( state == PARSE_STATE_COLLECT_RESOURCE_NAME )
			{
				// this isn't really the name, it's another part of the resource path
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->resource_path, "/", outbuf );
				state = PARSE_STATE_COLLECT_RESOURCE_NAME;
			}

			break;
		case '@':
			if( state == PARSE_STATE_COLLECT_USER )  // hit the colon between user and password
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->user, NULL, outbuf );
				state = PARSE_STATE_COLLECT_ADDRESS;
			}
			if( state == PARSE_STATE_COLLECT_PASSWORD )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->password, NULL, outbuf );
				state = PARSE_STATE_COLLECT_ADDRESS;
			}
			break;
		case ':':
			if( state == PARSE_STATE_COLLECT_PROTOCOL )
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->protocol, NULL, outbuf );
				{
					int n;
					for( n = 0; n < num_defaults; n++ )
					{
						if( strcmp( outbuf, default_ports[n].name ) == 0 )
						{
							data->default_port = default_ports[n].number;
						}
					}
				}
				state = PARSE_STATE_COLLECT_PROTOCOL_1;
			}
			else if( state == PARSE_STATE_COLLECT_USER )  // hit the colon between user and password
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->user, NULL, outbuf );
				state = PARSE_STATE_COLLECT_PASSWORD;
			}
			else if( state == PARSE_STATE_COLLECT_ADDRESS )  // hit the colon between address and port
			{
				outbuf[outchar] = 0;
				outchar = 0;
				AppendBuffer( &data->host, NULL, outbuf );
				state = PARSE_STATE_COLLECT_PORT;
			}
			else
			{
				; // error
			}
			break;
		default:
			switch( state )
			{
			case PARSE_STATE_COLLECT_PROTOCOL_1:
				// the thing after the ':' was not a '/', so this isn't the protocol.
				break;
			case PARSE_STATE_COLLECT_PROTOCOL_2:
				break;
			default:
				use_char = 1;
			}
			break;
		}
		if( use_char )
			outbuf[outchar++] = url[inchar++];
		else
		{
			if( _state == state 
				&& ( state != PARSE_STATE_COLLECT_RESOURCE_NAME )  // after starting the path, look for fliename, if the extension or other is not found
				&& ( state != PARSE_STATE_COLLECT_CGI_NAME ) // blank cgi names go & to & and stay in the same state
				)
				lprintf( "Dropping character (%d) '%c' in %s", inchar, url[inchar], url );
			inchar++;
		}
		_state = state;
	}
	switch( state )
	{
	// this means user name, but if we hit the end of the buffer, it's the address
	case PARSE_STATE_COLLECT_USER:
		outbuf[outchar] = 0;
		outchar = 0;
		AppendBuffer( &data->host, NULL, outbuf );
		break;
	// this is the first colon, but no @ found, but end of buffer name is host and this is port.
	case PARSE_STATE_COLLECT_PASSWORD:
		data->host = data->user;
		data->user = NULL;
		outbuf[outchar] = 0;
		outchar = 0;
		data->port = atoi( outbuf );
		//AppendBuffer( &data->port, NULL, outbuf );
		break;
	case PARSE_STATE_COLLECT_CGI_NAME:
		cgi_data = New( struct url_cgi_data );
		cgi_data->name = NULL;
		cgi_data->value = NULL;
		AddLink( &data->cgi_parameters, cgi_data );
		outbuf[outchar] = 0;
		outchar = 0;
		AppendBuffer( &cgi_data->name, NULL, outbuf );
		break;
	case PARSE_STATE_COLLECT_CGI_VALUE:
		outbuf[outchar] = 0;
		outchar = 0;
		AppendBuffer( &cgi_data->value, NULL, outbuf );
		break;
	default:
		if( outchar )
		{
			outbuf[outchar] = 0;
			lprintf( "Unused output: [%s]", outbuf );
		}
		break;
	}
	Release( outbuf );
	return data;
}

CTEXTSTR SACK_BuildURL( struct url_data *data )
{
	PVARTEXT pvt = VarTextCreate();
	CTEXTSTR tmp = NULL;
	CTEXTSTR tmp2 = NULL;
	if( data->protocol )
		vtprintf( pvt, "%s://", tmp = ConvertTextURI( data->protocol, StrLen( data->protocol ), 0 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	// must be a user to use the password, setting just a password is an error really
	if( data->user )
		vtprintf( pvt, "%s%s%s@"
				  , tmp = ConvertTextURI( data->user, StrLen( data->user ), 0 )
				  , data->password?":":""
				  , data->password?(tmp2 = ConvertTextURI( data->password, StrLen( data->password ), 0 )):"" );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	if( tmp2 )
	{
		Release( (POINTER)tmp2 );
		tmp2 = NULL;
	}

	if( data->host )
		vtprintf( pvt, "%s"
				  , tmp = ConvertTextURI( data->host, StrLen( data->host ), 0 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	if( data->port )
		vtprintf( pvt, ":%d", data->port );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}

	if( data->resource_path )
		vtprintf( pvt, "/%s"
				  , tmp = ConvertTextURI( data->resource_path, StrLen( data->resource_path), 1 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	if( data->resource_file )
		vtprintf( pvt, "/%s"
				  , tmp = ConvertTextURI( data->resource_file, StrLen( data->resource_file), 0 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	if( data->resource_extension )
		vtprintf( pvt, ".%s"
				  , tmp = ConvertTextURI( data->resource_extension, StrLen( data->resource_extension), 0 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}
	if( data->resource_anchor )
		vtprintf( pvt, "#%s"
				  , tmp = ConvertTextURI( data->resource_anchor, StrLen( data->resource_anchor), 0 ) );
	if( tmp )
	{
		Release( (POINTER)tmp );
		tmp = NULL;
	}

	if( data->cgi_parameters )
	{
		int first = 1;
		INDEX idx;
		struct url_cgi_data *cgi_data;
		LIST_FORALL( data->cgi_parameters, idx, struct url_cgi_data *, cgi_data )
		{
			if( cgi_data->value )
				vtprintf( pvt, "%s%s=%s", first?"?":"&", cgi_data->name, cgi_data->value );
			else
				vtprintf( pvt, "%s%s", first?"?":"&", cgi_data->name );
			first = 0;
		}
	}
	{
		PTEXT text_result = VarTextGet( pvt );
		CTEXTSTR result = StrDup( GetText( text_result ) );
		return result;
	}
}

void SACK_ReleaseURL( struct url_data *data )
{
	struct url_cgi_data *cgi_data;
	INDEX idx;
	LIST_FORALL( data->cgi_parameters, idx, struct url_cgi_data *, cgi_data )
	{
		Release( (POINTER)cgi_data->name );
		Release( (POINTER)cgi_data->value );
	}
	DeleteList( &data->cgi_parameters );
	Release( (POINTER)data->protocol );
	Release( (POINTER)data->user );
	Release( (POINTER)data->password );
	Release( (POINTER)data->host );
	Release( (POINTER)data->resource_path );
	Release( (POINTER)data->resource_file );
	Release( (POINTER)data->resource_extension );
	Release( (POINTER)data->resource_anchor );

	Release( data );
}

