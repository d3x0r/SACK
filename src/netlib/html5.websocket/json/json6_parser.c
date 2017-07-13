#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>

#include "json.h"

#include "unicode_non_identifiers.h"
//#define DEBUG_PARSING

/*
Code Point	Name	Abbreviation	Usage
U+200C	ZERO WIDTH NON-JOINER	<ZWNJ>	IdentifierPart
U+200D	ZERO WIDTH JOINER	<ZWJ>	IdentifierPart
U+FEFF	ZERO WIDTH NO-BREAK SPACE	<ZWNBSP>	WhiteSpace
*/

/*
ID_Start       XID_Start        Uppercase letters, lowercase letters, titlecase letters, modifier letters
                                , other letters, letter numbers, stability extensions
ID_Continue    XID_Continue     All of the above, plus nonspacing marks, spacing combining marks, decimal numbers
                                , connector punctuations, stability extensions. 
                                These are also known simply as Identifier Characters, since they are a superset of 
                                the ID_Start. The set of ID_Start characters minus the ID_Continue characters are 
                                known as ID_Only_Continue characters.
*/

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif

static PPARSE_CONTEXTSET parseContexts6;

TEXTSTR json6_escape_string( CTEXTSTR string ) {
	size_t n;
	size_t m = 0;
	TEXTSTR output;
	TEXTSTR _output;
	if( !string ) return NULL;
	for( n = 0; string[n]; n++ ) {
		if( ( string[n] == '"' ) || ( string[n] == '\\' ) || (string[n] == '`') || (string[n] == '\''))
			m++;
	}
	_output = output = NewArray( TEXTCHAR, n+m+1 );
	for( n = 0; string[n]; n++ ) {
		if( ( string[n] == '"' ) || ( string[n] == '\\' ) || ( string[n] == '`' )|| ( string[n] == '\'' )) {
			(*output++) = '\\';
		}
		(*output++) = string[n];
	}
	return _output;
}


#define _2char(result,from) (((*from) += 2),( ( result & 0x1F ) << 6 ) | ( ( result & 0x3f00 )>>8))
#define _zero(result,from)  ((*from)++,0) 
#define _3char(result,from) ( ((*from) += 3),( ( ( result & 0xF ) << 12 ) | ( ( result & 0x3F00 ) >> 2 ) | ( ( result & 0x3f0000 ) >> 16 )) )
                                                                                          
#define _4char(result,from)  ( ((*from) += 4), ( ( ( result & 0x7 ) << 18 )     \
						| ( ( result & 0x3F00 ) << 4 )   \
						| ( ( result & 0x3f0000 ) >> 10 )    \
						| ( ( result & 0x3f000000 ) >> 24 ) ) )

#define __GetUtfChar( result, from )           ((result = ((TEXTRUNE*)*from)[0]),     \
		( ( !(result & 0xFF) )    \
          ?0          \
                                               \
	  :( ( result & 0x80 )                       \
		?( ( result & 0xE0 ) == 0xC0 )   \
			?( ( ( result & 0xC000 ) == 0x8000 ) ?_2char(result,from) : _zero(result,from)  )    \
			:( ( ( result & 0xF0 ) == 0xE0 )                           \
				?( ( ( ( result & 0xC000 ) == 0x8000 ) && ( ( result & 0xC00000 ) == 0x800000 ) ) ? _3char(result,from) : _zero(result,from)  )   \
				:( ( ( result & 0xF8 ) == 0xF0 )   \
		                    ? ( ( ( ( result & 0xC000 ) == 0x8000 ) && ( ( result & 0xC00000 ) == 0x800000 ) && ( ( result & 0xC0000000 ) == 0x80000000 ) )  \
					?_4char(result,from):_zero(result,from) )                                                                                                              \
				    :( ( ( result & 0xC0 ) == 0x80 )                                                                                                 \
			 		?_zero(result,from)                                                                                                                       \
					: ( (*from)++, (result & 0x7F) ) ) ) )                                                                                       \
		: ( (*from)++, (result & 0x7F) ) ) ) )

#define GetUtfChar(x) __GetUtfChar(c,x)

