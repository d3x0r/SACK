// totally useless filter 
// other than this filter provides a shell for all other filters.
#include <stdhdrs.h>
#include <logging.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


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

static int myTypeID;

typedef struct mydatapath_tag {
	DATAPATH common;
   _32 state;
	struct {
		_32 bInvalid : 1;
		_32 bGet : 1;
		_32 bPost : 1;
		_32 bBinary : 1; // reading the content...
		_32 bValue : 1;
		_32 bCollecting : 1;
	} flags;
	PTEXT partial;
	PTEXT filename;
	PTEXT varname, varvalue;
   _32 content_length;
	TEXTCHAR *page;
	TEXTCHAR *cgi;
	TEXTCHAR *http_vars;
   PTEXT content;
   PVARTEXT pvt_result;
} MYDATAPATH, *PMYDATAPATH;


//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

void ProcessPostCGI( PSENTIENT ps, PTEXT block )
{
	PTEXT words = burst( block );
	PTEXT delete_segs = words;
	PTEXT varname = NULL;
	PTEXT varvalue = NULL;
	struct {
		_32 bBrokenValue : 1;
	} flags;
	_32 state = RESET;
   flags.bBrokenValue = 0;
	while( words )
	{
		switch( state )
		{
		case RESET:
			state = CGI_VARNAME;
			continue;
		case CGI_VARNAME:
			if( TextIs( words, WIDE("=") ) )
			{
				HTTPCollapse( &varname );
            state = CGI_VARVALUE;
			}
			else if( TextIs( words, WIDE("&") ) )
			{
				// this is kinda messed up...
				// something like varname1&varname2&varname3 with no actual value?
            // is that legal
            goto AddCGIVariable;
			}
			else
			{
				varname = SegAppend( varname, SegDuplicate( words ) );
			}
         break;
		case CGI_VARVALUE:
			if( TextIs( words, WIDE("=") ) )
			{
				// this would be illegal... something like
				// varname1=val1=junk3&value2&value3=1=3=4=5
            flags.bBrokenValue = 1;
				goto AddCGIVariable; // flush what we have... it's a confused state...
            // though after this next state is
			}
			else if( TextIs( words, WIDE("&") ) )
			{
			AddCGIVariable:
				HTTPCollapse( &varvalue );
				AddVariableExxx( ps, ps->Current, varname, varvalue, FALSE,TRUE,TRUE DBG_SRC );
				//AddVariable( ps, ps->Current, varname, varvalue );
				LineRelease( varname );
				LineRelease( varvalue );
				varname = NULL;
				varvalue = NULL;
				if( flags.bBrokenValue )
				{
               flags.bBrokenValue = 0;
					state = CGI_VARVALUE;

				}
            else
					state = CGI_VARNAME;
			}
			else
			{
				varvalue = SegAppend( varvalue, SegDuplicate( words ) );
			}
         break;
		}
      words = NEXTLINE( words );
	}
   // catch straggler values - end of stream with no &
	if( varvalue || varname )
	{
      if( state == CGI_VARVALUE )
			HTTPCollapse( &varvalue );
      else
			HTTPCollapse( &varname );
		//AddVariable( ps, ps->Current, varname, varvalue );
		AddVariableExxx( ps, ps->Current, varname, varvalue, FALSE,TRUE,TRUE DBG_SRC );
		LineRelease( varname );
		LineRelease( varvalue );
	}
   LineRelease( delete_segs );
}

//---------------------------------------------------------------------------

