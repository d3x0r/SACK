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
static PPARSE_BUFFERSET parseBuffers6;

char *json6_escape_string( const char *string ) {
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

#define GetUtfChar(x) __GetUtfChar(state->c,x)

static int gatherString( struct json_parse_state *state, struct json_input_buffer *input, struct json_output_buffer *output
				, TEXTRUNE start_c ) {
	//input->buf, &input->pos, input->size
	char *mOut = (output->buf);
	// collect a string
	int status = 0;
	size_t n;
	int escape;
	LOGICAL cr_escaped;
	const char *inPos = input->pos;
	TEXTRUNE c;
	escape = 0;
	cr_escaped = FALSE;
	while( ( n = inPos - input->buf ), (( n < input->size ) && (state->c = __GetUtfChar( c, &inPos ) )) && ( status >= 0 ) )
	{
		(state->col)++;
		if( state->c == '\\' )
		{
			if( escape ) (*mOut++) = '\\';
			else escape = 1;
		}
		else if( ( state->c == '"' ) || ( state->c == '\'' ) || ( state->c == '`' ) )
		{
			if( escape ) { (*mOut++) = state->c; escape = FALSE; }
			else if( state->c == start_c ) {
				status = 1;
				break;
			} else (*mOut++) = state->c; // other else is not valid close quote; just store as content.
		}
		else
		{
			if( cr_escaped ) {
				cr_escaped = FALSE;								
				if( state->c == '\n' ) {
					state->line++;
					state->col = 1;
					escape = FALSE;
					continue;
				}
			}
			if( escape )
			{
				switch( state->c )
				{
				case '\r':
					cr_escaped = TRUE;
					continue;
				case '\n':
					state->line++;
					state->col = 1;
					if( cr_escaped ) cr_escaped = FALSE;
					// fall through to clear escape status <CR><LF> support.
				case 2028: // LS (Line separator)
				case 2029: // PS (paragraph separate)
					escape = FALSE;
					continue;
				case '/':
				case '\\':
				case '"':
					(*mOut++) = state->c;
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
						TEXTRUNE oct_char = state->c - '0';
						int ofs;
						for( ofs = 0; ofs < 2; ofs++ )
						{
							state->c = __GetUtfChar( c, &inPos );
							oct_char *= 8;
							if( state->c >= '0' && state->c <= '9' )  oct_char += state->c - '0';
							else { inPos--; break; }
						}
						if( oct_char > 255 ) {
							lprintf( WIDE("(escaped character, parsing octal escape val=%d) fault while parsing; )") WIDE(" (near %*.*s[%c]%s)")
							         , oct_char
									 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
									 , (inPos) - ( (n>3)?3:n )
									 , state->c
									 , (inPos) + 1
									 );// fault
							status = -1;
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
							state->c = __GetUtfChar( c, &inPos );
							hex_char *= 16;
							if( state->c >= '0' && state->c <= '9' )      hex_char += state->c - '0';
							else if( state->c >= 'A' && state->c <= 'F' ) hex_char += ( state->c - 'A' ) + 10;
							else if( state->c >= 'a' && state->c <= 'f' ) hex_char += ( state->c - 'F' ) + 10;
							else {
								lprintf( WIDE("(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), state->c, n
										 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
										 , (inPos) - ( (n>3)?3:n )
										 , state->c
										 , (inPos) + 1
										 );// fault
								status = -1;
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
						for( ofs = 0; ofs < codePointLen && ( state->c != endCode ); ofs++ )
						{
							state->c = __GetUtfChar( c, &inPos );
							if( !ofs && state->c == '{' ) {
								codePointLen = 5; // collect up to 5 chars.
								endCode = '}';
								continue;
							} 
							if( state->c == '}' ) continue;
							hex_char *= 16;
							if( state->c >= '0' && state->c <= '9' )      hex_char += state->c - '0';
							else if( state->c >= 'A' && state->c <= 'F' ) hex_char += ( state->c - 'A' ) + 10;
							else if( state->c >= 'a' && state->c <= 'f' ) hex_char += ( state->c - 'F' ) + 10;
							else
								lprintf( WIDE("(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), state->c, n
										 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
										 , (inPos) - ( (n>3)?3:n )
										 , state->c
										 , (inPos) + 1
										 );// fault
						}
						mOut += ConvertToUTF8( mOut, hex_char );
					}
					break;
				default:
					if( cr_escaped ) {
						cr_escaped = FALSE;
						escape = FALSE;
						mOut += ConvertToUTF8( mOut, state->c );
					}										
					else {
						lprintf( WIDE("(escaped character) fault while parsing; '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), state->c, n
							 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
							 , (inPos) - ( (n>3)?3:n )
							 , state->c
							 , (inPos) + 1
							 );// fault
						status = -1;
					}
					break;
				}
				escape = 0;
			}
			else {
				mOut += ConvertToUTF8( mOut, state->c );
			}
		}
	}
	if( status )
		(*mOut++) = 0;  // terminate the string.
	input->pos = inPos;
	output->pos = mOut;
	return status;
}

struct json_parse_state * json6_begin_parse( void )
{
	struct json_parse_state *state = New( struct json_parse_state );

	state->elements = CreateDataList( sizeof( state->val ) );
	state->outBuffers = CreateLinkStack();
	state->inBuffers = CreateLinkQueue();
	state->outQueue = CreateLinkQueue();
	state->outValBuffers = NULL;

	state->line = 1;
	state->col = 1;
	state->n = 0; // character index;
	state->word = WORD_POS_RESET;
	state->status = TRUE;
	state->negative = FALSE;
	state->literalString = FALSE;

	state->context_stack = NULL;

	state->first_token = TRUE;
	state->context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
	state->parse_context = CONTEXT_UNKNOWN;
	state->comment = 0;
	state->completed = FALSE;
	//state->mOut = msg;// = NewArray( char, msglen );
	//state->msg_input = (char const *)msg;

	state->val.value_type = VALUE_UNSET;
	state->val.contains = NULL;
	state->val.name = NULL;
	state->val.string = NULL;

	state->complete_at_end = FALSE;
	state->gatheringString = FALSE;
	state->gatheringNumber = FALSE;

	state->pvtError = NULL;
	return state;
}


int json6_parse_add_data( struct json_parse_state *state
                            , char * msg
                            , size_t msglen )
{
	/* I guess this is a good parser */
	PPARSE_BUFFER input;
	struct json_output_buffer* output;
	int string_status;
	int retval = 0;
	if( !state->status )
		return -1;

	//char *token_begin;
	if( msg && msglen ) {
		input = GetFromSet( PARSE_BUFFER, &parseBuffers6 );
		input->pos = input->buf = msg;
		input->size = msglen;
		EnqueLink( &state->inBuffers, input );

		if( state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD ) {
			// have to extend the previous output buffer to include this one instead of allocating a split string.
			size_t offset;
			size_t offset2;
			output = (struct json_output_buffer*)DequeLink( &state->outQueue );
			//lprintf( "output from before is %p", output );
			offset = (output->pos - output->buf);
			offset2 = state->val.string - output->buf;
			AddLink( &state->outValBuffers, output->buf );
			output->buf = NewArray( char, output->size + msglen );
			MemCpy( output->buf + offset2, state->val.string, strlen( state->val.string ) );
			output->size += msglen;
			//lprintf( "previous val:%s", state->val.string, state->val.string );
			state->val.string = output->buf + offset2;
			output->pos = output->buf + offset;
			PrequeLink( &state->outQueue, output );
		}
		else {
			output = (struct json_output_buffer*)GetFromSet( PARSE_BUFFER, &parseBuffers6 );
			output->pos = output->buf = NewArray( char, msglen );
			output->size = msglen;
			EnqueLink( &state->outQueue, output );
		}
	}

	while( state->status && ( input = (PPARSE_BUFFER)DequeLink( &state->inBuffers ) ) ) {
		output = (struct json_output_buffer*)DequeLink( &state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;

		if( state->gatheringString ) {
			string_status = gatherString( state, input, output, state->gatheringStringFirstChar );
			if( string_status < 0 )
				state->status = FALSE;
			else if( string_status > 0 )
			{
				state->gatheringString = FALSE;
				state->n = input->pos - input->buf;
				if( state->status ) state->val.value_type = VALUE_STRING;
			}
			else {
				state->n = input->pos - input->buf;
			}
		}
		if( state->gatheringNumber ) {
			//lprintf( "continue gathering a string" );
			goto continueNumber;
		}

		//lprintf( "Completed at start?%d", state->completed );
		while( state->status && (state->n < input->size) && (state->c = GetUtfChar( &input->pos )) )
		{
			state->col++;
			state->n = input->pos - input->buf;

			if( state->comment ) {
				if( state->comment == 1 ) {
					if( state->c == '*' ) { state->comment = 3; continue; }
					if( state->c != '/' ) { 
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "Fault while parsing; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col ); 
						state->status = FALSE;
					}
					else state->comment = 2;
					continue;
				}
				if( state->comment == 2 ) {
					if( state->c == '\n' ) { state->comment = 0; continue; }
					else continue;
				}
				if( state->comment == 3 ) {
					if( state->c == '*' ) { state->comment = 4; continue; }
					else continue;
				}
				if( state->comment == 4 ) {
					if( state->c == '/' ) { state->comment = 0; continue; }
					else { if( state->c != '*' ) state->comment = 3; continue; }
				}
			}
			switch( state->c )
			{
			case '/':
				if( !state->comment ) state->comment = 1;
				break;
			case '{':
				{
					struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
#ifdef _DEBUG_PARSING
				lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
					state->val.value_type = VALUE_OBJECT;
					state->val.contains = CreateDataList( sizeof( state->val ) );
					AddDataItem( &state->elements, &state->val );
					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					state->elements = state->val.contains;
					PushLink( &state->context_stack, old_context );
					RESET_STATE_VAL();
					state->parse_context = CONTEXT_OBJECT_FIELD;
				}
				break;

			case '[':
				if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "Fault while parsing; while getting field name unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
					state->status = FALSE;
					break;
				}
				{
					struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts6 );
#ifdef _DEBUG_PARSING
					lprintf( "Begin a new array; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
					state->val.value_type = VALUE_ARRAY;
					state->val.contains = CreateDataList( sizeof( state->val ) );
					AddDataItem( &state->elements, &state->val );

					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					state->elements = state->val.contains;
					PushLink( &state->context_stack, old_context );

					RESET_STATE_VAL();
					state->parse_context = CONTEXT_IN_ARRAY;
				}
				break;

			case ':':
				if( state->parse_context == CONTEXT_OBJECT_FIELD )
				{
					if( state->word != WORD_POS_RESET
						&& state->word != WORD_POS_FIELD ) {
						// allow starting a new word
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "unquoted keyword used as object field name:parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
						break;
					}
					(*output->pos++) = 0;
					state->word = WORD_POS_RESET;
					if( state->val.name ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "two names single value?" );
					}
					state->val.name = state->val.string;
					state->val.string = NULL;
					state->parse_context = CONTEXT_OBJECT_FIELD_VALUE;
					state->val.value_type = VALUE_UNSET;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					if( state->parse_context == CONTEXT_IN_ARRAY )
						vtprintf( state->pvtError, WIDE( "(in array, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
					else
						vtprintf( state->pvtError, WIDE( "(outside any object, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
					state->status = FALSE;
				}
				break;
			case '}':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				// coming back after pushing an array or sub-object will reset the contxt to FIELD, so an end with a field should still push value.
				if( (state->parse_context == CONTEXT_OBJECT_FIELD) ) {
#ifdef _DEBUG_PARSING
					lprintf( "close object; empty object %d", state->val.value_type );
#endif
					RESET_STATE_VAL();

					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &state->context_stack );
						struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
					}
					if( state->parse_context == CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				}
				else if( (state->parse_context == CONTEXT_OBJECT_FIELD_VALUE) )
				{
					// first, add the last value
#ifdef _DEBUG_PARSING
					lprintf( "close object; push item %s %d", state->val.name, state->val.value_type );
#endif
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( &state->elements, &state->val );
					}
					RESET_STATE_VAL();

					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &state->context_stack );
						struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );

					}
					if( state->parse_context == CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
					//n++;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "Fault while parsing; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
					state->status = FALSE;
				}
				break;
			case ']':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				if( state->parse_context == CONTEXT_IN_ARRAY )
				{
#ifdef _DEBUG_PARSING
					lprintf( "close array, push last element: %d", state->val.value_type );
#endif
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( &state->elements, &state->val );
					}
					RESET_STATE_VAL();
					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &state->context_stack );
						struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
					}
					if( state->parse_context == CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context %d; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->parse_context, state->c, state->n, state->line, state->col );// fault
					state->status = FALSE;
				}
				break;
			case ',':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				if( state->parse_context == CONTEXT_IN_ARRAY )
				{
					if( state->val.value_type != VALUE_UNSET ) {
#ifdef _DEBUG_PARSING
						lprintf( "back in array; push item %d", state->val.value_type );
#endif
						AddDataItem( &state->elements, &state->val );
						RESET_STATE_VAL();
					}
					state->val.value_type = VALUE_UNDEFINED; // in an array, elements after a comma should init as undefined...
					// undefined allows [,,,] to be 4 values and [1,2,3,] to be 4 values with an undefined at end.
				}
				else if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE )
				{
					// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef _DEBUG_PARSING
					lprintf( "comma after field value, push field to object: %s", state->val.name );
#endif
					state->parse_context = CONTEXT_OBJECT_FIELD;
					if( state->val.value_type != VALUE_UNSET )
						AddDataItem( &state->elements, &state->val );
					RESET_STATE_VAL();
				}
				else
				{
					state->status = FALSE;
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );// fault
				}
				break;

			default:
				if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
					//lprintf( "gathering object field:%c  %*.*s", state->c, output->pos-output->buf, output->pos - output->buf, output->buf );
					if( state->c < 0xFF ) {
						if( nonIdentifiers8[state->c] ) {
							// invalid start/continue
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );	// fault
							break;
						}
					}
					else {
						int n;
						for( n = 0; n < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )); n++ ) {
							if( state->c == nonIdentifiers[n] ) {
								state->status = FALSE;
								if( !state->pvtError ) state->pvtError = VarTextCreate();
								vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );	// fault
								break;
							}
						}
						if( state->c < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )) )
							break;
					}
					switch( state->c )
					{
					case '`':
						// this should be a special case that passes continuation to gatherString
						// but gatherString now just gathers all strings
					case '"':
					case '\'':
						state->val.string = output->pos;
						state->gatheringString = TRUE;
						state->gatheringStringFirstChar = state->c;
						string_status = gatherString( state, input, output, state->c );
						//lprintf( "string gather status:%d", string_status );
						if( string_status < 0 )
							state->status = FALSE;
						else if( string_status > 0 )
							state->gatheringString = FALSE;
						state->n = input->pos - input->buf;
						if( state->status ) state->val.value_type = VALUE_STRING;
						break;
					case '\n':
						state->line++;
						state->col = 1;
						// fall through to normal space handling - just updated line/col position
					case ' ':
					case '\t':
					case '\r':
					case 0xFEFF: // ZWNBS is WS though
						if( state->word == WORD_POS_RESET )
							break;
						else if( state->word == WORD_POS_FIELD ) {
							state->word = WORD_POS_AFTER_FIELD;
							break;
						}
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n, state->line, state->col );	// fault
						// skip whitespace
						//n++;
						//lprintf( "whitespace skip..." );
						break;
					default:
						if( state->word == WORD_POS_RESET ) {
							state->word = WORD_POS_FIELD;
							state->val.string = output->pos;
						}
						if( state->c < 128 ) (*output->pos++) = state->c;
						else output->pos += ConvertToUTF8( output->pos, state->c );
						break; // default
					}
				}
				else switch( state->c )
				{
				case '`':
					// this should be a special case that passes continuation to gatherString
					// but gatherString now just gathers all strings
				case '"':
				case '\'':
					state->val.string = output->pos;
					state->gatheringString = TRUE;
					state->gatheringStringFirstChar = state->c;
					string_status = gatherString( state, input, output, state->c );
					//lprintf( "string gather status:%d", string_status );
					if( string_status < 0 )
						state->status = FALSE;
					else if( string_status > 0 )
						state->gatheringString = FALSE;
					else if( state->complete_at_end ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "End of string fail." );
						state->status = FALSE;
					}
					state->n = input->pos - input->buf;

					if( state->status ) {
						state->val.value_type = VALUE_STRING;
						if( state->complete_at_end ) {
							if( state->parse_context == CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
						}
					}
					break;
				case '\n':
					state->line++;
					state->col = 1;
					// FALLTHROUGH
				case ' ':
				case '\t':
				case '\r':
				case 0xFEFF:
					if( state->word == WORD_POS_END ) {
						state->word = WORD_POS_RESET;
						if( state->parse_context == CONTEXT_UNKNOWN ) {
							state->completed = TRUE;
						}
						break;
					}
					if( state->word == WORD_POS_RESET ) {
						break;
					}
					state->status = FALSE;
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n );	// fault
					// skip whitespace
					//n++;
					//lprintf( "whitespace skip..." );
					break;

					//----------------------------------------------------------
					//  catch characters for true/false/null/undefined which are values outside of quotes
				case 't':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_TRUE_1;
					else if( state->word == WORD_POS_INFINITY_6 ) state->word = WORD_POS_INFINITY_7;
					else { 
						state->status = FALSE; 
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
								, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'r':
					if( state->word == WORD_POS_TRUE_1 ) state->word = WORD_POS_TRUE_2;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'u':
					if( state->word == WORD_POS_TRUE_2 ) state->word = WORD_POS_TRUE_3;
					else if( state->word == WORD_POS_NULL_1 ) state->word = WORD_POS_NULL_2;
					else if( state->word == WORD_POS_RESET ) state->word = WORD_POS_UNDEFINED_1;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'e':
					if( state->word == WORD_POS_TRUE_3 ) {
						state->val.value_type = VALUE_TRUE;
						state->word = WORD_POS_END;
					}
					else if( state->word == WORD_POS_FALSE_4 ) {
						state->val.value_type = VALUE_FALSE;
						state->word = WORD_POS_END;
					}
					else if( state->word == WORD_POS_UNDEFINED_3 ) state->word = WORD_POS_UNDEFINED_4;
					else if( state->word == WORD_POS_UNDEFINED_7 ) state->word = WORD_POS_UNDEFINED_8;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'n':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_NULL_1;
					else if( state->word == WORD_POS_UNDEFINED_1 ) state->word = WORD_POS_UNDEFINED_2;
					else if( state->word == WORD_POS_UNDEFINED_6 ) state->word = WORD_POS_UNDEFINED_7;
					else if( state->word == WORD_POS_INFINITY_1 ) state->word = WORD_POS_INFINITY_2;
					else if( state->word == WORD_POS_INFINITY_4 ) state->word = WORD_POS_INFINITY_5;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'd':
					if( state->word == WORD_POS_UNDEFINED_2 ) state->word = WORD_POS_UNDEFINED_3;
					else if( state->word == WORD_POS_UNDEFINED_8 ) { state->val.value_type = VALUE_UNDEFINED; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'i':
					if( state->word == WORD_POS_UNDEFINED_5 ) state->word = WORD_POS_UNDEFINED_6;
					else if( state->word == WORD_POS_INFINITY_3 ) state->word = WORD_POS_INFINITY_4;
					else if( state->word == WORD_POS_INFINITY_5 ) state->word = WORD_POS_INFINITY_6;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'l':
					if( state->word == WORD_POS_NULL_2 ) state->word = WORD_POS_NULL_3;
					else if( state->word == WORD_POS_NULL_3 ) {
						state->val.value_type = VALUE_NULL;
						state->word = WORD_POS_END;
					}
					else if( state->word == WORD_POS_FALSE_2 ) state->word = WORD_POS_FALSE_3;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'f':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_FALSE_1;
					else if( state->word == WORD_POS_UNDEFINED_4 ) state->word = WORD_POS_UNDEFINED_5;
					else if( state->word == WORD_POS_INFINITY_2 ) state->word = WORD_POS_INFINITY_3;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'a':
					if( state->word == WORD_POS_FALSE_1 ) state->word = WORD_POS_FALSE_2;
					else if( state->word == WORD_POS_NAN_1 ) state->word = WORD_POS_NAN_2;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 's':
					if( state->word == WORD_POS_FALSE_3 ) state->word = WORD_POS_FALSE_4;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'I':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_INFINITY_1;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'N':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_NAN_1;
					else if( state->word == WORD_POS_NAN_2 ) { state->val.value_type = state->negative ? VALUE_NEG_NAN : VALUE_NAN; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
				case 'y':
					if( state->word == WORD_POS_INFINITY_7 ) { state->val.value_type = state->negative ? VALUE_NEG_INFINITY : VALUE_INFINITY; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, state->c, state->n, state->line, state->col );
					}// fault
					break;
					//
					//----------------------------------------------------------
				case '-':
					state->negative = !state->negative;
					break;

				default:
					if( (state->c >= '0' && state->c <= '9') || (state->c == '+') || (state->c == '.') )
					{
						LOGICAL fromHex;
						LOGICAL fromDate;
						LOGICAL exponent;
						const char *_msg_input; // to unwind last character past number.
			continueNumber:
						// always reset this here....
						// keep it set to determine what sort of value is ready.
						if( !state->gatheringNumber ) {
							exponent = FALSE;
							fromHex = FALSE;
							fromDate = FALSE;
							state->val.float_result = 0;
							state->val.string = output->pos;
							(*output->pos++) = state->c;  // terminate the string.
						}
						else
						{
							exponent = state->numberExponent;
							fromHex = state->numberFromHex;
							fromDate = state->numberFromDate;
						}
						while( (_msg_input = input->pos), ((state->n < input->size) && (state->c = GetUtfChar( &input->pos ))) )
						{
							//lprintf( "Number input:%c", state->c );
							state->col++;
							state->n = (input->pos - input->buf);
							// leading zeros should be forbidden.
							if( state->c == '_' )
								continue;

							if( (state->c >= '0' && state->c <= '9')
								|| (state->c == '+')
								)
							{
								(*output->pos++) = state->c;
							}
#if 0
							// to be implemented
							else if( state->c == ':' || state->c == '-' || state->c == 'Z' || state->c == '+' ) {
								/* toISOString()
								var today = new Date('05 October 2011 14:48 UTC');
								console.log(today.toISOString());
								// Returns 2011-10-05T14:48:00.000Z
								*/
								(*output->pos++) = state->c;

							}
#endif
							else if( state->c == 'x' || state->c == 'b' ) {
								// hex conversion.
								if( !fromHex ) {
									fromHex = TRUE;
									(*output->pos++) = state->c;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
									break;
								}
							}
							else if( (state->c == 'e') || (state->c == 'E') )
							{
								if( !exponent ) {
									state->val.float_result = 1;
									(*output->pos++) = state->c;
									exponent = TRUE;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
									break;
								}
							}
							else if( state->c == '.' )
							{
								if( !state->val.float_result ) {
									state->val.float_result = 1;
									(*output->pos++) = state->c;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->c, state->n, state->line, state->col );
									break;
								}
							}
							else
							{
								// in non streaming mode; these would be required to follow
								/*
								if( state->c == ' ' || state->c == '\t' || state->c == '\n' || state->c == '\r' || state->c == 0xFEFF
									|| state->c == ',' || state->c == '\'' || state->c == '\"' || state->c == '`' ) {

								}
								*/
								//lprintf( "Non numeric character received; push the value we have" );
								(*output->pos) = 0;
								break;
							}
						}
						input->pos = _msg_input;
						state->n = (input->pos - input->buf);

						//LogBinary( (uint8_t*)output->buf, output->size );
						if( (!state->complete_at_end) && state->n == input->size )
						{
							//lprintf( "completion mode is not end of string; and at end of string" );
							state->gatheringNumber = TRUE;
							state->numberExponent = exponent;
							state->numberFromDate = fromDate;
							state->numberFromHex = fromHex;
						}
						else
						{
							state->gatheringNumber = FALSE;
							(*output->pos++) = 0;
							//lprintf( "result with number:%s", state->val.string );

							if( state->val.float_result )
							{
								CTEXTSTR endpos;
								state->val.result_d = FloatCreateFromText( state->val.string, &endpos );
								if( state->negative ) { state->val.result_d = -state->val.result_d; state->negative = FALSE; }
							}
							else
							{
								state->val.result_n = IntCreateFromText( state->val.string );
								if( state->negative ) { state->val.result_n = -state->val.result_n; state->negative = FALSE; }
							}
							state->val.value_type = VALUE_NUMBER;
							if( state->parse_context == CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
						}
					}
					else
					{
						// fault, illegal characer
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault parsing '%c' unexpected %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), state->c, state->n
							, (int)((state->n > 4) ? 3 : (state->n-1)), (int)((state->n > 4) ? 3 : (state->n-1))
							, input->buf + state->n - ((state->n > 3) ? 3 : state->n)
							, state->c
							, input->buf + state->n
						);// fault
					}
					break; // default
				}
				break; // default of high level switch
			}
			// got a completed value; skip out
			if( state->completed ) {
				if( state->word == WORD_POS_END ) {
					state->word = WORD_POS_RESET;
				}
				break;
			}
		}
		//lprintf( "at end... %d %d comp:%d", state->n, input->size, state->completed );

		if( state->n == input->size ) {
			DeleteFromSet( PARSE_BUFFER, parseBuffers6, input );
			if( state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD ) {
				//lprintf( "output is still incomplete? " );
				PrequeLink( &state->outQueue, output );
				retval = 0;
			}
			else {
				PushLink( &state->outBuffers, output );
				if( state->parse_context == CONTEXT_UNKNOWN ) {
					state->completed = TRUE;
					retval = 1;
				}
			}
			//lprintf( "Is complete already?%d", state->completed );
		}
		else {
			// put these back into the stack.
			//lprintf( "put buffers back into queues..." );
			PrequeLink( &state->inBuffers, input );
			PrequeLink( &state->outQueue, output );
			retval = 2;  // if returning buffers, then obviously there's more in this one.
		}
		if( state->completed )
			break;
	} // while DequeInput

	if( !state->status ) {
		// some error condition; cannot resume parsing.
		return -1;
	}

	if( !state->gatheringNumber && !state->gatheringString )
		if( state->val.value_type != VALUE_UNSET ) {
			AddDataItem( &state->elements, &state->val );
			RESET_STATE_VAL();
		}
	
	if( state->completed ) {
		state->completed = FALSE;
	}

	return retval;