static LOGICAL gatherString( TEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut, size_t *line, size_t *col, TEXTRUNE start_c, int literalString ) {
	char *mOut = (*pmOut);
	// collect a string
	LOGICAL status = TRUE;
	size_t n;
	int escape;
	LOGICAL cr_escaped;
	TEXTRUNE c;
	escape = 0;
	cr_escaped = FALSE;
	while( ( n = (*msg_input) - msg ), (( n < msglen ) && (c = GetUtfChar( msg_input ) )) && status )
	{
		(*col)++;
		if( c == '\\' )
		{
			if( escape ) (*mOut++) = '\\';
			else escape = 1;
		}
		else if( ( c == '"' ) || ( c == '\'' ) || ( c == '`' ) )
		{
			if( escape ) { (*mOut++) = c; escape = FALSE; }
			else if( c == start_c ) {
				break;
			} else (*mOut++) = c; // other else is not valid close quote; just store as content.
		}
		else
		{
			if( cr_escaped ) {
				cr_escaped = FALSE;								
				if( c == '\n' ) {
					line[0]++;
					col[0] = 1;
					escape = FALSE;
					continue;
				}
			}
			if( escape )
			{
				switch( c )
				{
				case '\r':
					cr_escaped = TRUE;
					continue;
				case '\n':
					line[0]++;
					col[0] = 1;
					if( cr_escaped ) cr_escaped = FALSE;
					// fall through to clear escape status <CR><LF> support.
				case 2028: // LS (Line separator)
				case 2029: // PS (paragraph separate)
					escape = FALSE;
					continue;
				case '/':
				case '\\':
				case '"':
					(*mOut++) = c;
					break;
				case 't':
					(*mOut++) = '\t';
					break;
				case 'b':
					(*mOut++) = '\b';
					break;
				case 'n':
					(*mOut++) = '\n';
					break;
				case 'r':
					(*mOut++) = '\r';
					break;
				case 'f':
					(*mOut++) = '\f';
					break;
				case '0': case '1': case '2': case '3': 
					{
						TEXTRUNE oct_char = c - '0';
						int ofs;
						for( ofs = 0; ofs < 2; ofs++ )
						{
							c = GetUtfChar( msg_input );
							oct_char *= 8;
							if( c >= '0' && c <= '9' )  oct_char += c - '0';
							else { msg_input--; break; }
						}
						if( oct_char > 255 ) {
							lprintf( WIDE("(escaped character, parsing octal escape val=%d) fault while parsing; )") WIDE(" (near %*.*s[%c]%s)")
							                 , oct_char
									 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
									 , (*msg_input) - ( (n>3)?3:n )
									 , c
									 , (*msg_input) + 1
									 );// fault
							status = FALSE;
							break;
						} else { 
							if( oct_char < 128 ) (*mOut++) = oct_char;
							else mOut += ConvertToUTF8( mOut, oct_char );
						}
					}
					break;
				case 'x':
					{
						TEXTRUNE hex_char;
						int ofs;
						hex_char = 0;
						for( ofs = 0; ofs < 2; ofs++ )
						{
							c = GetUtfChar( msg_input );
							hex_char *= 16;
							if( c >= '0' && c <= '9' )      hex_char += c - '0';
							else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
							else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
							else {
								lprintf( WIDE("(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
										 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
										 , (*msg_input) - ( (n>3)?3:n )
										 , c
										 , (*msg_input) + 1
										 );// fault
								status = FALSE;
							}
						}
						if( hex_char < 128 ) (*mOut++) = hex_char;
						else mOut += ConvertToUTF8( mOut, hex_char );
					}
					break;
				case 'u':
					{
						TEXTRUNE hex_char;
						int ofs;
						int codePointLen;
						TEXTRUNE endCode;
						hex_char = 0;
						codePointLen = 4;
						endCode = 0;
						for( ofs = 0; ofs < codePointLen && ( c != endCode ); ofs++ )
						{
							c = GetUtfChar( msg_input );
							if( !ofs && c == '{' ) {
								codePointLen = 5; // collect up to 5 chars.
								endCode = '}';
								continue;
							} 
							if( c == '}' ) continue;
							hex_char *= 16;
							if( c >= '0' && c <= '9' )      hex_char += c - '0';
							else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
							else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
							else
								lprintf( WIDE("(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
										 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
										 , (*msg_input) - ( (n>3)?3:n )
										 , c
										 , (*msg_input) + 1
										 );// fault
						}
						mOut += ConvertToUTF8( mOut, hex_char );
					}
					break;
				default:
					if( cr_escaped ) {
						cr_escaped = FALSE;
						escape = FALSE;
						mOut += ConvertToUTF8( mOut, c );
					}										
					else {
						lprintf( WIDE("(escaped character) fault while parsing; '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
							 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
							 , (*msg_input) - ( (n>3)?3:n )
							 , c
							 , (*msg_input) + 1
							 );// fault
						status = FALSE;
					}
					break;
				}
				escape = 0;
			}
			else {
				mOut += ConvertToUTF8( mOut, c );
			}
		}
	}
	(*mOut++) = 0;  // terminate the string.
	(*pmOut) = mOut;
	return status;
}

LOGICAL json6_parse_message( TEXTSTR msg
                                 , size_t msglen
                                 , PDATALIST *_msg_output )
{
	/* I guess this is a good parser */
	PDATALIST elements = NULL;
	//size_t m = 0; // m is the output path; leave text inline; but escaped chars can offset/change the content
	TEXTSTR mOut = msg;// = NewArray( char, msglen );
	size_t line = 1;
	size_t col = 1;
	size_t n = 0; // character index;
	//size_t _n = 0; // character index; (restore1)
	int word = WORD_POS_RESET;
	TEXTRUNE c;
	LOGICAL status = TRUE;
	LOGICAL negative = FALSE;
	LOGICAL literalString = FALSE;

	PLINKSTACK context_stack = NULL;

	LOGICAL first_token = TRUE;
	PPARSE_CONTEXT context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
	int parse_context = CONTEXT_UNKNOWN;
	struct json_value_container val;
	int comment = 0;

	char const * msg_input = (char const *)msg;
	char const * _msg_input;
	//char *token_begin;
	if( !_msg_output )
		return FALSE;

	elements = CreateDataList( sizeof( val ) );

	val.value_type = VALUE_UNSET;
	val.contains = NULL;
	val.name = NULL;
	val.string = NULL;

	while( status && ( n < msglen ) && ( c = GetUtfChar( &msg_input ) ) )
	{
		col++;
		n = msg_input - msg;
		if( comment ) {
			if( comment == 1 ) {
				if( c == '*' ) { comment = 3; continue; }
				if( c != '/' ) { lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); status = FALSE; }
				else comment = 2;
				continue;
			}
			if( comment == 2 ) {
				if( c == '\n' ) { comment = 0; continue; }
				else continue;
			}
			if( comment == 3 ){
				if( c == '*' ) { comment = 4; continue; }
				else continue;
			}
			if( comment == 4 ) {
				if( c == '/' ) { comment = 0; continue; }
				else { if( c != '*' ) comment = 3; continue; }
			}
		}
		switch( c )
		{
		case '/':
			if( !comment ) comment = 1;
			break;
		case '{':
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
#ifdef _DEBUG_PARSING
				lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
				val.value_type = VALUE_OBJECT;
				val.contains = CreateDataList( sizeof( val ) );
				AddDataItem( &elements, &val );
				old_context->context = parse_context;
				old_context->elements = elements;
				elements = val.contains;
				PushLink( &context_stack, old_context );
				RESET_VAL();
				parse_context = CONTEXT_OBJECT_FIELD;
			}
			break;

		case '[':
			if( parse_context == CONTEXT_OBJECT_FIELD ) {
				lprintf( WIDE("Fault while parsing; while getting field name unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );
				status = FALSE;
				break;
			}
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
#ifdef _DEBUG_PARSING
				lprintf( "Begin a new array; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				

				val.value_type = VALUE_ARRAY;
				val.contains = CreateDataList( sizeof( val ) );
				AddDataItem( &elements, &val );

				old_context->context = parse_context;
				old_context->elements = elements;
				elements = val.contains;
				PushLink( &context_stack, old_context );

				RESET_VAL();
				parse_context = CONTEXT_IN_ARRAY;
			}
			break;

		case ':':
			if( parse_context == CONTEXT_OBJECT_FIELD )
			{
				(*mOut++) = 0;
				word = WORD_POS_RESET;
				if( val.name ) {
					lprintf( "two names single value?" );
				}
				val.name = val.string;
				val.string = NULL;
				parse_context = CONTEXT_OBJECT_FIELD_VALUE;
				val.value_type = VALUE_UNSET;
			}
			else
			{
				if( parse_context == CONTEXT_IN_ARRAY )
					lprintf( WIDE("(in array, got colon out of string):parsing fault; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );
				else
					lprintf( WIDE("(outside any object, got colon out of string):parsing fault; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );
				status = FALSE;
			}
			break;
		case '}':
			// coming back after pushing an array or sub-object will reset the contxt to FIELD, so an end with a field should still push value.
			if( ( parse_context == CONTEXT_OBJECT_FIELD ) ) {
#ifdef _DEBUG_PARSING
				lprintf( "close object; empty object %d", val.value_type );				
#endif
				RESET_VAL();

				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt-1 );
					oldVal->contains = elements;  // save updated elements list in the old value in the last pushed list.
					parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
					elements = old_context->elements;
					DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );

				}
			}
			else if( ( parse_context == CONTEXT_OBJECT_FIELD_VALUE ) )
			{
				// first, add the last value
#ifdef _DEBUG_PARSING
				lprintf( "close object; push item %s %d", val.name, val.value_type );
#endif                          
				if( val.value_type != VALUE_UNSET ) {
					AddDataItem( &elements, &val );
				}
				RESET_VAL();

				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt-1 );
					oldVal->contains = elements;  // save updated elements list in the old value in the last pushed list.
					parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
					elements = old_context->elements;
					DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );

				}
				//n++;
			}
			else
			{
				lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );
				status = FALSE;
			}
			break;
		case ']':
			if( parse_context == CONTEXT_IN_ARRAY )
			{
#ifdef _DEBUG_PARSING
				lprintf( "close array, push last element: %d", val.value_type );
#endif
				if( val.value_type != VALUE_UNSET ) {
					AddDataItem( &elements, &val );
				}
				RESET_VAL();
				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt-1 );
					oldVal->contains = elements;  // save updated elements list in the old value in the last pushed list.
					
					parse_context = old_context->context;
					elements = old_context->elements;
					DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
				}
			}
			else
			{
				lprintf( WIDE("bad context %d; fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, parse_context, c, n, line, col );// fault
				status = FALSE;
			}
			break;
		case ',':
			if( parse_context == CONTEXT_IN_ARRAY )
			{
				if( val.value_type != VALUE_UNSET ) {
#ifdef _DEBUG_PARSING
					lprintf( "back in array; push item %d", val.value_type );
#endif
					AddDataItem( &elements, &val );
					RESET_VAL();
				}
				val.value_type = VALUE_UNDEFINED; // in an array, elements after a comma should init as undefined...
				// undefined allows [,,,] to be 4 values and [1,2,3,] to be 4 values with an undefined at end.
			}
			else if( parse_context == CONTEXT_OBJECT_FIELD_VALUE )
			{
				// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef _DEBUG_PARSING
				lprintf( "comma after field value, push field to object: %s", val.name );
#endif
				parse_context = CONTEXT_OBJECT_FIELD;
				if( val.value_type != VALUE_UNSET )
					AddDataItem( &elements, &val );
				RESET_VAL();
			}
			else
			{
				status = FALSE;
				lprintf( WIDE("bad context; fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );// fault
			}
			break;

		default:
			if( parse_context == CONTEXT_OBJECT_FIELD ) {
				if( c < 0xFF ) {
					if( nonIdentifiers8[c] ) {
						// invalid start/continue
						status = FALSE;
						lprintf( WIDE("fault while parsing object field name; \\u00%02X unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );	// fault
						break;
					} 
				} else {
					int n;
					for( n = 0; n < ( sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] ) ); n++ ) {
						if( c == nonIdentifiers[n] ) {
							status = FALSE;
							lprintf( WIDE("fault while parsing object field name; \\u00%02X unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );	// fault
							break;
						}
					}
					if( c < ( sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] ) ) )
						break;
				}
				switch( c )
				{
				case '`':
					val.string = mOut;
					status = gatherString( msg, &msg_input, msglen, &mOut, &line, &col, c, TRUE );
					if( status ) val.value_type = VALUE_STRING;
					break;
					// yes, fall through to normal quote processing.
				case '"':
				case '\'':
					val.string = mOut;
					status = gatherString( msg, &msg_input, msglen, &mOut, &line, &col, c, FALSE );
					if( status ) val.value_type = VALUE_STRING;
					break;
				case '\n':
					line++;
					col = 1;
					// fall through to normal space handling - just updated line/col position
				case ' ':
				case '\t':
				case '\r':
				case 0xFEFF: // ZWNBS is WS though
					if( word == WORD_POS_RESET )
						break;
					else if( word == WORD_POS_FIELD ) {
						word = WORD_POS_AFTER_FIELD;
					}
					status = FALSE;
					lprintf( WIDE("fault while parsing; whitespace unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, n, line, col );	// fault
					// skip whitespace
					//n++;
					//lprintf( "whitespace skip..." );
					break;
				default:
					if( word == WORD_POS_RESET ) {
						word = WORD_POS_FIELD;
						val.string = mOut;
					}
					if( c < 128 ) (*mOut++) = c;
					else mOut += ConvertToUTF8( mOut, c );
					break; // default
				}
								
			}
			else switch( c )
			{
			case '`':
				val.string = mOut;
				status = gatherString( msg, &msg_input, msglen, &mOut, &line, &col, c, TRUE );
				if( status ) val.value_type = VALUE_STRING;
				break;
				// yes, fall through to normal quote processing.
			case '"':
			case '\'':
				val.string = mOut;
				status = gatherString( msg, &msg_input, msglen, &mOut, &line, &col, c, FALSE );
				if( status ) val.value_type = VALUE_STRING;
				break;
#if 0
				{
					// collect a string
					int escape;
					TEXTRUNE start_c;
					LOGICAL cr_escaped;
					escape = 0;
					start_c = c;
					cr_escaped = FALSE;
					val.string = mOut;
					while( (_n=n), (( n < msglen ) && (c = GetUtfChar( &msg_input ) )) && status )
					{
						if( c == '\\' )
						{
							if( escape ) (*mOut++) = '\\';
							else escape = 1;
						}
						else if( ( c == '"' ) || ( c == '\'' ) || ( c == '`' ) )
						{
							if( escape ) { (*mOut++) = c; escape = FALSE; }
							else if( c == start_c ) {
								break;
							} else (*mOut++) = c; // other else is not valid close quote; just store as content.
						}
						else
						{
							if( cr_escaped ) {
								cr_escaped = FALSE;								
								if( c == '\n' ) {
									escape = FALSE;
									continue;
								}
							}
							if( escape )
							{
								switch( c )
								{
								case '\r':
									cr_escaped = TRUE;
									continue;
								case '\n':
									if( cr_escaped ) cr_escaped = FALSE;
									// fall through to clear escape status <CR><LF> support.
								case 2028: // LS (Line separator)
								case 2029: // PS (paragraph separate)
									escape = FALSE;
									continue;
								case '/':
								case '\\':
								case '"':
									(*mOut++) = c;
									break;
								case 't':
									(*mOut++) = '\t';
									break;
								case 'b':
									(*mOut++) = '\b';
									break;
								case 'n':
									(*mOut++) = '\n';
									break;
								case 'r':
									(*mOut++) = '\r';
									break;
								case 'f':
									(*mOut++) = '\f';
									break;
								case '0': case '1': case '2': case '3': 
									{
										TEXTRUNE oct_char = c - '0';
										int ofs;
										for( ofs = 0; ofs < 2; ofs++ )
										{
											c = GetUtfChar( &msg_input );
											oct_char *= 8;
											if( c >= '0' && c <= '9' )  hex_char += c - '0';
											else { msg_input--; break; }
										}
										if( oct_char > 255 ) {
											lprintf( WIDE("(escaped character, parsing octal escape val=%d) fault while parsing; )") WIDE(" (near %*.*s[%c]%s)")
											                 , oct_char
													 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
													 , msg + n - ( (n>3)?3:n )
													 , c
													 , msg + n + 1
													 );// fault
											status = FALSE;
											break;
										} else { 
											if( oct_char < 128 ) (*mOut++) = oct_char;
											else mOut += ConvertToUTF8( mOut, oct_char );
										}
									}
									break;
								case 'x':
									{
										TEXTRUNE hex_char;
										int ofs;
										hex_char = 0;
										for( ofs = 0; ofs < 2; ofs++ )
										{
											c = GetUtfChar( &msg_input );
											hex_char *= 16;
											if( c >= '0' && c <= '9' )      hex_char += c - '0';
											else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
											else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
											else {
												lprintf( WIDE("(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
														 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
												status = FALSE;
											}
										}
										if( hex_char < 128 ) (*mOut++) = hex_char;
										else mOut += ConvertToUTF8( mOut, hex_char );
									}
									break;
								case 'u':
									{
										TEXTRUNE hex_char;
										int ofs;
										int codePointLen;
										TEXTRUNE endCode;
										hex_char = 0;
										codePointLen = 4;
										endCode = 0;
										for( ofs = 0; ofs < codePointLen && ( c != endCode ); ofs++ )
										{
											c = GetUtfChar( &msg_input );
											if( !ofs && c == '{' ) {
												codePointLen = 5; // collect up to 5 chars.
												endCode = '}';
												continue;
											} 
											if( c == '}' ) continue;
											hex_char *= 16;
											if( c >= '0' && c <= '9' )      hex_char += c - '0';
											else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
											else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
											else
												lprintf( WIDE("(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
														 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
										}
										mOut += ConvertToUTF8( mOut, hex_char );
									}
									break;
								default:
									if( cr_escaped ) {
										cr_escaped = FALSE;
										escape = FALSE;
										mOut += ConvertToUTF8( mOut, c );
									}										
									else {
										lprintf( WIDE("(escaped character) fault while parsing; '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
											 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
											 , msg + n - ( (n>3)?3:n )
											 , c
											 , msg + n + 1
											 );// fault
										status = FALSE;
									}
									break;
								}
								escape = 0;
							}
							else {
								mOut += ConvertToUTF8( mOut, c );
							}
						}
					}
					(*mOut++) = 0;  // terminate the string.
					val.value_type = VALUE_STRING;
					break;
				}
#endif

			case ' ':
			case '\t':
			case '\r':
				if( 0 ) {
			case '\n':
					line++;
					col = 1;
				}
			case 0xFEFF:
				if( word == WORD_POS_RESET )
					break;
				status = FALSE;
				lprintf( WIDE("fault while parsing; whitespace unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, n );	// fault
				// skip whitespace
				//n++;
				//lprintf( "whitespace skip..." );
				break;

		//----------------------------------------------------------
		//  catch characters for true/false/null/undefined which are values outside of quotes
			case 't':
				if( word == WORD_POS_RESET ) word = WORD_POS_TRUE_1;
				else if( word == WORD_POS_INFINITY_6 ) word = WORD_POS_INFINITY_7;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'r':
				if( word == WORD_POS_TRUE_1 ) word = WORD_POS_TRUE_2;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'u':
				if( word == WORD_POS_TRUE_2 ) word = WORD_POS_TRUE_3;
				else if( word == WORD_POS_NULL_1 ) word = WORD_POS_NULL_2;
				else if( word == WORD_POS_RESET ) word = WORD_POS_UNDEFINED_1;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'e':
				if( word == WORD_POS_TRUE_3 ) {
					val.value_type = VALUE_TRUE;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_4 ) {
					val.value_type = VALUE_FALSE;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_UNDEFINED_3 ) word = WORD_POS_UNDEFINED_4;
				else if( word == WORD_POS_UNDEFINED_7 ) word = WORD_POS_UNDEFINED_8;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'n':
				if( word == WORD_POS_RESET ) word = WORD_POS_NULL_1;
				else if( word == WORD_POS_UNDEFINED_1 ) word = WORD_POS_UNDEFINED_2;
				else if( word == WORD_POS_UNDEFINED_6 ) word = WORD_POS_UNDEFINED_7;
				else if( word == WORD_POS_INFINITY_1 ) word = WORD_POS_INFINITY_2;
				else if( word == WORD_POS_INFINITY_4 ) word = WORD_POS_INFINITY_5;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'd':
				if( word == WORD_POS_UNDEFINED_2 ) word = WORD_POS_UNDEFINED_3;
				else if( word == WORD_POS_UNDEFINED_8 ) { val.value_type=VALUE_UNDEFINED; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'i':
				if( word == WORD_POS_UNDEFINED_5 ) word = WORD_POS_UNDEFINED_6;
				else if( word == WORD_POS_INFINITY_3 ) word = WORD_POS_INFINITY_4;
				else if( word == WORD_POS_INFINITY_5 ) word = WORD_POS_INFINITY_6;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'l':
				if( word == WORD_POS_NULL_2 ) word = WORD_POS_NULL_3;
				else if( word == WORD_POS_NULL_3 ) {
					val.value_type = VALUE_NULL;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_2 ) word = WORD_POS_FALSE_3;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'f':
				if( word == WORD_POS_RESET ) word = WORD_POS_FALSE_1;
				else if( word == WORD_POS_UNDEFINED_4 ) word = WORD_POS_UNDEFINED_5;
				else if( word == WORD_POS_INFINITY_2 ) word = WORD_POS_INFINITY_3;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'a':
				if( word == WORD_POS_FALSE_1 ) word = WORD_POS_FALSE_2;
				else if( word == WORD_POS_NAN_1 ) word = WORD_POS_NAN_2;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 's':
				if( word == WORD_POS_FALSE_3 ) word = WORD_POS_FALSE_4;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'I':
				if( word == WORD_POS_RESET ) word = WORD_POS_INFINITY_1;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'N':
				if( word == WORD_POS_RESET ) word = WORD_POS_NAN_1;
				else if( word == WORD_POS_NAN_2 ) { val.value_type = negative ? VALUE_NEG_NAN : VALUE_NAN; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
			case 'y':
				if( word == WORD_POS_INFINITY_7 ) { val.value_type = negative ? VALUE_NEG_INFINITY : VALUE_INFINITY; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col ); }// fault
				break;
		//
 	 	//----------------------------------------------------------
			case '-':
				negative = !negative;
				break;

			default:
				if( ( c >= '0' && c <= '9' ) || ( c == '+' ) )
				{
					LOGICAL fromHex;
					fromHex = FALSE;
					// always reset this here....
					// keep it set to determine what sort of value is ready.
					val.float_result = 0;

					val.string = mOut;
					(*mOut++) = c;  // terminate the string.
					while( (_msg_input=msg_input),(( n < msglen ) && (c = GetUtfChar( &msg_input )) ) )
					{
						col++;
						n = (msg_input - msg );
						// leading zeros should be forbidden.
						if( ( c >= '0' && c <= '9' )
							|| ( c == '-' )
							|| ( c == '+' )
						  )
						{
							(*mOut++) = c;
						}
						else if( c == 'x' || c == 'b' ) {
							// hex conversion.
							if( !fromHex ) {
								fromHex = TRUE;
								(*mOut++) = c;
							}
							else {
								status = FALSE;
								lprintf( WIDE("fault wile parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, line, col );
								break;
							}
						}
						else if( ( c =='e' ) || ( c == 'E' ) || ( c == '.' ) )
						{
							val.float_result = 1;
							(*mOut++) = c;
						}
						else
						{
							break;
						}
					}
					{
						(*mOut++) = 0;

						if( val.float_result )
						{
							CTEXTSTR endpos;
							val.result_d = FloatCreateFromText( val.string, &endpos );
							if( negative ) { val.result_d = -val.result_d; negative = FALSE; }
						}
						else
						{
							val.result_n = IntCreateFromText( val.string );
							if( negative ) { val.result_n = -val.result_n; negative = FALSE; }
						}
					}
					msg_input = _msg_input;
					n = msg_input - msg;
					val.value_type = VALUE_NUMBER;
				}
				else
				{
					// fault, illegal characer (whitespace?)
					status = FALSE;
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
							 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
							 , msg + n - ( (n>3)?3:n )
							 , c
							 , msg + n + 1
							 );// fault
				}
				break; // default
			}
			break; // default of high level switch
		}
	}

	{
		struct json_parse_context *old_context;
		while( ( old_context = (struct json_parse_context *)PopLink( &context_stack ) ) ) {
			lprintf( "warning unclosed contexts...." );
			DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
		}
		if( context_stack )
			DeleteLinkStack( &context_stack );
	}

	if( val.value_type != VALUE_UNSET ) 
		AddDataItem( &elements, &val );

	(*_msg_output) = elements;
	// clean all contexts

	if( !status )
	{
		//Release( (*_msg_output) );
		//(*_msg_output) = NULL;
	}
	return status;
}

void json6_dispose_decoded_message( struct json6_context_object *format
                                 , POINTER msg_data )
{
	// a complex format might have sub-parts .... but for now we'll assume simple flat structures
	//Release( msg_data );
}

void json6_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		//if( val->name ) Release( val->name );
		//if( val->string ) Release( val->string );
		if( val->value_type == VALUE_OBJECT )
			json_dispose_message( &val->contains );
	}
	// quick method
	DeleteDataList( msg_data );

}

// puts the current collected value into the element; assumes conversion was correct
static void FillDataToElement6( struct json_context_object_element *element
							    , size_t object_offset
								, struct json_value_container *val
								, POINTER msg_output )
{
	if( !val->name )
		return;
	// remove name; indicate that the value has been used.
	Release( val->name );
	val->name = NULL;
	switch( element->type )
	{
	case JSON_Element_String:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NULL:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = NULL;
				break;
			case VALUE_STRING:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = StrDup( val->string );
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->value_type );
				break;
			}
		}
		break;
	case JSON_Element_Integer_64:
	case JSON_Element_Integer_32:
	case JSON_Element_Integer_16:
	case JSON_Element_Integer_8:
	case JSON_Element_Unsigned_Integer_64:
	case JSON_Element_Unsigned_Integer_32:
	case JSON_Element_Unsigned_Integer_16:
	case JSON_Element_Unsigned_Integer_8:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_TRUE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				}
				break;
			case VALUE_FALSE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				}
				break;
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int") );
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (int64_t)val->result_d;
				}
				else
				{
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_n;
				}
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->value_type );
				break;
			}
		}
		break;

	case JSON_Element_Float:
	case JSON_Element_Double:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_d;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_d;

				}
				else
				{
               // this is probably common (0 for instance)
					lprintf( WIDE("warning received int, converting to float") );
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_n;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (double)val->result_n;

				}
				break;
			default:
				lprintf( WIDE("Expected a float, but parsed result was a %d"), val->value_type );
				break;
			}

		}

		break;
	case JSON_Element_PTRSZVAL_BLANK_0:
	case JSON_Element_PTRSZVAL:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int (uintptr_t)") );
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_d;
				}
				else
				{
					// this is probably common (0 for instance)
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_n;
				}
				break;
			}
		}
		break;
	}
}