static PTEXT CPROC HttpRead( PDATAPATH pdp, PTEXT block )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	// lets see... http read handles HTTP GET/ HTTP POST
	// which means the first things to do is find this thing...
	//PTEXT page;// = burst( block );
	PTEXT line;
	PSENTIENT ps;
   ps = pmdp->common.Owner;
	if( pmdp->flags.bInvalid )
	{
      lprintf( WIDE("Throwing away a block, the HTTP connection has reached invalidity.") );
      LineRelease( block );
      return NULL;
	}
	if( !block )
	{
		// this is an end of data stream indicator...
      // causing a flush flush any partial input...
	}
	if( pmdp->state == GET_HTTP_CONTENT )
	{
		// blah... wth
		// gather line hopefully only consumed those
		// characters which were required for that line
		// we need to change read mode into binary block
      //
	}
	// allow for slow entry (a character at a time per block perhaps )
	// Im pretty sure that end of line causes the remainder of block to be saved
   // into partial.  Thus block should be NULL for all times to gather other than the frist.
	for( pmdp->flags.bBinary
		 ? (line=(pmdp->partial?pmdp->partial:block))
		 : (line = GatherLine( &pmdp->partial, NULL, FALSE, FALSE, block ));
		  line;
		  pmdp->flags.bBinary
			  ? (line=pmdp->partial) // happens only the first time we transition from meta_vars to content
			  : ( line = GatherLine( &pmdp->partial, NULL, FALSE, FALSE, NULL ) ) )
	{
		PTEXT words;
		PTEXT delete_seg = NULL;
		// we got a line, therefore we can process it...
		// only thing then is the last line in the block ...
		if( pmdp->flags.bBinary )
         pmdp->partial = NULL;
		if( pmdp->state == GET_HTTP_CONTENT )
		{
         words = line;
		}
		else
		{
			words = burst( line );
			delete_seg = words;
			// though...
			LineRelease( line );
		}
			if( !GetTextSize( words ) )
			{
				if( pmdp->state == GET_HTTP_EOL )
					pmdp->state = GET_HTTP_METAVAR;
				else
				{
					if( pmdp->content_length )
					{
                  pmdp->flags.bBinary = 1;
						pmdp->state = GET_HTTP_CONTENT;
					}
					else
					{
						InvokeBehavior( WIDE("http.request")
										  , pmdp->common.Owner->Current
										  , pmdp->common.Owner
										  , NULL );
						InvokeBehavior( WIDE("http_request")
										  , pmdp->common.Owner->Current
										  , pmdp->common.Owner
										  , NULL );
						// at this point, we should stop gathering the data... it's all over...
						// the rest would be posted data... or junk...
                  // guess it actually could be another request header with a streamer...
					}
				}
			}
			else while( words )
			{
            DECLTEXT( page, WIDE("page") );
            DECLTEXT( CGI, WIDE("CGI") );
				// what does stuff have now?  the whole thign?  a line?
				if( !GetTextSize( words ) ) switch( pmdp->state )
				{
				case GET_HTTP_EOL:
					pmdp->state = GET_HTTP_METAVAR;
               break;
				case GET_HTTP_METAVAR:
				case GET_CGI:
               goto AddCGIVariable;
					break;
				}
            else switch( pmdp->state )
				{
				case RESET:
					pmdp->state = GET_COMMAND;
               continue;  // skip ahead  and try new state;
					break;
				case GET_COMMAND:
					if( TextLike( words, WIDE("GET") ) )
					{
						pmdp->state = GET_FILENAME;
						pmdp->flags.bGet = TRUE;
					}
					else if( TextLike( words, WIDE("POST") ) )
					{
						pmdp->state = GET_FILENAME;
						pmdp->flags.bPost = TRUE;
					}
					else
					{
						pmdp->flags.bInvalid = TRUE;
					}
					break;
				case GET_FILENAME:
					if( !pmdp->filename && TextIs( words, WIDE("/") ) )
					{
						// this is rude, and should never be done,
						// however this filter consumes all data anyhow, SO
                  // mangling this will not hurt much...
						words->format.position.offset.spaces = 0;
					}
					if( TextIs( words, WIDE("?") ) || words->format.position.offset.spaces )
					{
                  if( !words->format.position.offset.spaces )
							pmdp->state = GET_CGI;
                  else
							pmdp->state = GET_HTTP_VERSION;
						pmdp->filename = NEXTLINE( pmdp->filename );
                  LineRelease( SegBreak( pmdp->filename ) );
                  HTTPCollapse( &pmdp->filename );
						AddVariableExxx( ps, ps->Current, (PTEXT)&page, pmdp->filename, FALSE,TRUE,TRUE DBG_SRC );
						AddVariable( ps, ps->Current, (PTEXT)&CGI, TextDuplicate( NEXTLINE( words ), FALSE ) );
						LineRelease( pmdp->filename );
						pmdp->filename = NULL;
					}
					else
					{
						pmdp->filename = SegAppend( pmdp->filename, SegDuplicate( words ) );
					}
					break;
				case GET_CGI:
					if( words->format.position.offset.spaces )
					{
						pmdp->state = GET_HTTP_VERSION;
                  goto AddCGIVariable;
					}
					else
					{
						if( TextIs( words, WIDE("=") ) )
						{
							HTTPCollapse( &pmdp->varname );
                     pmdp->flags.bValue = 1;
						}
						else if( TextIs( words, WIDE("&") ) )
						{
						AddCGIVariable:
							HTTPCollapse( &pmdp->varvalue );
							HTTPCollapse( &pmdp->varname );
							if( TextLike( pmdp->varname, WIDE("content-length") ) )
							{
                        pmdp->content_length= atoi( GetText( pmdp->varvalue ) );
							}
							AddVariableExxx( ps, ps->Current, pmdp->varname, pmdp->varvalue, FALSE,TRUE,TRUE DBG_SRC );
							LineRelease( pmdp->varname );
                     LineRelease( pmdp->varvalue );
							pmdp->varname = NULL;
                     pmdp->varvalue = NULL;
                     pmdp->flags.bValue = 0;
						}
						else
						{
							if( pmdp->flags.bValue )
							{
                        pmdp->varvalue = SegAppend( pmdp->varvalue, SegDuplicate( words ) );
							}
							else
							{
                        pmdp->varname = SegAppend( pmdp->varname, SegDuplicate( words ) );
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
                  // TextIs( words, WIDE("/") ); // this is a token before the number...
						// Version better be something like 1.1 1.0?
						// well wait for EOL...
						pmdp->state = GET_HTTP_EOL;
					}
					break;
				case GET_HTTP_EOL:
					if( !GetTextSize( words ) )
					{
						pmdp->state = GET_HTTP_METAVAR;
					}
					break;
				case GET_HTTP_METAVAR:
					{
						if( !pmdp->flags.bValue && TextIs( words, WIDE(":") ) )
						{
                     pmdp->flags.bValue = TRUE;
						}
						else
						{
							if( pmdp->flags.bValue )
							{
                        pmdp->varvalue = SegAppend( pmdp->varvalue, SegDuplicate( words ) );
							}
							else
							{
                        pmdp->varname = SegAppend( pmdp->varname, SegDuplicate( words ) );
							}
						}
					}
               break;
				case GET_HTTP_CONTENT:
					if( !pmdp->content_length )
					{
                  DebugBreak();
                  pmdp->state = RESET;
					}
					else
					{
						// hmm we've parsed everything up to here, but now we need blocks, binary blocks.
						pmdp->content = SegAppend( pmdp->content, words );
						if( LineLength( pmdp->content ) == pmdp->content_length )
						{
							ProcessPostCGI( pmdp->common.Owner, pmdp->content );
							AddVariableExxx( ps, ps->Current, (PTEXT)&CGI, pmdp->content, FALSE,TRUE,TRUE DBG_SRC );
							//AddVariable( ps, ps->Current, (PTEXT)&CGI, pmdp->content );
							LineRelease( pmdp->content );

							InvokeBehavior( WIDE("http.request"), pmdp->common.Owner->Current, pmdp->common.Owner, NULL );
							InvokeBehavior( WIDE("http_request"), pmdp->common.Owner->Current, pmdp->common.Owner, NULL );
							pmdp->state = RESET;
						}
                  words = NULL;
					}
					break;
				}
				words = NEXTLINE( words );
			}
         LineRelease( delete_seg );
	}
   LineRelease( block );
	return NULL;
}


//---------------------------------------------------------------------------

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, HttpRead );
}

