#define JSOX_PARSER_SOURCE
#define JSOX_PARSER_MAIN_SOURCE
#include <stdhdrs.h>
#define DEADSTART_ROOT_OPTION_THING
#include <deadstart.h>

#include <jsox_parser.h>

#include "jsox.h"

#include "unicode_non_identifiers.h"

//#define DEBUG_PARSING
//#define DEBUG_ARRAY_TYPE
//#define DEBUG_CHARACTER_PARSING
//#define DEBUG_CLASS_STATES
//#define DEBUG_STRING_LENGTH

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
#ifdef DEBUG_ARRAY_TYPE
	lprintf( "Init arrayType to -1");
#endif

	state->context_stack = GetFromSet( PLINKSTACK, &jxpsd.linkStacks );// NULL;
	if( state->context_stack[0] ) state->context_stack[0]->Top = 0;
	//state->first_token = TRUE;
	state->context = GetFromSet( JSOX_PARSE_CONTEXT, &state->parseContexts );
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
	if( !jxpsd.initialized ) {
		InitializeCriticalSec( &jxpsd.cs_states );
		jxpsd.initialized = TRUE;
	}
	EnterCriticalSec( &jxpsd.cs_states  );
	struct jsox_parse_state *state = GetFromSet( JSOX_PARSE_STATE, &jxpsd.parseStates );//New( struct json_parse_state );
	jsox_state_init( state );
	LeaveCriticalSec( &jxpsd.cs_states  );
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

#undef __GetUtfChar
#undef _zero


#define JSOX_BADUTF8 0xFFFFFFF
#define _2char(result,from) (((*from) += 2),( ( result & 0x1F ) << 6 ) | ( ( result & 0x3f00 )>>8))
#define _zero(result,from)  ((*from)++,JSOX_BADUTF8)
#define _gzero(result,from)  ((*from)++,0)
#define _3char(result,from) ( ((*from) += 3),( ( ( result & 0xF ) << 12 ) | ( ( result & 0x3F00 ) >> 2 ) | ( ( result & 0x3f0000 ) >> 16 )) )

#define _4char(result,from)  ( ((*from) += 4), ( ( ( result & 0x7 ) << 18 )     \
                        | ( ( result & 0x3F00 ) << 4 )   \
                        | ( ( result & 0x3f0000 ) >> 10 )    \
                        | ( ( result & 0x3f000000 ) >> 24 ) ) )

// load 4 bytes in a little endian way; might result in a 8 byte variable, but only 4 are valid.
#define get4Chars(p) ((((TEXTRUNE*) ((uintptr_t)(p) & ~(sizeof(uint32_t)-1)) )[0]  \
                >> (CHAR_BIT*((uintptr_t)(p) & (sizeof(uint32_t)-1))))             \
            | (( ((uintptr_t)(p)) & (sizeof(uint32_t)-1) )                          \
                ? (((TEXTRUNE*) ((uintptr_t)(p) & ~(sizeof(uint32_t)-1)) )[1]      \
                    << (CHAR_BIT*(4-((uintptr_t)(p) & (sizeof(uint32_t)-1)))))     \
                :(TEXTRUNE)0 ))

