#define NO_UNICODE_C
#ifdef _MSC_VER
//#define NO_CRT_SECURE_WARNINGS
#endif

#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sharemem.h>
#include <image.h>
#include <colordef.h>
#include <loadsock.h> // sockaddr, createaddress(str,defport)
#include <network.h>
#include <procreg.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#include <filesys.h>
#define FULL_TRACE
#define LOG_LINES_READ
////#define DEBUG_SLOWNESS
//#define DEBUG_SAVE_CONFIG

//#if defined( GCC ) && !defined( __arm__ )
//#define NEED_ASSEMBLY_CALLER
//#endif

#define DO_LOGGING
#include <logging.h>
#include <configscript.h>
#include <fractions.h>

#ifdef __cplusplus
namespace sack { namespace config {
using namespace sack::math::fraction;
#endif
#ifdef __LINUX__
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define CompareText(l1,l2)	( StrCaseCmp( GetText(l1), GetText(l2) ) )

// all matches to content are done case insensitive.

// consider vector/array declarations

enum config_types {
	CONFIG_UNKNOWN
	// must match case-insensative exact length.
	// literal text
	, CONFIG_TEXT
	// a yes/no field may be 0/1, y[es]/n[o], on/off
	//
	, CONFIG_YESNO
	, CONFIG_TRUEFALSE = CONFIG_YESNO
	, CONFIG_ONOFF = CONFIG_YESNO
	, CONFIG_OPENCLOSE = CONFIG_YESNO
	, CONFIG_BOOLEAN = CONFIG_YESNO

	// may not have a . point - therefore the . is a terminator and needs
	// to match the next word.
	, CONFIG_INTEGER
	// has to be a floating point number (perhaps integral ie. no decimal)
	, CONFIG_FLOAT

	// binary data storage - stored as base64 encoded passed as PDATA
	, CONFIG_BINARY

	// formated number [+/-][[ ]## ]##[/##]
	, CONFIG_FRACTION

	// matches any single word.
	, CONFIG_SINGLE_WORD

	// protocol://[user[:password]](ip/name)[:port][/filepath][?cgi]
	// by convention this will not contain spaces... but perhaps
	// &20; (?)
	, CONFIG_URL

	// matches several words in a row - the end word to match is supplied.
	, CONFIG_MULTI_WORD
	// file name - does not have any path part.
	// the following are all treated like multi_word since file names/paths
	// may contain spaces
	, CONFIG_FILE
	// ends in a / or \,
	, CONFIG_PATH
	// may have path and filename
	, CONFIG_FILEPATH

	// (IP/name)[:port]
	, CONFIG_ADDRESS

	// end of configuration line (match assumed)
	, CONFIG_PROCEDURE
	, CONFIG_PROCEDURE_EX

	, CONFIG_COLOR
	, CONFIG_NOTHING // to save as an option after variables
};

typedef struct config_element_tag CONFIG_ELEMENT, *PCONFIG_ELEMENT;
struct config_element_tag
{
	enum config_types type;
	struct config_test_tag *next;// if a match is found, follow this to next.
	struct config_element_tag *prior;
	struct config_element_tag **ppMe; // this is where we came from
	struct {
		BIT_FIELD vector : 1;
		BIT_FIELD multiword_terminator : 1; // prior == actual segment...
		BIT_FIELD singleword_terminator : 1; // prior == actual segment...
		// careful - assembly here requires absolute known
		// posisitioning - -fpack-struct will short-change
		// this structure to the minimal number of bits.
		BIT_FIELD filler:29;
	} flags;
	uint32_t element_count; // used with vector fields.
	union {
		PTEXT pText;
		struct {
				LOGICAL bTrue;
		} truefalse;
		uint64_t integer_number;
		double float_number;
		TEXTSTR pWord; // also pFilename, pPath, pURL
		struct {
			TEXTSTR pWord; // also pFilename, pPath, pURL
			PLIST pEnds; // also this ends single word...
			struct config_element_tag *pWhichEnd;
		} singleword;
		// maybe pURL should be burst into
		//	( address, user, password, path, page, params )
		SOCKADDR *psaSockaddr;
		struct {
			TEXTSTR pWords;
				 // next thing to match...
				 // this is probably a constant text thing, but
				 // may be say an integer, filename, or some known
				 // format thing...
			PLIST pEnds;//struct config_element_tag *pEnd;
			struct config_element_tag *pWhichEnd;
		} multiword;
		FRACTION fraction;
		USER_CONFIG_HANDLER Process;
		struct {
			USER_CONFIG_HANDLER_EX Process;
			uintptr_t arg;
		} ProcessEx;
		CDATA Color;
		struct {
			size_t length;
			POINTER data;
		} binary;
	} data[1]; // these are value holders... if there is a vector field,
				// either the count will be specified, or this will have to
				// be auto expanded....
};

#define CONFIG_EMPTY_EXTRA ,NULL,NULL,NULL,{0,0,0,0},0,{{0}}

typedef struct config_test_tag
{
	// this constant list could be a more optimized structure like
	// a tree...
	PLIST pConstElementList;  // list of words which are constant to be checked.
	PLIST pVarElementList;	// list of fields which are variables.
} CONFIG_TEST, *PCONFIG_TEST;

#define MAXCONFIG_TESTSPERSET 128
DeclareSet( CONFIG_TEST );
#define MAXCONFIG_ELEMENTSPERSET 128
DeclareSet( CONFIG_ELEMENT );

typedef struct config_file_tag CONFIG_HANDLER;
struct config_file_tag
{
	// this needs to be the first element - 
	// address of this IS the address of main structure
	CONFIG_TEST ConfigTestRoot;
	PDATASTACK ConfigStateStack;

	FILE *file;
	//gcroot<System::IO::StreamReader^> sr;
	//gcroot<System::IO::FileStream^> fs;
	PTEXT blocks;

	// this should be more than one...
	// and each will be called in order that it was
	// added, very importatn first in first process
	PLIST filters;
	PLIST filter_data; // a scratch pointer addrss passd to each filter ... (per config handler)
	//uintptr_t (CPROC *filter)(uintptr_t,int *,char**);

	uintptr_t psvUser;
	uintptr_t (CPROC *EndProcess)( uintptr_t );
	uintptr_t (CPROC *Unhandled)( uintptr_t, CTEXTSTR );

	PCONFIG_ELEMENTSET elements;
	PCONFIG_TESTSET test_elements;
	LOGICAL config_recovered;
	CTEXTSTR save_config_as;
	PLIST states; // history of saved coniguration states...
	struct config_file_flags {
		BIT_FIELD bConfigSaveNameUsed : 1; // if used, don't release
		BIT_FIELD bUnicode : 1;
		BIT_FIELD bUnicode8 : 1;
	} flags;
};


typedef struct configuation_state *PCONFIG_STATE;
typedef struct configuation_state {
	LOGICAL recovered;
	CTEXTSTR name;
	CONFIG_TEST ConfigTestRoot;
	uintptr_t psvUser;
	uintptr_t (CPROC *EndProcess)( uintptr_t );
	uintptr_t (CPROC *Unhandled)( uintptr_t, CTEXTSTR );
} CONFIG_STATE;

struct configscript_global {
	//LOGICAL bSaveMemDebug;
	int _last_allocate_logging;
	int _disabled_allocate_logging;

