#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>

#include "json.h"

#include "unicode_non_identifiers.h"

#define NUM_VALUE_NAMES  ((sizeof(value_type_names)/sizeof(value_type_names[0])))
const char *value_type_names[] = {
	"-unset-", "null", "true", "false"
	, "string", "number", "object"
	, "array", "NegNan", "Nan", "NegInf"
	, "Inf", "data", "EMPTY", "NeedsEval"
	, "variable", "function", "function Call"
	, "expression", "operator"
};

#define NUM_POS_NAMES  ((sizeof(word_pos_names)/sizeof(word_pos_names[0])))
const char *word_pos_names[] = {
	"WORD_POS_RESET" // not in a keyword
	,"WORD_POS_END"  // at end of a word, waiting for separator
	,"WORD_POS_TRUE_1"
	,"WORD_POS_TRUE_2"
	,"WORD_POS_TRUE_3"
	,"WORD_POS_TRUE_4"
	,"WORD_POS_FALSE_1" // 11
	,"WORD_POS_FALSE_2"
	,"WORD_POS_FALSE_3"
	,"WORD_POS_FALSE_4"
	,"WORD_POS_NULL_1" // 21  get u
	,"WORD_POS_NULL_2" //  get l
	,"WORD_POS_NULL_3" //  get l
	,"WORD_POS_UNDEFINED_1"  // 31 
	,"WORD_POS_UNDEFINED_2"
	,"WORD_POS_UNDEFINED_3"
	,"WORD_POS_UNDEFINED_4"
	,"WORD_POS_UNDEFINED_5"
	,"WORD_POS_UNDEFINED_6"
	,"WORD_POS_UNDEFINED_7"
	,"WORD_POS_UNDEFINED_8"
	//WORD_POS_UNDEFINED_9, // instead of stepping to this value here, go to RESET
	,"WORD_POS_NAN_1"
	,"WORD_POS_NAN_2"
	//WORD_POS_NAN_3,// instead of stepping to this value here, go to RESET
	,"WORD_POS_INFINITY_1"
	,"WORD_POS_INFINITY_2"
	,"WORD_POS_INFINITY_3"
	,"WORD_POS_INFINITY_4"
	,"WORD_POS_INFINITY_5"
	,"WORD_POS_INFINITY_6"
	,"WORD_POS_INFINITY_7"
	//WORD_POS_INFINITY_8,// instead of stepping to this value here, go to RESET
	,"WORD_POS_FIELD"
	,"WORD_POS_AFTER_FIELD"
	,"WORD_POS_DOT_OPERATOR"
	,"WORD_POS_PROPER_NAME"
	,"WORD_POS_AFTER_PROPER_NAME"
	,"WORD_POS_AFTER_GET"
	,"WORD_POS_AFTER_SET"

};
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