#define __GetUtfChar( result, from )           ((result = get4Chars(*from)),     \
        ( ( !(result & 0xFF) )    \
          ?_gzero(result,from)   \
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
	size_t nextN = ( *msg_input ) - msg;
	//int escape;
	//LOGICAL cr_escaped;
	TEXTRUNE c;
	//escape = 0;
	//cr_escaped = FALSE;
	while( ( ( n = nextN ), ( n < msglen ) )
		&& ( ( ( c = GetUtfChar( msg_input ) ) != JSOX_BADUTF8 )
			&& ( status >= 0 ) ) )
	{
		if( (nextN = msg_input[0] - msg ) > msglen ) {
			(msg_input[0]) = msg + n; // restore input position.
			return status;
		}
		(state->col)++;

		if( c == start_c ) {
			if( state->escape ) {
				if( state->cr_escaped ) { state->cr_escaped = FALSE; state->escape = FALSE; status = 1; break; }
				// otherwise, escaped quote, append it.
				( *mOut++ ) = c; state->escape = FALSE;
			}
			else {
				status = 1;
				break;
			}
		} else if( state->escape ) {
			if( state->unicodeWide ) {
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
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, "(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %" _size_f " (near %*.*s[%c]%s)", c, n
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
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %" _size_f " (near %*.*s[%c]%s)", c, n
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
			case 0x2028: // LS (Line separator)
			case 0x2029: // PS (paragraph separate)
				// escaped whitespace is nul'ed.
				state->escape = 0;
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
			case '0':
				( *mOut++ ) = '\0';
				break;
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
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, "(escaped character) fault while parsing; '%c' unexpected %" _size_f " (near %*.*s[%c]%s)", c, n
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
	enum jsox_parse_object_context_modes nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_NORMAL;
	PJSOX_CLASS cls = NULL;
	INDEX idx;
	//let tmpobj = {};
	if( state->word > JSOX_WORD_POS_RESET && state->word < JSOX_WORD_POS_FIELD ) {
		recoverIdent( state, output, -1 );
		if( !state->completedString ) {
			state->val.stringLen = output->pos - state->val.string;
		}
	}

	if( state->val.value_type == JSOX_VALUE_STRING ){
		if( state->parse_context == JSOX_CONTEXT_UNKNOWN )
			nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_FIELD;
		else
			nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_VALUE;
#ifdef DEBUG_CLASS_STATES
		lprintf( "Setting classname HERE OPenObject:%s", state->val.string );
#endif
		state->val.className = state->val.string;
		state->val.classNameLen = state->val.stringLen;
		state->val.string = NULL;
		state->val.value_type = JSOX_VALUE_UNSET;
		state->word = JSOX_WORD_POS_RESET;
	} else {
#ifdef DEBUG_CLASS_STATES
		lprintf( "Resetting classname HERE OPenObject" );
#endif
		state->val.className = NULL;
		state->val.classNameLen = 0;
		if( state->val.value_type != JSOX_VALUE_UNSET
			&& !( state->val.value_type == JSOX_VALUE_OBJECT && state->val.className )
			) {
			lprintf( "Unhandled value type preceeding object open: %d %s", state->val.value_type, state->val.string );	
		}
	}

	if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {

		nextMode = JSOX_CONTEXT_OBJECT_FIELD;
		//state->word = JSOX_WORD_POS_FIELD;

		if( state->val.className ) {
#ifdef DEBUG_PARSING
			lprintf( "define class: %*.*s", state->val.stringLen, state->val.stringLen, state->val.className );
#endif
			nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_FIELD;
			LIST_FORALL( state->classes, idx, PJSOX_CLASS, cls )
				if( memcmp( cls->name, state->val.className, state->val.classNameLen ) == 0 )
					break;

			if( !cls ) {
				cls = GetFromSet( JSOX_CLASS, &state->classPool );
				cls->name = state->val.className;
				cls->nameLen = state->val.stringLen;
				cls->fields = NULL;
#ifdef DEBUG_CLASS_STATES
				lprintf( "CREATING CLASS: %s", cls->name );
#endif
				AddLink( &state->classes, cls );
			} else {
				if( state->allowRedefinition ) {
					struct jsox_class_field* field;
					LIST_FORALL( cls->fields, idx, struct jsox_class_field*, field ) {
						DeleteFromSet( JSOX_CLASS_FIELD, state->class_fields, field );
					}
#ifdef DEBUG_CLASS_STATES
					lprintf( "RESETTING CLASS: %s", cls->name );
#endif
					DeleteList( &cls->fields );
					// now can re-define fields.
				}
				else {
					nextMode = JSOX_CONTEXT_OBJECT_FIELD; // no change to next...
					if( cls )
						// no colons, on comma push value.
						// maybe allow colons for extra values that aren't part of the pre-type
						nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_VALUE;
					else
						// this is a tagged class for later prototype revival.
						nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_NORMAL;
				}
			}
		}
		else {
		}
		state->word = JSOX_WORD_POS_RESET;
	} else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) {
		if( state->val.className ) { // this is a tagged object open.
			if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_VALUE ) {
				LIST_FORALL( state->classes, idx, PJSOX_CLASS, cls )
					if( memcmp( cls->name, state->val.className, state->val.classNameLen ) == 0 )
						break;
				// prototype classes don't have to have definitions
				// which the parser needs to know about.
				// but then it's got a className and is just a normal object otherwise.
				nextMode = JSOX_CONTEXT_OBJECT_FIELD; // no change to next...
				if( cls )
					// no colons, on comma push value.
					// maybe allow colons for extra values that aren't part of the pre-type
					nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_VALUE;
				else
					// this is a tagged class for later prototype revival.
					nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_NORMAL;
			} else {
				if( !state->pvtError ) state->pvtError = VarTextCreate();
				vtprintf( state->pvtError, "Fault while parsing; object open class field definition : (duplicate colon)" " '%c' at %" _size_f " %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
				state->status = FALSE;
				return FALSE;
			}
		} else {
			nextMode = JSOX_CONTEXT_OBJECT_FIELD;
			nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_NORMAL;
			// tis is just a simple object value in the field of a class type.
		}
	} else if( state->parse_context == JSOX_CONTEXT_IN_ARRAY
	         || state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE
	         ) {
		if( state->val.className ) {
			// a string value opening a class makes for having a className
			LIST_FORALL( state->classes, idx, PJSOX_CLASS, cls )
				if( memcmp( cls->name, state->val.className, state->val.classNameLen ) == 0 )
					break;
			// it's ok if it's a class type that's not known internally.  This should be returned to
			// the next assembling layer.
			//if( !cls ) lprintf( "Referenced class %s has not been defined", state->val.string );

			nextObjectMode = JSOX_OBJECT_CONTEXT_CLASS_VALUE;

		}
		else {
		}

		nextMode = JSOX_CONTEXT_OBJECT_FIELD;
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
		struct jsox_parse_context *old_context = GetFromSet( JSOX_PARSE_CONTEXT, &state->parseContexts );
#ifdef DEBUG_PARSING
		lprintf( "Begin a new object; previously pushed into elements; but wait until trailing comma or close previously:%d", state->val.value_type );
#endif
		old_context->context = state->parse_context;
		old_context->elements = state->elements;
		old_context->name = state->val.name;
		old_context->nameLen = state->val.nameLen;
		old_context->className = state->val.className;
		old_context->classNameLen = state->val.classNameLen;
		old_context->current_class = state->current_class;
		old_context->objectContext = state->objectContext;
#ifdef DEBUG_CLASS_STATES
		lprintf( "object Push class: %s %s", state->val.className, state->current_class?state->current_class->name:"");
#endif
		old_context->current_class_item = state->current_class_item;
#ifdef DEBUG_ARRAY_TYPE
		lprintf( "open object save arrayType:%d",state->arrayType );
#endif
		old_context->arrayType = state->arrayType;
		state->arrayType = -1;
		state->current_class = cls;
#ifdef DEBUG_CLASS_STATES
		lprintf( "set current class: %s", cls?cls->name:"<none>" );
#endif
		state->current_class_item = 0;
		EnterCriticalSec( &jxpsd.cs_states );
		state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
		LeaveCriticalSec( &jxpsd.cs_states );
		if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
		else state->elements[0]->Cnt = 0;
		PushLink( state->context_stack, old_context );
		JSOX_RESET_STATE_VAL();
		state->parse_context = nextMode;
		state->objectContext = nextObjectMode;
	}
	return TRUE;
}

#ifdef _MSC_VER
// this is about nLeast being uninitialized.
// LIST_FORALL initilaizes typeIndex.
// knownArrayTypeNames is always NOT NULL.
#  pragma warning( disable: 6001 )
#endif

static LOGICAL openArray( struct jsox_parse_state *state, struct jsox_output_buffer* output, int c ) {
	PJSOX_CLASS cls = NULL;
	int newArrayType = -1;
	if( state->word == JSOX_WORD_POS_FIELD ) {
		if( !state->pvtError ) state->pvtError = VarTextCreate();
		vtprintf( state->pvtError, "Fault while parsing; colon expected after field name %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
		state->status = FALSE;
		return FALSE;
        }

	if( state->val.value_type == JSOX_VALUE_STRING ) {
		state->val.className = state->val.string;
#ifdef DEBUG_CLASS_STATES
		lprintf( "SET class: %.*s %d", state->val.stringLen, state->val.className, state->val.stringLen );
#endif
		state->val.classNameLen = state->val.stringLen;
		state->val.string = NULL;
		state->val.value_type = JSOX_VALUE_UNSET;
		state->word = JSOX_WORD_POS_RESET;
	}
	else {
		if( state->val.value_type != JSOX_VALUE_UNSET ) {
			lprintf( "Unhandled value type preceeding object open: %d %s", state->val.value_type, state->val.string );
		}
	}

	if( state->val.className ) {
		char *name;
		INDEX typeIndex;// = knownArrayTypeNames.findIndex( type = > (type == = val.string) );
		(*output->pos++) = 0;
#ifdef DEBUG_PARSING
		lprintf( "define typed array:%s", state->val.className );
#endif
		if( !knownArrayTypeNames ) registerKnownArrayTypeNames();
		typeIndex = 0;
		LIST_FORALL( knownArrayTypeNames, typeIndex, char *, name )
			if( memcmp( state->val.className, name, state->val.classNameLen ) == 0 )
				break;
		if( typeIndex < 13 ) {
			state->word = JSOX_WORD_POS_FIELD;
			newArrayType = (int)typeIndex;
#ifdef DEBUG_PARSING
			lprintf( "setup array type... %d", typeIndex );
#endif
			// collect the next part as base64 data.
			state->val.string = output->pos;
		}
		else {
			// otherwise, the type name is saved with the array
			// and the higher level processor can fix it.
		}
	} else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) {
		if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_VALUE ) {
		}else {
			if( !state->pvtError ) state->pvtError = VarTextCreate();
			vtprintf( state->pvtError, "Fault while parsing; while getting field name unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
			vtprintf( state->pvtError, "\nError: word state: %d %d", state->parse_context, state->objectContext );
			state->status = FALSE;
			return FALSE;
		}
	}
	{
		struct jsox_parse_context *old_context = GetFromSet( JSOX_PARSE_CONTEXT, &state->parseContexts );
#ifdef DEBUG_PARSING
		lprintf( "Begin a new array; previously pushed into elements; but wait until trailing comma or close previously:%d", state->val.value_type );
#endif
		old_context->context = state->parse_context;
		old_context->elements = state->elements;
		old_context->name = state->val.name;
		old_context->nameLen = state->val.nameLen;
		old_context->className = state->val.className;
		old_context->classNameLen = state->val.classNameLen;
		old_context->objectContext = state->objectContext;
#ifdef DEBUG_CLASS_STATES
		lprintf( "Push2T class: %s", state->val.className );
#endif
		old_context->current_class = state->current_class;
		old_context->current_class_item = state->current_class_item;
#ifdef DEBUG_ARRAY_TYPE
		lprintf( "open array save arrayType:%d to %d", state->arrayType, newArrayType );
#endif		
		old_context->arrayType = state->arrayType;
		state->current_class = cls;
#ifdef DEBUG_CLASS_STATES
		lprintf( "set current class: %s", cls ? cls->name : "<none>" );
#endif
		state->current_class_item = 0;
		state->arrayType = newArrayType;
		EnterCriticalSec( &jxpsd.cs_states );
		state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
		LeaveCriticalSec( &jxpsd.cs_states );
		if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
		else state->elements[0]->Cnt = 0;
		PushLink( state->context_stack, old_context );

		JSOX_RESET_STATE_VAL();
		state->parse_context = JSOX_CONTEXT_IN_ARRAY;
		state->word = JSOX_WORD_POS_RESET;
	}
	return TRUE;
}
#ifdef _MSC_VER
// restore warning...
#  pragma warning( default: 6001 )
#endif

int recoverIdent( struct jsox_parse_state *state, struct jsox_output_buffer* output, int cInt ) {
	if( state->word == JSOX_WORD_POS_FIELD && cInt == ':' ) return 0;
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
				lprintf( "FAULT: UNEXPECTED VALUE TYPE RECOVERING IDENT:%d", state->val.value_type );
				break;
			case JSOX_VALUE_UNSET:
			case JSOX_VALUE_STRING:
				// this type is already in the right place.
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
	state->word = JSOX_WORD_POS_RESET; // but value is not UNSET.
	state->negative = FALSE;
	if( state->val.string && state->val.value_type == JSOX_VALUE_UNSET ) {
		state->val.value_type = JSOX_VALUE_STRING;
		state->completedString = FALSE;
	}

	if( cInt == 123/*'{'*/ )
		openObject( state, output, cInt );
	else if( cInt == 91/*'['*/ )
		openArray( state, output, 0 );
	else if( cInt == 58/*':'*/ ) {
		state->word = JSOX_WORD_POS_RESET; // well.. it is.  It's already a fairly commited value.
		if( !state->completedString ) {
			state->completedString = TRUE;
			state->val.stringLen = output->pos - state->val.string;
		}
#ifdef DEBUG_STRING_LENGTH
			lprintf( "Update stringLen  '%c'  :%d", cInt?cInt:'?', state->val.stringLen );
#endif
	} else if( cInt == 44/*','*/ ) {
		state->word = JSOX_WORD_POS_RESET; // well.. it is.  It's already a fairly commited value.
		if( !state->completedString ) {
			state->completedString = TRUE;
			state->val.stringLen = output->pos - state->val.string;
		}
#ifdef DEBUG_STRING_LENGTH
		lprintf( "Update stringLen  '%c'  :%d", cInt?cInt:'~', state->val.stringLen );
#endif
	} else if( cInt >= 0 ) {
		// ignore white space.
		if( cInt == 32/*' '*/ ||cInt==160/*nbsp*/|| cInt == 13 || cInt == 10 || cInt == 9 || cInt == 0xFEFF || cInt == 0x2028 || cInt == 0x2029 ) {
			state->word = JSOX_WORD_POS_END;
			if( !state->completedString ) {
				state->completedString = TRUE;
				state->val.stringLen = output->pos - state->val.string;
			}
#ifdef DEBUG_STRING_LENGTH
			lprintf( "Update stringLen  '%c'  :%d", cInt, state->val.stringLen );
#endif
			return 0;
		}

		if( cInt == '"' || cInt == '\'' || cInt == '`' ) {
			if( !state->val.string )  state->val.string = output->pos;
			state->val.value_type = JSOX_VALUE_STRING;
			if( !state->val.className ) {
				state->val.className = state->val.string;
				state->val.classNameLen = state->val.stringLen;
				state->val.string = output->pos;
				state->val.stringLen = 0;
				state->gatheringStringFirstChar = cInt;
				state->gatheringString = TRUE;
			}
		}
		else if( cInt == 44/*','*/ || cInt == 125/*'}'*/ || cInt == 93/*']'*/ || cInt == 58/*':'*/ )
			vtprintf( state->pvtError, "invalid character; unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, cInt, state->n, state->line, state->col );
		else {
			if( !state->val.string )  state->val.string = output->pos;
			state->val.value_type = JSOX_VALUE_STRING;
			state->word = JSOX_WORD_POS_END;
			if( cInt < 128 ) (*output->pos++) = cInt;
			else output->pos += ConvertToUTF8( output->pos, cInt );
#ifdef DEBUG_PARSING
			lprintf( "Collected .. %d %c  %*.*s", cInt, cInt, output->pos - state->val.string, output->pos - state->val.string, state->val.string );
#endif
			state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_STRING_LENGTH
			lprintf( "Update stringLen  '%c'  :%d", cInt, state->val.stringLen );
#endif
		}
	}

	return 0;
}

static void pushValue( struct jsox_parse_state *state, PDATALIST *pdl, struct jsox_value_container *val, int line ) {
#define pushValue(a,b,c) pushValue(a,b,c,__LINE__)
#ifdef DEBUG_PARSING
	lprintf( "pushValue:%p %d %d", val->contains, val->value_type, state->arrayType );
	if( val->name )
		lprintf( "push named:%*.*s %d", val->nameLen, val->nameLen, val->name, line );
#endif
	if( val->value_type == JSOX_VALUE_UNSET ) return; // no value to push.
	if( val->value_type >= JSOX_VALUE_TYPED_ARRAY && val->value_type <= JSOX_VALUE_TYPED_ARRAY_MAX ) {
		
		//lprintf( "Setting value type as JSOX_VALUD_TYPED_ARRAY+%d",val->value_type - JSOX_VALUE_TYPED_ARRAY );
		if( (val->value_type - JSOX_VALUE_TYPED_ARRAY) >= 0 && (val->value_type - JSOX_VALUE_TYPED_ARRAY) < 12 ) {
			struct jsox_value_container *innerVal = (struct jsox_value_container *)GetDataItem( &val->contains, 0 );
			val->string = (char*)DecodeBase64Ex( innerVal->string, innerVal->stringLen, &val->stringLen, NULL );
		}
	}
	AddDataItem( pdl, val );
#ifdef DEBUG_CLASS_STATES
	lprintf( "RESET CLASS NAME %.*s", val->classNameLen, val->className );
#endif
	val->className = NULL;
	val->classNameLen = 0;
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
		if( input = (PJSOX_PARSE_BUFFER)PeekQueue( state->inBuffers[0] ) ) {
			size_t used = input->pos - input->buf;
			size_t unused = input->size - used;
			if( input->tempBuf || ( unused < 6 ) ) {
				const char *newBuf = NewArray( const char, unused + msglen );
				memcpy( (char*)newBuf, input->pos, unused );
				memcpy( (char*)newBuf + unused, msg, msglen );
				if( input->tempBuf )
					Deallocate( CPOINTER, input->buf );
				input->pos = input->buf = newBuf;
				input->size = unused+msglen;
				input->tempBuf = TRUE;
			}
		}
		// no input; or this buffer wasn't appended to the previous buffer...
		if( !input || !input->tempBuf )
		{
			input = GetFromSet( JSOX_PARSE_BUFFER, &state->parseBuffers );
			input->pos = input->buf = msg;
			input->size = msglen;
			input->tempBuf = FALSE;
			EnqueLinkNL( state->inBuffers, input );
		}
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
			output = (struct jsox_output_buffer*)GetFromSet( JSOX_PARSE_BUFFER, &state->parseBuffers );
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
			if( state->numberFromDate ) {
				state->val.stringLen = ( output->pos - state->val.string );
				state->val.value_type = JSOX_VALUE_DATE;
			} else if( state->numberFromBigInt ) {
				state->val.stringLen = ( output->pos - state->val.string );
				state->val.value_type = JSOX_VALUE_BIGINT;
			} else {
				if( state->val.float_result ) {
					CTEXTSTR endpos;
					state->val.result_d = FloatCreateFromText( state->val.string, &endpos );
					if( state->negative ) { state->val.result_d = -state->val.result_d; state->negative = FALSE; }
				}
				else {
					state->val.result_n = IntCreateFromText( state->val.string );
					if( state->negative ) { state->val.result_n = -state->val.result_n; state->negative = FALSE; }
				}
				state->val.value_type = JSOX_VALUE_NUMBER;
			}
			if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
				state->completed = TRUE;
			}
			else { // this is a flush; and there's still an open object, array, etc.
				if( !state->pvtError ) state->pvtError = VarTextCreate();
				vtprintf( state->pvtError, "Fault while parsing; unexpected end of stream at %" _size_f "  %" _size_f ":%" _size_f, state->n, state->line, state->col );
				state->status = FALSE;
				return -1;
			}
			retval = 1;
		}
	}

	while( state->status && ( input = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) ) ) {
		size_t newN;
		output = (struct jsox_output_buffer*)DequeLinkNL( state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;
		if( state->n > input->size ) DebugBreak();
	gatherStringInput:
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
#ifdef DEBUG_STRING_LENGTH
				lprintf( "Update stringLen  collcting string :%d", state->val.stringLen );
#endif
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
				if( state->n > input->size ) {
					DebugBreak();
				}
			}
		}
		if( state->gatheringNumber ) {
			//lprintf( "continue gathering a string" );
			goto continueNumber;
		}

		//lprintf( "Completed at start?%d", state->completed );
		while( state->status && (state->n < input->size) && ( (c = GetUtfChar( &input->pos ))!= JSOX_BADUTF8) )
		{
#if defined( DEBUG_PARSING ) && defined( DEBUG_CHARACTER_PARSING )
			lprintf( "parse character %c %d %d %d %d", c<32?'.':c, state->word, state->parse_context, state->val.value_type, state->word );
#endif
			state->col++;
			newN = input->pos - input->buf;
			if( newN > input->size ) {
				// partial utf8 character across buffer boundaries.
				//DebugBreak();
				break;
			}
			state->n = newN;
			if( state->comment ) {
				if( state->comment == 1 ) {
					if( c == '*' ) { state->comment = 3; continue; }
					if( c != '/' ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "Fault while parsing; unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
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
			if( c > 127 ) {
				// irrelativent.
				if( c == 0x2028 || c == 0x2029 || c == 0xfeff ) {
					goto whitespace;
				}
				if( !state->val.string )  state->val.string = output->pos;
				state->val.value_type = JSOX_VALUE_STRING;
				output->pos += ConvertToUTF8( output->pos, c );
#ifdef DEBUG_PARSING
				lprintf( "Collected .. %d %c  %*.*s", c, c, output->pos - state->val.string, output->pos - state->val.string, state->val.string );
#endif
				state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_STRING_LENGTH
				lprintf( "Update stringLen  unicode character:%d", state->val.stringLen );
#endif
			}
			else switch( c )
			{
			case '#':
				if( !state->comment ) state->comment = 2; // pretend this is the second slash.
				break;
			case '/':
				if( !state->comment ) state->comment = 1;
				break;
			case '{':
				openObject( state, output, c );
				break;

			case '[':
				recoverIdent(state,output,c);  // gather any preceeding string.
				//openArray( state, output, c );
				break;

			case ':':
				recoverIdent(state,output,c);
				if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD )
				{
					if( state->val.stringLen == 0 ) {
						// allow starting a new word
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "unquoted keyword used as object field name:parsing fault; unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
						break;
					}
					if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_FIELD ) {
						if( GetLinkCount( state->current_class->fields ) == 0 ) {
							DeleteLink( &state->classes, state->current_class );
							DeleteFromSet( JSOX_CLASS, &state->classPool, state->current_class );
							state->current_class = NULL;
							state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_NORMAL;
						}
						// need to establish whether this is a tag definition state
						// or tagged-revival revival, or just a prototyped object that's not a tag-revival.
						//lprintf( "This would be a default value for the object... if I ignore this..." );
					}
					state->word = JSOX_WORD_POS_RESET;
					state->val.name = state->val.string;
					state->val.nameLen = state->val.stringLen;
					state->val.string = NULL;
					state->val.stringLen = 0;
					// classname will later indicate this was a class...
					// this can no longer be a prototype definition (class_field)
					// but if it's a value, we want to stay that we're collecting class values.
					state->parse_context = JSOX_CONTEXT_OBJECT_FIELD_VALUE;
					state->val.value_type = JSOX_VALUE_UNSET;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					if( state->parse_context == JSOX_CONTEXT_IN_ARRAY )
						vtprintf( state->pvtError, "(in array, got colon out of string):parsing fault; unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
					else
						vtprintf( state->pvtError, "(outside any object, got colon out of string):parsing fault; unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
					state->status = FALSE;
				}
				break;
			case '}':
				{
					int emitObject;
					emitObject = TRUE;
					if( state->word == JSOX_WORD_POS_AFTER_FIELD ) {
						state->word = JSOX_WORD_POS_RESET;
					}
					if( state->word == JSOX_WORD_POS_END ) {
						// allow starting a new word
						state->word = JSOX_WORD_POS_RESET;
					}

					if( ( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) ) {

						if( state->current_class ) {
							if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_FIELD ) {
								// allow blank comma at end to not be a field
								if( state->val.string ) {
									struct jsox_class_field* field = GetFromSet( JSOX_CLASS_FIELD, &state->class_fields );

									field->name = state->val.string;
									field->nameLen = output->pos - state->val.string;
									( *output->pos++ ) = 0;
									state->val.string = NULL;
#ifdef DEBUG_CLASS_STATES
									lprintf( "ClosingObject Adding field to class %p %s %p", state->current_class, state->current_class->name, state->current_class->fields );
#endif
									AddLink( &state->current_class->fields, field );
									//AddLink( &state->current_class->fields, state->val.string );
								}
								JSOX_RESET_STATE_VAL();
								emitObject = FALSE;
								state->word = JSOX_WORD_POS_RESET;
								lprintf( "closing in context...");
								break;
							}
							else if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_VALUE ) {
								lprintf( "class value" );
								if( state->val.value_type != JSOX_VALUE_UNSET ) {
									if( state->current_class->fields ) {
										if( !state->val.name ) {
											struct jsox_class_field* field = ( struct jsox_class_field* )GetLink( &state->current_class->fields, state->current_class_item++ );
											state->val.name = field->name;
											state->val.nameLen = field->nameLen;
										}
									}
									else {
										if( !state->val.name ) {
											vtprintf( state->pvtError, "State error; class fields, class has no fields, and one was needed; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f
												, c, state->n, state->line, state->col );
											state->status = FALSE;
											break;
										}
									}
								} // otherwise it's just a close after an empty comma...
							}
						}
						pushValue( state, state->elements, &state->val );

						//if( _DEBUG_PARSING ) lprintf( "close object; empty object", val, elements );
					}
					else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE ) {
						// current class doesn't really matter... we only get here after a ':'
						// which will have faulted or not...
						// this is just push the value.
						if( state->val.value_type != JSOX_VALUE_UNSET )
							pushValue( state, state->elements, &state->val );
						else {
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, "Fault while parsing(2); unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
							state->status = FALSE;
						}
					}
					else {
						if( state->val.value_type != JSOX_VALUE_UNSET ) {
							//pushValue( state, state->elements, &state->val );
						} else {
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, "Fault while parsing(3); unexpected %c at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
							state->status = FALSE;
							break;
						}
					}

					{
						struct jsox_parse_context* old_context = ( struct jsox_parse_context* )PopLink( state->context_stack );
						//if( _DEBUG_PARSING_STACK ) console.log( "object pop stack (close obj)", context_stack.length, old_context );

						state->val.contains = state->elements[0];
						state->val._contains = state->elements;

						state->parse_context = old_context->context; // this will restore as IN_ARRAY or OBJECT_FIELD

						// lose current class in the pop; have; have to report completed status before popping.
						if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
							if( !state->current_class )
								state->completed = TRUE;
						}

						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->val.className = old_context->className;
						state->val.classNameLen = old_context->classNameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->arrayType = old_context->arrayType;
#ifdef DEBUG_ARRAY_TYPE
						lprintf( "close object restore arrayType:%d",state->arrayType );
#endif
						state->objectContext = old_context->objectContext;
#ifdef DEBUG_CLASS_STATES
						lprintf( "POP CLASS NAME %s %s %p", state->val.className, state->current_class ? state->current_class->name : "", state->current_class ? state->current_class->fields : 0 );
#endif
						DeleteFromSet( JSOX_PARSE_CONTEXT, state->parseContexts, old_context );

						if( emitObject )
							state->val.value_type = JSOX_VALUE_OBJECT;
						else
							state->val.value_type = JSOX_VALUE_UNSET;
						state->val.string = NULL;

					}
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
#ifdef DEBUG_STRING_LENGTH
								lprintf( "Update stringLen  close array :%d", state->val.stringLen );
#endif
#ifdef DEBUG_PARSING
								lprintf( "STRING3: %s %d", state->val.string, state->val.stringLen );
#endif
								(*output->pos++) = 0;
							}
						}
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
					}

					if( state->arrayType >= 0 ) {
						state->val.value_type = (enum jsox_value_types)(JSOX_VALUE_TYPED_ARRAY + state->arrayType);
					}
					else
						state->val.value_type = JSOX_VALUE_ARRAY;

					//state->val.string = NULL;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					if( state->current_class )
						state->val.className = state->current_class->name;

					{
						struct jsox_parse_context *old_context = (struct jsox_parse_context *)PopLink( state->context_stack );
						//struct jsox_value_container *oldVal = (struct jsox_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						state->val.className = old_context->className;
						state->val.classNameLen = old_context->classNameLen;
						state->current_class = old_context->current_class;
						state->current_class_item = old_context->current_class_item;
						state->objectContext = old_context->objectContext;
#ifdef DEBUG_CLASS_STATES
						lprintf( "POP2 CLASS NAME %s %s %p", state->val.className, state->current_class ? state->current_class->name : "", state->current_class ? state->current_class->fields : 0 );
#endif
						state->arrayType = old_context->arrayType;
#ifdef DEBUG_ARRAY_TYPE
						lprintf( "close array restore arrayType:%d",state->arrayType );
#endif
						DeleteFromSet( JSOX_PARSE_CONTEXT, state->parseContexts, old_context );
					}
					if( state->parse_context == JSOX_CONTEXT_UNKNOWN ) {
						state->completed = TRUE;
					}
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, "bad context %d; fault while parsing; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, state->parse_context, c, state->n, state->line, state->col );// fault
					state->status = FALSE;
				}
				break;
			case ',':
				if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD ) {
					if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_VALUE ) {
						if( state->val.value_type != JSOX_VALUE_UNSET ) {
							// revive class value, get name from class definition
							if( state->current_class->fields ) {
								struct jsox_class_field *field = (struct jsox_class_field *)GetLink( &state->current_class->fields, state->current_class_item++ );
								state->val.name = field->name;
								state->val.nameLen = field->nameLen;
							}
							else if( !state->val.name ) {
								if( !state->pvtError ) state->pvtError = VarTextCreate();
								vtprintf( state->pvtError, "class field has no matching field definitions; fault while parsing; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, state->parse_context, c, state->n, state->line, state->col );// fault
								state->status = FALSE;
								break;
							}
							pushValue( state, state->elements, &state->val );

							JSOX_RESET_STATE_VAL();
							//state->parse_context = JSOX_CONTEXT_CLASS_FIELD;
							state->word = JSOX_WORD_POS_RESET;
						}
					}

					else if( state->objectContext == JSOX_OBJECT_CONTEXT_CLASS_FIELD ) {
						lprintf( "Adding defined name..." );
						if( state->current_class ) {
							struct jsox_class_field *field = GetFromSet( JSOX_CLASS_FIELD, &state->class_fields );
							field->name = state->val.string;
							field->nameLen = output->pos - state->val.string;
							(*output->pos++) = 0;
							state->val.string = NULL;
							state->val.value_type = JSOX_VALUE_UNSET;
#ifdef DEBUG_CLASS_STATES
							lprintf( "Comma Adding field to class %p %s %p", state->current_class, state->current_class->name, state->current_class->fields );
#endif
							AddLink( &state->current_class->fields, field );
							state->word = JSOX_WORD_POS_RESET;
						}
						else {
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, "lost class definition; fault while parsing; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, state->parse_context, c, state->n, state->line, state->col );// fault
							state->status = FALSE;
						}
					}
					// empty comma , field,field... woudlbe ,,,, '
				}
				else if( state->parse_context == JSOX_CONTEXT_IN_ARRAY ) {
					if( state->val.value_type == JSOX_VALUE_UNSET )
						state->val.value_type = JSOX_VALUE_EMPTY; // in an array, elements after a comma should init as undefined...
																 // undefined allows [,,,] to be 4 values and [1,2,3,] to be 4 values with an undefined at end.
					if( state->val.value_type != JSOX_VALUE_UNSET ) {
#ifdef DEBUG_PARSING
						lprintf( "back in array; push item %d", state->val.value_type );
#endif
						pushValue( state, state->elements, &state->val );
						JSOX_RESET_STATE_VAL();
						state->word = JSOX_WORD_POS_RESET;
					}
				}
				else if( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD_VALUE ) {
					// after an array value, it will have returned to OBJECT_FIELD anyway
					if( state->word > JSOX_WORD_POS_END
						&& state->word < JSOX_WORD_POS_AFTER_FIELD )
						recoverIdent( state, output, c );
#ifdef DEBUG_PARSING
					lprintf( "comma after field value, push field to object: %s", state->val.name );
#endif
					state->parse_context = JSOX_CONTEXT_OBJECT_FIELD;
					state->word = JSOX_WORD_POS_RESET;
					if( state->val.value_type != JSOX_VALUE_UNSET )
						pushValue( state, state->elements, &state->val );
					else {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "missing value for object field; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
						state->status = FALSE;
						break;
					}
					JSOX_RESET_STATE_VAL();
				}
				else {
					state->status = FALSE;
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, "bad context; fault while parsing; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );// fault
				}
				break;

			default:
				if( state->val.value_type == JSOX_VALUE_STRING && !state->completedString ) {
					// already faulted to a string?
					if( c == '\'' || c == '\"' || c == '`' ) {
#ifdef DEBUG_CLASS_STATES
						lprintf( "Setting classname for string here... %d %.*s", state->val.stringLen, state->val.stringLen, state->val.string );
#endif
						state->val.className = state->val.string;
						state->val.classNameLen = state->val.stringLen;
						state->val.string = output->pos;
						state->val.stringLen = 0;
						state->gatheringString = TRUE;
						state->gatheringStringFirstChar = c;
						goto gatherStringInput;
					}
					if( c == 32/*' '*/ || c == 160/*nbsp*/ || c == 13 || c == 10 || c == 9 || c == 0xFEFF || c == 0x2028 || c == 0x2029 ) {
						state->word = JSOX_WORD_POS_AFTER_FIELD;
						break;
					}
					if( state->word == JSOX_WORD_POS_AFTER_FIELD ) {
						if( state->val.className ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, "unquoted spaces between stings; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );// fault
							break;
						}
						else {
							if( !state->current_class ) {
								state->status = FALSE;
								if( !state->pvtError ) state->pvtError = VarTextCreate();
								vtprintf( state->pvtError, "?? This is like doublestring revival; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );// fault
								break;
							}
							lprintf( "THIS IS WRONG!" );
							lprintf( "STRING-STRING?" );
							state->val.className = state->val.string;
							state->val.classNameLen = state->val.stringLen;
							state->val.string = NULL;
							state->val.stringLen = 0;
							state->word = JSOX_WORD_POS_RESET;
							break;
						}
					}
					if( c < 128 ) ( *output->pos++ ) = c;
					else output->pos += ConvertToUTF8( output->pos, c );
					state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_STRING_LENGTH
					lprintf( "Update stringLen  already an ident %c :%d", c, state->val.stringLen );
#endif
					break;
				}

				if( ( state->parse_context == JSOX_CONTEXT_OBJECT_FIELD  && state->objectContext != JSOX_OBJECT_CONTEXT_CLASS_VALUE )
				) {
#ifdef DEBUG_PARSING
					if( state->val.string )
						lprintf( "gathering object field:%c  %d %.*s", c, output->pos- state->val.string, output->pos - state->val.string, state->val.string );
					else
						lprintf( "Gathering, but no string yet? %c", c );
#endif
					switch( c )
					{
					case '`':
						// this should be a special case that passes continuation to gatherString
						// but gatherString now just gathers all strings
					case '"':
					case '\'':
						if( state->val.value_type == JSOX_VALUE_STRING && state->val.className ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, "too many strings in a row; fault while parsing; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );// fault
							break;
						}
						if( state->word == JSOX_WORD_POS_FIELD
							|| ( state->val.value_type == JSOX_VALUE_STRING
								&& !state->val.className ) ) {
							(*output->pos++) = 0;
#ifdef DEBUG_PARSING
							lprintf( "Promoting previous string to... what? %d %.*s", state->val.stringLen, state->val.stringLen, state->val.string );
#endif
							state->val.className = state->val.string;
							state->val.classNameLen = state->val.stringLen;
#ifdef DEBUG_CLASS_STATES
							lprintf( "Setting classname HERE (why?):", state->val.string );
#endif
						}
#ifdef DEBUG_PARSING
						else {
							lprintf( "Was there already a string in progress?");
						}
#endif
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
#ifdef DEBUG_STRING_LENGTH
							lprintf( "Update stringLen  collcting string :%d", state->val.stringLen );
#endif
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
					case 160 :// case '\xa0': // nbsp
					case '\t':
					case '\r':
					case 0x2028: // LS (Line separator)
					case 0x2029: // PS (paragraph separate)
					case 0xFEFF: // ZWNBS is WS though
					whitespace:
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
							}
						}
						else {
							//state->status = FALSE;
							//if( !state->pvtError ) state->pvtError = VarTextCreate();
							//vtprintf( state->pvtError, "fault while parsing; whitespace unexpected at %" _size_f "  %" _size_f ":%" _size_f, state->n, state->line, state->col );	// fault
						}
						break;
					default:
						if( state->word == JSOX_WORD_POS_RESET && ( (c >= '0' && c <= '9') || (c == '+') || (c == '.') ) ) {
							goto beginNumber;
						}
						if( state->word == JSOX_WORD_POS_AFTER_FIELD ) {
							state->status = FALSE;
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							//lprintf( "Val is:%s", state->val.string );
							vtprintf( state->pvtError, "fault while parsing; second string in field name at %" _size_f "  %" _size_f ":%" _size_f, state->n, state->line, state->col );	// fault
							break;
						} else if( state->word == JSOX_WORD_POS_RESET ) {
							state->word = JSOX_WORD_POS_FIELD;
							state->val.string = output->pos;
							state->val.value_type = JSOX_VALUE_STRING;
							state->completedString = FALSE;
						}
						state->val.value_type = JSOX_VALUE_STRING;
						if( !state->val.string ) state->val.string = output->pos;
						if( c < 128 ) (*output->pos++) = c;
						else output->pos += ConvertToUTF8( output->pos, c );
						state->val.stringLen = output->pos - state->val.string;
#ifdef DEBUG_STRING_LENGTH
						lprintf( "Update stringLen  default collcting string :%d", state->val.stringLen );
#endif
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
						state->val.classNameLen = state->val.stringLen;
#ifdef DEBUG_CLASS_STATES
						lprintf( "Setting classname HERE (why?): %d %.*s", state->val.stringLen, state->val.stringLen, state->val.string );
#endif
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
#ifdef DEBUG_STRING_LENGTH
						lprintf( "Update stringLen  quoted string :%d", state->val.stringLen );
#endif
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
				case 160 :// case '\xa0': // nbsp
				case 0x2028: // LS (Line separator)
				case 0x2029: // PS (paragraph separate)
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
						recoverIdent( state, output, c );

						//state->status = FALSE;
						//if( !state->pvtError ) state->pvtError = VarTextCreate();
						//vtprintf( state->pvtError, "fault while parsing; whitespace unexpected at %" _size_f "  %" _size_f ":%" _size_f, state->n );	// fault
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
							state->fromHex = FALSE;
							state->val.float_result = (c == '.');
							state->val.string = output->pos;
							(*output->pos++) = c;  // terminate the string.
						}
						else
						{
					continueNumber: ;
						}
						while( (_msg_input = input->pos), ((state->n < input->size) && ( (c = GetUtfChar( &input->pos ))!= JSOX_BADUTF8)) )
						{
							newN = input->pos - input->buf;
							if( newN > input->size ) {
								break;
							}
							state->n = newN;

							//lprintf( "Number input:%c", c );
							state->col++;
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
								if( c != '+' && c != '-' )
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
									vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
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
									vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
									break;
								}
							}
							else if( c == '-' || c == '+' ) {
								if( !state->exponent ) {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
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
										vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
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
									vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
									break;
								}
							} else {
								// in non streaming mode; these would be required to follow
								if( c == ' ' || c == 160/*'\xa0'*/ || c == '\t' || c == '\n' || c == '\r' || c == 0xFEFF
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
										vtprintf( state->pvtError, "fault while parsing number; '%c' unexpected at %" _size_f "  %" _size_f ":%" _size_f, c, state->n, state->line, state->col );
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
						}
						else
						{
							(*output->pos++) = 0;
							state->val.stringLen = (output->pos - state->val.string) - 1;
#ifdef DEBUG_STRING_LENGTH
							lprintf( "Update stringLen  extra nul :%d", state->val.stringLen );
#endif
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
			if( state->gatheringString )
				goto gatherStringInput;
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
				if( input->tempBuf )
					Deallocate( CPOINTER, input->buf );

				DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, input );
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
	EnterCriticalSec( &jxpsd.cs_states );
	state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
	LeaveCriticalSec( &jxpsd.cs_states );
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
	EnterCriticalSec( &jxpsd.cs_states );
	DeleteFromSet( PDATALIST, jxpsd.dataLists, msg_data );
	LeaveCriticalSec( &jxpsd.cs_states );
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
			DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
		}
		while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
			DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
		while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
			Deallocate( const char*, buffer->buf );
			DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
		}
		DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->inBuffers );
		//DeleteLinkQueue( &state->inBuffers );
		DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->outQueue );
		//DeleteLinkQueue( &state->outQueue );
		DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->outBuffers );
		//DeleteLinkStack( &state->outBuffers );
		{
			char* buf;
			INDEX idx;
			LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
				Deallocate( char*, buf );
				SetLink( state->outValBuffers, idx, NULL ); // maybe it was saved?
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
		state->val.value_type = JSOX_VALUE_UNSET;
		{
			PDATALIST *result = state->elements;
			state->elements = GetFromSet( PDATALIST, &jxpsd.dataLists );// CreateDataList( sizeof( state->val ) );
			if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
			else state->elements[0]->Cnt = 0;
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
	EnterCriticalSec( &jxpsd.cs_states );
	_jsox_dispose_message( state->elements );
	//DeleteDataList( &state->elements );
	while( buffer = (PJSOX_PARSE_BUFFER)PopLink( state->outBuffers ) ) {
		Deallocate( const char *, buffer->buf );
		DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
	}
	{
		char* buf;
		INDEX idx;
		LIST_FORALL( state->outValBuffers[0], idx, char*, buf ) {
			Release( buf );
			SetLink( state->outValBuffers, idx, NULL ); // maybe it was saved?
		}
		DeleteFromSet( PLIST, jxpsd.listSet, state->outValBuffers );
		//DeleteList( &state->outValBuffers );
	}
	while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->inBuffers ) )
		DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
	while( buffer = (PJSOX_PARSE_BUFFER)DequeLinkNL( state->outQueue ) ) {
		Deallocate( const char*, buffer->buf );
		DeleteFromSet( JSOX_PARSE_BUFFER, state->parseBuffers, buffer );
	}
	DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->inBuffers );
	//DeleteLinkQueue( &state->inBuffers );
	DeleteFromSet( PLINKQUEUE, jxpsd.linkQueues, state->outQueue );
	//DeleteLinkQueue( &state->outQueue );
	DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->outBuffers );
	//DeleteLinkStack( &state->outBuffers );
	DeleteFromSet( JSOX_PARSE_CONTEXT, state->parseContexts, state->context );

	while( (old_context = (struct jsox_parse_context *)PopLink( state->context_stack )) ) {
		//lprintf( "warning unclosed contexts...." );
		DeleteFromSet( JSOX_PARSE_CONTEXT, state->parseContexts, old_context );
	}
	if( state->context_stack )
		DeleteFromSet( PLINKSTACK, jxpsd.linkStacks, state->context_stack );
		//DeleteLinkStack( &state->context_stack );
	DeleteFromSet( JSOX_PARSE_STATE, jxpsd.parseStates, state );
	LeaveCriticalSec( &jxpsd.cs_states );
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

