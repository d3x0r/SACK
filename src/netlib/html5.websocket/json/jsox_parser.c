#define JSOX_PARSER_SOURCE
#define JSOX_PARSER_MAIN_SOURCE
#include <stdhdrs.h>
#define DEADSTART_ROOT_OPTION_THING
#include <deadstart.h>

#include <jsox_parser.h>

#include "jsox.h"

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
SACK_NAMESPACE namespace network { namespace jsox {
#endif

PLIST knownArrayTypeNames;
static void registerKnownArrayTypeNames(void) {
	AddLink( &knownArrayTypeNames, "ab" );
	AddLink( &knownArrayTypeNames, "u8" );
	AddLink( &knownArrayTypeNames, "cu8" );
	AddLink( &knownArrayTypeNames, "s8" );
	AddLink( &knownArrayTypeNames, "u16" );
	AddLink( &knownArrayTypeNames, "s16" );
	AddLink( &knownArrayTypeNames, "u32" );
	AddLink( &knownArrayTypeNames, "s32" );
	AddLink( &knownArrayTypeNames, "u64" );
	AddLink( &knownArrayTypeNames, "s64" );
	AddLink( &knownArrayTypeNames, "f32" );
	AddLink( &knownArrayTypeNames, "f64" );
	AddLink( &knownArrayTypeNames, "ref" );
}



static void jsox_state_init( struct jsox_parse_state *state )
{
	PPDATALIST ppElements;
	PPLIST ppList;
	PPLINKQUEUE ppQueue;
	PPLINKSTACK ppStack;

	ppElements = GetFromSet( PDATALIST, &jxpsd.dataLists );
	if( !ppElements[0] ) ppElements[0] = CreateDataList( sizeof( state->val ) );
	state->elements = ppElements;
	state->elements[0]->Cnt = 0;

	ppStack = GetFromSet( PLINKSTACK, &jxpsd.linkStacks );
	if( !ppStack[0] ) ppStack[0] = CreateLinkStack();
	state->outBuffers = ppStack;
	state->outBuffers[0]->Top = 0;

	ppQueue = GetFromSet( PLINKQUEUE, &jxpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->inBuffers = ppQueue;// CreateLinkQueue();
	state->inBuffers[0]->Top = state->inBuffers[0]->Bottom = 0;

	ppQueue = GetFromSet( PLINKQUEUE, &jxpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->outQueue = ppQueue;// CreateLinkQueue();
	state->outQueue[0]->Top = state->outQueue[0]->Bottom = 0;

	ppList = GetFromSet( PLIST, &jxpsd.listSet );
	if( ppList[0] ) ppList[0]->Cnt = 0;
	state->outValBuffers = ppList;


	state->line = 1;
	state->col = 1;
	state->n = 0; // character index;
	state->word = JSOX_WORD_POS_RESET;
	state->status = TRUE;
	state->negative = FALSE;

	state->current_class = NULL;
	state->current_class_item = 0;
	state->arrayType = -1;


	state->context_stack = GetFromSet( PLINKSTACK, &jxpsd.linkStacks );// NULL;
	if( state->context_stack[0] ) state->context_stack[0]->Top = 0;
	//state->first_token = TRUE;
	state->context = GetFromSet( JSOX_PARSE_CONTEXT, &jxpsd.parseContexts );
	state->parse_context = JSOX_CONTEXT_UNKNOWN;
	state->comment = 0;
	state->completed = FALSE;
	//state->mOut = msg;// = NewArray( char, msglen );
	//state->msg_input = (char const *)msg;

	state->val.value_type = JSOX_VALUE_UNSET;
	state->val.contains = NULL;
	state->val._contains = NULL;
	state->val.name = NULL;
	state->val.string = NULL;

	state->complete_at_end = FALSE;
	state->gatheringString = FALSE;
	state->gatheringNumber = FALSE;

	state->pvtError = NULL;
}

/* I guess this is a good parser */
struct jsox_parse_state * jsox_begin_parse( void )
{
	struct jsox_parse_state *state = GetFromSet( JSOX_PARSE_STATE, &jxpsd.parseStates );//New( struct json_parse_state );
	jsox_state_init( state );
	return state;
}


char *jsox_escape_string_length( const char *string, size_t len, size_t *outlen ) {
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

char *jsox_escape_string( const char *string ) {
	return jsox_escape_string_length( string, strlen( string ), NULL );
}

#define _2char(result,from) (((*from) += 2),( ( result & 0x1F ) << 6 ) | ( ( result & 0x3f00 )>>8))
#define _zero(result,from)  ((*from)++,0) 
#define _3char(result,from) ( ((*from) += 3),( ( ( result & 0xF ) << 12 ) | ( ( result & 0x3F00 ) >> 2 ) | ( ( result & 0x3f0000 ) >> 16 )) )

#define _4char(result,from)  ( ((*from) += 4), ( ( ( result & 0x7 ) << 18 )     \
						| ( ( result & 0x3F00 ) << 4 )   \
						| ( ( result & 0x3f0000 ) >> 10 )    \
						| ( ( result & 0x3f000000 ) >> 24 ) ) )

#define get4Chars(p) ((((TEXTRUNE*) ((uintptr_t)(p) & ~0x3) )[0]  \
				>> (CHAR_BIT*((uintptr_t)(p) & 0x3)))             \
			| (( ((uintptr_t)(p)) & 0x3 )                          \
				? (((TEXTRUNE*) ((uintptr_t)(p) & ~0x3) )[1]      \
					<< (CHAR_BIT*(4-((uintptr_t)(p) & 0x3))))     \
				:(TEXTRUNE)0 ))

#define __GetUtfChar( result, from )           ((result = get4Chars(*from)),     \
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

static int gatherStringX(struct jsox_parse_state *state, CTEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut, TEXTRUNE start_c
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
	if( status ) {
		state->completedString = TRUE;
		(*mOut++) = 0;  // terminate the string.
	}
	(*pmOut) = mOut;
	return status;
}


static int recoverIdent( struct jsox_parse_state *state, struct jsox_output_buffer* output, int cInt );

static int openObject( struct jsox_parse_state *state, struct jsox_output_buffer* output, int c ) {
	enum jsox_parse_context_modes nextMode;
	PJSOX_CLASS cls = NULL;
	//let tmpobj = {};
	if( state->word > JSOX_WORD_POS_RESET && state->word < JSOX_WORD_POS_FIELD )
		recoverIdent( state, output, -1 );

	if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
		if( state->word == JSOX_WORD_POS_FIELD /*|| state->word == JSOX_WORD_POS_AFTER_FIELD*/ ) {
			INDEX idx;
			(*output->pos++) = 0;
#ifdef DEBUG_PARSING
			lprintf( "define class: %*.*s", output->pos - state->val.string, output->pos - state->val.string, state->val.string );
#endif
			LIST_FORALL( state->classes, idx, PJSOX_CLASS, cls ) 
				if( strcmp( cls->name, state->val.string ) == 0 )
					break;

			if( !cls ) {
				cls = GetFromSet( JSOX_CLASS, &jxpsd.classes );
				cls->name = state->val.string;
				cls->nameLen = output->pos - state->val.string;
				cls->fields = NULL;
				AddLink( &state->classes, cls );
				nextMode = JSOX_CONTEXT_CLASS_FIELD;
			} else {				
				//tmpobj = Object.assign( tmpobj, cls.protoObject );
				//Object.setPrototypeOf( tmpobj, Object.getPrototypeOf( cls.protoObject ) );
				nextMode = JSOX_CONTEXT_CLASS_VALUE;
			}
			state->word = JSOX_WORD_POS_RESET;
		}
		else {
			nextMode = JSOX_CONTEXT_OBJECT_FIELD;
			state->word = JSOX_WORD_POS_FIELD;
		}
	} else if( state->word == JSOX_WORD_POS_FIELD /*|| state->word == JSOX_WORD_POS_AFTER_FIELD*/ || state->parse_context == JSOX_CONTEXT_IN_ARRAY || state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE ) {
		if( state->word != JSOX_WORD_POS_RESET ) {
			INDEX idx;
			(*output->pos++) = 0;
			LIST_FORALL( state->classes, idx, PJSOX_CLASS, cls )
				if( strcmp( cls->name, state->val.string ) == 0 )
					break;
			if( !cls ) lprintf( "Referenced class %s has not been defined", state->val.string );
			nextMode = JSOX_CONTEXT_CLASS_VALUE;
			state->word = JSOX_WORD_POS_RESET;
		}
		else {
			nextMode = JSOX_CONTEXT_OBJECT_FIELD;
			state->word = JSOX_WORD_POS_RESET;
		}
	} else if( (state->parse_context == JSOX_CONTEXT_OBJECT_FIELD && state->word == JSOX_WORD_POS_RESET) ) {
		if( !state->pvtError ) state->pvtError = VarTextCreate();
		vtprintf( state->pvtError, "Fault while parsing; getting field name unexpected '%c' at %" _size_f " %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
		state->status = FALSE;
		return FALSE;
	}
	else
		nextMode = JSOX_CONTEXT_OBJECT_FIELD;

	// common code; create new object container layer...
	{
		struct jsox_parse_context *old_context = GetFromSet( JSOX_PARSE_CONTEXT, &jxpsd.parseContexts );
#ifdef DEBUG_PARSING
		lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", state->val.value_type );
#endif				
		old_context->context = state->parse_context;
		old_context->elements = state->elements;
		old_context->name = state->val.name;
		old_context->nameLen = state->val.nameLen;
		old_context->current_class = state->current_class;
		old_context->current_class_item = state->current_class_item;
		old_context->arrayType = state->arrayType;
		state->current_class = cls;
		state->current_class_item = 0;
		state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
		if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
		else state->elements[0]->Cnt = 0;
		PushLink( state->context_stack, old_context );
		JSOX_RESET_STATE_VAL();
		state->parse_context = nextMode;
	}
	return TRUE;
}

static LOGICAL openArray( struct jsox_parse_state *state, struct jsox_output_buffer* output, int c ) {
	if( state->word > JSOX_WORD_POS_RESET && state->word < JSOX_WORD_POS_FIELD )
		recoverIdent(state,output,c);
	if( state->word == JSOX_WORD_POS_FIELD ) {
		char *name;
		INDEX typeIndex;// = knownArrayTypeNames.findIndex( type = > (type == = val.string) );
		(*output->pos++) = 0;
#ifdef DEBUG_PARSING
		lprintf( "define typed array:%s", state->val.string );
#endif
		if( !knownArrayTypeNames ) registerKnownArrayTypeNames();
		LIST_FORALL( knownArrayTypeNames, typeIndex, char *, name ) {
			if( strcmp( state->val.string, name ) == 0 )
				break;
		}
		if( typeIndex < 13 ) {
			state->word = JSOX_WORD_POS_FIELD;
			state->arrayType = (int)typeIndex;
#ifdef DEBUG_PARSING
			lprintf( "setup array type... %d", typeIndex );
#endif
			state->val.string = output->pos;
		}
		else {
			if( !state->pvtError ) state->pvtError = VarTextCreate();
			vtprintf( state->pvtError, WIDE( "Unknown type specified for array:; %s at '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
				, state->val.string, c, state->n, state->line, state->col );
			state->status = FALSE;
			return FALSE;
		}
	} else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) {
		if( !state->pvtError ) state->pvtError = VarTextCreate();
		vtprintf( state->pvtError, WIDE( "Fault while parsing; while getting field name unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
		state->status = FALSE;
		return FALSE;
	}
	{
		struct jsox_parse_context *old_context = GetFromSet( JSOX_PARSE_CONTEXT, &jxpsd.parseContexts );
#ifdef DEBUG_PARSING
		lprintf( "Begin a new array; previously pushed into elements; but wait until trailing comma or close previously:%d", state->val.value_type );
#endif				
		old_context->context = state->parse_context;
		old_context->elements = state->elements;
		old_context->name = state->val.name;
		old_context->nameLen = state->val.nameLen;
		old_context->current_class = state->current_class;
		old_context->current_class_item = state->current_class_item;
		old_context->arrayType = state->arrayType;
		state->current_class = NULL;
		state->current_class_item = 0;
		state->arrayType = -1;
		state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
		if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
		else state->elements[0]->Cnt = 0;
		PushLink( state->context_stack, old_context );

		JSOX_RESET_STATE_VAL();
		state->parse_context = JSOX_CONTEXT_IN_ARRAY;
		state->word = JSOX_WORD_POS_RESET;
	}
	return TRUE;
}

int recoverIdent( struct jsox_parse_state *state, struct jsox_output_buffer* output, int cInt ) {
	if( state->word != JSOX_WORD_POS_RESET ) {
		if( !state->val.string ) {
#ifdef DEBUG_PARSINGs
			lprintf( "Updating string postion?" );
#endif
			state->val.string = output->pos;
		}
		if( state->word == JSOX_WORD_POS_END ) {
			switch( state->val.value_type ) {
			default:
				lprintf( "FAULT: UNEXPECTED VALUE TYPE RECOVERINT IDENT:%d", state->val.value_type );
				break;
			case JSOX_VALUE_TRUE:
				(*output->pos++) = 't';
				(*output->pos++) = 'r';
				(*output->pos++) = 'u';
				(*output->pos++) = 'e';
				break;
			case JSOX_VALUE_FALSE:
				(*output->pos++) = 'f';
				(*output->pos++) = 'a';
				(*output->pos++) = 'l';
				(*output->pos++) = 's';
				(*output->pos++) = 'e';
				break;
			case JSOX_VALUE_NULL:
				(*output->pos++) = 'n';
				(*output->pos++) = 'u';
				(*output->pos++) = 'l';
				(*output->pos++) = 'l';
				break;
			case JSOX_VALUE_UNDEFINED:
				(*output->pos++) = 'u';
				(*output->pos++) = 'n';
				(*output->pos++) = 'd';
				(*output->pos++) = 'e';
				(*output->pos++) = 'f';
				(*output->pos++) = 'i';
				(*output->pos++) = 'n';
				(*output->pos++) = 'e';
				(*output->pos++) = 'd';
				break;
			case JSOX_VALUE_NEG_NAN:
				(*output->pos++) = '-';
			case JSOX_VALUE_NAN:
				(*output->pos++) = 'N';
				(*output->pos++) = 'a';
				(*output->pos++) = 'N';
				break;
			case JSOX_VALUE_NEG_INFINITY:
				(*output->pos++) = '-';
			case JSOX_VALUE_INFINITY:
				(*output->pos++) = 'I';
				(*output->pos++) = 'n';
				(*output->pos++) = 'f';
				(*output->pos++) = 'i';
				(*output->pos++) = 'n';
				(*output->pos++) = 'i';
				(*output->pos++) = 't';
				(*output->pos++) = 'y';
				break;
			}
		}
		switch( state->word ) {
		default:
			lprintf( "FAULT: UNEXPECTED VALUE WORD POS RECOVERING IDENT:%d", state->word );
			break;
		case JSOX_WORD_POS_AFTER_FIELD:
		case JSOX_WORD_POS_FIELD:
		case JSOX_WORD_POS_END:  // full text fro before.
			break;
		case JSOX_WORD_POS_TRUE_1:
			(*output->pos++) = 't';
			break;
		case JSOX_WORD_POS_TRUE_2:
			(*output->pos++) = 't';
			(*output->pos++) = 'r';
			break;
		case JSOX_WORD_POS_TRUE_3:
			(*output->pos++) = 't';
			(*output->pos++) = 'r';
			(*output->pos++) = 'u';
			break;
		case JSOX_WORD_POS_FALSE_1: // 11
			(*output->pos++) = 'f';
			break;
		case JSOX_WORD_POS_FALSE_2:
			(*output->pos++) = 'f';
			(*output->pos++) = 'a';
			break;
		case JSOX_WORD_POS_FALSE_3:
			(*output->pos++) = 'f';
			(*output->pos++) = 'a';
			(*output->pos++) = 'l';
			break;
		case JSOX_WORD_POS_FALSE_4:
			(*output->pos++) = 'f';
			(*output->pos++) = 'a';
			(*output->pos++) = 'l';
			(*output->pos++) = 's';
			break;
		case JSOX_WORD_POS_NULL_1: // 21  get u
			(*output->pos++) = 'n';
			break;
		case JSOX_WORD_POS_NULL_2: //  get l
			(*output->pos++) = 'n';
			(*output->pos++) = 'u';
			break;
		case JSOX_WORD_POS_NULL_3: //  get l
			(*output->pos++) = 'n';
			(*output->pos++) = 'u';
			(*output->pos++) = 'l';
			break;
		case JSOX_WORD_POS_UNDEFINED_1:  // 31 
			(*output->pos++) = 'u';
			break;
		case JSOX_WORD_POS_UNDEFINED_2:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			break;
		case JSOX_WORD_POS_UNDEFINED_3:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			break;
		case JSOX_WORD_POS_UNDEFINED_4:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			(*output->pos++) = 'e';
			break;
		case JSOX_WORD_POS_UNDEFINED_5:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			(*output->pos++) = 'e';
			(*output->pos++) = 'f';
			break;
		case JSOX_WORD_POS_UNDEFINED_6:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			(*output->pos++) = 'e';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			break;
		case JSOX_WORD_POS_UNDEFINED_7:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			(*output->pos++) = 'e';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			(*output->pos++) = 'n';
			break;
		case JSOX_WORD_POS_UNDEFINED_8:
			(*output->pos++) = 'u';
			(*output->pos++) = 'n';
			(*output->pos++) = 'd';
			(*output->pos++) = 'e';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			(*output->pos++) = 'n';
			(*output->pos++) = 'e';
			break;

		case JSOX_WORD_POS_NAN_1:
			(*output->pos++) = 'N';
			break;
		case JSOX_WORD_POS_NAN_2:
			(*output->pos++) = 'N';
			(*output->pos++) = 'a';
			break;
		case JSOX_WORD_POS_INFINITY_1:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			break;
		case JSOX_WORD_POS_INFINITY_2:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			break;
		case JSOX_WORD_POS_INFINITY_3:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			(*output->pos++) = 'f';
			break;
		case JSOX_WORD_POS_INFINITY_4:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			break;
		case JSOX_WORD_POS_INFINITY_5:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			(*output->pos++) = 'n';
			break;
		case JSOX_WORD_POS_INFINITY_6:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			(*output->pos++) = 'n';
			(*output->pos++) = 'i';
			break;
		case JSOX_WORD_POS_INFINITY_7:
			if( state->negative )
				(*output->pos++) = '-';
			(*output->pos++) = 'I';
			(*output->pos++) = 'n';
			(*output->pos++) = 'f';
			(*output->pos++) = 'i';
			(*output->pos++) = 'n';
			(*output->pos++) = 'i';
			(*output->pos++) = 't';
			break;
		}
	}
#ifdef DEBUG_PARSING
	lprintf( "RECOVER IDENT: TURN INTO FIELD NAME" );
#endif
	state->word = JSOX_WORD_POS_FIELD;
	state->negative = FALSE;
	state->val.value_type = JSOX_VALUE_STRING;
	state->completedString = FALSE;


	if( cInt == 123/*'{'*/ )
		openObject( state, output, cInt );
	else if( cInt == 91/*'['*/ )
		openArray( state, output, cInt );
	else if( cInt >= 0 ) {
		// ignore white space.
		if( cInt == 32/*' '*/ || cInt == 13 || cInt == 10 || cInt == 9 || cInt == 0xFEFF || cInt == 2028 || cInt == 2029 ) {
			state->word = JSOX_WORD_POS_END;
			state->val.stringLen = output->pos - state->val.string;
			return 0;
		}

		if( cInt == 44/*','*/ || cInt == 125/*'}'*/ || cInt == 93/*']'*/ || cInt == 58/*':'*/ )
			vtprintf( state->pvtError, WIDE( "invalid character; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, cInt, state->n, state->line, state->col );
		else {
			if( !state->val.string )  state->val.string = output->pos;
			if( cInt < 128 ) (*output->pos++) = cInt;
			else output->pos += ConvertToUTF8( output->pos, cInt );
#ifdef DEBUG_PARSING
			lprintf( "Collected .. %d %c  %*.*s", cInt, cInt, output->pos - state->val.string, output->pos - state->val.string, state->val.string );
#endif
			state->val.stringLen = output->pos - state->val.string;
		}
	}

	return 0;
}

static void pushValue( struct jsox_parse_state *state, PDATALIST *pdl, struct jsox_value_container *val, int line ) {
#define pushValue(a,b,c) pushValue(a,b,c,__LINE__)
#ifdef DEBUG_PARSING
	lprintf( "pushValue:%d %d", val->value_type, state->arrayType );
	if( val->name )
		lprintf( "push named:%*.*s %d", val->nameLen, val->nameLen, val->name, line );
#endif
	if( val->value_type == JSOX_VALUE_ARRAY ) {

		if( state->arrayType >= 0 ) {
			//size_t size;
			val->className = (char*)GetLink( &knownArrayTypeNames, state->arrayType );
			val->value_type = (enum jsox_value_types)(JSOX_VALUE_TYPED_ARRAY + state->arrayType);
			//lprintf( "INPUT:%d %s", val->stringLen, val->string );
			val->string = (char*)DecodeBase64Ex( val->string, val->stringLen, &val->stringLen, NULL );
			//lprintf( "base:%s", EncodeBase64Ex( "HELLO, World!", 13, NULL, NULL ) );
			//lprintf( "Resolve base64 string:%s", val->string );
		}
	}
	AddDataItem( pdl, val );
}

static LOGICAL isNonIdentifier( TEXTRUNE c ) {
	if( c < 0xFF ) {
		if( nonIdentifiers8[c] ) {
			return TRUE;
		}
	}
	else {
		int n;
		for( n = 0; n < (sizeof( nonIdentifierBits ) / sizeof( nonIdentifierBits[0] )); n++ ) {
			if( c >= (TEXTRUNE)nonIdentifierBits[n].firstChar && c < (TEXTRUNE)nonIdentifierBits[n].lastChar &&
				(nonIdentifierBits[n].bits[(c - nonIdentifierBits[n].firstChar) / 24]
					& (1 << ((c - nonIdentifierBits[n].firstChar) % 24))) )
				return TRUE;
		}
	}
	return FALSE;
}

int jsox_parse_add_data( struct jsox_parse_state *state
                            , const char * msg
                            , size_t msglen )
{
	/* I guess this is a good parser */
	TEXTRUNE c;
	PJSOX_PARSE_BUFFER input;
	struct jsox_output_buffer* output;
	int string_status;
	int retval = 0;
	if( !state->status )
		return -1;

	if( msg && msglen ) {
		input = GetFromSet( JSOX_PARSE_BUFFER, &jxpsd.parseBuffers );
		input->pos = input->buf = msg;
		input->size = msglen;
		EnqueLinkNL( state->inBuffers, input );

		if( state->gatheringString
			|| state->gatheringNumber
			|| state->word == JSOX_WORD_POS_FIELD
			|| state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) {
			// have to extend the previous output buffer to include this one instead of allocating a split string.
			size_t offset;
			size_t offset2;
			output = (struct jsox_output_buffer*)DequeLinkNL( state->outQueue );
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
			output = (struct jsox_output_buffer*)GetFromSet( JSOX_PARSE_BUFFER, &jxpsd.parseBuffers );
			output->pos = output->buf = NewArray( char, msglen + 1 );
			output->size = msglen;
			EnqueLinkNL( state->outQueue, output );
		}
	}
	else {
		// zero length input buffer... terminate a number.
		if( state->gatheringNumber ) {
			//console.log( "Force completed.")
			output = (struct jsox_output_buffer*)DequeLinkNL( state->outQueue );
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
			state->val.value_type = JSOX_VALUE_NUMBER;
			if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
				state->completed = TRUE;
			}
			retval = 1;
		}
	}

	while( state->status && ( input = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) ) ) {
		output = (struct jsox_output_buffer*)DequeLinkNL( state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;
		if( state->n > input->size ) DebugBreak();

		if( state->gatheringString ) {
			string_status = gatherStringX( state, input->buf, &input->pos, input->size, &output->pos, state->gatheringStringFirstChar );
			if( string_status < 0 )
				state->status = FALSE;
			else if( string_status > 0 )
			{
				state->gatheringString = FALSE;
				state->n = input->pos - input->buf;
				if( state->n > input->size ) DebugBreak();
				state->val.stringLen = (output->pos - state->val.string)-1;
#ifdef DEBUG_PARSING
				lprintf( "STRING1: %s %d", state->val.string, state->val.stringLen );
#endif
				if( state->status ) {
					state->val.value_type = JSOX_VALUE_STRING;
					state->completedString = TRUE;
				}
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
#ifdef DEBUG_PARSING
			lprintf( "parse character %c %d %d %d %d", c<32?'.':c, state->word, state->parse_context, state->parse_context, state->word );
#endif
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
				openObject( state, output, c );
				break;

			case '[':
				openArray( state, output, c );
				break;

			case ':':
				if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD )
				{
					if( state->word != JSOX_WORD_POS_RESET
						&& state->word != JSOX_WORD_POS_FIELD
						&& state->word != JSOX_WORD_POS_AFTER_FIELD ) {
						// allow starting a new word
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "unquoted keyword used as object field name:parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
						break;
					}
					else if( state->word == JSOX_WORD_POS_FIELD ) {
						state->val.stringLen = ( output->pos - state->val.string );
						(*output->pos++) = 0;
					}
					else if( (state->val.value_type == JSOX_VALUE_STRING) && !state->completedString ) {
						state->val.stringLen = ( output->pos - state->val.string );
						(*output->pos++) = 0;
					}
					state->word = JSOX_WORD_POS_RESET;
					if( state->val.name ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "two names single value?" );
					}
					state->word = JSOX_WORD_POS_RESET;
					state->val.name = state->val.string;
					state->val.nameLen = state->val.stringLen;
					state->val.string = NULL;
					state->val.stringLen = 0;
					state->parse_context = JSOX_CONTEXT_OBJECT_FIELD_VALUE;
					state->val.value_type = JSOX_VALUE_UNSET;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					if( state->parse_context == JSOX_CONTEXT_IN_ARRAY )
						vtprintf( state->pvtError, WIDE( "(in array, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
					else
						vtprintf( state->pvtError, WIDE( "(outside any object, got colon out of string):parsing fault; unexpected %c at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
					state->status = FALSE;
				}
				break;
			case '}':
				if( state->word == JSOX_WORD_POS_END ) {
					// allow starting a new word
					state->word = JSOX_WORD_POS_RESET;
				}

				if( state->parse_context == JSOX_CONTEXT_CLASS_FIELD ) {	
					if( state->current_class ) {
						// allow blank comma at end to not be a field
						struct jsox_parse_context *old_context = (struct jsox_parse_context *)PopLink( state->context_stack );
						
						if(state->val.string) { 
							struct jsox_class_field *field = GetFromSet( JSOX_CLASS_FIELD, &jxpsd.class_fields );
							field->name = state->val.string;
							field->nameLen = output->pos - state->val.string;
							(*output->pos++) = 0;
							state->val.string = NULL;
							AddLink( &state->current_class->fields, field );
							//AddLink( &state->current_class->fields, state->val.string );
						}

						JSOX_RESET_STATE_VAL();
						state->word = JSOX_WORD_POS_RESET;
#ifdef DEBUG_PARSING_STCK
						lprintf( "object pop stack (close obj) %d %p", context_stack.length, old_context );
#endif
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->arrayType = old_context->arrayType;
					
						DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, old_context );
					} else {
						vtprintf( state->pvtError, WIDE( "State error; gathering class fields, and lost the class; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f
							, c, state->n, state->line, state->col );
					}
				} else if( ( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) || state->parse_context == JSOX_CONTEXT_CLASS_VALUE ) {
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
						struct jsox_class_field *field = (struct jsox_class_field *)GetLink( &state->current_class->fields, state->current_class_item++ );
						state->val.name = field->name;
						state->val.nameLen = field->nameLen;
#ifdef DEBUG_PARSING
						lprintf( "Push value closing class value %d %p", state->current_class_item, state->current_class );
#endif
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
					}
					//if( _DEBUG_PARSING ) lprintf( "close object; empty object", val, elements );
					state->val.value_type = JSOX_VALUE_OBJECT;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					state->val.className = state->current_class->name;
					state->val.string = NULL;
					{
						struct jsox_parse_context *old_context = (struct jsox_parse_context *)PopLink( state->context_stack );
						//if( _DEBUG_PARSING_STACK ) console.log( "object pop stack (close obj)", context_stack.length, old_context );
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->arrayType = old_context->arrayType;
						DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, old_context );
					}
					if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				} else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE ) {
					//enum json_value_types 
#ifdef DEBUG_PARSING
					lprintf( "close object; empty object %d", state->val.value_type );
#endif
					//if( (state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE) )
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
						if( state->val.string ) {
							if( state->val.value_type != JSOX_VALUE_STRING ) {
								state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_PARSING
								lprintf( "STRING2: %s %d", state->val.string, state->val.stringLen );
#endif
								(*output->pos++) = 0;
							}
						}
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
					}
					//JSOX_RESET_STATE_VAL();
					state->val.value_type = JSOX_VALUE_OBJECT;
					state->val.string = NULL;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					{
						struct jsox_parse_context *old_context = (struct jsox_parse_context *)PopLink( state->context_stack );
						//struct jsox_value_container *oldVal = (struct jsox_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.
						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->arrayType = old_context->arrayType;
						DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, old_context );
					}
					if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
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
				state->word = JSOX_WORD_POS_RESET;
				if( state->parse_context == JSOX_CONTEXT_IN_ARRAY )
				{
#ifdef DEBUG_PARSING
					lprintf( "close array, push last element: %d", state->val.value_type );
#endif
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
						if( state->val.string ) {
							if( state->val.value_type != JSOX_VALUE_STRING ) {
								state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_PARSING
								lprintf( "STRING3: %s %d", state->val.string, state->val.stringLen );
#endif
								(*output->pos++) = 0;
							}
						}
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
					}
					state->val.value_type = JSOX_VALUE_ARRAY;

					//state->val.string = NULL;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;

					{
						struct jsox_parse_context *old_context = (struct jsox_parse_context *)PopLink( state->context_stack );
						//struct jsox_value_container *oldVal = (struct jsox_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->arrayType = old_context->arrayType;
						DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, old_context );
					}
					if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
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
				if( state->word == JSOX_WORD_POS_END || state->word == JSOX_WORD_POS_FIELD ) {
					// allow starting a new word
					state->word = JSOX_WORD_POS_RESET;
				}

				if( state->parse_context == JSOX_CONTEXT_CLASS_FIELD ) {
					if( state->current_class ) {
						struct jsox_class_field *field = GetFromSet( JSOX_CLASS_FIELD, &jxpsd.class_fields );
						field->name = state->val.string;
						field->nameLen = output->pos - state->val.string;
						(*output->pos++) = 0;
						state->val.string = NULL;
						AddLink( &state->current_class->fields, field );
						state->word = JSOX_WORD_POS_FIELD;
					}
					else {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "lost class definition; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->parse_context, c, state->n, state->line, state->col );// fault
						state->status = FALSE;
					}
				}
				else if( state->parse_context == JSOX_CONTEXT_CLASS_VALUE ) {
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
						struct jsox_class_field *field = (struct jsox_class_field *)GetLink( &state->current_class->fields, state->current_class_item++ );
						state->val.name = field->name;
						state->val.nameLen = field->nameLen;
						pushValue( state, state->elements, &state->val );

						JSOX_RESET_STATE_VAL();
						state->word = JSOX_WORD_POS_RESET;
					}
				}
				else if( state->parse_context == JSOX_CONTEXT_IN_ARRAY )
				{
					if( state->val.value_type == JSOX_VALUE_UNSET )
						state->val.value_type = JSOX_VALUE_EMPTY; // in an array, elements after a comma should init as undefined...
																 // undefined allows [,,,] to be 4 values and [1,2,3,] to be 4 values with an undefined at end.
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
#ifdef DEBUG_PARSING
						lprintf( "back in array; push item %d", state->val.value_type );
#endif
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
					}
				}
				else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE )
				{
					// after an array value, it will have returned to OBJECT_FIELD anyway	
#ifdef DEBUG_PARSING
					lprintf( "comma after field value, push field to object: %s", state->val.name );
#endif
					state->parse_context = JSOX_CONTEXT_OBJECT_FIELD;
					state->word = JSOX_WORD_POS_RESET;
					if( state->val.value_type != JSOX_VALUE_UNSET )
						pushValue( state, state->elements, &state->val );
					JSOX_RESET_STATE_VAL();
				}
				else
				{
					state->status = FALSE;
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );// fault
				}
				break;

			default:
				if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD 
				   //|| state->parse_context == JSOX_CONTEXT_UNKNOWN 
				   //|| state->parse_context == JSOX_CONTEXT_IN_ARRAY
				   || (state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE && state->word == JSOX_WORD_POS_FIELD )
				   || state->parse_context == JSOX_CONTEXT_CLASS_FIELD
				) {
					//lprintf( "gathering object field:%c  %*.*s", c, output->pos- state->val.string, output->pos - state->val.string, state->val.string );
					switch( c )
					{
					case '`':
						// this should be a special case that passes continuation to gatherString
						// but gatherString now just gathers all strings
					case '"':
					case '\'':
						if( state->val.value_type == JSOX_VALUE_STRING
							&& state->val.className ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "too many strings in a row; fault while parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );// fault
							break;
						}
						if( state->word == JSOX_WORD_POS_FIELD
							|| ( state->val.value_type == JSOX_VALUE_STRING 
								&& !state->val.className ) ) {
							(*output->pos++) = 0;
							state->val.className = state->val.string;
						}
						state->val.string = output->pos;
						state->gatheringString = TRUE;
						state->gatheringStringFirstChar = c;
						string_status = gatherStringX( state, input->buf, &input->pos, input->size, &output->pos, c );
						//lprintf( "string gather status:%d", string_status );
						if( string_status < 0 )
							state->status = FALSE;
						else if( string_status > 0 ) {
							state->gatheringString = FALSE;
							state->val.stringLen = (output->pos - state->val.string) - 1;
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) state->completed = TRUE;
#ifdef DEBUG_PARSING
							lprintf( "STRING4: %s %d", state->val.string, state->val.stringLen );
#endif
						}
						state->n = input->pos - input->buf;
						if( state->n > input->size ) DebugBreak();
						if( state->status ) {
							state->val.value_type = JSOX_VALUE_STRING;
							state->completedString = TRUE;
							state->word = JSOX_WORD_POS_AFTER_FIELD;
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
					case 2028: // LS (Line separator)
					case 2029: // PS (paragraph separate)
					case 0xFEFF: // ZWNBS is WS though
						if( state->word == JSOX_WORD_POS_END ) {
							state->word = JSOX_WORD_POS_RESET;
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
							break;
						}
						if( (state->word == JSOX_WORD_POS_RESET) || ( state->word == JSOX_WORD_POS_AFTER_FIELD ) )
							break;
						else if( state->word == JSOX_WORD_POS_FIELD ) {
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
								break;
							}							
							if( state->val.string ) {
								state->val.value_type = JSOX_VALUE_STRING;
								state->word = JSOX_WORD_POS_AFTER_FIELD;
								if( state->parse_context == JSOX_CONTEXT_UNKNOWN )
									state->completed = TRUE;
							}
						}
						else {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing; whitespace unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n, state->line, state->col );	// fault
						}
						// skip whitespace
						//n++;
						//lprintf( "whitespace skip..." );
						break;
					default:
						if( state->word == JSOX_WORD_POS_RESET && ( (c >= '0' && c <= '9') || (c == '+') || (c == '.') ) ) {
							goto beginNumber;
						}
						if( state->word == JSOX_WORD_POS_AFTER_FIELD ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing; second string in field name at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, state->n, state->line, state->col );	// fault
							break;
						} else if( state->word == JSOX_WORD_POS_RESET ) {
							state->word = JSOX_WORD_POS_FIELD;
							state->val.string = output->pos;
							state->val.value_type = JSOX_VALUE_STRING;
							state->completedString = FALSE;
						}
						if( isNonIdentifier( c ) ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "fault while parsing object field name; \\u00%02X unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );	// fault
							break;
						}
						if( !state->val.string ) state->val.string = output->pos;
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
					if( state->word == JSOX_WORD_POS_FIELD
						|| ( state->val.value_type == JSOX_VALUE_STRING
							 && !state->val.className ) ) {
						(*output->pos++) = 0;
						state->val.className = state->val.string;
					}
					state->val.string = output->pos;
					state->gatheringString = TRUE;
					state->gatheringStringFirstChar = c;
					string_status = gatherStringX( state, input->buf, &input->pos, input->size, &output->pos, c );
					//lprintf( "string gather status:%d", string_status );
					if( string_status < 0 )
						state->status = FALSE;
					else if( string_status > 0 ) {
						state->gatheringString = FALSE;
						state->val.stringLen = (output->pos - state->val.string) - 1;
#ifdef DEBUG_PARSING
						lprintf( "STRING5: %s %d", state->val.string, state->val.stringLen );
#endif
					} else if( state->complete_at_end ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "End of string fail." );
						state->status = FALSE;
					}
					state->n = input->pos - input->buf;
					if( state->n > input->size ) DebugBreak();

					if( state->status ) {
						state->val.value_type = JSOX_VALUE_STRING;
						state->completedString = TRUE;
						state->word = JSOX_WORD_POS_END;
						if( state->complete_at_end ) {
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
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
				case 2028: // LS (Line separator)
				case 2029: // PS (paragraph separate)
				case '\t':
				case '\r':
				case 0xFEFF:
					if( state->word == JSOX_WORD_POS_END ) {
						state->word = JSOX_WORD_POS_RESET;
						if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
							state->completed = TRUE;
						}
						break;
					}
					if( state->word == JSOX_WORD_POS_RESET || (state->word == JSOX_WORD_POS_AFTER_FIELD) ) {
						break;
					}
					else if( state->word == JSOX_WORD_POS_FIELD ) {
						if( state->val.string ) {
							state->val.value_type = JSOX_VALUE_STRING;
							state->word = JSOX_WORD_POS_AFTER_FIELD;
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN )
								state->completed = TRUE;
						}
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
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_TRUE_1;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_6 ) state->word = JSOX_WORD_POS_INFINITY_7;
					else recoverIdent( state, output, c );
					break;
				case 'r':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_TRUE_1 ) state->word = JSOX_WORD_POS_TRUE_2;
					else {
						recoverIdent( state, output, c );
					}// fault
					break;
				case 'u':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_TRUE_2 ) state->word = JSOX_WORD_POS_TRUE_3;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_NULL_1 ) state->word = JSOX_WORD_POS_NULL_2;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_UNDEFINED_1;
					else recoverIdent( state, output, c );
					break;
				case 'e':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_TRUE_3 ) {
						state->val.value_type = JSOX_VALUE_TRUE;
						state->word = JSOX_WORD_POS_END;
					}
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_FALSE_4 ) {
						state->val.value_type = JSOX_VALUE_FALSE;
						state->word = JSOX_WORD_POS_END;
					}
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_3 ) state->word = JSOX_WORD_POS_UNDEFINED_4;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_7 ) state->word = JSOX_WORD_POS_UNDEFINED_8;
					else recoverIdent( state, output, c );
					break;
				case 'n':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_NULL_1;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_1 ) state->word = JSOX_WORD_POS_UNDEFINED_2;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_6 ) state->word = JSOX_WORD_POS_UNDEFINED_7;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_1 ) state->word = JSOX_WORD_POS_INFINITY_2;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_4 ) state->word = JSOX_WORD_POS_INFINITY_5;
					else recoverIdent( state, output, c );
					break;
				case 'd':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_2 ) state->word = JSOX_WORD_POS_UNDEFINED_3;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_8 ) { state->val.value_type = JSOX_VALUE_UNDEFINED; state->word = JSOX_WORD_POS_END; }
					else recoverIdent( state, output, c );
					break;
				case 'i':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_5 ) state->word = JSOX_WORD_POS_UNDEFINED_6;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_3 ) state->word = JSOX_WORD_POS_INFINITY_4;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_5 ) state->word = JSOX_WORD_POS_INFINITY_6;
					else recoverIdent( state, output, c );
					break;
				case 'l':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_NULL_2 ) state->word = JSOX_WORD_POS_NULL_3;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_NULL_3 ) {
						state->val.value_type = JSOX_VALUE_NULL;
						state->word = JSOX_WORD_POS_END;
					}
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_FALSE_2 ) state->word = JSOX_WORD_POS_FALSE_3;
					else recoverIdent( state, output, c );
					break;
				case 'f':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_FALSE_1;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_UNDEFINED_4 ) state->word = JSOX_WORD_POS_UNDEFINED_5;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_2 ) state->word = JSOX_WORD_POS_INFINITY_3;
					else recoverIdent( state, output, c );
					break;
				case 'a':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_FALSE_1 ) state->word = JSOX_WORD_POS_FALSE_2;
					else if(state->val.value_type == JSOX_VALUE_UNSET &&  state->word == JSOX_WORD_POS_NAN_1 ) state->word = JSOX_WORD_POS_NAN_2;
					else recoverIdent( state, output, c );
					break;
				case 's':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_FALSE_3 ) state->word = JSOX_WORD_POS_FALSE_4;
					else recoverIdent( state, output, c );
					break;
				case 'I':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_INFINITY_1;
					else recoverIdent( state, output, c );
					break;
				case 'N':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_RESET ) state->word = JSOX_WORD_POS_NAN_1;
					else if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_NAN_2 ) { state->val.value_type = state->negative ? JSOX_VALUE_NEG_NAN : JSOX_VALUE_NAN; state->word = JSOX_WORD_POS_END; }
					else recoverIdent( state, output, c );
					break;
				case 'y':
					if( state->val.value_type == JSOX_VALUE_UNSET && state->word == JSOX_WORD_POS_INFINITY_7 ) { state->val.value_type = state->negative ? JSOX_VALUE_NEG_INFINITY : JSOX_VALUE_INFINITY; state->word = JSOX_WORD_POS_END; }
					else recoverIdent( state, output, c );
					break;
					//
					//----------------------------------------------------------
				case '-':
					state->negative = !state->negative;
					break;

				default:
					if( state->word == JSOX_WORD_POS_RESET && ( (c >= '0' && c <= '9') || (c == '+') || (c == '.') ) )
					{
						LOGICAL fromDate;
						const char *_msg_input; // to unwind last character past number.
						// always reset this here....
						// keep it set to determine what sort of value is ready.
					beginNumber:
						if( !state->gatheringNumber ) {
							state->numberFromBigInt = FALSE;
							state->numberFromDate = FALSE;
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
							// to be implemented
							else if( c == ':' || c == '-' || c == 'T' || c == 'Z' || c == '+' ) {
								/* toISOString()
								var today = new Date('05 October 2011 14:48 UTC');
								console.log(today.toISOString());
								// Returns 2011-10-05T14:48:00.000Z
								*/
								(*output->pos++) = c;
								state->numberFromDate = TRUE;
							}
							else if( ( c == 'x' || c == 'b' || c =='o' || c == 'X' || c == 'B' || c == 'O')
							       && ( output->pos - state->val.string) == 1
							       && state->val.string[0] == '0' ) {
								// hex conversion.
								if( !state->fromHex ) {
									state->fromHex = TRUE;
									(*output->pos++) = c | 0x20; // force lower case.
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault while parsing number; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
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
							} else if( c == 'n' ) {
								(*output->pos++) = c;
								state->numberFromBigInt = TRUE;
								_msg_input = input->pos; // consume character.
								break;
							} else if( c == '.' ) {
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
							} else {
								// in non streaming mode; these would be required to follow
								if( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xFEFF
									|| c == ',' || c == ']' || c == '}'  || c == ':' ) {
									//lprintf( "Non numeric character received; push the value we have" );
									(*output->pos) = 0;
									break;
								}
								else {
									if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
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
#ifdef DEBUG_PARSING
							lprintf( "STRING6: %s %d", state->val.string, state->val.stringLen );
#endif
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
							if( state->numberFromDate )
								state->val.value_type = JSOX_VALUE_DATE;
							else if( state->numberFromBigInt )
								state->val.value_type = JSOX_VALUE_BIGINT;
							else
								state->val.value_type = JSOX_VALUE_NUMBER;
							if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
						}
					}
					else
					{
						recoverIdent( state, output, c );
					}
					break; // default
				}
				break; // default of high level switch
			}
			// got a completed value; skip out
			if( state->completed ) {
				if( state->word == JSOX_WORD_POS_END ) {
					state->word = JSOX_WORD_POS_RESET;
				}
				break;
			}
		}
		//lprintf( "at end... %d %d comp:%d", state->n, input->size, state->completed );
		if( input ) {
			if( state->n >= input->size ) {
				DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, input );
				if( state->gatheringString
					|| state->gatheringNumber
					|| state->parse_context == JSOX_CONTEXT_OBJECT_FIELD
					|| state->word == JSOX_WORD_POS_FIELD ) {
					//lprintf( "output is still incomplete? " );
					PrequeLink( state->outQueue, output );
					retval = 0;
				}
				else {
					PushLink( state->outBuffers, output );
					if( state->parse_context == JSOX_CONTEXT_UNKNOWN
					  && ( state->val.value_type != JSOX_VALUE_UNSET
					     || state->elements[0]->Cnt ) ) {
						if( state->word == JSOX_WORD_POS_END ) {
							state->word = JSOX_WORD_POS_RESET;
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
		if( state->val.value_type != JSOX_VALUE_UNSET ) {
			pushValue( state, state->elements, &state->val );
			JSOX_RESET_STATE_VAL();
		}
		state->completed = FALSE;
	}
	return retval;
}

PDATALIST jsox_parse_get_data( struct jsox_parse_state *state ) {
	PDATALIST *result = state->elements;
	state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
	if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
	else state->elements[0]->Cnt = 0;
	return result[0];
}

const char *jsox_get_parse_buffer( struct jsox_parse_state *pState, const char *buf ) {
	int idx;
	PJSOX_PARSE_BUFFER buffer;
	for( idx = 0; ; idx-- )
		while( buffer = (PJSOX_PARSE_BUFFER)PeekLinkEx( pState->outBuffers, idx ) ) {
			if( !buffer ) break;
			if( ((uintptr_t)buf) >= ((uintptr_t)buffer->buf) && ((uintptr_t)buf) < ((uintptr_t)buffer->pos) )
				return buffer->buf;
		}
	lprintf( "FAILED TO FIND BUFFER TO RETURN" );
	return NULL;
}

void _jsox_dispose_message( PDATALIST *msg_data )
{
	struct jsox_value_container *val;
	INDEX idx;
	if( !msg_data ) return;
	DATA_FORALL( (*msg_data), idx, struct jsox_value_container*, val )
	{
		// names and string buffers for JSON parsed values in a single buffer
		// associated with the root message.
		//if( val->name ) Release( val->name );
		//if( val->string ) Release( val->string );
		if( val->contains )
			_jsox_dispose_message( val->_contains );
	}
	// quick method
	DeleteDataList( msg_data );
	DeleteFromSet( PDATALIST, jxpsd.dataLists, msg_data );
}

static uintptr_t jsox_FindDataList( void*p, uintptr_t psv ) {
	if( ((PPDATALIST)p)[0] == (PDATALIST)psv )
		return (uintptr_t)p;
	return 0;
}

void jsox_dispose_message( PDATALIST *msg_data ) {
	uintptr_t actual = ForAllInSet( PDATALIST, jxpsd.dataLists, jsox_FindDataList, (uintptr_t)msg_data[0] );
	_jsox_dispose_message( (PDATALIST*)actual );
	msg_data[0] = NULL;
}



void jsox_parse_clear_state( struct jsox_parse_state *state ) {
	if( state ) {
		PJSOX_PARSE_BUFFER buffer;
		while( buffer = (PJSOX_PARSE_BUFFER)PopLink( state->outBuffers ) ) {
			Deallocate( const char *, buffer->buf );
			DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
		}
		while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
			DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
		while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
			Deallocate( const char*, buffer->buf );
			DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
		}
		DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->inBuffers );
		//DeleteLinkQueue( &state->inBuffers );
		DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->outQueue );
		//DeleteLinkQueue( &state->outQueue );
		DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->outBuffers );
		//DeleteLinkStack( &state->outBuffers );
		{
			char *buf;
			INDEX idx;
			LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
				Deallocate( char*, buf );
			}
			DeleteFromSet( PLIST, jxpsd.listSet, state->outValBuffers );
			//DeleteList( &state->outValBuffers );
		}
		state->status = TRUE;
		state->parse_context = JSOX_CONTEXT_UNKNOWN;
		state->word = JSOX_WORD_POS_RESET;
		state->n = 0;
		state->col = 1;
		state->line = 1;
		state->gatheringString = FALSE;
		state->gatheringNumber = FALSE;
		{
			PDATALIST *result = state->elements;
			state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
			if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
			else state->elements[0]->Cnt = 0;
			//state->elements = CreateDataList( sizeof( state->val ) );
			jsox_dispose_message( result );
		}
	}
}

