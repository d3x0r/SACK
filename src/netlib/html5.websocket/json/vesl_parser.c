#include <stdhdrs.h>
#include <deadstart.h>
#define VESL_EMITTER_SOURCE
#define VESL_PARSER_MAIN_SOURCE
#include <vesl_emitter.h>

#include "vesl.h"

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
SACK_NAMESPACE namespace network { namespace vesl {
#endif

#define _2char(result,from) (((*from) += 2),( ( result & 0x1F ) << 6 ) | ( ( result & 0x3f00 )>>8))
#define _zero(result,from)  ((*from)++,0) 
#define _3char(result,from) ( ((*from) += 3),( ( ( result & 0xF ) << 12 ) | ( ( result & 0x3F00 ) >> 2 ) | ( ( result & 0x3f0000 ) >> 16 )) )

#define _4char(result,from)  ( ((*from) += 4), ( ( ( result & 0x7 ) << 18 )     \
                               | ( ( result & 0x3F00 ) << 4 )          \
                               | ( ( result & 0x3f0000 ) >> 10 )       \
                               | ( ( result & 0x3f000000 ) >> 24 ) ) )

#define __GetUtfChar( result, from )           ((result = ((TEXTRUNE*)*from)[0]),     \
        ( ( !(result & 0xFF) )           \
          ?_zero(result,from)            \
                                         \
      :( ( result & 0x80 )               \
        ?( ( result & 0xE0 ) == 0xC0 )   \
            ?( ( ( result & 0xC000 ) == 0x8000 ) ?_2char(result,from) : _zero(result,from)  )    \
            :( ( ( result & 0xF0 ) == 0xE0 )                           \
                ?( ( ( ( result & 0xC000 ) == 0x8000 ) && ( ( result & 0xC00000 ) == 0x800000 ) ) ? _3char(result,from) : _zero(result,from)  )   \
                :( ( ( result & 0xF8 ) == 0xF0 )   \
                   ? ( ( ( ( result & 0xC000 ) == 0x8000 ) && ( ( result & 0xC00000 ) == 0x800000 ) && ( ( result & 0xC0000000 ) == 0x80000000 ) )  \
                       ?_4char(result,from):_zero(result,from) )                                                                                    \
                    :( ( ( result & 0xC0 ) == 0x80 )                                                                                                \
                    ?_zero(result,from)                                                                                                             \
                    : ( (*from)++, (result & 0x7F) ) ) ) )                                                                                       \
        : ( (*from)++, (result & 0x7F) ) ) ) )

#define GetUtfChar(x) __GetUtfChar(c,x)