struct jsox_parse_state *jsox_get_message_parser( void ) {
	return jxpsd._state;
}

static void stepPath( const char **path ) {
	int skipped;
	do {
		skipped = 0;
		switch( path[0][0] ) {
		case '.':
		case '/':
		case '\\':
		case ' ':
			path[0]++;
			skipped = 1;
			break;
		}

	} while( skipped );
}



struct jsox_value_container *jsox_get_parsed_array_value( struct jsox_value_container *val, const char *path
	, void( *callback )(uintptr_t psv, struct jsox_value_container *val), uintptr_t psv
) {
	if( path[0] == '[' )
		path++;
	int64_t index = IntCreateFromTextRef( &path );
	if( path[0] == ']' )
		path++;
	struct jsox_value_container * member = (struct jsox_value_container*)GetDataItem( &val->contains, (int)index );
	stepPath( &path );
	if( !path[0] ) {
		callback( psv, member );
		return member;
	}
	else {
		if( member->value_type == JSOX_VALUE_ARRAY ) {
			return jsox_get_parsed_array_value( member, path, callback, psv );
		}
		else if( member->value_type == JSOX_VALUE_OBJECT ) {
			return jsox_get_parsed_object_value( member, path, callback, psv );
		}
		else {
			lprintf( "Path across pimitive value...." );
		}
	}
	return NULL;
}

