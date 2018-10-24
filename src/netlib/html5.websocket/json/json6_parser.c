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


char *json6_escape_string_length( const char *string, size_t len, size_t *outlen ) {
	size_t m = 0;
	size_t ch;
	const char *input;
	TEXTSTR output;
	TEXTSTR _output;
	if( !( input = string ) ) return NULL;
	for( ch = 0; ch < len; ch++, input++ ) {
		if( (input[0] == '"' ) || (input[0] == '\\' ) || (input[0] == '`') || (input[0] == '\'') /*|| (input[0] == '\n') || (input[0] == '\t')*/ )
			m++;
	}
	_output = output = NewArray( TEXTCHAR, len+m+1 );
	for( (ch = 0), (input = string); ch < len; ch++, input++ ) {
		if( (input[0] == '"' ) || (input[0] == '\\' ) || (input[0] == '`' )|| (input[0] == '\'' )) {
			(*output++) = '\\';
		}
		(*output++) = input[0];
	}
	(*output) = 0;
	if( outlen ) (*outlen) = output - _output;
	return _output;
}

char *json6_escape_string( const char *string ) {
	return json6_escape_string_length( string, strlen( string ), NULL );
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
          ?_zero(result,from)   \
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

static int gatherString6(struct json_parse_state *state, CTEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut, TEXTRUNE start_c
		//, int literalString 
		) {
	char *mOut = (*pmOut);
	// collect a string
	int status = 0;
	size_t n;
	//int escape;
	//LOGICAL cr_escaped;
	TEXTRUNE c;
	//escape = 0;
	//cr_escaped = FALSE;
	while( ( ( n = (*msg_input) - msg ), ( n < msglen ) ) && ( ( c = GetUtfChar( msg_input ) ), ( status >= 0 ) ) )
	{
		(state->col)++;

		if( c == start_c ) {
			if( state->escape ) { ( *mOut++ ) = c; state->escape = FALSE; }
			else if( c == start_c ) {
				status = 1;
				break;
			} else ( *mOut++ ) = c; // other else is not valid close quote; just store as content.
		} else if( state->escape ) {
			if( state->stringOct ) {
				if( state->hex_char_len < 3 && c >= 48/*'0'*/ && c <= 57/*'9'*/ ) {
					state->hex_char *= 8;
					state->hex_char += c/*.codePointAt(0)*/ - 0x30;
					state->hex_char_len++;
					if( state->hex_char_len == 3 ) {
						mOut += ConvertToUTF8(mOut, state->hex_char);
						state->stringOct = FALSE;
						state->escape = FALSE;
						continue;
					}
					continue;
				} else {
					if( state->hex_char > 255 ) {
						lprintf(WIDE("(escaped character, parsing octal escape val=%d) fault while parsing; )") WIDE(" (near %*.*s[%c]%s)")
							, state->hex_char
							, (int)( ( n>3 ) ? 3 : n ), (int)( ( n>3 ) ? 3 : n )
							, ( *msg_input ) - ( ( n>3 ) ? 3 : n )
							, c
							, ( *msg_input ) + 1
						);// fault
						status = -1;
						break;
					}
					mOut += ConvertToUTF8(mOut, state->hex_char);
					state->stringOct = FALSE;
					state->escape = FALSE;
					continue;
				}

			} else if( state->unicodeWide ) {
				if( c == '}' ) {
					mOut += ConvertToUTF8(mOut, state->hex_char);
					state->unicodeWide = FALSE;
					state->stringUnicode = FALSE;
					state->escape = FALSE;
					continue;
				}
				state->hex_char *= 16;
				if( c >= '0' && c <= '9' )      state->hex_char += c - '0';
				else if( c >= 'A' && c <= 'F' ) state->hex_char += ( c - 'A' ) + 10;
				else if( c >= 'a' && c <= 'f' ) state->hex_char += ( c - 'a' ) + 10;
				else {
					lprintf(WIDE("(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
						, (int)( ( n > 3 ) ? 3 : n ), (int)( ( n > 3 ) ? 3 : n )
						, ( *msg_input ) - ( ( n > 3 ) ? 3 : n )
						, c
						, ( *msg_input ) + 1
					);// fault
					status = -1;
					state->unicodeWide = FALSE;
					state->escape = FALSE;
				}
				continue;
			} else if( state->stringHex || state->stringUnicode ) {
				if( state->hex_char_len == 0 && c == '{' ) {
					state->unicodeWide = TRUE;
					continue;
				}
				if( state->hex_char_len < 2 || ( state->stringUnicode && state->hex_char_len < 4 ) ) {
					state->hex_char *= 16;
					if( c >= '0' && c <= '9' )      state->hex_char += c - '0';
					else if( c >= 'A' && c <= 'F' ) state->hex_char += ( c - 'A' ) + 10;
					else if( c >= 'a' && c <= 'f' ) state->hex_char += ( c - 'a' ) + 10;
					else {
						lprintf(WIDE("(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
							, (int)( ( n>3 ) ? 3 : n ), (int)( ( n>3 ) ? 3 : n )
							, ( *msg_input ) - ( ( n>3 ) ? 3 : n )
							, c
							, ( *msg_input ) + 1
						);// fault
						status = -1;
						state->stringHex = FALSE;
						state->escape = FALSE;
						continue;
					}
				}
				state->hex_char_len++;
				if( state->stringUnicode ) {
					if( state->hex_char_len == 4 ) {
						mOut += ConvertToUTF8(mOut, state->hex_char);
						state->stringUnicode = FALSE;
						state->escape = FALSE;
					}
				} else if( state->hex_char_len == 2 ) {
					mOut += ConvertToUTF8(mOut, state->hex_char);
					state->stringHex = FALSE;
					state->escape = FALSE;
				}
				continue;
			}
			switch( c ) {
			case '\r':
				state->cr_escaped = TRUE;
				continue;
			case '\n':
				state->line++;
				state->col = 1;
				if( state->cr_escaped ) state->cr_escaped = FALSE;
				// fall through to clear escape status <CR><LF> support.
			case 2028: // LS (Line separator)
			case 2029: // PS (paragraph separate)
				continue;
			case '/':
			case '\\':
			case '\'':
			case '"':
			case '`':
				( *mOut++ ) = c;
				break;
			case 't':
				( *mOut++ ) = '\t';
				break;
			case 'b':
				( *mOut++ ) = '\b';
				break;
			case 'n':
				( *mOut++ ) = '\n';
				break;
			case 'r':
				( *mOut++ ) = '\r';
				break;
			case 'f':
				( *mOut++ ) = '\f';
				break;
			case '0': case '1': case '2': case '3':
				state->stringOct = TRUE;
				state->hex_char = c - 48;
				state->hex_char_len = 1;
				continue;
			case 'x':
				state->stringHex = TRUE;
				state->hex_char_len = 0;
				state->hex_char = 0;
				continue;
			case 'u':
				state->stringUnicode = TRUE;
				state->hex_char_len = 0;
				state->hex_char = 0;
				continue;
			default:
				if( state->cr_escaped ) {
					state->cr_escaped = FALSE;
					state->escape = FALSE;
					mOut += ConvertToUTF8(mOut, c);
				} else {
					lprintf(WIDE("(escaped character) fault while parsing; '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
						, (int)( ( n>3 ) ? 3 : n ), (int)( ( n>3 ) ? 3 : n )
						, ( *msg_input ) - ( ( n>3 ) ? 3 : n )
						, c
						, ( *msg_input ) + 1
					);// fault
					status = -1;
				}
				break;
			}
			state->escape = 0;
		} else if( c == '\\' ) {
			if( state->escape ) {
				(*mOut++) = '\\';
				state->escape = 0;
			}
			else state->escape = 1;
		}
		else
		{
			if( state->cr_escaped ) {
				state->cr_escaped = FALSE;
				if( c == '\n' ) {
					state->line++;
					state->col = 1;
					state->escape = FALSE;
					continue;
				}
			}
			mOut += ConvertToUTF8( mOut, c );
		}
	}
	if( status )
		(*mOut++) = 0;  // terminate the string.
	(*pmOut) = mOut;
	return status;
}


int json6_parse_add_data( struct json_parse_state *state
                            , const char * msg
                            , size_t msglen )
{
	/* I guess this is a good parser */
	TEXTRUNE c;
	PPARSE_BUFFER input;
	struct json_output_buffer* output;
	int string_status;
	int retval = 0;
	if( !state->status )
		return -1;

	if( msg && msglen ) {
		input = GetFromSet( PARSE_BUFFER, &jpsd.parseBuffers );
		input->pos = input->buf = msg;
		input->size = msglen;
		EnqueLinkNL( state->inBuffers, input );

		if( state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD ) {
			// have to extend the previous output buffer to include this one instead of allocating a split string.
			size_t offset;
			size_t offset2;
			output = (struct json_output_buffer*)DequeLinkNL( state->outQueue );
			//lprintf( "output from before is %p", output );
			offset = (output->pos - output->buf);
			offset2 = state->val.string ? (state->val.string - output->buf) : 0;
			AddLink( state->outValBuffers, output->buf );
			output->buf = NewArray( char, output->size + msglen + 1 );
			if( state->val.string ) {
				MemCpy( output->buf + offset2, state->val.string, offset - offset2 );
				state->val.string = output->buf + offset2;
			}
			output->size += msglen;
			//lprintf( "previous val:%s", state->val.string, state->val.string );
			output->pos = output->buf + offset;
			PrequeLink( state->outQueue, output );
		}
		else {
			output = (struct json_output_buffer*)GetFromSet( PARSE_BUFFER, &jpsd.parseBuffers );
			output->pos = output->buf = NewArray( char, msglen + 1 );
			output->size = msglen;
			EnqueLinkNL( state->outQueue, output );
		}
	}
	else {
		// zero length input buffer... terminate a number.
		if( state->gatheringNumber ) {
			//console.log( "Force completed.")
			output = (struct json_output_buffer*)DequeLinkNL( state->outQueue );
			output->pos[0] = 0;
			PushLink( state->outBuffers, output );
			state->gatheringNumber = FALSE;
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
			retval = 1;
		}
	}

	while( state->status && ( input = (PPARSE_BUFFER)DequeLinkNL( state->inBuffers ) ) ) {
		output = (struct json_output_buffer*)DequeLinkNL( state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;
		if( state->n > input->size ) DebugBreak();

		if( state->gatheringString ) {
			string_status = gatherString6( state, input->buf, &input->pos, input->size, &output->pos, state->gatheringStringFirstChar );
			if( string_status < 0 )
				state->status = FALSE;
			else if( string_status > 0 )
			{
				state->gatheringString = FALSE;
				state->n = input->pos - input->buf;
				if( state->n > input->size ) DebugBreak();
				state->val.stringLen = (output->pos - state->val.string)-1;
				if( state->status ) state->val.value_type = VALUE_STRING;
			}
			else {
				state->n = input->pos - input->buf;
				if( state->n > input->size ) DebugBreak();
			}
		}
		if( state->gatheringNumber ) {
			//lprintf( "continue gathering a string" );
			goto continueNumber;
		}

		//lprintf( "Completed at start?%d", state->completed );
		while( state->status && (state->n < input->size) && (c = GetUtfChar( &input->pos )) )
		{
			state->col++;
			state->n = input->pos - input->buf;
			if( state->n > input->size ) DebugBreak();

			if( state->comment ) {
				if( state->comment == 1 ) {
					if( c == '*' ) { state->comment = 3; continue; }
					if( c != '/' ) { 
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "Fault while parsing; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
						state->status = FALSE;
					}
					else state->comment = 2;
					continue;
				}
				if( state->comment == 2 ) {
					if( c == '\n' ) { state->comment = 0; continue; }
					else continue;
				}
				if( state->comment == 3 ) {
					if( c == '*' ) { state->comment = 4; continue; }
					else continue;
				}
				if( state->comment == 4 ) {
					if( c == '/' ) { state->comment = 0; continue; }
					else { if( c != '*' ) state->comment = 3; continue; }
				}
			}
			switch( c )
			{
			case '/':
				if( !state->comment ) state->comment = 1;
				break;
			case '{':
				if( state->word == WORD_POS_FIELD || state->word == WORD_POS_AFTER_FIELD || (state->parse_context == CONTEXT_OBJECT_FIELD && state->word == WORD_POS_RESET) ) {
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, "Fault while parsing; getting field name unexpected '%c' at %" _size_f " %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
					state->status = FALSE;
					break;
				}
				{
					struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
#ifdef _DEBUG_PARSING
					lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					old_context->name = state->val.name;
					old_context->nameLen = state->val.nameLen;
					state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
					if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
					else state->elements[0]->Cnt = 0;
					PushLink( state->context_stack, old_context );
					RESET_STATE_VAL();
					state->parse_context = CONTEXT_OBJECT_FIELD;
				}
				break;

			case '[':
				if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "Fault while parsing; while getting field name unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
					state->status = FALSE;
					break;
				}
				{
					struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
#ifdef _DEBUG_PARSING
					lprintf( "Begin a new array; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					old_context->name = state->val.name;
					old_context->nameLen = state->val.nameLen;
					state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
					if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
					else state->elements[0]->Cnt = 0;
					PushLink( state->context_stack, old_context );

					RESET_STATE_VAL();
					state->parse_context = CONTEXT_IN_ARRAY;
				}
				break;

			case ':':
				if( state->parse_context == CONTEXT_OBJECT_FIELD )
				{
					if( state->word != WORD_POS_RESET
						&& state->word != WORD_POS_FIELD
						&& state->word != WORD_POS_AFTER_FIELD ) {
						// allow starting a new word
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "unquoted keyword used as object field name:parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
						break;
					}
					else if( state->word == WORD_POS_FIELD ) {
						//state->val.stringLen = output->pos - state->val.string;
						//lprintf( "Set string length:%d", state->val.stringLen );
					}
					if( !( state->val.value_type == VALUE_STRING ) )
						(*output->pos++) = 0;
					state->word = WORD_POS_RESET;
					if( state->val.name ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "two names single value?" );
					}
					state->val.name = state->val.string;
					state->val.nameLen = ( output->pos - state->val.string ) - 1;
					state->val.string = NULL;
					state->val.stringLen = 0;
					state->parse_context = CONTEXT_OBJECT_FIELD_VALUE;
					state->val.value_type = VALUE_UNSET;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					if( state->parse_context == CONTEXT_IN_ARRAY )
						vtprintf( state->pvtError, WIDE( "(in array, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
					else
						vtprintf( state->pvtError, WIDE( "(outside any object, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
					state->status = FALSE;
				}
				break;
			case '}':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				// coming back after pushing an array or sub-object will reset the contxt to FIELD, so an end with a field should still push value.
				if( (state->parse_context == CONTEXT_OBJECT_FIELD) || (state->parse_context == CONTEXT_OBJECT_FIELD_VALUE) ) {
#ifdef _DEBUG_PARSING
					lprintf( "close object; empty object %d", state->val.value_type );
#endif
					//if( (state->parse_context == CONTEXT_OBJECT_FIELD_VALUE) )
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( state->elements, &state->val );
					}
					//RESET_STATE_VAL();
					state->val.value_type = VALUE_OBJECT;
					state->val.string = NULL;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( state->context_stack );
						//struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						DeleteFromSet( PARSE_CONTEXT, jpsd.parseContexts, old_context );
					}
					if( state->parse_context == CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "Fault while parsing; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
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
						AddDataItem( state->elements, &state->val );
					}
					state->val.value_type = VALUE_ARRAY;
					state->val.string = NULL;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;

					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( state->context_stack );
						//struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						DeleteFromSet( PARSE_CONTEXT, jpsd.parseContexts, old_context );
					}
					if( state->parse_context == CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context %d; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->parse_context, c, state->n, state->line, state->col );// fault
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
					if( state->val.value_type == VALUE_UNSET )
						state->val.value_type = VALUE_EMPTY; // in an array, elements after a comma should init as undefined...
																 // undefined allows [,,,] to be 4 values and [1,2,3,] to be 4 values with an undefined at end.
					if( state->val.value_type != VALUE_UNSET ) {
#ifdef _DEBUG_PARSING
						lprintf( "back in array; push item %d", state->val.value_type );
#endif
						AddDataItem( state->elements, &state->val );
						RESET_STATE_VAL();
					}
				}
				else if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE )
				{
					// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef _DEBUG_PARSING
					lprintf( "comma after field value, push field to object: %s", state->val.name );
#endif
					state->parse_context = CONTEXT_OBJECT_FIELD;
					if( state->val.value_type != VALUE_UNSET )
						AddDataItem( state->elements, &state->val );
					RESET_STATE_VAL();
				}
				else
				{
					state->status = FALSE;
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );// fault
				}
				break;

			default:
				if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
					//lprintf( "gathering object field:%c  %*.*s", c, output->pos-output->buf, output->pos - output->buf, output->buf );
					if( c < 0xFF ) {
						if( nonIdentifiers8[c] ) {
							// invalid start/continue
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );	// fault
							break;
						}
					}
					else {
						int n;
						for( n = 0; n < (sizeof( nonIdentifierBits ) / sizeof( nonIdentifierBits[0] )); n++ ) {
							if( c >= (TEXTRUNE)nonIdentifierBits[n].firstChar && c < (TEXTRUNE)nonIdentifierBits[n].lastChar &&
								(nonIdentifierBits[n].bits[(c - nonIdentifierBits[n].firstChar) / 24]
									& (1 << ((c - nonIdentifierBits[n].firstChar) % 24))) ) {
								state->status = FALSE;
								if( !state->pvtError ) state->pvtError = VarTextCreate();
								vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );	// fault
								break;
							}
						}
						if( c < (sizeof( nonIdentifierBits ) / sizeof( nonIdentifierBits[0] )) )
							break;
					}
					switch( c )
					{
					case '`':
						// this should be a special case that passes continuation to gatherString
						// but gatherString now just gathers all strings
					case '"':
					case '\'':
						state->val.string = output->pos;
						state->gatheringString = TRUE;
						state->gatheringStringFirstChar = c;
						string_status = gatherString6( state, input->buf, &input->pos, input->size, &output->pos, c );
						//lprintf( "string gather status:%d", string_status );
						if( string_status < 0 )
							state->status = FALSE;
						else if( string_status > 0 ) {
							state->gatheringString = FALSE;
							state->val.stringLen = (output->pos - state->val.string) - 1;
						}
						state->n = input->pos - input->buf;
						if( state->n > input->size ) DebugBreak();
						if( state->status ) {
							state->val.value_type = VALUE_STRING;
							//state->val.stringLen = (output->pos - state->val.string - 1);
							//lprintf( "Set string length:%d", state->val.stringLen );
						}
						break;
					case '\n':
						state->line++;
						state->col = 1;
						// fall through to normal space handling - just updated line/col position
					case ' ':
					case '\t':
					case '\r':
					case 0xFEFF: // ZWNBS is WS though
						if( state->word == WORD_POS_RESET || state->word == WORD_POS_AFTER_FIELD )
							break;
						else if( state->word == WORD_POS_FIELD ) {
							state->word = WORD_POS_AFTER_FIELD;
							//state->val.stringLen = output->pos - state->val.string;
							//lprintf( "Set string length:%d", state->val.stringLen );
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
						if( state->word == WORD_POS_AFTER_FIELD ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing; unquoted space in field name at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n, state->line, state->col );	// fault
							break;
						} else if( state->word == WORD_POS_RESET ) {
							state->word = WORD_POS_FIELD;
							state->val.string = output->pos;
						}
						if( c < 128 ) (*output->pos++) = c;
						else output->pos += ConvertToUTF8( output->pos, c );
						break; // default
					}
				}
				else switch( c )
				{
				case '`':
					// this should be a special case that passes continuation to gatherString
					// but gatherString now just gathers all strings
				case '"':
				case '\'':
					state->val.string = output->pos;
					state->gatheringString = TRUE;
					state->gatheringStringFirstChar = c;
					string_status = gatherString6( state, input->buf, &input->pos, input->size, &output->pos, c );
					//lprintf( "string gather status:%d", string_status );
					if( string_status < 0 )
						state->status = FALSE;
					else if( string_status > 0 ) {
						state->gatheringString = FALSE;
						state->val.stringLen = (output->pos - state->val.string) - 1;
					} else if( state->complete_at_end ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "End of string fail." );
						state->status = FALSE;
					}
					state->n = input->pos - input->buf;
					if( state->n > input->size ) DebugBreak();

					if( state->status ) {
						state->val.value_type = VALUE_STRING;
						state->word = WORD_POS_END;
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
					else if( state->word == WORD_POS_FIELD ) {
						state->word = WORD_POS_AFTER_FIELD;
					}
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n );	// fault
					}
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
								, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'r':
					if( state->word == WORD_POS_TRUE_1 ) state->word = WORD_POS_TRUE_2;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'd':
					if( state->word == WORD_POS_UNDEFINED_2 ) state->word = WORD_POS_UNDEFINED_3;
					else if( state->word == WORD_POS_UNDEFINED_8 ) { state->val.value_type = VALUE_UNDEFINED; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
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
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'a':
					if( state->word == WORD_POS_FALSE_1 ) state->word = WORD_POS_FALSE_2;
					else if( state->word == WORD_POS_NAN_1 ) state->word = WORD_POS_NAN_2;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 's':
					if( state->word == WORD_POS_FALSE_3 ) state->word = WORD_POS_FALSE_4;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'I':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_INFINITY_1;
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'N':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_NAN_1;
					else if( state->word == WORD_POS_NAN_2 ) { state->val.value_type = state->negative ? VALUE_NEG_NAN : VALUE_NAN; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}// fault
					break;
				case 'y':
					if( state->word == WORD_POS_INFINITY_7 ) { state->val.value_type = state->negative ? VALUE_NEG_INFINITY : VALUE_INFINITY; state->word = WORD_POS_END; }
					else {
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}// fault
					break;
					//
					//----------------------------------------------------------
				case '-':
					state->negative = !state->negative;
					break;

				default:
					if( (c >= '0' && c <= '9') || (c == '+') || (c == '.') )
					{
						LOGICAL fromDate;
						const char *_msg_input; // to unwind last character past number.
						// always reset this here....
						// keep it set to determine what sort of value is ready.
						if( !state->gatheringNumber ) {
							state->exponent = FALSE;
							state->exponent_sign = FALSE;
							state->exponent_digit = FALSE;
							fromDate = FALSE;
							state->fromHex = FALSE;
							state->val.float_result = (c == '.');
							state->val.string = output->pos;
							(*output->pos++) = c;  // terminate the string.
						}
						else
						{
						continueNumber:
							fromDate = state->numberFromDate;
						}
						while( (_msg_input = input->pos), ((state->n < input->size) && (c = GetUtfChar( &input->pos ))) )
						{
							//lprintf( "Number input:%c", c );
							state->col++;
							state->n = (input->pos - input->buf);
							if( state->n > input->size ) DebugBreak();
							// leading zeros should be forbidden.
							if( c == '_' )
								continue;

							if( c >= '0' && c <= '9' )
							{
								(*output->pos++) = c;
								if( state->exponent )
									state->exponent_digit = TRUE;
							}
#if 0
							// to be implemented
							else if( c == ':' || c == '-' || c == 'Z' || c == '+' ) {
								/* toISOString()
								var today = new Date('05 October 2011 14:48 UTC');
								console.log(today.toISOString());
								// Returns 2011-10-05T14:48:00.000Z
								*/
								(*output->pos++) = c;

							}
#endif
							else if( ( c == 'x' || c == 'b' || c =='o' || c == 'X' || c == 'B' || c == 'O')
							       && ( output->pos - state->val.string ) == 1
							       && state->val.string[0] == '0' ) {
								// hex conversion.
								if( !state->fromHex ) {
									state->fromHex = TRUE;
									(*output->pos++) = c | 0x20; // force lower case.
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
							}
							else if( (c == 'e') || (c == 'E') )
							{
								if( !state->exponent ) {
									state->val.float_result = 1;
									(*output->pos++) = c;
									state->exponent = TRUE;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
							}
							else if( c == '-' || c == '+' ) {
								if( !state->exponent ) {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
								else {
									if( !state->exponent_sign && !state->exponent_digit ) {
										(*output->pos++) = c;
										state->exponent_sign = 1;
									}
									else {
										state->status = FALSE;
										if( !state->pvtError ) state->pvtError = VarTextCreate();
										vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
										break;
									}
								}
							}
							else if( c == '.' )
							{
								if( !state->val.float_result && !state->fromHex ) {
									state->val.float_result = 1;
									(*output->pos++) = c;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
							}
							else
							{
								// in non streaming mode; these would be required to follow
								if( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xFEFF
									|| c == ',' || c == ']' || c == '}'  || c == ':' ) {
									//lprintf( "Non numeric character received; push the value we have" );
									(*output->pos) = 0;
									break;
								}
								else {
									if( state->parse_context == CONTEXT_UNKNOWN ) {
										(*output->pos) = 0;
										break;
									}
									else {
										state->status = FALSE;
										if( !state->pvtError ) state->pvtError = VarTextCreate();
										vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
										break;
									}
								}
							}
						}
						if( input ) {
							input->pos = _msg_input;
							state->n = (input->pos - input->buf);
							if( state->n > input->size ) DebugBreak();
						}
						//LogBinary( (uint8_t*)output->buf, output->size );
						if( input && (!state->complete_at_end) && state->n == input->size )
						{
							//lprintf( "completion mode is not end of string; and at end of string" );
							state->gatheringNumber = TRUE;
							state->numberFromDate = fromDate;
						}
						else
						{
							(*output->pos++) = 0;
							state->val.stringLen = (output->pos - state->val.string) - 1;
							state->gatheringNumber = FALSE;
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
						vtprintf( state->pvtError, WIDE( "fault parsing '%c' unexpected %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), c, state->n
							, (int)((state->n > 4) ? 3 : (state->n-1)), (int)((state->n > 4) ? 3 : (state->n-1))
							, input->buf + state->n - ((state->n > 3) ? 3 : state->n)
							, c
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
		if( input ) {
			if( state->n >= input->size ) {
				DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, input );
				if( state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD ) {
					//lprintf( "output is still incomplete? " );
					PrequeLink( state->outQueue, output );
					retval = 0;
				}
				else {
					PushLink( state->outBuffers, output );
					if( state->parse_context == CONTEXT_UNKNOWN
					  && ( state->val.value_type != VALUE_UNSET
					     || state->elements[0]->Cnt ) ) {
						if( state->word == WORD_POS_END ) {
							state->word = WORD_POS_RESET;
						}
						state->completed = TRUE;
						retval = 1;
					}
				}
				//lprintf( "Is complete already?%d", state->completed );
			}
			else {
				// put these back into the stack.
				//lprintf( "put buffers back into queues..." );
				PrequeLink( state->inBuffers, input );
				PrequeLink( state->outQueue, output );
				retval = 2;  // if returning buffers, then obviously there's more in this one.
			}
		}
		if( state->completed )
			break;
	} // while DequeInput

	if( !state->status ) {
		// some error condition; cannot resume parsing.
		return -1;
	}

	if( state->completed ) {
		if( state->val.value_type != VALUE_UNSET ) {
			AddDataItem( state->elements, &state->val );
			RESET_STATE_VAL();
		}
		state->completed = FALSE;
	}
	return retval;
}

PDATALIST json_parse_get_data( struct json_parse_state *state ) {
	PDATALIST *result = state->elements;
	state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
	if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
	else state->elements[0]->Cnt = 0;
	return result[0];
}

void json_parse_clear_state( struct json_parse_state *state ) {
	if( state ) {
		PPARSE_BUFFER buffer;
		while( buffer = (PPARSE_BUFFER)PopLink( state->outBuffers ) ) {
			Deallocate( const char *, buffer->buf );
			DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
		}
		while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
			DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
		while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
			Deallocate( const char*, buffer->buf );
			DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
		}
		DeleteFromSet( PLINKQUEUE, jpsd.linkQueues, state->inBuffers );
		//DeleteLinkQueue( &state->inBuffers );
		DeleteFromSet( PLINKQUEUE, jpsd.linkQueues, state->outQueue );
		//DeleteLinkQueue( &state->outQueue );
		DeleteFromSet( PLINKSTACK, jpsd.linkStacks, state->outBuffers );
		//DeleteLinkStack( &state->outBuffers );
		{
			char *buf;
			INDEX idx;
			LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
				Deallocate( char*, buf );
			}
			DeleteFromSet( PLIST, jpsd.listSet, state->outValBuffers );
			//DeleteList( &state->outValBuffers );
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
			PDATALIST *result = state->elements;
			state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
			if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
			else state->elements[0]->Cnt = 0;
			//state->elements = CreateDataList( sizeof( state->val ) );
			json6_dispose_message( result );
		}
	}
}

PTEXT json_parse_get_error( struct json_parse_state *state ) {
	if( !state )
		state = jpsd.last_parse_state;
	if( !state )
		return NULL;
	if( state->pvtError ) {
		PTEXT error = VarTextGet( state->pvtError );
		return error;
	}
	return NULL;
}

void json_parse_dispose_state( struct json_parse_state **ppState ) {
	struct json_parse_state *state = (*ppState);
	struct json_parse_context *old_context;
	PPARSE_BUFFER buffer;
	_json_dispose_message( state->elements );
	//DeleteDataList( &state->elements );
	while( buffer = (PPARSE_BUFFER)PopLink( state->outBuffers ) ) {
		Deallocate( const char *, buffer->buf );
		DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
	}
	{
		char *buf;
		INDEX idx;
		LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
			Deallocate( char*, buf );
		}
		DeleteFromSet( PLIST, jpsd.listSet, state->outValBuffers );
		//DeleteList( &state->outValBuffers );
	}
	while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
		DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
	while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
		Deallocate( const char*, buffer->buf );
		DeleteFromSet( PARSE_BUFFER, jpsd.parseBuffers, buffer );
	}
	DeleteFromSet( PLINKQUEUE, jpsd.linkQueues, state->inBuffers );
	//DeleteLinkQueue( &state->inBuffers );
	DeleteFromSet( PLINKQUEUE, jpsd.linkQueues, state->outQueue );
	//DeleteLinkQueue( &state->outQueue );
	DeleteFromSet( PLINKSTACK, jpsd.linkStacks, state->outBuffers );
	//DeleteLinkStack( &state->outBuffers );
	DeleteFromSet( PARSE_CONTEXT, jpsd.parseContexts, state->context );

	while( (old_context = (struct json_parse_context *)PopLink( state->context_stack )) ) {
		//lprintf( "warning unclosed contexts...." );
		DeleteFromSet( PARSE_CONTEXT, jpsd.parseContexts, old_context );
	}
	if( state->context_stack )
		DeleteFromSet( PLINKSTACK, jpsd.linkStacks, state->context_stack );
		//DeleteLinkStack( &state->context_stack );
	DeleteFromSet( PARSE_STATE, jpsd.parseStates, state );
	//Deallocate( struct json_parse_state *, state );
	(*ppState) = NULL;
}

LOGICAL json6_parse_message( const char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct json_parse_state *state = json_begin_parse();
	static struct json_parse_state *_state;
	state->complete_at_end = TRUE;
	int result = json6_parse_add_data( state, msg, msglen );
	if( _state ) json_parse_dispose_state( &_state );
	if( result > 0 ) {
		(*_msg_output) = json_parse_get_data( state );
		_state = state;
		//json6_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	jpsd.last_parse_state = state;
	_state = state;
	return FALSE;
}

void json6_dispose_message( PDATALIST *msg_data )
{
	json_dispose_message( msg_data );
	return;
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
			default:
				lprintf( "FAULT: UNEXPECTED VALUE TYPE RECOVERINT IDENT:%d", val->value_type );
				break;
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



#undef GetUtfChar

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