static void vesl_state_init( struct vesl_parse_state *state )
{
	PPDATALIST ppElements;
	PPLIST ppList;
	PPLINKQUEUE ppQueue;
	PPLINKSTACK ppStack;

	ppElements = GetFromSet( PDATALIST, &vpsd.dataLists );
	if( !ppElements[0] ) ppElements[0] = CreateDataList( sizeof( state->val ) );
	state->elements = ppElements;
	state->elements[0]->Cnt = 0;

	ppStack = GetFromSet( PLINKSTACK, &vpsd.linkStacks );
	if( !ppStack[0] ) ppStack[0] = CreateLinkStack();
	state->outBuffers = ppStack;
	state->outBuffers[0]->Top = 0;

	ppQueue = GetFromSet( PLINKQUEUE, &vpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->inBuffers = ppQueue;// CreateLinkQueue();
	state->inBuffers[0]->Top = state->inBuffers[0]->Bottom = 0;

	ppQueue = GetFromSet( PLINKQUEUE, &vpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->outQueue = ppQueue;// CreateLinkQueue();
	state->outQueue[0]->Top = state->outQueue[0]->Bottom = 0;

	ppList = GetFromSet( PLIST, &vpsd.listSet );
	if( ppList[0] ) ppList[0]->Cnt = 0;
	state->outValBuffers = ppList;


	state->line = 1;
	state->col = 1;
	state->n = 0; // character index;
	state->word = WORD_POS_RESET;
	state->status = TRUE;
	state->negative = FALSE;

	state->context_stack = GetFromSet( PLINKSTACK, &vpsd.linkStacks );// NULL;
	if( state->context_stack[0] ) state->context_stack[0]->Top = 0;
	//state->first_token = TRUE;
	//state->context = GetFromSet( PARSE_CONTEXT, &vpsd.parseContexts );
	state->parse_context = CONTEXT_UNKNOWN;
	state->comment = 0;
	state->completed = FALSE;
	//state->mOut = msg;// = NewArray( char, msglen );
	//state->msg_input = (char const *)msg;

	state->val.value_type = VESL_VALUE_UNSET;
	//state->val.contains = NULL;
	state->val._contains = NULL;
	state->val.string = NULL;

	state->complete_at_end = FALSE;
	state->gatheringString = FALSE;
	state->gatheringNumber = FALSE;

	state->pvtError = NULL;
}

static void vesl_start_container( struct vesl_parse_state *state ) {
	{
		struct vesl_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &vpsd.parseContexts );
#ifdef _DEBUG_PARSING
		lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif				
		old_context->context = state->parse_context;
		old_context->elements = state->elements;
		old_context->valState = state->val;
		state->elements = state->val._contains;
		if( !state->elements ) old_context->valState._contains = state->elements = GetFromSet( PDATALIST, &vpsd.dataLists );// CreateDataList( sizeof( state->val ) );
		if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
		if( !state->root ) state->root = state->elements[0];
		//else state->elements[0]->Cnt = 0;
		lprintf( "Pushing pending thing, so this object is assicated under it as a list: %s", state->val.string );
		PushLink( state->context_stack, old_context );
		state->word = WORD_POS_RESET;
		RESET_STATE_VAL();
	}
}

static void commitPending( struct vesl_parse_state *state ) {
	if( state->val.value_type ) {
		if( state->val.string )
			state->val.stringLen = state->output->pos - state->val.string;
		AddDataItem( state->elements, &state->val );
		RESET_STATE_VAL();
		state->word = WORD_POS_RESET;
		state->operatorAccum = 0;
	}
}

static void vesl_start_expression( struct vesl_parse_state *state ) {
	commitPending( state );
	state->val.value_type = VESL_VALUE_EXPRESSION;
	//AddDataItem( state->elements, &state->val );
	//RESET_STATE_VAL();
	vesl_start_container( state );
	state->parse_context = CONTEXT_OBJECT_FIELD;
}

static void vesl_start_array( struct vesl_parse_state *state ) {
	commitPending( state );

	state->val.value_type = VESL_VALUE_ARRAY;
	//AddDataItem( state->elements, &state->val );
	//RESET_STATE_VAL();
	vesl_start_container( state );
	state->parse_context = CONTEXT_IN_ARRAY;
}



static void vesl_close_expression_array( struct vesl_parse_state *state ) {
	commitPending( state );

	{
		struct vesl_parse_context *old_context = (struct vesl_parse_context *)PopLink( state->context_stack );
		//struct vesl_value_container *oldVal = (struct vesl_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
		//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

		state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
		state->elements = old_context->elements;
		state->val = old_context->valState;
		DeleteFromSet( PARSE_CONTEXT, vpsd.parseContexts, old_context );
	}
}

static void vesl_dump_parse_level( PDATALIST *pdl, int level ) {
	struct vesl_value_container *val;
	INDEX idx;
	int n;
	DATA_FORALL( pdl[0], idx, struct vesl_value_container *, val ) {
		for( n = 0; n < level; n++ )
			printf( "\t" );
		if( val->value_type < 0 )
			printf( "undefined" );
		else if( val->value_type < NUM_VALUE_NAMES )
			printf( "%s:", value_type_names[val->value_type] );
		else
			printf( "%d:", val->value_type );
		if( val->string )
			printf( "STRING(%*.*s)", (int)val->stringLen, (int)val->stringLen, val->string );
		printf( "\n" );
		if( val->_contains )
			vesl_dump_parse_level( val->_contains, level + 1 );
	}

}

static void vesl_dump_parse( PDATALIST pdl ) {
	vesl_dump_parse_level( &pdl, 0 );
}


static int gatherString6v(struct vesl_parse_state *state, CTEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut
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

		if( c == state->gatheringStringFirstChar ) {
			if( state->escape ) { ( *mOut++ ) = c; state->escape = FALSE; }
			else if( c == state->gatheringStringFirstChar ) {
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
			if( c < 127 )
				(*mOut++) = (char)c;
			else
				mOut += ConvertToUTF8( mOut, c );
		}
	}
	// this CAN nul terminate; since the end is a quote, which is lost; but let's be conservative.

	//if( status )
	//	(*mOut++) = 0;  // terminate the string.
	(*pmOut) = mOut;
	return status;
}

static int gatherIdentifier( struct vesl_parse_state *state, CTEXTSTR msg
	, CTEXTSTR *msg_input, size_t msglen, TEXTRUNE *unused
	, TEXTSTR *pmOut 
) {
	char *mOut = (*pmOut);
	// collect an identifier
	int status = 0;
	size_t n;
	TEXTRUNE c = (*unused);
	do
	{
		(state->col)++;
		if( c < 0xFF ) {
			if( nonIdentifiers8[c] ) {
				status = 1;
				(*unused) = c;
				break;
			}
		} else {
			int n;
			for( n = 0; n < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )); n++ ) {
				if( c == nonIdentifiers[n] ) break;
			}
			if( c < (sizeof( nonIdentifiers ) / sizeof( nonIdentifiers[0] )) ) {
				status = 1;
				(*unused) = c;
				break;
			}
		}
		if( state->val.value_type == VESL_VALUE_UNSET ) {
			state->val.value_type = VESL_VALUE_VARIABLE;
			state->val.string = mOut;
		}
		if( c < 127 )
			(*mOut++) = (char)c;
		else
			mOut += ConvertToUTF8( mOut, c );
	}
	while( ((n = (*msg_input) - msg), (n < msglen)) && ((c = GetUtfChar( msg_input )), (status >= 0)) );
	if( (*pmOut) != mOut ) {
		status |= 2;
		(*pmOut) = mOut;
	}
	return status;
}