struct jsox_value_container *jsox_get_parsed_object_value( struct jsox_value_container *val, const char *path
	, void( *callback )(uintptr_t psv, struct jsox_value_container *val), uintptr_t psv
) {
	INDEX idx;
	struct jsox_value_container * member;
	DATA_FORALL( val->contains, idx, struct jsox_value_container *, member ) {
		if( StrCmpEx( member->name, path, member->nameLen ) == 0 ) {
			const char *subpath = path + member->nameLen;
			stepPath( &subpath );
			if( !subpath[0] ) {
				callback( psv, member );
				return member;
			}
			else {
				if( member->value_type == JSOX_VALUE_ARRAY ) {
					return jsox_get_parsed_array_value( member, subpath, callback, psv );
				}
				else if( member->value_type == JSOX_VALUE_OBJECT ) {
					return jsox_get_parsed_object_value( member, subpath, callback, psv );
				}
				else {
					lprintf( "Path across pimitive value...." );
				}
			}
		}
	}
	return NULL;
}

struct jsox_value_container *jsox_get_parsed_value( PDATALIST pdlMessage, const char *path
	, void( *callback )(uintptr_t psv, struct jsox_value_container *val), uintptr_t psv
) {
	INDEX idx;
	struct jsox_value_container * val;
	DATA_FORALL( pdlMessage, idx, struct jsox_value_container *, val ) {
		if( !path || !path[0] ) {
			callback( psv, val );
			return val;
		}
		if( val->value_type == JSOX_VALUE_OBJECT ) {
			return jsox_get_parsed_object_value( val, path, callback, psv );
		}
		else if( val->value_type == JSOX_VALUE_ARRAY ) {
			return jsox_get_parsed_array_value( val, path, callback, psv );
		}
		else {
			if( path && path[0] ) {
				lprintf( "Error; path across a primitive value" );
			}
		}
	}
	return NULL;
}



#undef GetUtfChar
#undef __GetUtfChar
#undef _zero

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