//---------------------------------------------------------------------------

static PTEXT CPROC HttpWrite( PDATAPATH pdp, PTEXT block )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	PTEXT tmp = NULL;
	if( (block->flags & TF_INDIRECT) || NEXTLINE( block ) )
		tmp = BuildLine( block );
	VarTextAddData( pmdp->pvt_result, GetText( tmp?tmp:block ), GetTextSize( tmp?tmp:block ) );
	if( tmp )
		LineRelease( tmp );
   return block;
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( (PDATAPATH)pdp, HttpWrite );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdp )
{
   PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	VarTextDestroy( &pmdp->pvt_result );
   pmdp->common.Type = 0; // allow delete
   return 0;
}

//---------------------------------------------------------------------------

static int HandleOption( WIDE("http"), WIDE("flush"), WIDE("Sends all gathered output to the requesting client (completes with content-length)") )( PDATAPATH pdp, PSENTIENT ps, PTEXT params )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	PTEXT send = VarTextPeek( pmdp->pvt_result );
	if( send )
	{
      PVARTEXT pvt_header = VarTextCreate();
		vtprintf( pvt_header, WIDE("HTTP-1.1 200 OK\r\n") );
		vtprintf( pvt_header, WIDE("Context-Length: %d\r\n"), GetTextSize( send ) );
		vtprintf( pvt_header, WIDE("\r\n") );
      D_MSG( pdp->pPrior, GetText( VarTextPeek( pvt_header ) ) );
      D_MSG( pdp->pPrior, GetText( send ) );
      VarTextDestroy( &pvt_header );
	}
   return 1;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( strnicmp( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   //PTEXT option;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->common.Type = myTypeID;
   pdp->common.Read = Read;
   pdp->common.Write = Write;
	pdp->common.Close = Close;
   // burst unfriendly name.
	AddBehavior( ps->Current, WIDE("http.request"), WIDE("http request complete, please handle request.") );
   // burst friendly name.
	AddBehavior( ps->Current, WIDE("http_request"), WIDE("http request complete, please handle request.") );
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("http"), WIDE("Handles HTTP request parsing..."), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("http") );
}
// $Log: nil.c,v $
// Revision 1.9  2005/02/21 12:08:34  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.8  2005/01/17 08:45:48  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.7  2004/04/05 22:58:54  d3x0r
// Take advantage of extended 16 bit col pos
//
// Revision 1.6  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.5  2003/03/31 01:19:08  panther
// Remove unused option
//
// Revision 1.4  2003/03/25 08:59:02  panther
// Added CVS logging
//