static FLAGSET( isOp, 128 );
static FLAGSET( isOp2[128], 128 );

static void InitOperatorSyms( void ) {
	static const char *ops = "=<>+-*/%^~!&|?:.";
	//@#\$_
	static const char *op2[] = { /*=*/"=", /*<*/"<=", /*>*/">=", /*+*/"+=", /*-*/"-=", /***/"=", /*/*/"=/*", /*%*/"="
							   , /*^*/"=", /*~*/"=", /*!*/"=><&|", /*&*/"=&", /*|*/"=|", /*?*/NULL, /*:*/NULL, /*.*/NULL };
	int n;
	int m;
	for( n = 0; ops[n]; n++ ) {
		SETFLAG( isOp, ops[n] );
		if( op2[n] ) for( m = 0; op2[n][m]; m++ ) SETFLAG( isOp2[ops[n]], op2[n][m] );
	}
}

static void setOperator( struct vesl_parse_state *state, TEXTRUNE c ) {

}


PRELOAD( InitVESLOpSyms ) {
	InitOperatorSyms();
}


int vesl_parse_add_data( struct vesl_parse_state *state
                            , const char * msg
                            , size_t msglen )
{
	/* I guess this is a good parser */
	TEXTRUNE c;
	PPARSE_BUFFER input;
	struct vesl_output_buffer* output;
	int string_status;
	int retval = 0;
	if( !state->status )
		return -1;

	if( msg && msglen ) {
		input = GetFromSet( PARSE_BUFFER, &vpsd.parseBuffers );
		input->pos = input->buf = msg;
		input->size = msglen;
		EnqueLinkNL( state->inBuffers, input );

		output = (struct vesl_output_buffer*)DequeLinkNL( state->outQueue );
		if( output && (state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD) ) {
			// have to extend the previous output buffer to include this one instead of allocating a split string.
			size_t offset;
			size_t offset2;
			output = (struct vesl_output_buffer*)DequeLinkNL( state->outQueue );
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
			if( output )
				PrequeLink( state->outQueue, output );
			output = (struct vesl_output_buffer*)GetFromSet( PARSE_BUFFER, &vpsd.parseBuffers );
			output->pos = output->buf = NewArray( char, msglen + 1 );
			output->size = msglen;
			EnqueLinkNL( state->outQueue, output );
		}
	}
	else {
		// zero length input buffer... terminate a number.
		if( state->gatheringNumber ) {
			//console.log( "Force completed.")
			output = (struct vesl_output_buffer*)DequeLinkNL( state->outQueue );
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
			state->val.value_type = VESL_VALUE_NUMBER;
			if( state->parse_context == CONTEXT_UNKNOWN ) {
				state->completed = TRUE;
			}
			retval = 1;
		}
	}

	while( state->status && ( input = (PPARSE_BUFFER)DequeLinkNL( state->inBuffers ) ) ) {
		state->output = output = (struct vesl_output_buffer*)DequeLinkNL( state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;
		if( state->n > input->size ) DebugBreak();

		if( state->gatheringString ) {
			string_status = gatherString6v( state, input->buf, &input->pos, input->size, &output->pos );
			if( string_status < 0 )
				state->status = FALSE;
			else if( string_status > 0 )
			{
				state->gatheringString = FALSE;
				state->n = input->pos - input->buf;
				if( state->n > input->size ) DebugBreak();
				state->val.stringLen = (output->pos - state->val.string)-1;
				if( state->status ) state->val.value_type = VESL_VALUE_STRING;
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
		retry:
			state->col++;
			state->n = input->pos - input->buf;
			if( state->n > input->size ) DebugBreak();

			lprintf( "  --- Character %c(%d) val:%d(%s) context:%d word:%d(%s)  isOp:%d"
				, c<32?'.':c, c, state->val.value_type, (state->val.value_type >= 0 && state->val.value_type < NUM_VALUE_NAMES)?value_type_names[state->val.value_type]:"???"
				, state->parse_context, state->word, word_pos_names[state->word]
				, (c<127)?TESTFLAG(isOp,c):0);
			vesl_dump_parse( state->root );
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
			case '(':
			case '{':
				vesl_start_expression( state );
				break;
			case '[':
				vesl_start_array( state );
				break;
			case '}':
			case ')':
				vesl_close_expression_array( state );
				break;
			case ']':
				vesl_close_expression_array( state );
				break;

			default:
				if( c == ' ' || c == 0xFEFF ) {
					state->weakSpace = TRUE;
					continue;
				}
				if( c == '\n' ) {
					state->line++;
					state->col = 1;
					state->weakSpace = FALSE;
					continue;
				}
				if( c == ',' || c == ';' || c == '\t' || c == '\r' ) {
					state->weakSpace = FALSE;
					continue;
				}

				if( c < 0xff ) {

					if( (c >= '0' && c <= '9') )
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
							// to be implemented (date parsing?)
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
								//if( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xFEFF
								//	|| c == ',' || c == ';' || c == ']' || c == '}' || c == ':' ) {
								//lprintf( "Non numeric character received; push the value we have" );

								// operator may be following, which is not lost.
								//(*output->pos) = 0;
								break;
								//}
								//else {
								//	state->status = FALSE;
								//	if( !state->pvtError ) state->pvtError = VarTextCreate();
								//	vtprintf( state->pvtError, WIDE( "fault white parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
								//	break;
								//}
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
							state->val.stringLen = (output->pos - state->val.string);
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
							state->val.value_type = VESL_VALUE_NUMBER;
							if( state->parse_context == CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
						}
						continue;
					}

					if( !state->operatorAccum ) {
						if( TESTFLAG( isOp, c ) ) {
							commitPending( state );
							state->operatorAccum = c;
							continue; // is an operator... next!
						}
					}
					else {
						state->val.value_type = VESL_VALUE_OPERATOR;
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
							commitPending( state );
							break;
						}
						lprintf( "flush operator; push value:%s %p", value_type_names[state->val.value_type], state->elements );
						commitPending( state );
						//goto retry;
					}
				}
				if( c == '`' || c == '"' || c == '\'' ) {
					if( !state->val.string )
						state->val.string = output->pos;
					state->gatheringString = TRUE;
					state->gatheringStringFirstChar = c;
					string_status = gatherString6v( state, input->buf, &input->pos, input->size, &output->pos );
					//lprintf( "string gather status:%d", string_status );
					if( string_status < 0 )
						state->status = FALSE;
					else if( string_status > 0 ) {
						state->gatheringString = FALSE;
						state->val.stringLen = (output->pos - state->val.string);
					}
					state->n = input->pos - input->buf;
					if( state->n > input->size ) DebugBreak();

					if( state->status ) {
						state->val.value_type = VESL_VALUE_STRING;
						commitPending( state );
					}
					continue;
				}
				switch( gatherIdentifier( state, input->buf, &input->pos, input->size, &c, &output->pos ) ) {
				case 0: // no data.  C will not have data.
					break;
				case 1: // c has a character
					goto retry;
				case 2: // length, but ran out of data, no next character.
					commitPending( state );
					break;
				case 3: // length, but ran out of data, no next character.
					commitPending( state );
					goto retry;
				}
			}
		}

		//lprintf( "at end... %d %d comp:%d", state->n, input->size, state->completed );
		if( input ) {
			if( state->n >= input->size ) {
				DeleteFromSet( PARSE_BUFFER, vpsd.parseBuffers, input );
				if( state->gatheringString || state->gatheringNumber || state->word != WORD_POS_RESET ) {
					//lprintf( "output is still incomplete? " );
					PrequeLink( state->outQueue, output );
					retval = 0;
				}
				else {
					PushLink( state->outBuffers, output );
					if( state->parse_context == CONTEXT_OBJECT_FIELD
					  && ( state->val.value_type != VESL_VALUE_UNSET
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

	state->root = NULL;

	if( state->completed ) {
		if( state->val.value_type != VESL_VALUE_UNSET ) {
			lprintf( "Final completed, push expression; push value:%s", value_type_names[state->val.value_type] );
			AddDataItem( state->elements, &state->val );
			RESET_STATE_VAL();
		}
		state->completed = FALSE;
	}
	return retval;
}

void vesl_preinit_state( struct vesl_parse_state *state ) {
	struct vesl_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &vpsd.parseContexts );
#ifdef _DEBUG_PARSING
	lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", val.value_type );
#endif
	// it's going to be an expression list.
	state->val.value_type = VESL_VALUE_EXPRESSION;
	old_context->context = state->parse_context;
	old_context->elements = state->elements;
	old_context->valState = state->val;

	state->elements = state->val._contains;
	if( !state->elements ) state->elements = GetFromSet( PDATALIST, &vpsd.dataLists );
	if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
	//if( !state->root ) state->root = state->elements[0];
	//else state->elements[0]->Cnt = 0;
	PushLink( state->context_stack, old_context );
	RESET_STATE_VAL();
	state->parse_context = CONTEXT_OBJECT_FIELD;
}

/* I guess this is a good parser */
static struct vesl_parse_state * vesl_begin_parse( void )
{
	struct vesl_parse_state *state = GetFromSet( PARSE_STATE, &vpsd.parseStates );//New( struct vesl_parse_state );
	vesl_state_init( state );
	return state;
}


static PDATALIST vesl_parse_get_data( struct vesl_parse_state *state ) {
	PDATALIST *result = state->elements;
	state->elements = GetFromSet( PDATALIST, &vpsd.dataLists );// CreateDataList( sizeof( state->val ) );
	if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
	else state->elements[0]->Cnt = 0;
	return result[0];
}

static void _vesl_dispose_message( PDATALIST *msg_data )
{
	struct vesl_value_container *val;
	INDEX idx;
	if( !msg_data ) return;
	DATA_FORALL( (*msg_data), idx, struct vesl_value_container*, val )
	{
		// names and string buffers for JSON parsed values in a single buffer
		// associated with the root message.
		//if( val->name ) Release( val->name );
		//if( val->string ) Release( val->string );
		if( val->_contains )
			_vesl_dispose_message( val->_contains );
	}
	// quick method
	DeleteFromSet( PDATALIST, vpsd.dataLists, msg_data );
	(*msg_data) = NULL;
	//DeleteDataList( msg_data );

}


static void vesl_parse_dispose_state( struct vesl_parse_state **ppState ) {
	struct vesl_parse_state *state = (*ppState);
	struct vesl_parse_context *old_context;
	PPARSE_BUFFER buffer;
	_vesl_dispose_message( state->elements );
	//DeleteDataList( &state->elements );
	while( buffer = (PPARSE_BUFFER)PopLink( state->outBuffers ) ) {
		Deallocate( const char *, buffer->buf );
		DeleteFromSet( PARSE_BUFFER, vpsd.parseBuffers, buffer );
	}
	{
		char *buf;
		INDEX idx;
		LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
			Deallocate( char*, buf );
		}
		DeleteFromSet( PLIST, vpsd.listSet, state->outValBuffers );
		//DeleteList( &state->outValBuffers );
	}
	while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
		DeleteFromSet( PARSE_BUFFER, vpsd.parseBuffers, buffer );
	while( buffer = (PPARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
		Deallocate( const char*, buffer->buf );
		DeleteFromSet( PARSE_BUFFER, vpsd.parseBuffers, buffer );
	}
	DeleteFromSet( PLINKQUEUE, vpsd.linkQueues, state->inBuffers );
	//DeleteLinkQueue( &state->inBuffers );
	DeleteFromSet( PLINKQUEUE, vpsd.linkQueues, state->outQueue );
	//DeleteLinkQueue( &state->outQueue );
	DeleteFromSet( PLINKSTACK, vpsd.linkStacks, state->outBuffers );
	//DeleteLinkStack( &state->outBuffers );
	//DeleteFromSet( PARSE_CONTEXT, vpsd.parseContexts, state->context );

	while( (old_context = (struct vesl_parse_context *)PopLink( state->context_stack )) ) {
		//lprintf( "warning unclosed contexts...." );
		DeleteFromSet( PARSE_CONTEXT, vpsd.parseContexts, old_context );
	}
	if( state->context_stack )
		DeleteFromSet( PLINKSTACK, vpsd.linkStacks, state->context_stack );
	//DeleteLinkStack( &state->context_stack );
	DeleteFromSet( PARSE_STATE, vpsd.parseStates, state );
	//Deallocate( struct vesl_parse_state *, state );
	(*ppState) = NULL;
}


LOGICAL vesl_parse_message( const char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct vesl_parse_state *state = vesl_begin_parse();
	static struct vesl_parse_state *_state;
	int result;
	vesl_preinit_state( state );
	state->complete_at_end = TRUE;
	result = vesl_parse_add_data( state, msg, msglen );
	if( _state ) vesl_parse_dispose_state( &_state );
	if( result > 0 ) {
		(*_msg_output) = vesl_parse_get_data( state );
		vesl_dump_parse( (*_msg_output) );
		_state = state;
		//vesl_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	vpsd.last_parse_state = state;
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
	//vesl_dispose_message( msg_data );
	return;
}


#undef GetUtfChar

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