	struct {
		BIT_FIELD bInitialized : 1;
		BIT_FIELD bDisableMemoryLogging : 1;
		BIT_FIELD bLogUnhandled : 1;
		BIT_FIELD bLogTrace : 1;
		BIT_FIELD bLogLines : 1;
	} flags;
};

#ifdef g
#  undef g
#endif
#ifndef __STATIC_GLOBALS__
static struct configscript_global *global_config_data;
PRIORITY_PRELOAD( InitGlobalConfigScript, CONFIG_SCRIPT_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( global_config_data );
}

#else
static struct configscript_global _global_config_data;
static struct configscript_global *global_config_data = &_global_config_data;
#endif
#define g (*global_config_data)

void DoInit( void )
{
#ifndef __STATIC_GLOBALS__
	if( !global_config_data )
		SimpleRegisterAndCreateGlobal( global_config_data );
#endif

	if( !g.flags.bInitialized )
	{
		g.flags.bDisableMemoryLogging = 1;
		g.flags.bLogUnhandled = 0;
		g.flags.bInitialized = 1;
	}
}

PRELOAD( InitGlobalConfig2 )
{
	DoInit();
#if !defined( __NO_OPTIONS__ )
	// later, set options - core startup configs like sql.config cannot read options.
	g.flags.bDisableMemoryLogging = SACK_GetProfileIntEx( "SACK/Config Script", "Disable Memory Logging", g.flags.bDisableMemoryLogging, TRUE );
	g.flags.bLogUnhandled = SACK_GetProfileIntEx( "SACK/Config Script", "Log Unhandled if no application handler", g.flags.bLogUnhandled, TRUE );
	g.flags.bLogTrace = SACK_GetProfileIntEx( "SACK/Config Script", "Log configuration processing(trace)", g.flags.bLogTrace, TRUE );
	g.flags.bLogLines = SACK_GetProfileIntEx( "SACK/Config Script", "Log configuration line input", g.flags.bLogLines, TRUE );
#endif
}


//#else
//#define g global_config_data
//static GLOBAL g;
//#endif


#include <configscript.h>

//---------------------------------------------------------------------

void LogElementEx( CTEXTSTR leader, PCONFIG_ELEMENT pce DBG_PASS)
#define LogElement(leader,pc) LogElementEx(leader,pc DBG_SRC )

{
	if( !pce )
	{
		_lprintf(DBG_RELAY)( "Nothing." );
		return;
	}
	switch( pce->type )
	{
	case CONFIG_UNKNOWN:
		_lprintf(DBG_RELAY)( "This thing was never configured?" );
		break;
	case CONFIG_TEXT:
		_lprintf(DBG_RELAY)( "%s text constant: %s", leader, GetText( pce->data[0].pText ) );
		break;
	case CONFIG_BOOLEAN:
		_lprintf(DBG_RELAY)( "%s a boolean", leader );
		break;
	case CONFIG_INTEGER:
		_lprintf(DBG_RELAY)( "%s integer", leader );
		break;
	case CONFIG_COLOR:
		_lprintf(DBG_RELAY)( "%s color", leader );
		break;
	case CONFIG_BINARY:
		_lprintf(DBG_RELAY)( "%s binary", leader );
		break;
	case CONFIG_FLOAT:
		_lprintf(DBG_RELAY)( "%s Floating", leader );
		break;
	case CONFIG_FRACTION:
		_lprintf(DBG_RELAY)( "%s fraction", leader );
		break;
	case CONFIG_SINGLE_WORD:
		_lprintf(DBG_RELAY)( "%s a single word:%p", leader, pce->data[0].pWord );
		break;
	case CONFIG_MULTI_WORD:
		_lprintf(DBG_RELAY)( "%s a multi word", leader );
		break;
	case CONFIG_PROCEDURE:
		_lprintf(DBG_RELAY)( "%s a procedure to call.", leader );
		break;
	case CONFIG_PROCEDURE_EX:
		_lprintf( DBG_RELAY )("%s a procedure to call.", leader);
		break;
	case CONFIG_URL:
	_lprintf(DBG_RELAY)( "%s a url?", leader );
	break;
	case CONFIG_FILE:
	_lprintf(DBG_RELAY)( "%s a filename", leader );
	break;
	case CONFIG_PATH:
	_lprintf(DBG_RELAY)( "%s a path name", leader );
	break;
	case CONFIG_FILEPATH:
	_lprintf(DBG_RELAY)( "%s a full path and file name", leader );
	break;
	case CONFIG_ADDRESS:
	_lprintf(DBG_RELAY)( "%s an address", leader );
	break;
	default:
		_lprintf(DBG_RELAY)( "Do not know what this is." );
		break;
	}
}

//---------------------------------------------------------------------

void DumpConfigurationEvaluator( PCONFIG_HANDLER pch )
{
	INDEX idx;
	PCONFIG_ELEMENT pce;
	LIST_FORALL( pch->ConfigTestRoot.pConstElementList, idx, PCONFIG_ELEMENT, pce )
	{
		LogElement( "const", pce );
	}
	LIST_FORALL( pch->ConfigTestRoot.pVarElementList, idx, PCONFIG_ELEMENT, pce )
	{
		LogElement( "var", pce );
	}
}

//---------------------------------------------------------------------

// man what a friggin mess just to deal with ANY
// kind of mangled input and spit out some kinda lines
// uhmm...
static PTEXT CPROC FilterLines( POINTER *scratch, PTEXT buffer )
{
	struct my_scratch_data {
		size_t skip;
		int lastread;
		PTEXT linebuf;
	} *data = (struct my_scratch_data*)scratch[0];
	size_t total_length;
	size_t n;
	size_t thisskip;
	if( !data && buffer )
	{
		scratch[0] = data = New( struct my_scratch_data );
		data[0].skip = 0;
		data[0].linebuf = 0;
		data[0].lastread = 0;
	}
	else if( !data )
		return NULL;
	thisskip = data->skip; // skip N characters in first buffer.
	if( buffer )
	{
		data->lastread = FALSE;
		data->linebuf = SegAppend( data->linebuf, buffer );
	}
#if 0
	// this routine is a low level raw data input, line result routine.
	// it would be the sort of thing that produces zerosegs.
	// this will NEVER run - the above condition catches it.
	else if( buffer && !GetTextSize( buffer ) )
	{
		// if the buffer is a zeroseg, then it's a end of line marker.
		// assuming other things filter before this.... in reality
		// burst returns newlines I think?
		PTEXT text = data->linebuf;
		data->lastread = FALSE;
		Release( scratch[0] );
		scratch[0] = NULL;
		if( text )
		{
			LineRelease( buffer );
			lprintf( "Returning buffer [%s]", GetText( text ) );
			return text;
		}
		else
		{
			lprintf( "Returning buffer [%s]", GetText( buffer ) );
			return buffer; // pass it on to others - end of stream..
		}
	}
#endif
	else if( !buffer )
	{
		if( data->lastread )
		{
			PTEXT final = NULL;
			if( data->linebuf )
				final = SegCreateFromText( GetText( data->linebuf ) + data->skip );
			LineRelease( data->linebuf );
			Release( scratch[0] );
			scratch[0] = NULL;
			if( g.flags.bLogLines )
				lprintf( "Returning buffer [%s]", GetText( final ) );
			return final;
		}
	}
	buffer = data->linebuf;
	total_length = 0;
	while( buffer )
	{
		// full new buffer, which may or may not add to prior segments...
		int end = 0;
		size_t length = GetTextSize( buffer );
		CTEXTSTR chardata = GetText( buffer );
		//lprintf( "Considering buffer %s", GetText( buffer ) + data->skip );
		if( !length )
			LineRelease( SegGrab( buffer ) );
		for( n = thisskip; n < length; n++ )
		{
			if( chardata[n] == '\n' ||
				chardata[n] == '\r' )
			{
				if( chardata[n] == '\n' )
				{
					end = 1;
					n++; // include this character.
					//lprintf( "BLANK LINE - CONSUMED" );
					break;
				}
				if( end ) // \r\r is two lines too
				{
					break;
				}
				end = 1;
			}
			else if( end )
			{
				// any other character... after a \r.. don't include the character.
				break;
			}
		}
		total_length += n - thisskip;
		if( end )
		{
			// new character, trim at -1 from here...
			PTEXT result = SegCreate( (int32_t)total_length );
			size_t ofs;
			buffer = data->linebuf;
			thisskip = data->skip;
			n = thisskip;
			ofs = 0;
			while( ofs < total_length )
			{
				size_t len = GetTextSize( buffer );
				if( len > ( len - thisskip ) )
					len = len - thisskip;
				if( len > ( total_length - ofs ) )
					len = total_length - ofs;
				MemCpy( GetText( result ) + ofs
					, GetText( buffer ) + thisskip
					, sizeof( TEXTCHAR)*len );
				ofs += len;
				n += len;
				if( ofs < total_length )
				{
					n = 0;
					thisskip = 0;
					buffer = NEXTLINE( buffer );
				}
			}
			if( buffer )
			{
				data->skip = n;
				LineRelease( SegBreak( buffer ) );
				data->linebuf = buffer;
			}
			else
				data->skip = 0;
			GetText(result)[total_length] = 0;
			//lprintf( "Considering buffer %s", GetText( result ) );
			if( g.flags.bLogLines )
				lprintf( "Returning buffer [%s]", GetText( result ) );
			return result;
		}
		else
		{
			//lprintf( "Had no end within the buffer, waiting for another..." );
		}
		thisskip = 0; // no more skips.
		buffer = NEXTLINE( buffer );
	}
	data->lastread = TRUE;
	return NULL;
}

//---------------------------------------------------------------------

static PTEXT CPROC FilterTerminators( POINTER *scratch, PTEXT buffer )
{
	struct my_scratch_data {
		PTEXT newline;
	} *data = (struct my_scratch_data*)scratch[0];
	if( !data && buffer )
	{
		scratch[0] = data = New( struct my_scratch_data );
		data[0].newline = NULL;
	}
	else if( !data )
		return NULL;
	if( !buffer )
	{
		if( data[0].newline )
		{
			PTEXT tmp = data[0].newline;
			data[0].newline = NULL;
			return tmp;
		}
		else
			return NULL;
	}
	{
		int modified;
		int end = TRUE;
		// filter \r\n\\ just cause...
		data[0].newline = SegAppend( data[0].newline, buffer );
		while( buffer )
		{
			TEXTSTR chardata = GetText( buffer );
			size_t length = GetTextSize( buffer );
			//LogBinary( chardata, length );
			do
			{
				modified = 0;
				if( length && chardata[length-1] == '\n' )
				{
					//Log( "Removing newline..." );
					end = TRUE;
					length--;
					modified = 1;
				}
				if( length && chardata[length-1] == '\r' )
				{
					//lprintf( "Removing cr..." );
					end = TRUE;
					length--;
					modified = 1;
				}
				if( length && chardata[length-1] == '\\' )
				{
					if( ( length > 1 ) && ( chardata[length-2] != '\\' ) )
					{
						//lprintf( "Removing continue slash..." );
						end = FALSE;
						length--;
						modified = 1;
					}
				}
			} while( modified );
			if( !length )
			{
				LineRelease( SegGrab( buffer ) );
				data[0].newline = NULL;
				return NULL;
			}
			chardata[length] = 0;
			//lprintf( "Resulting line: %s", chardata );
			SetTextSize( buffer, length );
			buffer = NEXTLINE( buffer );
		}

		if( end )
		{
			PTEXT result = data[0].newline;
			data[0].newline = NULL;
			return result;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------

/* does not need scratch buffer... */
static PTEXT CPROC FilterEscapesAndComments( POINTER *scratch, PTEXT pText )
{
	TEXTSTR text = GetText( pText );
	PTEXT pNewText;
   (void)scratch;
	if( text /*&& strchr( text, '\\' )*/ )
	{
		PTEXT tmp;
		tmp = pNewText = TextDuplicate( pText, FALSE );
		while( tmp )
		{
			int dest = 0, src = 0;
			text = GetText( tmp );
			while( text && text[src] )
			{
				if( text[src] == '\\' )
				{
					src++;
					switch( text[src] )
					{
					case 0:
						lprintf( "Continuation at end of line... save this and append next line please." );
						break;
					default:
						text[dest++] = text[src];
						break;
					}
				}
				else
				{
					if( text[src] == '#' )
						break;
					text[dest++] = text[src];
				}
				src++;
			}
			text[dest] = 0;
			SetTextSize( tmp, dest );
			tmp = NEXTLINE( tmp );
		}
		LineRelease( pText );
	}
	else
		pNewText = pText;
	return pNewText;
}

//---------------------------------------------------------------------

static PTEXT get_line(PCONFIG_HANDLER pch /*FOLD00*/
							, int bReturnBlank )
{
#define WORKSPACE 1024  // character for workspace
	FILE *source = pch->file;
	PLIST *filters = &pch->filters;
	PLIST *filter_data = &pch->filter_data;
	PTEXT newline = NULL;
	int one_more_read = 0;
	//if( !source && !pch->blocks )
	//	return NULL;
	// create a workspace to read input from the file.
	// read a line of input from the file.
	// can't use fgets with encrypted data, but have to use something
	// more like read(), pass to decrypt, then decode lines.
	{
		INDEX idx;
		PTEXT (CPROC *filter)(POINTER*,PTEXT);
		size_t readlen;
		do
		{
			int didone;
		do_filters:
			do
			{
				didone = FALSE;

				LIST_FORALL( filters[0], idx, PTEXT (CPROC *)(POINTER*,PTEXT), filter )
				{
					// request any existing data without adding more...
					newline = filter( GetLinkAddress( filter_data, idx ), newline );
					//lprintf( "Process line: %s", GetText( newline ) );
					//lprintf( "after filter %d line = %s", idx, GetText( newline ) );
					if( newline )
					{
						one_more_read = 0;
						didone = TRUE;
					}
					else
					{
						//lprintf( "no more newline... it's been deleted..." );
						if( didone ) // had one, lost it, try for another from the top of blank...
						{
							if( bReturnBlank )
								return SegCreate(0);
							didone = 0;
							break;
						}
					}
				}
			}
			while( !newline && didone );
			if( !newline )
			{
				if( source )
				{
					char buffer[WORKSPACE];
					wchar_t wbuffer[WORKSPACE];
					newline = NULL;
					if( pch->flags.bUnicode )
					{
						if( !one_more_read )
						{
							if( ( readlen = sack_fread( wbuffer, sizeof( wchar_t ), WORKSPACE-1, source ) ) )
							{
								wbuffer[readlen] = 0;
								newline = SegCreateFromWideLen( wbuffer, readlen );
							}
							else
							{
								newline = NULL;
								one_more_read = 1;
							}
						}
						else
							one_more_read = 0;
					}
					else
					{
						if( !one_more_read )
						{
							if( ( readlen = sack_fread( buffer, sizeof( char ), WORKSPACE-1, source ) ) )
							{
								buffer[readlen] = 0;
								newline = SegCreateFromCharLen( buffer, readlen );
							}
							else
							{
								newline = NULL;
								one_more_read = 1;
							}
						}
						else
							one_more_read = 0;
					}
				}
				else
				{
					if( !one_more_read )
					{
						newline = pch->blocks;
						pch->blocks = NULL; // received block.
						one_more_read = 1;
					}
					else if( !newline )
					{
						readlen = 0;
						one_more_read = 0;
					}
				}
				if( newline || one_more_read )
					goto do_filters;
				/*
				LIST_FORALL( filters[0], idx, PTEXT (CPROC *)(POINTER*,PTEXT), filter )
				{
					newline = filter( GetLinkAddress( filter_data, idx ), newline );
					//lprintf( "Process line: %s", GetText( newline ) );
					if( !newline )
						break; // get next bit of data ....
				}
				*/
			}
		}
		while( !newline && readlen );
	}
	return newline;		// return the line read from the file.
}

//---------------------------------------------------------------------

static PTEXT GetConfigurationLine( PCONFIG_HANDLER pConfigHandler )
{
	PTEXT line, p;
	while( ( line = get_line( pConfigHandler
									, pConfigHandler->Unhandled?TRUE:FALSE ) ) )
	{
		if( g.flags.bLogLines )
			lprintf( "Process line: %s", GetText( line ) );

		p = burst( line );
		LineRelease( line );
		if( g.flags.bLogTrace || g.flags.bLogLines )
		{
			{
				PTEXT x = p;
				while( x )
				{
					lprintf( "Word is: %s (%" _size_f ",%d)", GetText( x ), GetTextSize( x ), x->format.position.offset.spaces );
					x = NEXTLINE( x );
				}
			}
			{
				PTEXT tmp = BuildLine( p );
				lprintf( "input: %s", GetText( tmp ) );
				LineRelease( tmp );
			}
		}
		if( p &&
			GetTextSize( p ) )
		{
			return p;
		}
		else // drop and consume blank lines.
		{
			//lprintf( "Okay got a blank line... might handle it with 'unhandled'" );
			if( pConfigHandler->Unhandled )
				pConfigHandler->Unhandled( pConfigHandler->psvUser, NULL );
			LineRelease(p);
		}
	}
	return NULL;
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Is____Var results in true/false, plus the config element is filled
// with the current value.  start will also be updated to the current
// location to process.  Otherwise, false will be returned, the element
// will remain uninitialized, and start will not be updated.
//---------------------------------------------------------------------
int IsAnyVarEx( PCONFIG_ELEMENT pce, PTEXT *start DBG_PASS );
int IsAnyVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	return IsAnyVarEx( pce, start DBG_SRC );
}

#define IsAnyVar(a,b) IsAnyVarEx(a,b DBG_SRC )

//---------------------------------------------------------------------

int IsConstTextEx( PCONFIG_ELEMENT pce, PTEXT *start DBG_PASS )
#define IsConstText(a,b) IsConstTextEx(a,b DBG_SRC )
{
	if( pce->type != CONFIG_TEXT )
	{
		return FALSE;
	}
	if( g.flags.bLogTrace )
		_lprintf(DBG_RELAY)( "Testing %s vs %s", GetText( *start ), GetText( pce->data[0].pText ) );
	if( CompareText( *start, pce->data[0].pText ) == 0 )
	{
		if( g.flags.bLogTrace )
			lprintf( "%s matched %s", GetText( *start ), GetText( pce->data[0].pText ) );
		*start = NEXTLINE( *start );
		return TRUE;
	}
	if( g.flags.bLogTrace )
		lprintf( "isn't constant..." );
	return FALSE;
}

//---------------------------------------------------------------------
static CTEXTSTR charset1 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-+";
static CTEXTSTR charset2 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY.-Z+";
typedef union bintobase{
	struct {
		uint8_t bytes[3];
	} bin;

	// these need to be unsigned.
	// the warning is 'nonstandard extension used : bit field types other than int'
	// but 'int' will NOT work, because it's signed.
	struct {
		uint32_t data1 : 6;
		uint32_t data2 : 6;
		uint32_t data3 : 6;
		uint32_t data4 : 6;
	} base;
} BINBASE;

void EncodeBinaryConfig( TEXTSTR *encode, POINTER data, size_t length )
{
	BINBASE convert;
	CTEXTSTR charset = charset2;
	unsigned char *p;
	TEXTCHAR *q;
	uint32_t l;
	int bExtraBytes;
	q = (TEXTCHAR*)((*encode) = NewArray( TEXTCHAR, ( ( ( 1 + (length + 2) / 3 ) * 4 ) + 3 ) * 2 ));
	(q++)[0]= '[';
	p = (unsigned char*)&length;

	convert.bin.bytes[0] = (p++)[0];
	convert.bin.bytes[1] = (p++)[0];
	convert.bin.bytes[2] = (p++)[0];
#define EXPLOIT_BURST_FEATURE()	if( ((q[-1] == '+')?(q[0]='0'),1:0 ) || \
	((q[-1] == '.')?(q[0]='1'),1:0 ) ||										\
	((q[-1] == '-')?(q[0]='2'),1:0 ) )											\
	{																\
	q[-1] = '.';										\
	q++;													\
	}
	(q++)[0] = charset[convert.base.data1];
	EXPLOIT_BURST_FEATURE();
	(q++)[0] = charset[convert.base.data2];
	EXPLOIT_BURST_FEATURE();
	(q++)[0] = charset[convert.base.data3];
	EXPLOIT_BURST_FEATURE();
	(q++)[0] = charset[convert.base.data4];
	EXPLOIT_BURST_FEATURE();
	p = (unsigned char*)data;
	for( l = 0; l < length - 2; l += 3 )
	{
		convert.bin.bytes[0] = (p++)[0];
		convert.bin.bytes[1] = (p++)[0];
		convert.bin.bytes[2] = (p++)[0];
		(q++)[0] = charset[convert.base.data1];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data2];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data3];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data4];
	EXPLOIT_BURST_FEATURE();
	}
	bExtraBytes = 0;
	if( l < length )
	{
		bExtraBytes = 1;
		convert.bin.bytes[0] = (p++)[0];
		l++;
	}
	if( l < length )
	{
		bExtraBytes = 1;
		convert.bin.bytes[1] = (p++)[0];
		l++;
	}
	else if( bExtraBytes )
		convert.bin.bytes[1] = 0;
	if( l < length )
	{
		bExtraBytes = 1;
		convert.bin.bytes[2] = (p++)[0];
		l++;
	}
	else if( bExtraBytes )
		convert.bin.bytes[2] = 0;
	if( bExtraBytes )
	{
		(q++)[0] = charset[convert.base.data1];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data2];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data3];
	EXPLOIT_BURST_FEATURE();
		(q++)[0] = charset[convert.base.data4];
	EXPLOIT_BURST_FEATURE();
	}
	(q++)[0]= '}';
	(q++)[0] = 0;

}