static int gatherString6v(struct json_parse_state *state, CTEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut, TEXTRUNE start_c
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

static FLAGSET( isOp, 128 );
static FLAGSET( isOp2[128], 128 );

static void InitOperatorSyms( void ) {
	static const char *ops = "=<>+-*/%^~!&|?:,.";
	//@#\$_
	static const char *op2[] = { /*=*/"=", /*<*/"<=", /*>*/">=", /*+*/"+=", /*-*/"-=", /***/"=", /*/*/"=/*", /*%*/"="
							   , /*^*/"=", /*~*/"=", /*!*/"=><&|", /*&*/"=&", /*|*/"=|", /*?*/NULL, /*:*/NULL, /*,*/NULL, /*.*/NULL };
	int n;
	int m;
	for( n = 0; ops[n]; n++ ) {
		SETFLAG( isOp, ops[n] );
		if( op2[n] ) for( m = 0; op2[n][m]; m++ ) SETFLAG( isOp2[ops[n]], op2[n][m] );
	}
}


PRELOAD( InitVESLOpSyms ) {
	InitOperatorSyms();
}

int vesl_parse_add_data( struct json_parse_state *state
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
			string_status = gatherString6v( state, input->buf, &input->pos, input->size, &output->pos, state->gatheringStringFirstChar );
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
			lprintf( "  --- Character %c(%d) val:%d(%s) context:%d word:%d(%s)  isOp:%d"
				, c, c, state->val.value_type, (state->val.value_type >= 0 && state->val.value_type < NUM_VALUE_NAMES)?value_type_names[state->val.value_type]:"???"
				, state->parse_context, state->word, word_pos_names[state->word]
				, (c<127)?TESTFLAG(isOp,c):0);
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
			case '(':
			case '{':
				if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE 
				  || state->parse_context == CONTEXT_IN_ARRAY 
				  || state->parse_context == CONTEXT_OBJECT_FIELD ) {
					struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
#ifdef _DEBUG_PARSING
					lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
					// looking for a field, and got another paren, 
					// is a unnamed expression within this oen.
					if( state->parse_context == CONTEXT_OBJECT_FIELD || state->parse_context == CONTEXT_OBJECT_FIELD_VALUE ) {
						if( state->word == WORD_POS_FIELD || state->word == WORD_POS_RESET ) {
							state->val.value_type = VALUE_EXPRESSION;
						}
						state->parse_context = CONTEXT_OBJECT_FIELD_VALUE;
					}
					if( state->val.value_type && state->val.string ) {
						// terminate the string.
						state->val.stringLen = (output->pos - state->val.string);
						(*output->pos++) = 0;
					}
					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					old_context->valState = state->val;
					state->elements = state->val._contains;
					if( !state->elements ) state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
					if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
					else state->elements[0]->Cnt = 0;
					lprintf( "Pushing pending thing, so this object is assicated under it as a list: %s", state->val.string );
					PushLink( state->context_stack, old_context );
					RESET_STATE_VAL();
					state->word = WORD_POS_RESET;
					state->parse_context = CONTEXT_OBJECT_FIELD;
					break;
				}

				if( state->word == WORD_POS_FIELD || state->word == WORD_POS_AFTER_FIELD || state->word == WORD_POS_DOT_OPERATOR ) {
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
					if( c == '(' && !state->val.value_type ) // it's going to be an expression list.
						state->val.value_type = VALUE_EXPRESSION;
					old_context->context = state->parse_context;
					old_context->elements = state->elements;
					old_context->valState = state->val;
					state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
					if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
					else state->elements[0]->Cnt = 0;
					PushLink( state->context_stack, old_context );
					RESET_STATE_VAL();
					state->parse_context = CONTEXT_OBJECT_FIELD;
				}
				break;

			case '[':
				if( state->parse_context == CONTEXT_OBJECT_FIELD || state->word == WORD_POS_DOT_OPERATOR ) {
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
					old_context->valState = state->val;
					state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
					if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
					else state->elements[0]->Cnt = 0;
					PushLink( state->context_stack, old_context );

					RESET_STATE_VAL();
					state->parse_context = CONTEXT_IN_ARRAY;
				}
				break;

			case ':':
			case '=':
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
			case ')':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				// coming back after pushing an array or sub-object will reset the contxt to FIELD, so an end with a field should still push value.
				if( (state->parse_context == CONTEXT_OBJECT_FIELD) 
				  || (state->parse_context == CONTEXT_OBJECT_FIELD_VALUE) ) {
#ifdef _DEBUG_PARSING
					lprintf( "close object; empty object %d", state->val.value_type );
#endif
					//if( (state->parse_context == CONTEXT_OBJECT_FIELD_VALUE) )
					if( state->val.value_type == VALUE_UNSET ) {
						state->val.value_type = VALUE_EMPTY;
					}
					if( state->val.value_type != VALUE_UNSET ) {
						lprintf( "Close Expression; push value:%s %p", value_type_names[state->val.value_type], state->elements );
						AddDataItem( state->elements, &state->val );
					}
					//RESET_STATE_VAL();
					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( state->context_stack );
						//struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.
						old_context->valState.contains = state->elements[0];
						old_context->valState._contains = state->elements;
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						state->val = old_context->valState;
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
						lprintf( "Close Array; push value:%s", value_type_names[state->val.value_type] );
						AddDataItem( state->elements, &state->val );
					}
					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( state->context_stack );
						//struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						old_context->valState.contains = state->elements[0];
						old_context->valState._contains = state->elements;
						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						state->val = old_context->valState;
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
			case ';':				
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
						lprintf( "Comma;semi; push value:%s", value_type_names[state->val.value_type] );
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
					if( state->val.value_type != VALUE_UNSET ) {
						lprintf( "comma;semi-2; push value:%s", value_type_names[state->val.value_type] );
						AddDataItem( state->elements, &state->val );
					}
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
				if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE
					&& c < 127 ) {
					if( c == ':' ) {
						// if not after a ? in expressions..... 
						if( state->word == WORD_POS_PROPER_NAME && !state->val.name ) {
							(*output->pos++) = 0;
							state->val.value_type = VALUE_UNSET;
							state->word = WORD_POS_RESET;
							state->val.name = state->val.string;
							state->val.nameLen = output->pos - state->val.name;
							state->val.string = NULL;
							continue;
						}
					}
					if( !state->operatorAccum ) {
						if( TESTFLAG( isOp, c ) ) {
							if( state->val.value_type ) {
								lprintf( "is an op; push value:%s %p", value_type_names[state->val.value_type], state->elements );
								AddDataItem( state->elements, &state->val );
								RESET_STATE_VAL();
								state->word = WORD_POS_RESET;
							}
							state->operatorAccum = c;
							continue;
						}
					}
					else {
						if( state->val.value_type ) {
							lprintf( "operator accumulatored; push value:%s %p", value_type_names[state->val.value_type], state->elements );
							AddDataItem( state->elements, &state->val );
							RESET_STATE_VAL();
							state->word = WORD_POS_RESET;
						}
						state->val.value_type = VALUE_OPERATOR;
						state->val.string = output->pos;
						state->val.stringLen = 1;
						(*output->pos++) = state->operatorAccum;
						if( TESTFLAG( isOp2[state->operatorAccum], c ) ) {
							if( state->operatorAccum == '/' && c == '/' ) {
								state->comment = 2;
								continue;
							}
							if( state->operatorAccum == '/' && c == '*' ) {
								state->comment = 3;
								continue;
							}
							state->val.stringLen = 2;
							(*output->pos++) = c;
						}
						// have to not set NUL character here, or else operator
						// tokens could overflow the output buffer.  Have to rely
						// instead on setting the stringLength;
						lprintf( "flush operator; push value:%s %p", value_type_names[state->val.value_type], state->elements );
						AddDataItem( state->elements, &state->val );
						RESET_STATE_VAL();
						state->word = WORD_POS_RESET;
						state->operatorAccum = 0;
					}
				}
				if( state->parse_context == CONTEXT_UNKNOWN ) {
					if( c == '=' ) {
						if( state->word = WORD_POS_AFTER_PROPER_NAME ){
							state->val.value_type = VALUE_VARIABLE;
						}
					}
				}
				if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
					//lprintf( "gathering object field:%c  %*.*s", c, output->pos-output->buf, output->pos - output->buf, output->buf );
					if( c < 0xFF ) {
						if( c == '@' ) {
							if( state->word == WORD_POS_FIELD ) {
								(*output->pos++) = c;
								break;
							}
						}
						if( c == '.' ) {
							if( state->word == WORD_POS_RESET ) {
								state->word = WORD_POS_DOT_OPERATOR;
								state->val.string = output->pos;
							}
							else if( state->word == WORD_POS_FIELD )
								state->parse_context = CONTEXT_OBJECT_FIELD_VALUE;
							(*output->pos++) = c;
							break;
						}
						if( nonIdentifiers8[c] ) {
							if( state->operatorAccum ) {
								break;
							}
							// invalid start/continue
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );	// fault
							break;
						}
					}
					else {
						int n;
						for( n = 0; n < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )); n++ ) {
							if( c == nonIdentifiers[n] ) {
								state->status = FALSE;
								if( !state->pvtError ) state->pvtError = VarTextCreate();
								vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );	// fault
								break;
							}
						}
						if( c < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )) )
							break;
					}
					switch( c )
					{
					case '`':
						// this should be a special case that passes continuation to gatherString
						// but gatherString now just gathers all strings
					case '"':
					case '\'':
						if( !state->val.string )
							state->val.string = output->pos;
						state->gatheringString = TRUE;
						state->gatheringStringFirstChar = c;
						string_status = gatherString6v( state, input->buf, &input->pos, input->size, &output->pos, c );
						//lprintf( "string gather status:%d", string_status );
						if( string_status < 0 )
							state->status = FALSE;
						else if( string_status > 0 ) {
							state->gatheringString = FALSE;
							state->val.stringLen = (output->pos - state->val.string) - 1;
						}
						state->n = input->pos - input->buf;
						if( state->n > input->size ) DebugBreak();
						if( state->parse_context == CONTEXT_OBJECT_FIELD ) {
							if( state->word == WORD_POS_DOT_OPERATOR ) {
								state->word = WORD_POS_FIELD;
								break;
							}
						}
						if( state->status ) {
							state->val.value_type = VALUE_STRING;
							//state->val.stringLen = (output->pos - state->val.string - 1);
							//lprintf( "Set string length:%d", state->val.stringLen );
						}
						break;
					case ' ':
						state->weakSpace = TRUE;
						if( 0 ) {
					case '\n':
						state->line++;
						state->col = 1;
						// fall through to normal space handling - just updated line/col position
					case '\t':
					case '\r':
						state->weakSpace = FALSE;
						}
					case 0xFEFF: // ZWNBS is WS though
						if( !state->weakSpace && state->parse_context == CONTEXT_OBJECT_FIELD_VALUE
						  && ( state->word == WORD_POS_RESET ) && state->val.value_type )
						{
							// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef _DEBUG_PARSING
							lprintf( "space after field value, push field to object: %s", state->val.name );
#endif
							state->parse_context = CONTEXT_OBJECT_FIELD;
							if( state->val.value_type != VALUE_UNSET ) {
								lprintf( "hard space after a value type...; push value:%s %p", value_type_names[state->val.value_type], state->elements );
								AddDataItem( state->elements, &state->val );
							}
							RESET_STATE_VAL();
						}
						if( state->weakSpace 
							&& state->parse_context == CONTEXT_OBJECT_FIELD_VALUE
							&& (state->word == WORD_POS_RESET) 
							&& state->val.value_type )
						{
							// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef _DEBUG_PARSING
							lprintf( "space after field value, push field to object: %s", state->val.name );
#endif
							//state->parse_context = CONTEXT_OBJECT_FIELD;
							if( state->val.value_type != VALUE_UNSET ) {
								lprintf( "space after a value in field value; push value:%s %p", value_type_names[state->val.value_type], state->elements );
								AddDataItem( state->elements, &state->val );
							}
							RESET_STATE_VAL();
						}
						if( state->word == WORD_POS_RESET || state->word == WORD_POS_AFTER_FIELD )
							break;
						else if( state->word == WORD_POS_FIELD ) {
							if( strncmp( state->val.string, "get", 3 ) == 0 ) {
								state->word = WORD_POS_AFTER_GET;
								
							} else if( strncmp( state->val.string, "set", 3 ) == 0 ) {
								state->word = WORD_POS_AFTER_SET;
							} else
								state->word = WORD_POS_AFTER_FIELD;
							//state->val.stringLen = output->pos - state->val.string;
							//lprintf( "Set string length:%d", state->val.stringLen );
							break;
						}
						if( state->val.value_type ) {
							if( state->val.string )
								state->val.stringLen = (output->pos - state->val.string);
							lprintf( "space also after a val; push value:%s", value_type_names[state->val.value_type] );
							AddDataItem( state->elements, &state->val );
							RESET_STATE_VAL();
						}
						//state->status = FALSE;
						//if( !state->pvtError ) state->pvtError = VarTextCreate();
						//vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n, state->line, state->col );	// fault
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
						} else if( state->word == WORD_POS_DOT_OPERATOR ) {
							if( state->parse_context == CONTEXT_OBJECT_FIELD )
								state->word = WORD_POS_FIELD;
							else if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE )
								state->word = WORD_POS_AFTER_FIELD;
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
					string_status = gatherString6v( state, input->buf, &input->pos, input->size, &output->pos, c );
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
				case ' ':
					state->weakSpace = TRUE;
					if(0) {
				case '\n':
					state->line++;
					state->col = 1;
					// FALLTHROUGH
				case '\t':
				case '\r':
				case 0xFEFF:
					state->weakSpace = FALSE;
					}
					if( state->word == WORD_POS_END ) {
						state->word = WORD_POS_RESET;
						if( state->parse_context == CONTEXT_UNKNOWN ) {
							state->completed = TRUE;
						}
						break;
					}
					if( state->word == WORD_POS_RESET ) {
						if( !state->weakSpace && state->parse_context == CONTEXT_OBJECT_FIELD_VALUE && state->val.value_type )
						{
							// after an array value, it will have returned to OBJECT_FIELD anyway
#ifdef _DEBUG_PARSING
							lprintf( "comma after field value, push field to object: %s", state->val.name );
#endif
							state->parse_context = CONTEXT_OBJECT_FIELD;
							if( state->val.value_type != VALUE_UNSET ) {
								lprintf( "space not in field, but after field?; push value:%s", value_type_names[state->val.value_type] );
								AddDataItem( state->elements, &state->val );
							}
							RESET_STATE_VAL();
						}
						break;
					}
					else if( state->word == WORD_POS_FIELD ) {
						state->word = WORD_POS_AFTER_FIELD;
					}
					else {
						if( state->val.value_type ) {
							if( state->val.string )
								state->val.stringLen = (output->pos - state->val.string);
							lprintf( "space after field; push value:%s", value_type_names[state->val.value_type] );
							AddDataItem( state->elements, &state->val );
							RESET_STATE_VAL();
						}
						//state->status = FALSE;
						//if( !state->pvtError ) state->pvtError = VarTextCreate();
						//vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n );	// fault
					}
					// skip whitespace
					//n++;
					//lprintf( "whitespace skip..." );
					break;
					//----------------------------------------------------------
					//  catch characters for true/false/null/undefined which are values outside of quotes
					//  (get/set/....)
					//----------------------------------------------------------

				default:
					if( c == '.' ) {
						if( state->parse_context == CONTEXT_OBJECT_FIELD_VALUE
						  ||( state->parse_context == CONTEXT_OBJECT_FIELD 
						    && state->word == WORD_POS_FIELD ) ) {
							(*output->pos++) = c;
							if( !state->val.value_type )
								state->val.value_type = VALUE_OPERATOR;
						}
						break;
					}
					if( state->word == WORD_POS_RESET ) {
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
								else if( (c == 'x' || c == 'b' || c == 'o' || c == 'X' || c == 'B' || c == 'O')
									&& (output->pos - output->buf) == 1
									&& output->buf[0] == '0' ) {
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
										|| c == ',' || c == ']' || c == '}' || c == ':' ) {
										//lprintf( "Non numeric character received; push the value we have" );
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
							break;
						}
					}
					if( state->word == WORD_POS_RESET ) {
						if( state->val.value_type ) {
							if( state->val.string )
								state->val.stringLen = (output->pos - state->val.string);
							lprintf( "Start new proper name; push value:%s", value_type_names[state->val.value_type] );
							AddDataItem( state->elements, &state->val );
							RESET_STATE_VAL();
						}
						state->word = WORD_POS_PROPER_NAME;
						state->val.value_type = VALUE_OPERATOR;
						state->val.string = output->pos;
					}
					if( c < 128 ) (*output->pos++) = c;
					else output->pos += ConvertToUTF8( output->pos, c );
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
			lprintf( "Final completed, push expression; push value:%s", value_type_names[state->val.value_type] );
			AddDataItem( state->elements, &state->val );
			RESET_STATE_VAL();
		}
		state->completed = FALSE;
	}
	return retval;
}

