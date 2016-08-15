#define NO_FILEOP_ALIAS
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <configscript.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <image.h>


enum {
	RESET = 0
	  , GET_COMMAND
	  , GET_FILENAME
	  , GET_CGI
	  , GET_HTTP_VERSION
	  , GET_HTTP_METAVAR
	  , GET_HTTP_CONTENT
	  , GET_HTTP_EOL
	  , CGI_VARNAME
     , CGI_VARVALUE
};


struct VAR
{
	PTEXT varname;
   PTEXT varvalue;
};

static struct {
   PLIST vars;
} l;


static PTEXT GetSubst( int _2bytecode )
{
	DECLTEXTSZ( subst, 2);
   subst.data.size = 1;
	switch( _2bytecode )
	{
	case '+': subst.data.data[0] = ' '; return SegDuplicate( (PTEXT)&subst );
	case '02': subst.data.data[0] = '\x20'; return SegDuplicate( (PTEXT)&subst );
	case 'c2': case 'C2': subst.data.data[0] = '\x2C'; return SegDuplicate( (PTEXT)&subst );
	case 'b5': case 'B5': subst.data.data[0] = '\x5B'; return SegDuplicate( (PTEXT)&subst );
	case 'd5': case 'D5': subst.data.data[0] = '\x5D'; return SegDuplicate( (PTEXT)&subst );
	case 'f2': case 'F2': subst.data.data[0] = '\x2F'; return SegDuplicate( (PTEXT)&subst );
	case '22': subst.data.data[0] = '\x22'; return SegDuplicate( (PTEXT)&subst );
	case '72': subst.data.data[0] = '\x27'; return SegDuplicate( (PTEXT)&subst );
	case '62': subst.data.data[0] = '\x26'; return SegDuplicate( (PTEXT)&subst );
	}
   return NULL;
}


void HTTPCollapse( PTEXT *ppText )
{
	PTEXT output;
	PTEXT input = *ppText;
	while( input )
	{
		if( GetText( input )[0] == '+' )
		{
			PTEXT subst;
			// sometimes a + can be attached to a number
			// so much for the natural language parser dealing with
         // a machine oriented protocol...
			if( GetTextSize( input ) > 1 )
			{
            SegSplit( &input, 1 );
			}
         subst = GetSubst( '+' );
			SegInsert( subst, input );
			LineRelease( SegGrab( input ) );
         input = subst;
			if( !PRIORLINE( input ) )
            (*ppText) = input;
		}
		else if( TextIs( input, WIDE("%") ) )
		{
			PTEXT next = NEXTLINE( input );
			if( next )
			{
				PTEXT subst;
				SegSplit( &next, 2 );
				subst = GetSubst( *(unsigned short*)GetText( next ) );
				if( subst )
				{
					PTEXT nextnext = NEXTLINE( next );
					PTEXT prior = SegBreak( input );
					if( !prior )
					{
						*ppText = SegAppend( subst, nextnext );
					}
					if( nextnext )
					{
						SegBreak( nextnext );
						if( !prior )
							*ppText = nextnext;
						else
							SegAppend( prior, nextnext );
						SegInsert( subst, nextnext );
						input = nextnext;
						continue;
					}
					else
					{
						SegAppend( prior, (PTEXT)&subst );
						input = NULL;
						continue;
					}
				}
            // if not subst... just continue stepping, no replacement nessecary?
			}
		}
      input = NEXTLINE( input );
	}
	output = BuildLine( *ppText );
	LineRelease( *ppText );
   (*ppText) = output;
}