#if 0
	{
		struct json_parse_context *old_context;
		while( ( old_context = (struct json_parse_context *)PopLink( &state->context_stack ) ) ) {
			lprintf( "warning unclosed contexts...." );
			DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
		}
		if( state->context_stack )
			DeleteLinkStack( &state->context_stack );
	}
#endif

	return state->status;
}

PDATALIST json6_parse_get_data( struct json_parse_state *state ) {
	PDATALIST result = state->elements;
	state->elements = CreateDataList( sizeof( state->val ) );
	return result;
}

void json6_parse_clear_state( struct json_parse_state *state ) {
	if( state ) {
		PPARSE_BUFFER buffer;
		while( buffer = (PPARSE_BUFFER)PopLink( &state->outBuffers ) ) {
			Deallocate( const char *, buffer->buf );
			DeleteFromSet( PARSE_BUFFER, parseBuffers6, buffer );
		}
		while( buffer = (PPARSE_BUFFER)DequeLink( &state->inBuffers ) )
			DeleteFromSet( PARSE_BUFFER, parseBuffers6, buffer );
		DeleteLinkQueue( &state->inBuffers );
		DeleteLinkStack( &state->outBuffers );
		{
			char *buf;
			INDEX idx;
			LIST_FORALL( state->outValBuffers, idx, char*, buf ) {
				Deallocate( char*, buf );
			}
			DeleteList( &state->outValBuffers );
		}
		state->status = TRUE;
		state->parse_context = CONTEXT_UNKNOWN;
		state->word = WORD_POS_RESET;
		state->n = 0;
		state->col = 1;
		state->line = 1;
		state->gatheringString = FALSE;
		state->gatheringNumber = FALSE;
		{
			PDATALIST result = state->elements;
			state->elements = CreateDataList( sizeof( state->val ) );
			json6_dispose_message( &result );
		}
	}
}