//---------------------------------------------------------------------

int DoDecodeBinary( PTEXT *start, POINTER *binary_buffer, size_t *buflen )
{
	static int *reverse;
	static int reverse1[256];
	static int reverse2[256];
	size_t failsafe_len;
	CTEXTSTR charset;
	POINTER failsafe_buffer;
	if( reverse1['B'] == 0 )
	{
		int c;
		for( c = 0; charset2[c]; c++ )
		{
			reverse1[(int)charset1[c]] = c;
			reverse2[(int)charset2[c]] = c;
		}
	}
	if( !buflen )
		buflen = &failsafe_len;
	if( !binary_buffer )
		binary_buffer = &failsafe_buffer;

	(*buflen) = 0;
	(*binary_buffer) = NULL;
	if( GetText( *start )[0] == '{' )
	{
		reverse = reverse1;
		charset = charset1;
	}
	else if( GetText( *start )[0] == '[' )
	{
		reverse = reverse2;
		charset = charset2;
	}
	else
		charset = NULL;
	if( charset )
	{
		BINBASE convert;
		size_t len;
		CTEXTSTR p = GetText( NEXTLINE( *start ) );
		// looks like a good chance this is a binary blob
		char *q;
		TEXTCHAR ch;
#define HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION()  \
	if( (ch=(p++)[0]) == '.' ) {\
	if( p[0] == '0' ) {p++;ch='+';}							\
	else if ( p[0] == '1' ) {p++;ch = '.';}				\
	else if ( p[0] == '2' ) {p++;ch = '-';}				\
	}
		// if it is empty data... null, and 0
		if( NEXTLINE( *start ) &&
			( ( charset == charset1 && GetText( NEXTLINE( *start  ) )[0] == '}' )
			|| ( charset == charset2 && GetText( NEXTLINE( *start  ) )[0] == ']' ) ) )

		{
			(*binary_buffer) = NULL;
			(*buflen) = 0;
			(*start) = NEXTLINE( *start ); // step from { onto }
			(*start) = NEXTLINE( *start ); // step from } onto next token
			return TRUE;
		}
		if( !GetText( NEXTLINE( NEXTLINE( *start  ) ) ) ||
			( GetText( NEXTLINE( NEXTLINE( *start  ) ) )[0] != '}'
			&& GetText( NEXTLINE( NEXTLINE( *start  ) ) )[0] != ']' )
		)
		{
			return FALSE;
		}


		// HANLDE_BURST_EXPLOIT converts .0, .1, .2 into .-+ characters... and sets 'ch'
		// to the character to find.
		HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
		convert.base.data1 = reverse[(int)ch];
		HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
		convert.base.data2 = reverse[(int)ch];
		HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
		convert.base.data3 = reverse[(int)ch];
		HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
		convert.base.data4 = reverse[(int)ch];
		q = (char*)buflen;
		q[0] = convert.bin.bytes[0];
		q[1] = convert.bin.bytes[1];
		q[2] = convert.bin.bytes[2];
		// may be as much as 2 extra bytes (expressed as 3)
		(*binary_buffer) = Allocate( len = (*buflen) );
		q = (char*)(*binary_buffer);
		while( p[0] && len )
		{
			HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
			convert.base.data1 = reverse[(int)ch];
			HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
			convert.base.data2 = reverse[(int)ch];
			HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
			convert.base.data3 = reverse[(int)ch];
			HANDLE_BURST_PECULIARITY_WITH_DECIMALS_AND_NUMBER_COLLATION();
			convert.base.data4 = reverse[(int)ch];
			if( len && len-- )
				(q++)[0] = convert.bin.bytes[0];
			if( len && len-- )
				(q++)[0] = convert.bin.bytes[1];
			if( len && len-- )
				(q++)[0] = convert.bin.bytes[2];
		}
		(*start) = NEXTLINE( *start ); // step from { onto 0235
		(*start) = NEXTLINE( *start ); // step from 25234 onto }
		(*start) = NEXTLINE( *start ); // step from } onto next token
		return TRUE;
	}
	else
	{
		(*binary_buffer) = NULL;
		(*buflen) = 0;
	}
	return FALSE;
}

int DecodeBinaryConfig( CTEXTSTR string, POINTER *binary_buffer, size_t *buflen )
{
	int status;
	PTEXT tmp1 = SegCreateFromText( string );
	PTEXT start = burst( tmp1 );
	PTEXT delete_string = start; // save this decode updates position...
	LineRelease( tmp1 );
	status = DoDecodeBinary( &start, binary_buffer, buflen );
	LineRelease( delete_string );
	return status;
}

int IsBinaryVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	if( pce->type != CONFIG_BINARY )
		return FALSE;
	if( pce->data[0].binary.data )
	{
		Release( pce->data[0].binary.data );
		//pce->data[0].binary.data = NULL; // decode binary should do this?!
	}
	return DoDecodeBinary( start, &pce->data[0].binary.data, &pce->data[0].binary.length );
}

//---------------------------------------------------------------------

int IsBooleanVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	size_t len = GetTextSize( *start );
	int bOkay = FALSE;
	if( pce && pce->type != CONFIG_BOOLEAN )
		return FALSE;
	//lprintf( "Is %s boolean?", GetText( *start ) );
#define CmpMin(constlen)  ((len <= (constlen))?(len):0)
#define NearText(text,const)	( CmpMin( sizeof( const ) - 1 ) &&		\
					( StrCaseCmpEx( GetText( text ), const, len ) == 0 ) )
	if( NearText( *start, "yes" ) ||
		NearText( *start, "1" ) ||
		NearText( *start, "on" ) ||
		NearText( *start, "true" ) ||
		NearText( *start, "open" )
		)
	{
		if(pce)pce->data[0].truefalse.bTrue = 1;
		bOkay = TRUE;
	}
	else if( NearText( *start, "no" ) ||
			NearText( *start, "0" ) ||
			NearText( *start, "off" ) ||
		NearText( *start, "false" ) ||
		NearText( *start, "close" )
		)
	{
		if(pce)pce->data[0].truefalse.bTrue = 0;
		bOkay = TRUE;
	}
	else if( NearText( *start, "are" ) ||
			NearText( *start, "is" ) )
	{
		PTEXT word;
		bOkay = TRUE;
		if(pce)pce->data[0].truefalse.bTrue = 1;
		if( ( word = NEXTLINE( *start ) ) )
		{
		if( NearText( *start, "not" ) )
		{
				if(pce)pce->data[0].truefalse.bTrue = 0;
				*start = word;
		}
		}
	}
	if( bOkay )
	{
		*start = NEXTLINE( *start );
		return bOkay;
	}
	return FALSE;
}