LOGICAL json6_decode_message( struct json_context *format
								, PDATALIST msg_data
								, struct json_context_object **result_format
								, POINTER *_msgbuf )
{
	#if 0
	PLIST elements = format->elements;
	struct json_context_object_element *element;

	// message is allcoated +7 bytes in case last part is a uint8_t type
	// all integer stores use uint64_t type to store the collected value.
	if( !msg_output )
		msg_output = (*_msg_output)
			= NewArray( uint8_t, format->object_size + 7  );

	LOGICAL first_token = TRUE;

				if( first_token )
				{
					// begin an object.
					// content is
					elements = format->members;
					PushLink( &element_lists, elements );
					first_token = 0;
					//n++;
				}
				else
				{
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element *, element )
					{
						if( StrCaseCmp( element->name, GetText( val.name ) ) == 0 )
						{
							if( ( element->type == JSON_Element_Object )
								|| ( element->type == JSON_Element_ObjectPointer ) )
							{
								if( element->object )
								{
									// save prior element list; when we return to this one?
									struct json_parse_context *old_context = New( struct json_parse_context );
									old_context->context = parse_context;
									old_context->elements = elements;
									old_context->object = format;
									PushLink( &context_stack, old_context );
									format = element->object;
									elements = element->object->members;
									//n++;
									break;
								}
								else
								{
									lprintf( WIDE("Error; object type does not have an object") );
									status = FALSE;
								}
							}
							else
							{
								lprintf( WIDE("Incompatible value expected object type, type is %d"), element->type );
							}
						}
					}
				}
#endif
	return FALSE;
}

#undef GetUtfChar

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