PTEXT json6_parse_get_error( struct json_parse_state *state ) {
	if( state->pvtError ) {
		PTEXT error = VarTextGet( state->pvtError );
		return error;
	}
	return NULL;
}

void json6_parse_dispose_state( struct json_parse_state **ppState ) {
	struct json_parse_state *state = (*ppState);
	struct json_parse_context *old_context;
	PPARSE_BUFFER buffer;
	json6_dispose_message( &state->elements );
	DeleteDataList( &state->elements );
	while( buffer = (PPARSE_BUFFER)PopLink( &state->outBuffers ) ) {
		Deallocate( const char *, buffer->buf );
		DeleteFromSet( PARSE_BUFFER, parseBuffers6, buffer );
	}
	{
		char *buf;
		INDEX idx;
		LIST_FORALL( state->outValBuffers, idx, char*, buf ) {
			Deallocate( char*, buf );
		}
		DeleteList( &state->outValBuffers );
	}
	while( buffer = (PPARSE_BUFFER)DequeLink( &state->inBuffers ) )
		DeleteFromSet( PARSE_BUFFER, parseBuffers6, buffer );
	DeleteLinkQueue( &state->inBuffers );
	DeleteLinkStack( &state->outBuffers );
	DeleteFromSet( PARSE_CONTEXT, parseContexts6, state->context );

	while( (old_context = (struct json_parse_context *)PopLink( &state->context_stack )) ) {
		lprintf( "warning unclosed contexts...." );
		DeleteFromSet( PARSE_CONTEXT, parseContexts6, old_context );
	}
	if( state->context_stack )
		DeleteLinkStack( &state->context_stack );
	Deallocate( struct json_parse_state *, state );
	(*ppState) = NULL;
}