int GetBooleanVar( PTEXT *start, LOGICAL *data )
{
	CONFIG_ELEMENT element = { CONFIG_BOOLEAN CONFIG_EMPTY_EXTRA };
	if( IsBooleanVar( &element, start ) )
	{
		if( data )
			(*data) = element.data[0].truefalse.bTrue;
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

static TEXTCHAR maxbase1[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static TEXTCHAR maxbase2[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int TextToInt( CTEXTSTR text, int64_t* out )
{
	struct {
		uint32_t neg : 1;
		uint32_t success : 1;
	} flags;
	uint32_t base;
	int64_t accum;
	flags.neg = 0;
	flags.success = 1;
	if( text[0] == '-' )
	{
		flags.neg = 1;
		text++;
	}
	else if( text[0] == '+' )
	{
		text++;
	}
	base = 10;
	if( text[0] == '0' )
	{
		base = 8;
		text++;
		if( text[0] == 'x' )
		{
				base = 16;
				text++;
		}
		else if( text[0] == 'b' )
		{
				base = 2;
				text++;
		}
	}
	accum = 0;
	while( text[0] )
	{
		CTEXTSTR c;
		uint32_t val;
		if( ( c = StrChr( maxbase1, text[0] ) ) ) val = (uint32_t)(c - maxbase1);
		if( !c ) if( ( c = StrChr( maxbase2, text[0] ) ) ) val = (uint32_t)(c - maxbase2);
		if( !c ) { flags.success = 0; break; }
		if( val < base )
		{
				accum *= base;
				accum += val;
		}
		else
		{
			// yeah this works... there are times when badly behaved programs generate
			// output that should not match...
			// launch at screenX by ScreenY	"launch at %i by %i" fails
			// launch at 50 by 59						"					works
			// another rule that took launchat %w by %w could also work... in case
			// there is some variadric thing .. but a different config proc will probably be
			// called for such a case.
				//Log3( "Base Error : [%c]%s is not within base %d", text[0], text+1, base );
				flags.success = 0;
				break;
		}
		text++;
	}
	if( flags.success )
	{
		if( out )
		{
			if( flags.neg )
				*out = -accum;
			else
				*out = accum;
		}
		return TRUE;
	}
	return FALSE;
}

int IsIntegerVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	CTEXTSTR text;
	int64_t accum;
	if( pce->type != CONFIG_INTEGER )
		return FALSE;
	text = GetText( *start );
	if( TextToInt( text, &accum ) )
	{
		pce->data[0].integer_number = accum;
		*start = NEXTLINE( *start );
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

int IsColorVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	if( pce->type != CONFIG_COLOR )
		return FALSE;
	// might consider color names also, (of course that table
	// would have to be configurable)

	//lprintf( "COlor testing: %s", GetText( *start ) );
	if( GetText( *start )[0] == '$' )
	{
		int ofs = 0;
		PTEXT val = NEXTLINE( *start );
		if( GetTextSize( *start ) == 1 )
			val = NEXTLINE( *start );
		else
		{
			val = *start;
			ofs = 1;
		}

		// potentially a hex variation...
		if( val )
		{
			uint32_t accum = 0;
			CTEXTSTR digit;
			digit = GetText( val ) + ofs;
			//lprintf( "COlor testing: %s", digit );
			while( digit && digit[0] )
			{
				int n;
				CTEXTSTR p;
				n = 16;
				p = StrChr( maxbase1, digit[0] );
				if( p )
					n = (uint32_t)(p-maxbase1);
				else
				{	
					p = StrChr( maxbase2, digit[0] );
					if( p )
						n = (uint32_t)(p-maxbase2);
				}	
				if( n < 16 )
				{
					accum *= 16;
					accum += n;
				}
				else
					break;
				digit++;	
			}
			if( ( digit - GetText( val ) ) < 6 )
			{
				lprintf( "Perhaps an error in color variable..." );
				pce->data[0].Color = accum | 0xFF000000;	
			}
			else
			{
				if( ( digit - GetText( val ) ) == 6 )
					pce->data[0].Color = accum | 0xFF000000;	
				else
					pce->data[0].Color = accum;	
			}

			{
				uint32_t file_color = pce->data[0].Color;
				COLOR_CHANNEL a = (COLOR_CHANNEL)( file_color >> 24 ) & 0xFF;
				COLOR_CHANNEL r = (COLOR_CHANNEL)( file_color >> 16 ) & 0xFF;
				COLOR_CHANNEL grn = (COLOR_CHANNEL)( file_color >> 8 ) & 0xFF;
				COLOR_CHANNEL b = (COLOR_CHANNEL)( file_color >> 0 ) & 0xFF;
				pce->data[0].Color = AColor( r,grn,b,a );
			}
			//Log4( "Color is: $%08X/(%d,%d,%d)"
			//		, pce->data[0].Color
			//		, RedVal( pce->data[0].Color ), GreenVal(pce->data[0].Color), BlueVal(pce->data[0].Color) );
			*start = NEXTLINE( val );
		}
		return TRUE;
	}
	else if( GetText( *start )[0] == '(' )
	{
		// potentially a parenthetical variation.
		PTEXT val;
		int components = 0;
		uint32_t color = 0;
		for( val = *start;
			val && GetText( val )[0] != ')';
			val = NEXTLINE( val ) )
		{
			int64_t accum = 0;
			//lprintf( "Test : %s", GetText( val ) );
			if( GetText(val)[0] == ',' )
				continue;
			if( TextToInt( GetText( val ), &accum ) )
			{	
				components++;
				color <<= 8;
				color |= ( accum & 0xFF );
			}
		}
		if( val )
		{
			pce->data[0].Color = color;
			if( components < 4 )
				pce->data[0].Color |= 0xFF000000;	
			//Log4( "Color is: $%08X/(%d,%d,%d)"
			//		, pce->data[0].Color
			//		, RedVal( pce->data[0].Color ), GreenVal(pce->data[0].Color), BlueVal(pce->data[0].Color) );
			*start = NEXTLINE( val );
			return TRUE;
		}
	}
	return FALSE;
}

int GetColorVar( PTEXT *start, CDATA *data )
{
	CONFIG_ELEMENT element = { CONFIG_COLOR CONFIG_EMPTY_EXTRA};
	if( IsColorVar( &element, start ) )
	{
		if( data )
			(*data) = element.data[0].Color;
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

int IsFloatVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
   (void)start;
	if( pce->type != CONFIG_FLOAT )
		return FALSE;

	//text = GetText( *start );
	lprintf( "Floating values are not processed yet." );

	return FALSE;
}

//---------------------------------------------------------------------

int IsFractionVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	int32_t accum1, accum2, accum3;
	int neg1, neg2, neg3;
	PTEXT current;
	CTEXTSTR text;
	if( pce->type != CONFIG_FRACTION )
		return FALSE;
	// this may consume multiple segments (4?)  (+/-)##(1) ##(2)/(3)##(4)
	current = *start;
	text = GetText( current );
	accum1 = 0;
	neg1 = 1;

	if( text[0] == '-' )
	{
		neg1 = -1;
		text++;
	}
	else if( text[0] == '+' )
	{
		text++;
	}
	while( text[0] && text[0] >= '0' && text[0] <= '9' )
	{
		accum1 *= 10;
		accum1 += text[0] - '0';
		text++;
	}
	if( text[0] )
	{
		lprintf( "Error in first argument of format of fraction." );
		return FALSE;
	}
// collect numerator after whole number.
	current = NEXTLINE( current );
	if( current )
	{
	text = GetText( current );
	accum2 = 0;
		neg2 = 1;
	if( text[0] == '/' )
	{
		//lprintf( "Promoting whole to numerator, getting denominaotr" );
		neg2	= neg1;
		accum2 = accum1;
		neg1	= 1;
		accum1 = 0;
		goto collect_denominator;
	}

	if( text[0] == '-' )
	{
		neg2 = -1;
		text++;
		}
		else if( text[0] == '+' )
		{
		text++;
		}
		while( text[0] && text[0] >= '0' && text[0] <= '9' )
		{
		accum2 *= 10;
		accum2 += text[0] - '0';
		text++;
		}
		if( text[0] )
		lprintf( "Error in format of numerator" );
	}
	else
	{
	//lprintf( "End of line before numerator" );
		pce->data[0].fraction.numerator = accum1;
		pce->data[0].fraction.denominator = neg1;
		*start = NEXTLINE( current );
	return TRUE;
	}
	current = NEXTLINE( current );
	if( current )
	{
	text = GetText( current );
	if( text[0] != '/' )
	{
		lprintf( "No denominator specified - error in fraction." );
		return FALSE;
	}
	}
	else
	{
	lprintf( "End of line before '/'" );
	}
collect_denominator:
	current = NEXTLINE( current );
	if( current )
	{
		text = GetText( current );
		accum3 = 0;
		neg3 = 1;
		if( text[0] == '-' )
		{
		neg3 = -1;
		text++;
		}
		else if( text[0] == '+' )
		{
		text++;
		}
		while( text[0] && text[0] >= '0' && text[0] <= '9' )
		{
		accum3 *= 10;
		accum3 += text[0] - '0';
		text++;
		}
		if( text[0] )
		{
		lprintf( "Error in format of denominator." );
		return FALSE;
		}
		//Log7( "%d*%d+%d / %d*%d*%d*%d", accum1, accum3, accum2, neg1, neg2, neg3, accum3 );
		pce->data[0].fraction.numerator = accum1 * accum3 + accum2;
		pce->data[0].fraction.denominator = neg1 * neg2 * neg3 * accum3;
		*start = NEXTLINE( current );
	}
	else
	{
		lprintf( "End of line before denominator." );
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------

int IsSingleWordVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	struct config_element_tag *pEnd;
	PTEXT pWords = NULL;
	int matched = 1;
	struct config_element_tag *default_EOL = NULL;
	//PTEXT __start = *start;
	if( pce->type != CONFIG_SINGLE_WORD )
		return FALSE;
	if( pce->data[0].singleword.pWord )
	{
		Release( pce->data[0].singleword.pWord );
		pce->data[0].singleword.pWord = NULL;
	}
	while( *start )
	{
		if( pWords )
		{
			INDEX idx;
			if( (*start)->format.position.offset.spaces )
			{
				//if( pWords )
				//	LineRelease( pWords );
				// so at a space, stop appending.
				if( g.flags.bLogTrace )
					lprintf( "next word has spaces... [%s](%d)", GetText( *start ), (*start)->format.position.offset.spaces );
				break;
			}

			LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd ){
				PTEXT _start = *start;
				if( ( matched = IsAnyVar( pEnd, start ) ) != 0 )
				{
					pce->data[0].singleword.pWhichEnd = pEnd;
					pce->next = pEnd->next;
					if( g.flags.bLogTrace )
						lprintf( "Failed check for var check.." );
					*start = _start; // restore start..
					break;
				}
				else if( pEnd->type == CONFIG_NOTHING ) {
					if( !default_EOL )
						default_EOL = pce;
				}
				//*start = _start;
				//return TRUE;
			}
		}
		// was more than one word.
		if( pWords && ( (*start)->format.position.offset.spaces || (*start)->format.position.offset.tabs ) ) {
			LineRelease( pWords );
			pWords = NULL;
			return FALSE;
		}
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		if( g.flags.bLogTrace )
			lprintf( "Appending more to single word...[%s]", GetText( (*start) ) );
		*start = NEXTLINE( *start );
	}
	if( (!*start) )
	{
		if( !matched && !default_EOL )
		{
			LineRelease( pWords );
			pWords = NULL;
			// multiword ended - end of line, and no match on the next tag...
			return FALSE;
		}
		else if( g.flags.bLogTrace )
			lprintf( "is alright - gathered to end of line ok." );
	}
	if( pWords )
	{
		PTEXT pText;
		pWords->format.position.offset.spaces= 0;
		pText = BuildLine( pWords );
		LineRelease( pWords );

		if( g.flags.bLogTrace )
			lprintf( "Setting text word..." );
		pce->data[0].singleword.pWord = StrDup( GetText( pText ) );
		LineRelease( pText );
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

int IsMultiWordVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	struct config_element_tag *pEnd;
	int matched = 1;
	PTEXT pWords = NULL;
	struct config_element_tag *default_EOL = NULL;
	if( pce->type != CONFIG_MULTI_WORD )
		return FALSE;
	if( pce->data[0].multiword.pWords )
	{
		Release( pce->data[0].multiword.pWords );
		pce->data[0].multiword.pWords = NULL;
	}
	while( *start )
	{
		INDEX idx;
		LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
		{
			if( ( matched = IsAnyVar( pEnd, start ) ) != 0 ){
				pce->data[0].multiword.pWhichEnd = pEnd;
				if( g.flags.bLogTrace )
					lprintf( "Matched one of several?  set next to %p", pEnd, pEnd->next );
				pce->next = pEnd->next;
				break;
			}
			else if( !default_EOL && pEnd->type == CONFIG_NOTHING ) {
				default_EOL = pEnd;
			}
		}
		if( pEnd )
			break;
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		*start = NEXTLINE( *start );
	}
	if( (!*start) )
	{
		if( !matched && !default_EOL )
		{
			LineRelease( pWords );
			pWords = NULL;
			// multiword ended - end of line, and no match on the next tag...
			return FALSE;
		}
		else {
			if( default_EOL )
				pce->next = default_EOL->next;
			if( g.flags.bLogTrace )
				lprintf( "is alright - gathered to end of line ok. (or matched)" );
		}
	}
	//if( !pWords )
	//	pWords = SegCreate(0);
	if( pWords )
	{
		pWords->format.position.offset.spaces = 0;
		{
			PTEXT out = BuildLine( pWords );
			TEXTSTR buf = StrDup( GetText( out ) );
			LineRelease( out );
			LineRelease( pWords );
			pce->data[0].multiword.pWords = buf;
		}
		return TRUE;
	}
	/* can have empty space for multiword, but was an OK result anyway...*/
	return matched;
}

//---------------------------------------------------------------------

int IsPathVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	struct config_element_tag *pEnd;
	PTEXT pWords = NULL;
	if( pce->type != CONFIG_PATH )
		return FALSE;
	if( pce->data[0].multiword.pWords )
	{
		Release( pce->data[0].multiword.pWords );
		pce->data[0].multiword.pWords = NULL;
	}
	while( *start  )
	{
		INDEX idx;
		LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
		{
			if( IsAnyVar( pEnd, start ) ) {
				pce->data[0].multiword.pWhichEnd = pEnd;
				if( g.flags.bLogTrace )
					lprintf( "Matched one of several?  set next to %p", pEnd->next );
				pce->next = pEnd->next;
				break;
			}
		}
		if( pEnd )
         break;
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		*start = NEXTLINE( *start );
	}
	if( pWords )
	{
		pWords->format.position.offset.spaces = 0;
		{
			PTEXT out = BuildLine( pWords );
			size_t length;
			TEXTSTR buf = NewArray( TEXTCHAR, length = GetTextSize( out ) + 1 );
			StrCpyEx( buf, GetText( out ), length * sizeof( TEXTCHAR ) );
			LineRelease( out );
			LineRelease( pWords );
			pce->data[0].multiword.pWords = buf;
		}
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

int IsFileVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	struct config_element_tag *pEnd;
	PTEXT pWords = NULL;
	if( pce->type != CONFIG_FILE )
		return FALSE;
	if( pce->data[0].multiword.pWords )
	{
		Release( pce->data[0].multiword.pWords );
		pce->data[0].multiword.pWords = NULL;
	}
	while( *start )
	{
		INDEX idx;
		LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
		{
			if( IsAnyVar( pEnd, start ) ) {
				pce->data[0].multiword.pWhichEnd = pEnd;
				lprintf( "Matched one of several?  set next to %p", pEnd->next );
				pce->next = pEnd->next;
				break;
			}
		}
		if( pEnd )
			break;
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		*start = NEXTLINE( *start );
	}
	if( pWords )
	{
		pWords->format.position.offset.spaces = 0;
		{
			PTEXT out = BuildLine( pWords );
			size_t length;
			TEXTSTR buf = NewArray( TEXTCHAR, length = GetTextSize( out ) + 1 );
			StrCpyEx( buf, GetText( out ), length * sizeof( TEXTCHAR ) );
			LineRelease( out );
			LineRelease( pWords );
			pce->data[0].multiword.pWords = buf;
		}
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------

int IsFilePathVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	struct config_element_tag *pEnd;
	PTEXT pWords = NULL;
	if( pce->type != CONFIG_FILEPATH )
		return FALSE;
	if( pce->data[0].multiword.pWords )
	{
		Release( pce->data[0].multiword.pWords );
		pce->data[0].multiword.pWords = NULL;
	}
	while( *start )
	{
		INDEX idx;
		LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
		{
			if( IsAnyVar( pEnd, start ) ) {
				pce->data[0].multiword.pWhichEnd = pEnd;
				pce->next = pEnd->next;
				break;
			}
		}
		if( pEnd )
         break;
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		*start = NEXTLINE( *start );
	}
	if( pWords )
	{
		INDEX idx;
		LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
		{
			if( IsAnyVar( pEnd, start ) ) {
				pce->data[0].multiword.pWhichEnd = pEnd;
				pce->next = pEnd->next;
				break;
			}
		}
		pWords->format.position.offset.spaces = 0;
		{
			PTEXT out = BuildLine( pWords );
			size_t length;
			TEXTSTR buf = NewArray( TEXTCHAR, length = GetTextSize( out ) + 1 );
			StrCpyEx( buf, GetText( out ), length * sizeof( TEXTCHAR ) );
			LineRelease( out );
			LineRelease( pWords );
			pce->data[0].multiword.pWords = buf;
		}
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------
#if !defined( __NO_NETWORK__ ) && 0
int IsAddressVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	PTEXT pWords = NULL;
	if( pce->type != CONFIG_ADDRESS )
		return FALSE;
	if( pce->data[0].psaSockaddr )
	{
		ReleaseAddress( pce->data[0].psaSockaddr );
		pce->data[0].psaSockaddr = NULL;
	}
	while( *start && !(*start)->format.position.offset.spaces )
	{
		pWords = SegAppend( pWords, SegDuplicate( *start ) );
		*start = NEXTLINE( *start );
	}
	if( pWords )
	{
		pWords->format.position.offset.spaces = 0;
		{
			PTEXT pText = BuildLine( pWords );
			LineRelease( pWords );
			pce->data[0].psaSockaddr = CreateSockAddress( (CTEXTSTR)GetText( pText ), 0 );
			LineRelease( pText );
		}
		return TRUE;
	}
	return FALSE;
}
#endif
//---------------------------------------------------------------------

int IsURLVar( PCONFIG_ELEMENT pce, PTEXT *start )
{
	if( pce->type != CONFIG_URL )
		return FALSE;
	return FALSE;
}

//---------------------------------------------------------------------

int IsAnyVarEx( PCONFIG_ELEMENT pce, PTEXT *start DBG_PASS )
{
	if( !pce || !start )
	{
		//lprintf( "No pce or no start" );
		return FALSE;
	}
	if( g.flags.bLogTrace )
		_lprintf(DBG_RELAY)( "IsAnyVar" );

	if( pce->type == CONFIG_NOTHING && !(*start) )
		return TRUE;

	return( ( IsConstText( pce, start ) ) ||
			( IsBooleanVar( pce, start ) ) ||
			( IsBinaryVar( pce, start ) ) ||
			( IsIntegerVar( pce, start ) ) ||
			( IsFloatVar( pce, start ) ) ||
			( IsFractionVar( pce, start ) ) ||
			( IsSingleWordVar( pce, start ) ) ||
			( IsMultiWordVar( pce, start ) ) ||
			( IsPathVar( pce, start ) ) ||
			( IsFileVar( pce, start ) ) ||
			( IsFilePathVar( pce, start ) ) ||
			( IsURLVar( pce, start ) ) ||
			( IsColorVar( pce, start ) ) );
}

//---------------------------------------------------------------------
//#define PushArgument( type, arg ) ( (parampack = Preallocate( parampack, argsize += sizeof( arg )) )?(*(type*)(parampack) = (arg)),0:0)
//#define PopArguments( sz ) Release( parampack )
//---------------------------------------------------------------------

void DoProcedure( uintptr_t *ppsvUser, PCONFIG_TEST Check )
{
	INDEX idx;
	PCONFIG_ELEMENT pce = NULL;
	va_args parampack;
#ifdef __WATCOMC__
	va_args save_parampack;
#endif
	init_args( parampack );
	LIST_FORALL( Check->pVarElementList, idx, PCONFIG_ELEMENT, pce )
	{
		if( pce->type == CONFIG_PROCEDURE || pce->type == CONFIG_PROCEDURE_EX )
		{
			if( pce->data[0].Process)
			{
#ifdef NEED_ASSEMBLY_CALLER
				CallProcedure( ppsvUser, pce );
#else
				PCONFIG_ELEMENT pcePush = pce->prior;
				// push arguments in reverse order...
				//lprintf( "Calling process... " );
				while( pcePush )
				{
					if( g.flags.bLogTrace )
						LogElement( "pushing", pcePush );
					switch( pcePush->type )
					{
					case CONFIG_TEXT:
						break;
					case CONFIG_BINARY:
						PushArgument( parampack, CONFIG_ARG_DATA, POINTER, pcePush->data[0].binary.data );
						PushArgument( parampack, CONFIG_ARG_DATA_SIZE, size_t, pcePush->data[0].binary.length );
						break;
					case CONFIG_BOOLEAN:
						{
							LOGICAL val = pcePush->data[0].truefalse.bTrue;
							PushArgument( parampack, CONFIG_ARG_LOGICAL, LOGICAL, val );
						}
						break;
					case CONFIG_INTEGER:
						PushArgument( parampack, CONFIG_ARG_INT64, int64_t, pcePush->data[0].integer_number );
						break;
					case CONFIG_FLOAT:
						PushArgument( parampack, CONFIG_ARG_FLOAT, float, (float)pcePush->data[0].float_number );
						break;
					case CONFIG_FRACTION:
						PushArgument( parampack, CONFIG_ARG_FRACTION, FRACTION, pcePush->data[0].fraction );
						break;
					case CONFIG_SINGLE_WORD:
						PushArgument( parampack, CONFIG_ARG_STRING, CTEXTSTR, pcePush->data[0].pWord );
						break;
					case CONFIG_COLOR:
						PushArgument( parampack, CONFIG_ARG_COLOR, CDATA, pcePush->data[0].Color );
						break;
					case CONFIG_MULTI_WORD:
					case CONFIG_FILEPATH:
						PushArgument( parampack, CONFIG_ARG_STRING, CTEXTSTR, pcePush->data[0].multiword.pWords );
						break;
					default:
						break;
					}
					//lprintf( "Total args are now: %d", argsize );
					pcePush = pcePush->prior;
				}
				if( pce->type == CONFIG_PROCEDURE_EX )
					(*ppsvUser) = pce->data[0].ProcessEx.Process( *ppsvUser, pce->data[0].ProcessEx.arg, pass_args( parampack ) );
				else
					(*ppsvUser) = pce->data[0].Process( *ppsvUser, pass_args(parampack) );
				PopArguments( parampack );
#endif
				break; // done, end of list, please leave and do not iterate further!
			}
		}
		else
		{
			switch( pce->type )
			{
			case CONFIG_MULTI_WORD:
			case CONFIG_SINGLE_WORD:
				// null content ?
				break;
			default:
				// actually this probably means that there was no content to complete
				// the match, and NULL is not a valid responce to data...
				lprintf( "Multiple options here for what to do at end of line?" );
			}
		}
	}
}

void ProcessConfigurationLine( PCONFIG_HANDLER pch, PTEXT line )
		{
			PCONFIG_TEST Check = &pch->ConfigTestRoot;
			PTEXT word = line;
			while( word && Check )
			{
					INDEX idx;
					PCONFIG_ELEMENT pce = NULL;
					if( g.flags.bLogTrace )
						lprintf( "Test word (%s) vs constant elements", GetText( word ) );
					LIST_FORALL( Check->pConstElementList, idx, PCONFIG_ELEMENT, pce )
					{
						if( g.flags.bLogTrace )
							lprintf( "Is %s == %s?", GetText( word ), GetText( pce->data[0].pText ) );
						if( IsConstText( pce, &word ) )
						{
							Check = pce->next;
							break;
						}
					}
					if( g.flags.bLogTrace )
						lprintf( "Test word (%s) vs variable elements", GetText( word ) );
					{
						if( !pce )
						{
							int found = 0;
							LIST_FORALL( Check->pVarElementList, idx, PCONFIG_ELEMENT, pce )
							{
								// end of the line, match should be the process...
								if( g.flags.bLogTrace )
								{
									lprintf( "Is %s a Thing", GetText( word ) );
									LogElement( "Thing is", pce );
								}
								if( IsAnyVar( pce, &word ) )
								{
									if( g.flags.bLogTrace )
										lprintf( "Yes, it is any var." );

									found = 1;
									if( g.flags.bLogTrace )
										lprintf( "pce->next is %p  word is %p(%s)", pce->next, word, GetText( word ) );
									Check = pce->next;
									break;
								}
								else if( g.flags.bLogTrace )
								{
									lprintf( "But it's not anything I know." );
								}
							}
							if( g.flags.bLogTrace )
								lprintf( "is %s", found?"found":"Not found" );
							if( !found )
							{
								PTEXT pLine = BuildLine( line );
								if( g.flags.bLogTrace )
									lprintf( "Line not matched[%s]", GetText( pLine ) );
								if( pch->Unhandled )
									pch->Unhandled( pch->psvUser, GetText( pLine ) );
								else
								{
									if( g.flags.bLogUnhandled )
										xlprintf(LOG_NOISE)( "Unknown Configuration Line(No unhandled proc): %s", GetText( pLine ) );
								}

								break;
							}
						}
					}
			}
			if( !Check )
			{
				lprintf( "Fell off the end the line processor, still have data..." );
				{
					PTEXT pLine = BuildLine( line );
					if( pch->Unhandled )
						pch->Unhandled( pch->psvUser, GetText( pLine ) );
					else
					{
						// I didn't want to get rid of this...
						// but ahh well, it's noisy and it works.
						//lprintf( "Unknown Configuration Line: %s", GetText( pLine ) );
					}
				}
			}
			else if( !word )	// otherwise we may have bailed early.
			{
				// check here for Procedure at end of line (word == NULL)
				DoProcedure( &pch->psvUser, Check );
			}
#if 0
			else
			{
				if( g.flags.bLogTrace )
					lprintf( "line partially matched... need to recover and re-evaluate.." );
				{
					PTEXT pLine = BuildLine( line );
					if( pch->Unhandled )
						pch->Unhandled( pch->psvUser, GetText( pLine ) );
					else
					{
						// I didn't want to get rid of this...
						// but ahh well, it's noisy and it works.
						//lprintf( "Unknown Configuration Line: %s", GetText( pLine ) );
					}
				}
			}
#endif
		}

static void TestUnicode( PCONFIG_HANDLER pch )
{
	int return_pos = 0;
	pch->flags.bUnicode	= 0;

	{
		char charbuf[64];
		size_t len_read; // could be really short
		size_t char_check;
		int ascii_unicode = 1;
		len_read = sack_fread( charbuf, 1, 64, pch->file );
		if( ( ((uint16_t*)charbuf)[0] == 0xFEFF )
			|| ( ((uint16_t*)charbuf)[0] == 0xFFFE )
			|| ( ((uint16_t*)charbuf)[0] == 0xFDEF ) )
		{
			return_pos = 2;
			pch->flags.bUnicode = 1;
		}
		else if( ( charbuf[0] == (char)0xef ) && ( charbuf[1] == (char)0xbb ) && ( charbuf[0] == (char)0xbf ) )
		{
			return_pos = 1;
			pch->flags.bUnicode8 = 1;
		}
		else for( char_check = 0; char_check < len_read; char_check++ )
		{
		// every other byte is a 0 for flat unicode text...
			if( ( char_check & 1 ) && ( charbuf[char_check] != 0 ) )
			{
				ascii_unicode = 0;
				break;
			}
		}
		if( ascii_unicode )
		{
			pch->flags.bUnicode = 1;
		}
		else
		{
			int ascii = 1;
			for( char_check = 0; char_check < len_read; char_check++ )
				if( charbuf[char_check] & 0x80 )
				{
					ascii = 0;
					break;
				}
			if( ascii )
			{
				// hmm this is probably a binary thing?
			}
		}
	}

	sack_fseek( pch->file, return_pos, SEEK_SET );
	// rewind 
}
		
//---------------------------------------------------------------------

CONFIGSCR_PROC( int, ProcessConfigurationFile )( PCONFIG_HANDLER pch, CTEXTSTR name, uintptr_t psv )
{
	PTEXT line;

#if !defined( __ANDROID__ ) && !defined( __EMSCRIPTEN__ )
	int absolute_path = IsAbsolutePath( name ); // don't prefix with anything.
#endif
	pch->file = sack_fopen( 0, name, "rb" );

#if !defined( __ANDROID__ ) && !defined( __EMSCRIPTEN__ )
#  ifndef UNDER_CE
	if( !pch->file && !absolute_path )
	{
		TEXTCHAR pathname[255];
		tnprintf( pathname, sizeof( pathname ), "./%s", name );
#	ifdef _MSC_VER
		pathname[sizeof(pathname)/sizeof(pathname[0])-1]=0;
#	endif
		pch->file = sack_fopen( 0, pathname, "rb" );
	}
	if( !pch->file && !absolute_path )
	{
		TEXTCHAR pathname[255];
		tnprintf( pathname, sizeof( pathname ), "@/%s", name );
#	ifdef _MSC_VER
		pathname[sizeof(pathname)/sizeof(pathname[0])-1]=0;
#	endif
		pch->file = sack_fopen( 0, pathname, "rb" );
	}
#  endif
	if( !pch->file && !absolute_path )
	{
		TEXTCHAR pathname[255];
		tnprintf( pathname, sizeof( pathname ), "*/%s", name );
#	ifdef _MSC_VER
		pathname[sizeof(pathname)/sizeof(pathname[0])-1]=0;
#	endif
		pch->file = sack_fopen( 0, pathname, "rb" );
	}
	if( !pch->file && !absolute_path )
	{
		TEXTCHAR pathname[255];
		tnprintf( pathname, sizeof( pathname ), "#/%s", name );
#	ifdef _MSC_VER
		pathname[sizeof(pathname)/sizeof(pathname[0])-1]=0;
#	endif
		pch->file = sack_fopen( 0, pathname, "rb" );
	}
	if( !pch->file && !absolute_path )
	{
		TEXTCHAR pathname[255];
		tnprintf( pathname, sizeof( pathname ), "/etc/%s", name );
#  ifdef _MSC_VER
		pathname[sizeof(pathname)/sizeof(pathname[0])-1]=0;
#  endif
		pch->file = sack_fopen( 0, pathname, "rb" );
	}
#endif
	pch->psvUser = psv;
	if( pch->file )
	{
		TestUnicode( pch );
		while( ( line = GetConfigurationLine( pch ) ) )
		{
			ProcessConfigurationLine( pch, line );
			LineRelease( line );
		}
		sack_fclose( pch->file );
		if( pch->EndProcess )
			pch->EndProcess( pch->psvUser );
		pch->file = NULL;
		return TRUE;
	}
	else
	{
		//lprintf( "Failed to open config file: %s", name );
		return FALSE;
	}
}

//---------------------------------------------------------------------

CONFIGSCR_PROC( uintptr_t, ProcessConfigurationInput )( PCONFIG_HANDLER pch, CTEXTSTR data, size_t size, uintptr_t psv )
{
	pch->psvUser = psv;
	{
		PTEXT line;
		PTEXT block = SegCreate( size + 1 );
		MemCpy( GetText( block ), data, size );
		GetText( block )[size] = 0;
		SetTextSize( block, size );
		pch->blocks = block;
		while( ( line = GetConfigurationLine( pch ) ) )
		{
			ProcessConfigurationLine( pch, line );
			LineRelease( line );
		}
		// this block will have been moved into internal accumulators.
		//LineRelease( block );
	}
	return pch->psvUser;
}

//---------------------------------------------------------------------

static PCONFIG_ELEMENT NewConfigTestElement( PCONFIG_HANDLER pch )
{

	PCONFIG_ELEMENT pceNew = GetFromSet( CONFIG_ELEMENT, &pch->elements ); //&(PCONFIG_ELEMENT)Allocate( sizeof( CONFIG_ELEMENT ) );
	MemSet( pceNew, 0, sizeof( CONFIG_ELEMENT ) );
	return pceNew;
}

//---------------------------------------------------------------------

//---------------------------------------------------------------------

static PCONFIG_TEST NewConfigTest( PCONFIG_HANDLER pch )
{
	PCONFIG_TEST pctNew = GetFromSet( CONFIG_TEST, &pch->test_elements ); //(PCONFIG_TEST)AllocateEx( sizeof( CONFIG_TEST ) DBG_RELAY );
	MemSet( pctNew, 0, sizeof( CONFIG_TEST ) );
	// I don't actually have to create list...
	// they will be filled in and allocated on demand.
	pctNew->pConstElementList = NULL;//CreateListEx( DBG_VOIDRELAY );
	pctNew->pVarElementList = NULL;//CreateListEx( DBG_VOIDRELAY );
	return pctNew;
}

//---------------------------------------------------------------------

void DestroyConfigElement( PCONFIG_HANDLER pch, PCONFIG_ELEMENT pct );

//---------------------------------------------------------------------

void BeginConfiguration( PCONFIG_HANDLER pch )
{
	// pushes the config file handler state...
	if( pch )
	{
		CONFIG_STATE save_state;
		if( !pch->ConfigStateStack )
			pch->ConfigStateStack = CreateDataStack( sizeof( CONFIG_STATE ) );
#ifdef DEBUG_SAVE_CONFIG
		lprintf( "Saving config and psvUser is %08x", pch->psvUser );
		DumpConfigurationEvaluator( pch );
#endif
		save_state.recovered = pch->config_recovered;
		save_state.ConfigTestRoot = pch->ConfigTestRoot;
		save_state.psvUser = pch->psvUser;
		save_state.EndProcess = pch->EndProcess;
		save_state.Unhandled = pch->Unhandled;
		save_state.name = pch->save_config_as;
		pch->flags.bConfigSaveNameUsed = 1;
		PushData( &pch->ConfigStateStack, &save_state );
#ifdef DEBUG_SAVE_CONFIG
		lprintf( "<BEGIN>" );
#endif
		pch->ConfigTestRoot.pConstElementList = NULL;
		pch->ConfigTestRoot.pVarElementList = NULL;
		pch->EndProcess = NULL;
		pch->Unhandled = NULL;
		pch->config_recovered = FALSE;
#ifdef DEBUG_SAVE_CONFIG
		lprintf( "Setting config as not savable." );
#endif
		pch->save_config_as = NULL;
	}
}

LOGICAL BeginNamedConfiguration( PCONFIG_HANDLER pch, CTEXTSTR name )
{
	// returns true if the configuration previously exists...
	// returns false if it needs to be built.
	INDEX idx;
	PCONFIG_STATE state;
	LIST_FORALL( pch->states, idx, PCONFIG_STATE, state )
	{
		if( StrCmp( state->name, name ) == 0 )
		{
#ifdef DEBUG_SAVE_CONFIG
			lprintf( "found previous config, setting this up, and resulting." );
#endif
			BeginConfiguration( pch );
			pch->ConfigTestRoot = state->ConfigTestRoot;
			pch->EndProcess = state->EndProcess;
			pch->Unhandled = state->Unhandled;
			pch->config_recovered = TRUE;
#ifdef DEBUG_SAVE_CONFIG
			lprintf( "Beginning a named configuration..." );
#endif
			pch->save_config_as = state->name;
#ifdef DEBUG_SAVE_CONFIG
			DumpConfigurationEvaluator( pch );
#endif
			return TRUE;
		}
	}
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "No previous found, setting this up to save at end config..." );
#endif
	BeginConfiguration( pch );
	pch->config_recovered = FALSE;
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "Setting named configuration." );
#endif
	pch->flags.bConfigSaveNameUsed = 0;
	pch->save_config_as = StrDup( name );
	return FALSE;
}


void DestroyConfigTest( PCONFIG_HANDLER pch, PCONFIG_TEST pct, int dealloc );


// this is more like a pop configuration.
void EndConfiguration( PCONFIG_HANDLER pch )
{
	PCONFIG_STATE prior_state;
	//PCONFIG_TEST prior;
	if( pch->EndProcess )
		pch->EndProcess( pch->psvUser );
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "<END>" );
#endif
	prior_state = (PCONFIG_STATE)PopData( &pch->ConfigStateStack );
	if( prior_state )
	{
		if( pch->save_config_as )
		{
			if( !pch->config_recovered )
			{
				CONFIG_STATE *state = New( CONFIG_STATE );
				state->ConfigTestRoot = pch->ConfigTestRoot;
				state->EndProcess = pch->EndProcess;
				state->Unhandled = pch->Unhandled;
				pch->flags.bConfigSaveNameUsed = 1;
				state->name = pch->save_config_as;
				AddLink( &pch->states, state );
			}
			// otherwise there's no action to do... already have it saved.
		}
		else
		{
			//lprintf( "Config was not saved, destroying." );
			DestroyConfigTest( pch, &pch->ConfigTestRoot, FALSE );
		}
		// didn't recover from states list.
		pch->config_recovered = prior_state->recovered;
		pch->ConfigTestRoot = prior_state->ConfigTestRoot;
		pch->EndProcess = prior_state->EndProcess;
		pch->Unhandled = prior_state->Unhandled;
		pch->psvUser = prior_state->psvUser;
		pch->save_config_as = prior_state->name;
	}
}

PCONFIG_ELEMENT _AddConfigurationEx( PCONFIG_HANDLER pch, CTEXTSTR format, USER_CONFIG_HANDLER Process DBG_PASS )
{
	PTEXT pTemp = SegCreateFromText( format );
	PTEXT pLine; 
	PTEXT pWord;
	struct {
		uint32_t vartag : 1;
		uint32_t vector : 1;
		uint32_t ignore_new : 1;
		uint32_t store_next_as_end : 1;
		uint32_t also_store_next_as_end : 1;
		uint32_t store_as_end : 1;
		uint32_t also_store_as_end : 1;
	}flags;
	PCONFIG_TEST pct;
	PCONFIG_ELEMENT pceNew, pcePrior;
	((uint32_t*)&flags)[0] = 0;
	//flags.dw = 0;

//#if defined( FULL_TRACE ) || defined( DEBUG_SLOWNESS )
	if( g.flags.bLogTrace )
		lprintf( "Burst..." );
//#endif
	pLine = burst( pTemp );
	LineRelease( pTemp );

	pct = &pch->ConfigTestRoot;
	pceNew = NewConfigTestElement( pch );
	pcePrior = NULL;
	pWord = pLine;
	while( pWord )
	{
		if( g.flags.bLogTrace )
			lprintf( "Evaluating %s ... ", GetText( pWord ) );
		if( flags.vartag )
		{
			CTEXTSTR pWordText = GetText( pWord );
			if( pWordText[0] == 'v' )
			{
				flags.vector = 1;
				pWordText++;
				if( !pWordText[0] )
				{
					lprintf( "Format: %s", format );
					lprintf( "Configuration error %%v[no type]" );
				}
				LineRelease( pLine );
				lprintf( "Destroy config element %p", pceNew );
				DestroyConfigElement( pch, pceNew );
				return NULL;
			}
			switch( pWordText[0] )
			{
			case 'b':
				//lprintf( "is a boolean..." );
				pceNew->type = CONFIG_BOOLEAN;
				pceNew->flags.vector = flags.vector;
				break;
			case 'B':
				//lprintf( "is a binary..." );
				pceNew->type = CONFIG_BINARY;
				pceNew->flags.vector = flags.vector;
				break;
			case 'i':
				//lprintf( "Is an integer... ") ;
				pceNew->type = CONFIG_INTEGER;
				pceNew->flags.vector = flags.vector;
				break;
			case 'c':
				pceNew->type = CONFIG_COLOR;
				pceNew->flags.vector = flags.vector;
				break;
			case 'f':
				pceNew->type = CONFIG_FLOAT;
				pceNew->flags.vector = flags.vector;
				break;
			case 'q':
				pceNew->type = CONFIG_FRACTION;
				pceNew->flags.vector = flags.vector;
				break;
			case 'w':
				if( g.flags.bLogTrace )
					lprintf( "Setting new as type SINGLE_WORD" );
				pceNew->type = CONFIG_SINGLE_WORD;
				pceNew->flags.vector = flags.vector;
				flags.also_store_next_as_end = 1;
				break;
			case 'm':
				if( g.flags.bLogTrace )
					lprintf( "Setting new as type MULTI_WORD" );
				pceNew->type = CONFIG_MULTI_WORD;
				pceNew->flags.vector = flags.vector;
				flags.store_next_as_end = 1;
				break;
			case 'u':
				pceNew->type = CONFIG_URL;
				pceNew->flags.vector = flags.vector;
				break;
			case 'd':
				pceNew->type = CONFIG_PATH;
				pceNew->flags.vector = flags.vector;
				flags.store_next_as_end = 1;
				break;
			case 'n':
				pceNew->type = CONFIG_FILE;
				pceNew->flags.vector = flags.vector;
				flags.store_next_as_end = 1;
				break;
			case 'p':
				pceNew->type = CONFIG_FILEPATH;
				pceNew->flags.vector = flags.vector;
				flags.store_next_as_end = 1;
				break;
			case 'a':
				pceNew->type = CONFIG_ADDRESS;
				pceNew->flags.vector = flags.vector;
				break;
			default:
				lprintf( "Format: %s", format );
				lprintf( "Unknown format character: %c", pWordText[0] );
				flags.ignore_new = 1;
				break;
			}
			if( !flags.ignore_new )
			{
				if( g.flags.bLogTrace )
					lprintf( "Not ignoring the new thing..." );
				if( flags.store_as_end )
				{
					if( g.flags.bLogTrace )
						lprintf( "Storing as end..." );
					{
						INDEX idx;
						struct config_element_tag *pEnd;
						LIST_FORALL( pcePrior->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd ){ //-V522
							if( pceNew->type == pEnd->type ) {
								break;
							}
						}
						if( !pEnd ){
							AddLink( &pcePrior->data[0].multiword.pEnds, pceNew );
							pceNew->flags.multiword_terminator = 1;
							pceNew->prior = pcePrior;
							pct = pceNew->next = NewConfigTest( pch );
							if( g.flags.bLogTrace )
								lprintf( "%p pceNew next is %p", pceNew, pct );
						}
						else{
							if( g.flags.bLogTrace )
								lprintf( "Need to do something with original to chain?" );
							DestroyConfigElement( pch, pceNew );
							pceNew = pEnd;
							pct = pEnd->next;
						}

					}
					pcePrior = pceNew;
					pceNew = NewConfigTestElement( pch );

					flags.store_as_end = 0;
				}
				else
				{
					if( flags.store_next_as_end )
					{
						flags.store_as_end = 1; // flag to store next thing as end condition.
						flags.store_next_as_end = 0;
					}
					if( flags.also_store_as_end )
					{
						if( g.flags.bLogTrace )
							lprintf( "Also Storing as end..." );
						AddLink( &pcePrior->data[0].singleword.pEnds, pceNew );
						pceNew->flags.singleword_terminator = 1;
						flags.also_store_as_end = 0;
					}
					if( flags.also_store_next_as_end )
					{
						flags.also_store_as_end = 1; // flag to store next thing as end condition.
						flags.also_store_next_as_end = 0;
					}

					{
						INDEX idx;
						PCONFIG_ELEMENT pceCheck;
						LIST_FORALL( pct->pVarElementList, idx, PCONFIG_ELEMENT, pceCheck )
						{
							if( pceCheck->type == pceNew->type )
							{
								// any data set will be overwritten...
								// uhmm should probably update to new links.
								if( !pceCheck->next )
								{
									lprintf( "Something fishy here... second instance of same type..." );
								}
								pcePrior = pceCheck;
								pct = pceCheck->next;
								break;
							}
						}
						if( !pceCheck )
						{
//#if defined( FULL_TRACE ) || defined( DEBUG_SLOWNESS )
							if( g.flags.bLogTrace )
								lprintf( "Adding into a new config test" );
//#endif
							AddLink( &pct->pVarElementList, pceNew );
							pct = pceNew->next = NewConfigTest( pch );
							pceNew->prior = pcePrior;
							pcePrior = pceNew;
							pceNew = NewConfigTestElement( pch );
//#if defined( FULL_TRACE ) || defined( DEBUG_SLOWNESS )
							if( g.flags.bLogTrace )
								lprintf( "Added." );
//#endif
						}
					}
				}
			}
			else
			{
				lprintf( "ignoreing NEW!?" );
			}

			flags.vartag = 0;
			flags.vector = 0;
			flags.ignore_new = 0;
		}
		else
		{
			if( TextIs( pWord, "%" ) )
			{
				if( g.flags.bLogTrace )
					lprintf( "next thing is a format character" );
				flags.vartag = 1;
			}
			else // is static text - literal match.
			{
				INDEX idx;
				PCONFIG_ELEMENT pConst = NULL;
				if( g.flags.bLogTrace )
					lprintf( "Storing %s as a constant text", GetText( pWord ) );
				if( flags.store_as_end )
				{
					pceNew->type = CONFIG_TEXT;
					if( g.flags.bLogTrace )
						lprintf( "Adding %s as the terminator", GetText( pWord ) );
					pceNew->data[0].pText = SegDuplicate( pWord );
					{
						INDEX idx;
						struct config_element_tag *pEnd;
						LIST_FORALL( pcePrior->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd ){
							if( pceNew->type == pEnd->type ) {
								if( SameText( pceNew->data[0].pText, pEnd->data[0].pText ) == 0 )
									break;
							}
						}
						if( !pEnd ){
							if( g.flags.bLogTrace )
								lprintf( "Added new terminator branch... pce needs a pct" );
							AddLink( &pcePrior->data[0].multiword.pEnds, pceNew );
							pceNew->flags.multiword_terminator = 1;
							pceNew->prior = pcePrior;
                     pct = pceNew->next = NewConfigTest( pch );
						}
						else{
                     // use existing one, so delete this one.
                     DestroyConfigElement( pch, pceNew );
							pceNew = pEnd;
                     pct = pEnd->next;
							if( g.flags.bLogTrace )
								lprintf( "Recovered an old pce to resume from...%p", pEnd );
						}

					}
					pcePrior = pceNew;
					pceNew = NewConfigTestElement( pch );
					flags.store_as_end = 0;
				}
				else
				{
					LIST_FORALL( pct->pConstElementList, idx, PCONFIG_ELEMENT, pConst )
					{
						if( IsConstText( pConst, &pWord ) )
						{
							pct = pConst->next;
							break;
						}
						pConst = NULL; // this is not always cleared...
					}
					if( pConst ) // continue outer loop (while word)
					{
						if( g.flags.bLogTrace )
							lprintf( "Found constant already in tree" );
						if( flags.also_store_as_end )
						{
							if( g.flags.bLogTrace )
								lprintf( "Also Storing as end... %s", GetText( pceNew->data[0].pText ) );
							AddLink( &pcePrior->data[0].singleword.pEnds, pceNew );
							pceNew->flags.singleword_terminator = 1;
							flags.also_store_as_end = 0;
						}

						continue;
					}
					else
					{
						if( g.flags.bLogTrace )
							lprintf( "Adding new constant to tree:%s", GetText( pWord ) );
						if( flags.also_store_as_end )
						{
							if( g.flags.bLogTrace )
								lprintf( "Also Storing as end... %s", GetText( pceNew->data[0].pText ) );
							AddLink( &pcePrior->data[0].singleword.pEnds, pceNew );
							pceNew->flags.singleword_terminator = 1;
							flags.also_store_as_end = 0;
						}

						pceNew->type = CONFIG_TEXT;
						pceNew->data[0].pText = SegDuplicate( pWord );
						AddLink( &pct->pConstElementList, pceNew );
						pct = pceNew->next = NewConfigTest( pch);
						pceNew->prior = pcePrior;
						pcePrior = pceNew;
						pceNew = NewConfigTestElement(pch );
					}
				}
			}
		}
		pWord = NEXTLINE( pWord );
	}
	if( flags.store_as_end ) {
		pceNew->type = CONFIG_NOTHING;
		AddLink( &pcePrior->data[0].multiword.pEnds, pceNew );
		pceNew->flags.multiword_terminator = 1;
		pceNew->prior = pcePrior;
		pct = pceNew->next = NewConfigTest( pch );
		if( g.flags.bLogTrace )
			lprintf( "%p pceNew next is %p", pceNew, pct );

		pcePrior = pceNew;
		pceNew = NewConfigTestElement( pch );

		flags.store_as_end = 0;
	}
	LineRelease( pLine );
	// end of the format line - add the procedure.
	pceNew->prior = pcePrior;
	// no need to update pcePrior - we're done.
	pceNew->type = CONFIG_PROCEDURE;
	pceNew->data[0].Process = Process;
	AddLink( &pct->pVarElementList, pceNew );
	return pceNew;
}

void AddConfigurationExx( PCONFIG_HANDLER pch, CTEXTSTR format, USER_CONFIG_HANDLER_EX Process, uintptr_t arg DBG_PASS ) {
	PCONFIG_ELEMENT element = _AddConfigurationEx( pch, format, (USER_CONFIG_HANDLER)Process DBG_RELAY );
	if( !element ) 
		return;
	element->type = CONFIG_PROCEDURE_EX;
	element->data[0].ProcessEx.Process = Process;
	element->data[0].ProcessEx.arg = arg;
}

void AddConfigurationEx( PCONFIG_HANDLER pch, CTEXTSTR format, USER_CONFIG_HANDLER Process DBG_PASS ) {
	_AddConfigurationEx( pch, format, Process DBG_RELAY );
}


//---------------------------------------------------------------------
#undef AddConfiguration
CONFIGSCR_PROC( void, AddConfiguration )( PCONFIG_HANDLER pch, CTEXTSTR format, USER_CONFIG_HANDLER Process )
{
	AddConfigurationEx( pch, format, Process DBG_SRC );
}

//---------------------------------------------------------------------

	CONFIGSCR_PROC( void, SetConfigurationEndProc )( PCONFIG_HANDLER pch
													, uintptr_t (CPROC *Process)( uintptr_t ) )
{
	pch->EndProcess = Process;
}

//---------------------------------------------------------------------

CONFIGSCR_PROC( void, SetConfigurationUnhandled )( PCONFIG_HANDLER pch
																, uintptr_t (CPROC *Process)( uintptr_t, CTEXTSTR ) )
{
	pch->Unhandled = Process;
}

//---------------------------------------------------------------------
CONFIGSCR_PROC( PCONFIG_HANDLER, CreateConfigurationEvaluator )( void )
{
	PCONFIG_HANDLER pch;
	DoInit();

	if( g.flags.bDisableMemoryLogging )
	{
		if( !(g._disabled_allocate_logging++) )
		{
			g._last_allocate_logging = SetAllocateLogging( FALSE );
		}
	}
	pch = (PCONFIG_HANDLER)Allocate( sizeof( CONFIG_HANDLER ) );
	MemSet( pch, 0, sizeof( *pch ) );
	// break input chunks into lines....
	AddLink( &pch->filters, FilterLines );
	SetLink( &pch->filter_data, FindLink( &pch->filters, (POINTER)FilterLines ), 0 );
	// end the lines at # (also remove \r, \n)
	AddLink( &pch->filters, FilterTerminators );
	SetLink( &pch->filter_data, FindLink( &pch->filters, (POINTER)FilterTerminators ), 0 );
	// get rid of \\ and \# to replace with appropriate things
	// since \# might be escaped, this is the only place I can handle the '#' character
	// to terminate lines at comment points.
	AddLink( &pch->filters, FilterEscapesAndComments );
	SetLink( &pch->filter_data, FindLink( &pch->filters, (POINTER)FilterEscapesAndComments ), 0 );
	//pch->ConfigTestRoot.pConstElementList = NULL; //CreateList();
	//pch->ConfigTestRoot.pVarElementList = NULL;//CreateList();
	return pch;
}

//---------------------------------------------------------------------

CONFIGSCR_PROC( void, ClearDefaultFilters )( PCONFIG_HANDLER pch )
{
	EmptyList( &pch->filters );
}

CONFIGSCR_PROC( void, AddConfigurationFilter )( PCONFIG_HANDLER pch, USER_FILTER filter )
{
	AddLink( &pch->filters, filter );
}


//---------------------------------------------------------------------

void DestroyConfigElement( PCONFIG_HANDLER pch, PCONFIG_ELEMENT pce )
{
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "Destroy element %p %p", pch, pce );
#endif
	DestroyConfigTest( pch, pce->next, TRUE );
	switch( pce->type )
	{
	case CONFIG_BINARY:
		if( pce->data[0].binary.data )
			Release( pce->data[0].binary.data );
		break;
	case CONFIG_TEXT:
		if( pce->data[0].pText )
			LineRelease( pce->data[0].pText );
		break;
	case CONFIG_SINGLE_WORD:
		if( pce->data[0].pWord )
			Release( pce->data[0].pWord );
		DeleteList( &pce->data[0].singleword.pEnds );
		break;
	case CONFIG_MULTI_WORD:
		if( pce->data[0].multiword.pWords )
			Release( pce->data[0].multiword.pWords );
		if( pce->data[0].multiword.pEnds )
		{
			struct config_element_tag *pEnd;
			INDEX idx;
			LIST_FORALL( pce->data[0].multiword.pEnds, idx, struct config_element_tag *, pEnd )
			{
#ifdef DEBUG_SAVE_CONFIG
				lprintf( "Destroy config element %p", pEnd );
#endif
				DestroyConfigElement( pch, pEnd );
			}
		}
		DeleteList( &pce->data[0].multiword.pEnds );
		break;
	case CONFIG_URL:
	case CONFIG_PATH:
	case CONFIG_FILE:
	case CONFIG_FILEPATH:
	case CONFIG_ADDRESS:
		break;
	case CONFIG_COLOR:
	case CONFIG_PROCEDURE:
	case CONFIG_PROCEDURE_EX:
	case CONFIG_UNKNOWN:
	case CONFIG_BOOLEAN:
	case CONFIG_INTEGER:
	case CONFIG_FRACTION:
	case CONFIG_FLOAT:
	case CONFIG_NOTHING:
		break;
	}
	DeleteFromSet( CONFIG_ELEMENT, pch->elements, pce );
	//Release( pce );
}

void DestroyConfigTest( PCONFIG_HANDLER pch, PCONFIG_TEST pct, int deallocate )
{
	PCONFIG_ELEMENT pce;
	INDEX idx;
	if( !pct )
		return;
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "Destroy Config %p %p", pch, pct );
#endif
	LIST_FORALL( pct->pConstElementList, idx, PCONFIG_ELEMENT, pce )
	{
#ifdef DEBUG_SAVE_CONFIG
		lprintf( "Delete const element" );
#endif
		DestroyConfigElement( pch, pce );
	}
	DeleteList( &pct->pConstElementList );
	LIST_FORALL( pct->pVarElementList, idx, PCONFIG_ELEMENT, pce )
	{
#ifdef DEBUG_SAVE_CONFIG
		lprintf( "Delete var element" );
#endif
		DestroyConfigElement( pch, pce );
	}
	DeleteList( &pct->pVarElementList );
	// a pconfig_handler has one static root - guaranteed root.
	// if this pct IS the root, don't attempt to delete from set
	if( deallocate )
		//if( pct != &pch->ConfigTestRoot )
		DeleteFromSet( CONFIG_TEST, pch->test_elements, pct );
	//Release( pct );
}

//---------------------------------------------------------------------

CONFIGSCR_PROC( void, DestroyConfigurationEvaluator )( PCONFIG_HANDLER pch )
{
	// since as noted in the structure
	//	// address of this IS the address of main structure
	// the configtest Release() will quickly free this .
	PLIST save_list_pConstElementList = pch->ConfigTestRoot.pConstElementList;
	PLIST save_list_pVarElementList = pch->ConfigTestRoot.pVarElementList;
#ifdef DEBUG_SAVE_CONFIG
	lprintf( "Destroy evaluator %p", pch );
#endif
	DestroyConfigTest( pch, &pch->ConfigTestRoot, FALSE );
	{
		INDEX idx;
		PCONFIG_STATE state;
		LIST_FORALL( pch->states, idx, PCONFIG_STATE, state )
		{
			if( ( save_list_pConstElementList
				&& (state->ConfigTestRoot.pConstElementList == save_list_pConstElementList ))
				|| ( save_list_pVarElementList
					&& (state->ConfigTestRoot.pVarElementList == save_list_pVarElementList ) )
			)
			{
				//probably this was the error I found in the field... there were
				// probably macros or something that caused excessive layering.
				// or - it was the old bug of not saving the state quite correctly.
				continue;
			}
			DestroyConfigTest( pch, &state->ConfigTestRoot, FALSE );
		}
		LIST_FORALL( pch->states, idx, PCONFIG_STATE, state )
		{
			Release( (POINTER)state->name );
			Release( state );
		}
	}
	if( pch->save_config_as && !pch->flags.bConfigSaveNameUsed )
		Release( (POINTER)pch->save_config_as );
	//lprintf( "Setting memory logging to %d", _last_allocate_logging );
	if( g.flags.bDisableMemoryLogging )
		if( g._disabled_allocate_logging )
		{
			g._disabled_allocate_logging--;
			if( !g._disabled_allocate_logging )
			{
				SetAllocateLogging( g._last_allocate_logging );
			}
		}
	DeleteList( &pch->states );
	DeleteList( &pch->filters );
	DeleteList( &pch->filter_data );
	DeleteSet( (PGENERICSET*)&(pch->elements) );
	DeleteSet( (PGENERICSET*)&(pch->test_elements) );
	Release( pch );
}


void StripConfigString( TEXTSTR out, CTEXTSTR in )
{
	// convert \r\n to literal characters...
	if( !in )
	{
		out[0] = 0;
		return;
	}
	for( ; in[0]; in++,out++ )
	{
		if( in[0] == '\\' )
		{
			switch( in[1] )
			{
			case 'n':
				in++;
				out[0] = '\n';
				break;
			default:
				out[0] = in[1];
				in++;
				break;
			}
		}
		else
			out[0] = in[0];
	}
	out[0] = in[0];
}

void ExpandConfigString( TEXTSTR out, CTEXTSTR in )
{
	// convert \r\n to literal characters...
	if( !in )
	{
		out[0] = 0;
		return;
	}
	for( ; in[0]; in++,out++ )
	{
		if( in[0] == '\n' )
		{
			out[0] = '\\';
			out++;
			out[0] = 'n';
		}
		else if( in[0] == '\\' )
		{
			out[0] = '\\';
			out++;
			out[0] = '\\';
		}
		else if( in[0] == '#' )
		{
			out[0] = '\\';
			out++;
			out[0] = '#';
		}
		else
			out[0] = in[0];
	}
	out[0] = in[0];
}

CTEXTSTR FormatColor( CDATA color )
{
	static TEXTCHAR color_buf[14];
	tnprintf( color_buf, sizeof( color_buf ), "$%02X%02X%02X%02X"
			, (int)AlphaVal( color )
			, (int)RedVal( color )
			, (int)GreenVal( color )
			, (int)BlueVal( color )
			);
	return color_buf;
}


#ifdef __cplusplus
}} //namespace sack { namespace config {
#endif