PTEXT jsox_parse_get_error( struct jsox_parse_state *state ) {
	if( !state )
		state = jxpsd.last_parse_state;
	if( !state )
		return NULL;
	if( state->pvtError ) {
		PTEXT error = VarTextGet( state->pvtError );
		return error;
	}
	return NULL;
}

void jsox_parse_dispose_state( struct jsox_parse_state **ppState ) {
	struct jsox_parse_state *state = (*ppState);
	struct jsox_parse_context *old_context;
	PJSOX_PARSE_BUFFER buffer;
	_jsox_dispose_message( state->elements );
	//DeleteDataList( &state->elements );
	while( buffer = (PJSOX_PARSE_BUFFER)PopLink( state->outBuffers ) ) {
		Deallocate( const char *, buffer->buf );
		DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
	}
	{
		char *buf;
		INDEX idx;
		LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
			Deallocate( char*, buf );
		}
		DeleteFromSet( PLIST, jxpsd.listSet, state->outValBuffers );
		//DeleteList( &state->outValBuffers );
	}
	while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
		DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
	while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
		Deallocate( const char*, buffer->buf );
		DeleteFromSet( JSOX_PARSE_BUFFER, jxpsd.parseBuffers, buffer );
	}
	DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->inBuffers );
	//DeleteLinkQueue( &state->inBuffers );
	DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->outQueue );
	//DeleteLinkQueue( &state->outQueue );
	DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->outBuffers );
	//DeleteLinkStack( &state->outBuffers );
	DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, state->context );

	while( (old_context = (struct jsox_parse_context *)PopLink( state->context_stack )) ) {
		//lprintf( "warning unclosed contexts...." );
		DeleteFromSet( JSOX_PARSE_CONTEXT, jxpsd.parseContexts, old_context );
	}
	if( state->context_stack )
		DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->context_stack );
		//DeleteLinkStack( &state->context_stack );
	DeleteFromSet( JSOX_PARSE_STATE, jxpsd.parseStates, state );
	//Deallocate( struct jsox_parse_state *, state );
	(*ppState) = NULL;
}

LOGICAL jsox_parse_message( const char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct jsox_parse_state *state = jsox_begin_parse();
	//static struct jsox_parse_state *_state;
	state->complete_at_end = TRUE;
	int result = jsox_parse_add_data( state, msg, msglen );
	if( jxpsd._state ) jsox_parse_dispose_state( &jxpsd._state );
	if( result > 0 ) {
		(*_msg_output) = jsox_parse_get_data( state );
		jxpsd._state = state;
		//jsox_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	jxpsd.last_parse_state = state;
	jxpsd._state = state;
	return FALSE;
}

struct jsox_parse_state *jsox_get_messge_parser( void ) {
	return jxpsd._state;
}

#undef GetUtfChar

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