void ParseURI( CTEXTSTR string )
{
   int state;
	PTEXT words;
	PTEXT delete_seg = NULL;
	PTEXT line = SegCreateFromText( string );
	PTEXT filename = NULL;
   PTEXT varname = NULL;
	PTEXT varvalue = NULL;
   PTEXT content = NULL;
   int content_length;

	struct {
		uint32_t bInvalid : 1;
		uint32_t bGet : 1;
		uint32_t bPost : 1;
		uint32_t bBinary : 1; // reading the content...
		uint32_t bValue : 1;
	} flags;
// we got a line, therefore we can process it...
	// only thing then is the last line in the block ...

	state = GET_FILENAME;

	words = burst( line );
	delete_seg = words;
	// though...
	LineRelease( line );
	flags.bValue = 0;

	while( words )
	{
		DECLTEXT( page, WIDE("page") );
		DECLTEXT( CGI, WIDE("CGI") );
		//printf( "state:%d word:%s \r\n",state, GetText(words ) );
		// what does stuff have now?  the whole thign?  a line?
		if( !GetTextSize( words ) ) switch( state )
		{
		case GET_HTTP_EOL:
			state = GET_HTTP_METAVAR;
			break;
		case GET_HTTP_METAVAR:
		case GET_CGI:
			goto AddCGIVariable;
			break;
		}
		else switch( state )
		{
		case RESET:
			state = GET_COMMAND;
			continue;  // skip ahead  and try new state;
			break;
		case GET_COMMAND:
			if( TextLike( words, WIDE("GET") ) )
			{
				state = GET_FILENAME;
				//flags.bGet = TRUE;
			}
			else if( TextLike( words, WIDE("POST") ) )
			{
				state = GET_FILENAME;
				//flags.bPost = TRUE;
			}
			else
			{
				flags.bInvalid = TRUE;
			}
			break;
		case GET_FILENAME:
			if( !filename && TextIs( words, WIDE("/") ) )
			{
				// this is rude, and should never be done,
				// however this filter consumes all data anyhow, SO
				// mangling this will not hurt much...
				words->format.position.offset.spaces = 0;
			}
			if( TextIs( words, WIDE("?") ) || words->format.position.offset.spaces )
			{
				if( !words->format.position.offset.spaces )
					state = GET_CGI;
				else
					state = GET_HTTP_VERSION;
				filename = NEXTLINE( filename );
				LineRelease( SegBreak( filename ) );
				HTTPCollapse( &filename );
				//AddVariableExxx( ps, ps->Current, (PTEXT)&page, filename, FALSE,TRUE,TRUE DBG_SRC );
				//AddVariable( ps, ps->Current, (PTEXT)&CGI, TextDuplicate( NEXTLINE( words ), FALSE ) );
				LineRelease( filename );
				filename = NULL;
			}
			else
			{
				filename = SegAppend( filename, SegDuplicate( words ) );
			}
			break;
		case GET_CGI:
			if( words->format.position.offset.spaces )
			{
				state = GET_HTTP_VERSION;
				goto AddCGIVariable;
			}
			else
			{
				if( TextIs( words, WIDE("=") ) )
				{
					HTTPCollapse( &varname );
					flags.bValue = 1;
				}
				else if( TextIs( words, WIDE("&") ) )
				{
				AddCGIVariable:
					HTTPCollapse( &varvalue );
					HTTPCollapse( &varname );
					if( TextLike( varname, WIDE("content-length") ) )
					{
						content_length= IntCreateFromText( GetText( varvalue ) );
					}
					{
						struct VAR *v = New( struct VAR );
						v->varname = varname;
						varname = NULL;
						v->varvalue = varvalue;
						varvalue = NULL;
						AddLink( &l.vars, v );
					}
					//AddVariableExxx( ps, ps->Current, pmdp->varname, pmdp->varvalue, FALSE,TRUE,TRUE DBG_SRC );
					//LineRelease( varname );
					//LineRelease( varvalue );
					//varname = NULL;
					//varvalue = NULL;
					flags.bValue = 0;
				}
				else
				{
					if( flags.bValue )
					{
						varvalue = SegAppend( varvalue, SegDuplicate( words ) );
					}
					else
					{
						//printf( "add var" );
						varname = SegAppend( varname, SegDuplicate( words ) );
					}
				}
			}
			break;
		case GET_HTTP_VERSION:
			if( TextIs( words, WIDE("HTTP") ) )
					{
						// okay - don't really do anything... next word is the version...
					}
					else
					{
                  // TextIs( words, "/" ); // this is a token before the number...
						// Version better be something like 1.1 1.0?
						// well wait for EOL...
						state = GET_HTTP_EOL;
					}
					break;
				case GET_HTTP_EOL:
					if( !GetTextSize( words ) )
					{
						state = GET_HTTP_METAVAR;
					}
					break;
				case GET_HTTP_METAVAR:
					{
						if( !flags.bValue && TextIs( words, WIDE(":") ) )
						{
                     flags.bValue = TRUE;
						}
						else
						{
							if( flags.bValue )
							{
                        varvalue = SegAppend( varvalue, SegDuplicate( words ) );
							}
							else
							{
                        varname = SegAppend( varname, SegDuplicate( words ) );
							}
						}
					}
               break;
				case GET_HTTP_CONTENT:
					if( !content_length )
					{
                  DebugBreak();
                  state = RESET;
					}
					else
					{
						// hmm we've parsed everything up to here, but now we need blocks, binary blocks.
						content = SegAppend( content, words );
						if( LineLength( content ) == content_length )
						{
							//ProcessPostCGI( common.Owner, content );
							//AddVariableExxx( ps, ps->Current, (PTEXT)&CGI, content, FALSE,TRUE,TRUE DBG_SRC );
							//AddVariable( ps, ps->Current, (PTEXT)&CGI, content );
							LineRelease( content );

							//InvokeBehavior( "http.request", common.Owner->Current, common.Owner, NULL );
							//InvokeBehavior( "http_request", common.Owner->Current, common.Owner, NULL );
							state = RESET;
						}
                  words = NULL;
					}
					break;
		}
		words = NEXTLINE( words );
	}
	LineRelease( delete_seg );
	if( varname && varvalue )
	{
		HTTPCollapse( &varvalue );
		HTTPCollapse( &varname );
		{
			struct VAR *v = New( struct VAR );
			v->varname = varname;
			varname = NULL;
			v->varvalue = varvalue;
			varvalue = NULL;
			AddLink( &l.vars, v );
		}
	}
}

