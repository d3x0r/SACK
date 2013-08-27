#include <stdhdrs.h>


struct default_port
{
	CTEXTSTR name;
   int number;
};
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

void SACK_ParseURL( CTEXSTR url
						, CTEXTSTR *protocol
						, CTEXTSTR *user
						, CTEXTSTR *password
						, CTEXTSTR *host
						, CTEXTSTR *port
						, CTEXTSTR *resource_path
                  , CTEXTSTR *resource_file
                  , CTEXTSTR *resource_extension
                  , CTEXTSTR *anchor_name
						)
{
	CTEXTSTR colon;
	CTEXTSTR slash;
	TEXTSTR tmpbuf;
	CTEXTSTR addr_start;


	// http specific encoding... (anchor reference <a ref="pagelabel"> )
   // initialize these...
	if( anchor_name )
		(*anchor_name) = NULL;
	if( resource_path )
		(*resource_path) = NULL;
	if( resource_file )
		(*resource_file) = NULL;
	if( resource_extension )
		(*resource_extension) = NULL;

	colon = StrChr( url, ':' );
	if( colon[1] == '/' && colon[2] == '/' )
	{
      addr_start = colon + 3;
		tmpbuf = NewArray( TEXTCHAR, colon - url + 1);
		StrCpyEx( tmpbuf, colon, colon - url );
      tmpbuf[colon-url] = 0;
		if( protocol )
			(*protocol) = SaveName( tmpbuf );
      Release( tmpbuf );
      if( port )
		{
			int n;
         (*port) = 0;
			for( n = 0; n < ( sizeof( default_ports ) / sizeof( default_ports[0] ) ); n++ )
			{
				if( StrCaseCmp( (*protocol), default_ports[n].name ) == 0 )
				{
					(*port) = default_ports[n].port;
					break;
				}
			}
		}
	}
	else
	{
		if( (*protocol) )
			(*protocol) = NULL;
		if( port )
         (*port) = 0;
		addr_start = url;
	}

	atsymbol = StrChr( addr_start, '@' );
	if( atsymbol )
	{
		colon = StrChr( addr_start, ':' );
		if( colon )
		{
			if( name )
			{
				tmpbuf = ConvertURIText( addr_start, colon - addr_start );
				(*name) = SaveName( tmpbuf );
				Release( tmpbuf );
			}
			if( password )
			{
				tmpbuf = ConvertURIText( addr_start, atsymbol - colon );
				(*password) = SaveName( tmpbuf );
				Release( tmpbuf );
			}
		}
		else
		{
			if( password )
				(*password) = NULL;
			tmpbuf = ConvertURIText( addr_start, atsymbol - addr_start );
			(*name) = SaveName( tmpbuf );
			Release( tmpbuf );

		}
		addr_start = atsymbol + 1;
	}
	else
	{
		if( user )
         (*user) = NULL;
		if( password )
         (*password) = NULL;
	}

	if( addr_start[0] == '[' )
	{
		if( host )
		{
			CTEXTSTR end = StrChr( addr_start, ']' );
         tmpbuf = ConvertURIText( addr_start + 1, ( end - addr_start ) - 2 );
			(*host) = SaveName( tmpbuf );
         Release( tmpbuf );
			addr_start = end + 1;
		}
	}
	colon = StrChr( addr_start, ':' );
	slash = StrChr( addr_start, '/' );

   if( colon == addr_start )
	{
		// collected address already
		if( slash )
		{
		}
		else
		{
		}
	}
	else if( colon )
	{
		if( colon < slash )
		{
			tmpbuf = ConvertURLText( addr_start, colon - addr_start + 1 );
         if( add
         StrCpyEx( tmpbuf, addr_start, colon - addr_start );
         Release( tmpbuf );
		}
		else
		{
			tmpbuf = ConvertURLText( addr_start, slash - addr_start + 1 );
			StrCpyEx( tmpbuf, addr_start, slash - addr_start );
         Release( tmpbuf );
		}
	}
	else
	{
		if( slash )
		{

		}
		else
		{
			if( resource_path )
			{
			}
		}
	}
}

struct url_cgi_data
{
	CTEXTSTR name;
   CTEXTSTR value;
}

struct url_data
{
	CTEXTSTR protocol;
	CTEXTSTR user;
	CTEXTSTR password;
	CTEXTSTR host;
	CTEXTSTR port;
	CTEXTSTR resource_path;
	CTEXTSTR resource_file;
	CTEXTSTR resource_extension;
	CTEXTSTR anchor_name;
   // list of struct url_cgi_data *
	PLIST cgi_parameters;
};

struct url_data *SACK_ParseURLEx( CTEXSTR url )
{
   struct url_data *data = New( struct url_data );
	SACK_ParseURL( url
					 , &data->protocol
                , &data->user
                , &data->password
                , &data->host
                , &data->port
                , &data->resource_path
                , &data->resource_file
                , &data->resource_extension
					 , &data->anchor_name
					 , &data->cgi_parameters
					 );
   return data;
}


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

static void Parse2( CTEXTSTR url )
{
	int inchar;
	int outchar;
	TEXTSTR outbuf = NewArray( TEXTCHAR, StrLen( url ) + 1 );
   int state;
	while( url[inchar] )
	{
		int use_char;
		use_char = 0;
		switch( url[inchar] )
		{
		case '&':
			if( state == PARSE_STATE_COLLECT_CGI_VALUE )
            state = PARSE_STATE_COLLECT_CGI_NAME;
         break;
		case '=':
			if( state == PARSE_STATE_COLLECT_CGI_NAME )
            state = PARSE_STATE_COLLECT_CGI_VALUE;
         break;
		case '?':
			if( ( state == PARSE_STATE_COLLECT_EXTENSION )
				|| ( state == PARSE_STATE_COLLECT_NAME )
            || ( state == PARSE_STATE_COLLECT_RESOURCE_ANCHOR )
			  )
				state = PARSE_STATE_COLLECT_CGI_NAME;
         break;
		case '#':
			if( ( state == PARSE_STATE_COLLECT_EXTENSION )
				|| ( state == PARSE_STATE_COLLECT_NAME ) )
				state = PARSE_STATE_COLLECT_RESOURCE_ANCHOR;
         break;
		case '.':
			if( state == PARSE_STATE_COLLECT_RESOURCE_PATH
				|| state == PARSE_STATE_COLLECT_RESOURCE_NAME )
			{
				state = PARSE_STATE_COLLECT_RESOURCE_EXTENSION;
			}
         break;
		case '/':
			if( state == PARSE_STATE_COLLECT_PROTOCOL_1 )
            state = PARSE_STATE_COLLECT_PROTOCOL_2;
			else if( state == PARSE_STATE_COLLECT_PROTOCOL_2 )
				state = PARSE_STATE_COLLECT_USER;
			else if( state == PARSE_STATE_COLLECT_ADDRESS )
				state = PARSE_STATE_COLLECT_RESOURCE_PATH;
			else if( state == PARSE_STATE_COLLECT_PATH )
				state = PARSE_STATE_COLLECT_RESOURCE_NAME;
			else if( state == PARSE_STATE_COLLECT_NAME )
			{
				// this isn't really the, it's another part of the resource path
				state = PARSE_STATE_COLLECT_RESOURCE_NAME;
			}
			else
            use_char = 1;
         break;
		case '@':
			if( ( state == PARSE_STATE_COLLECT_USER )  // hit the colon between user and password
				|| ( state == PARSE_STATE_COLLECT_PASSWORD ) )
				state = PARSE_STATE_COLLECT_ADDRESS;
         break;
		case ':':
			if( state == PARSE_STATE_COLLECT_PROTOCOL )
				state = PARSE_STATE_COLLECT_PROTOCOL_1;
			else if( state == PARSE_STATE_COLLECT_USER )  // hit the colon between user and password
				state = PARSE_STATE_COLLECT_PASSWORD;
			else if( state == PARSE_STATE_COLLECT_ADDRESS )  // hit the colon between address and port
				state = PARSE_STATE_COLLECT_PORT;
			else
            ; // error
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
            usechar = 1;
			}
         break;
		}
      if( usechar )
			outbuf[outchar++] = inbuf[inchar++];
      else
			inchar++;
	}
}