static void vesl_dump_parse_level( PDATALIST pdl, int level ) {
	struct json_value_container *val;
	INDEX idx;
	int n;
	DATA_FORALL( pdl, idx, struct json_value_container *, val ) {
		for( n = 0; n < level; n++ )
			printf( "\t" );
		if( val->value_type < 0 )
			printf( "undefined" );
		else if( val->value_type < NUM_VALUE_NAMES )
			printf( "%s:", value_type_names[val->value_type] );
		else
			printf( "%d:", val->value_type );
		if( val->name )
			printf( "NAME(%s)", val->name );
		if( val->string )
			printf( "STRING(%*.*s)", val->stringLen, val->stringLen, val->string );
		printf( "\n" );
		if( val->contains )
			vesl_dump_parse_level( val->contains, level + 1 );
	}

}

static void vesl_dump_parse( PDATALIST pdl ) {
	vesl_dump_parse_level( pdl, 0 );
}

LOGICAL vesl_parse_message( const char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct json_parse_state *state = json_begin_parse();
	static struct json_parse_state *_state;
	state->complete_at_end = TRUE;
	int result = vesl_parse_add_data( state, msg, msglen );
	if( _state ) json_parse_dispose_state( &_state );
	if( result > 0 ) {
		(*_msg_output) = json_parse_get_data( state );
		vesl_dump_parse( (*_msg_output) );
		_state = state;
		//vesl_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	jpsd.last_parse_state = state;
	_state = state;
	return FALSE;
}

void vesl_dispose_decoded_message( struct vesl_context_object *format
                                 , POINTER msg_data )
{
	// a complex format might have sub-parts .... but for now we'll assume simple flat structures
	//Release( msg_data );
}

void vesl_dispose_message( PDATALIST *msg_data )
{
	json_dispose_message( msg_data );
	return;
}


#undef GetUtfChar

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