void DumpValue( void )
{
	INDEX idx;
	struct VAR *v;
	LIST_FORALL( l.vars, idx, struct VAR*, v )
	{
      lprintf( WIDE("%s=%s"), GetText( v->varname ), GetText( v->varvalue ) );
	}
}
CTEXTSTR GetValue( CTEXTSTR name )
{
	INDEX idx;
	struct VAR *v;
	LIST_FORALL( l.vars, idx, struct VAR*, v )
	{
		if( StrCaseCmp( GetText( v->varname ), name )== 0)
		{
         return GetText( v->varvalue );
		}
	}
   return NULL;
}

PTEXT GetValueText( CTEXTSTR name )
{
	INDEX idx;
	struct VAR *v;
	LIST_FORALL( l.vars, idx, struct VAR*, v )
	{
		if( StrCaseCmp( GetText( v->varname ), name )== 0)
		{
         return v->varvalue;
		}
	}
   return NULL;
}

int main( void )
{
	const TEXTCHAR *method = getenv( WIDE("REQUEST_METHOD") );
	const TEXTCHAR *url = getenv( WIDE("QUERY_STRING") );
	ParseURI( getenv( WIDE("REQUEST_URI") ) );
	DumpValue();
	//printf( "content-type:plain/text\r\n\r\n" );
	{
		const TEXTCHAR *file = GetValue( WIDE("file") );
      PTEXT red = GetValueText( WIDE("red") );
      PTEXT green = GetValueText( WIDE("green") );
		PTEXT blue = GetValueText( WIDE("blue") );
		PTEXT x_source = GetValueText( WIDE("x") );
		PTEXT y_source = GetValueText( WIDE("y") );
		PTEXT width_source = GetValueText( WIDE("width") );
		PTEXT height_source = GetValueText( WIDE("height") );
      CDATA cred, cblue, cgreen;
		if( file && red && blue && green )
		{
         int x, y, width, height;
			Image image = LoadImageFile( file );
         if( x_source )
				x = IntCreateFromSeg( x_source );
			else
				x = 0;
         if( y_source )
				y = IntCreateFromSeg( y_source );
			else
				y = 0;
         if( width_source )
				width = IntCreateFromSeg( width_source );
			else
				width = image->width;
         if( height_source )
				height = IntCreateFromSeg( height_source );
			else
				height = image->height;

			if( image )
			{
				Image out = MakeImageFile( width, height );
            lprintf(WIDE("%s %s %s"), GetText( red ), GetText( green) , GetText( blue ) );
				if( !GetColorVar( &red, &cred ) )
               lprintf( WIDE("FAIL RED") );
				if( !GetColorVar( &blue, &cblue ) )
					lprintf( WIDE("FAIL BLUE") );

				if( !GetColorVar( &green, &cgreen ) )
					lprintf( WIDE("FAIL gREEN") );

				ClearImage( out );
				lprintf( WIDE("uhmm... %08x %08x %08x"), cred, cgreen, cblue );
				BlotImageSizedEx( out, image, 0, 0, x, y, width, height, ALPHA_TRANSPARENT, BLOT_MULTISHADE, cred, cgreen, cblue );

				//BlotImageMultiShaded( out, image, 0, 0, cred, cgreen, cblue );
				{
					size_t size;
					uint8_t *buf;
					if( PngImageFile( out, &buf, &size ) )
					{
						printf( "content-length:%d\r\n", size );
						printf( "content-type:image/png\r\n" );
						printf( "\r\n" );
						fwrite( buf, 1, size, stdout );
					}
				}
			}
         else
			{
				printf( "content-type:text/plain\r\n" );
				printf( "\r\n" );
				printf( "Bad Image." );
			}
		}
		else
		{
			printf( "content-type:text/plain\r\n" );
			printf( "\r\n" );
         printf( "Bad arguments." );
		}
	}
	return 0;
}