LOGICAL json6_parse_message( char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct json_parse_state *state = json6_begin_parse();
	static struct json_parse_state *_state;
	state->complete_at_end = TRUE;
	int result = json6_parse_add_data( state, msg, msglen );
	if( _state ) json6_parse_dispose_state( &_state );
	if( result > 0 ) {
		(*_msg_output) = json6_parse_get_data( state );
		_state = state;
		//json6_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	json6_parse_dispose_state( &state );
	return FALSE;
}

#undef GetUtfChar
#define GetUtfChar(x) __GetUtfChar(c,x)

LOGICAL _json6_parse_message( char * msg
                                 , size_t msglen
                                 , PDATALIST *_msg_output )
{
	struct json_parse_state state;
	struct json_input_buffer input;
	struct json_output_buffer output;
	/* I guess this is a good parser */
	PDATALIST elements = NULL;
	//size_t m = 0; // m is the output path; leave text inline; but escaped chars can offset/change the content
	TEXTSTR mOut = msg;// = NewArray( char, msglen );
	//size_t line = 1;
	//size_t col = 1;
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
	enum parse_context_modes parse_context = CONTEXT_UNKNOWN;
	struct json_value_container val;
	int comment = 0;

	char const * msg_input = (char const *)msg;
	char const * _msg_input;
	//char *token_begin;
	if( !_msg_output )
		return FALSE;

	state.line = 0;
	state.col = 0;

	elements = CreateDataList( sizeof( val ) );

	val.value_type = VALUE_UNSET;
	val.contains = NULL;
	val.name = NULL;
	val.string = NULL;

	while( status && ( n < msglen ) && ( c = GetUtfChar( &msg_input ) ) )
	{
		state.col++;
		n = msg_input - msg;
		if( comment ) {
			if( comment == 1 ) {
				if( c == '*' ) { comment = 3; continue; }
				if( c != '/' ) { lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); status = FALSE; }
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
				lprintf( WIDE("Fault while parsing; while getting field name unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
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
					lprintf( WIDE("(in array, got colon out of string):parsing fault; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
				else
					lprintf( WIDE("(outside any object, got colon out of string):parsing fault; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
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
				lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
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
				lprintf( WIDE("bad context %d; fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, parse_context, c, n, state.line, state.col );// fault
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
				lprintf( WIDE("bad context; fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );// fault
			}
			break;

		default:
			if( parse_context == CONTEXT_OBJECT_FIELD ) {
				if( c < 0xFF ) {
					if( nonIdentifiers8[c] ) {
						// invalid start/continue
						status = FALSE;
						lprintf( WIDE("fault while parsing object field name; \\u00%02X unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );	// fault
						break;
					} 
				} else {
					int n;
					for( n = 0; n < ( sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] ) ); n++ ) {
						if( c == nonIdentifiers[n] ) {
							status = FALSE;
							lprintf( WIDE("fault while parsing object field name; \\u00%02X unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );	// fault
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
					status = gatherString( &state, &input, &output, c ) >= 0;
					if( status ) val.value_type = VALUE_STRING;
					break;
					// yes, fall through to normal quote processing.
				case '"':
				case '\'':
					val.string = mOut;
					status = gatherString( &state, &input, &output, c ) >= 0;
					if( status ) val.value_type = VALUE_STRING;
					break;
				case '\n':
					state.line++;
					state.col = 1;
					// fall through to normal space handling - just updated state.line/state.col position
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
					lprintf( WIDE("fault while parsing; whitespace unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, n, state.line, state.col );	// fault
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
				status = gatherString( &state, &input, &output, c) >= 0;
				if( status ) val.value_type = VALUE_STRING;
				break;
				// yes, fall through to normal quote processing.
			case '"':
			case '\'':
				val.string = mOut;
				status = gatherString( &state, &input, &output, c ) >= 0;
				if( status ) val.value_type = VALUE_STRING;
				break;
			case ' ':
			case '\t':
			case '\r':
				if( 0 ) {
			case '\n':
					state.line++;
					state.col = 1;
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
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'r':
				if( word == WORD_POS_TRUE_1 ) word = WORD_POS_TRUE_2;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'u':
				if( word == WORD_POS_TRUE_2 ) word = WORD_POS_TRUE_3;
				else if( word == WORD_POS_NULL_1 ) word = WORD_POS_NULL_2;
				else if( word == WORD_POS_RESET ) word = WORD_POS_UNDEFINED_1;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
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
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'n':
				if( word == WORD_POS_RESET ) word = WORD_POS_NULL_1;
				else if( word == WORD_POS_UNDEFINED_1 ) word = WORD_POS_UNDEFINED_2;
				else if( word == WORD_POS_UNDEFINED_6 ) word = WORD_POS_UNDEFINED_7;
				else if( word == WORD_POS_INFINITY_1 ) word = WORD_POS_INFINITY_2;
				else if( word == WORD_POS_INFINITY_4 ) word = WORD_POS_INFINITY_5;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'd':
				if( word == WORD_POS_UNDEFINED_2 ) word = WORD_POS_UNDEFINED_3;
				else if( word == WORD_POS_UNDEFINED_8 ) { val.value_type=VALUE_UNDEFINED; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'i':
				if( word == WORD_POS_UNDEFINED_5 ) word = WORD_POS_UNDEFINED_6;
				else if( word == WORD_POS_INFINITY_3 ) word = WORD_POS_INFINITY_4;
				else if( word == WORD_POS_INFINITY_5 ) word = WORD_POS_INFINITY_6;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'l':
				if( word == WORD_POS_NULL_2 ) word = WORD_POS_NULL_3;
				else if( word == WORD_POS_NULL_3 ) {
					val.value_type = VALUE_NULL;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_2 ) word = WORD_POS_FALSE_3;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'f':
				if( word == WORD_POS_RESET ) word = WORD_POS_FALSE_1;
				else if( word == WORD_POS_UNDEFINED_4 ) word = WORD_POS_UNDEFINED_5;
				else if( word == WORD_POS_INFINITY_2 ) word = WORD_POS_INFINITY_3;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'a':
				if( word == WORD_POS_FALSE_1 ) word = WORD_POS_FALSE_2;
				else if( word == WORD_POS_NAN_1 ) word = WORD_POS_NAN_2;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 's':
				if( word == WORD_POS_FALSE_3 ) word = WORD_POS_FALSE_4;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'I':
				if( word == WORD_POS_RESET ) word = WORD_POS_INFINITY_1;
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'N':
				if( word == WORD_POS_RESET ) word = WORD_POS_NAN_1;
				else if( word == WORD_POS_NAN_2 ) { val.value_type = negative ? VALUE_NEG_NAN : VALUE_NAN; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
			case 'y':
				if( word == WORD_POS_INFINITY_7 ) { val.value_type = negative ? VALUE_NEG_INFINITY : VALUE_INFINITY; word = WORD_POS_RESET; }
				else { status = FALSE; lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col ); }// fault
				break;
		//
 	 	//----------------------------------------------------------
			case '-':
				negative = !negative;
				break;

			default:
				if( ( c >= '0' && c <= '9' ) || ( c == '+' ) || ( c == '.' ) )
				{
					LOGICAL fromHex;
					LOGICAL fromDate;
					LOGICAL exponent;
					exponent = FALSE;
					fromHex = FALSE;
					fromDate = FALSE;

					// always reset this here....
					// keep it set to determine what sort of value is ready.
					val.float_result = 0;

					val.string = mOut;
					(*mOut++) = c;  // terminate the string.
					while( (_msg_input=msg_input),(( n < msglen ) && (c = GetUtfChar( &msg_input )) ) )
					{
						state.col++;
						n = (msg_input - msg );
						// leading zeros should be forbidden.
						if( c == '_' )
							continue;

						if( ( c >= '0' && c <= '9' )
							|| ( c == '+' )
						  )
						{
							(*mOut++) = c;
						}
#if 0
// to be implemented
 						else if( c == ':' || c == '-' || c == 'Z' || c == '+' ) {
/* toISOString()
var today = new Date('05 October 2011 14:48 UTC');
console.log(today.toISOString()); 
// Returns 2011-10-05T14:48:00.000Z
*/
							(*mOut++) = c;
							
						}
#endif
						else if( c == 'x' || c == 'b' ) {
							// hex conversion.
							if( !fromHex ) {
								fromHex = TRUE;
								(*mOut++) = c;
							}
							else {
								status = FALSE;
								lprintf( WIDE("fault wile parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
								break;
							}
						}
						else if( ( c =='e' ) || ( c == 'E' ) )
						{
							if( !exponent ) {
								val.float_result = 1;
								(*mOut++) = c;
								exponent = TRUE;
							} else {
								status = FALSE;
								lprintf( WIDE("fault wile parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
								break;
							}
						}
						else if( c == '.' )
						{
							if( !val.float_result ) {
								val.float_result = 1;
								(*mOut++) = c;
							} else {
								status = FALSE;
								lprintf( WIDE("fault wile parsing; '%c' unexpected at %") _size_f WIDE("  %") _size_f WIDE(":%") _size_f, c, n, state.line, state.col );
								break;
							}
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
							 , (int)( (n>4)?3:(n-1) ), (int)( (n>4)?3:(n-1) )
							 , msg + n - ( (n>4)?4:n )
							 , c
							 , msg + n
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



