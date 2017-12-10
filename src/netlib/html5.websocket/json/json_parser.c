#include <stdhdrs.h>
#ifndef JSON_EMITTER_SOURCE
#  define JSON_EMITTER_SOURCE
#endif
#ifndef JSON_PARSER_MAIN_SOURCE
#  define JSON_PARSER_MAIN_SOURCE
#endif
#include <json_emitter.h>

#include "json.h"


#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


char *json_escape_string_length( const char *string, size_t length, size_t *outlen ) {
	size_t m = 0;
	size_t ch;
	const char *input;
	TEXTSTR output, _output;
	if( !(input = string) ) return NULL;
	for( ch = 0; ch < length; ch++, input++ ) {
		if( input[0] == '"' || input[0] == '\\' )
			m++;
		else if( input[0] == '\n' )
			m++;
		else if( input[0] == '\t' )
			m++;
	}
	_output = output = NewArray( char, length+m+1 );
	for( (ch = 0), (input = string); ch < length; ch++, input++ ) {
		if( input[0] == '"' || input[0] == '\\' ) {
			(*output++) = '\\';
		}
		else if( input[0] == '\n' ) {
			(*output++) = '\\'; (*output++) = 'n'; continue;
		}
		else if( input[0] == '\t' ) {
			(*output++) = '\\'; (*output++) = 't'; continue;
		}
		(*output++) = input[0];
	}
	(*output) = 0; // include nul character terminator.
	if( outlen ) (*outlen) = output - _output;
	return _output;
}

char *json_escape_string( const char *string ) {
	return json_escape_string_length( string, strlen( string ), NULL );
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


static int gatherString( CTEXTSTR msg, CTEXTSTR *msg_input, size_t msglen, TEXTSTR *pmOut, size_t *line, size_t *col, TEXTRUNE start_c, struct json_parse_state *state ) {
	char *mOut = (*pmOut);
	// collect a string
	int status = 0;
	size_t n;
	int escape;
	LOGICAL cr_escaped;
	TEXTRUNE c;
	escape = 0;
	cr_escaped = FALSE;
	while( (n = (*msg_input) - msg), ((n < msglen) && (c = GetUtfChar( msg_input ))) && (status >= 0) )
	{
		(*col)++;
		if( c == '\\' )
		{
			if( escape ) {
				(*mOut++) = '\\'; 
				escape = 0;
			}
			else escape = 1;
		}
		else if( (c == '"') || (c == '\'') || (c == '`') )
		{
			if( escape ) { (*mOut++) = c; escape = FALSE; }
			else if( c == start_c ) {
				status = 1;
				break;
			}
			else (*mOut++) = c; // other else is not valid close quote; just store as content.
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
					(*mOut++) = c;
					escape = FALSE;
					break;
				case 't':
					(*mOut++) = '\t';
					escape = FALSE;
					break;
				case 'b':
					(*mOut++) = '\b';
					escape = FALSE;
					break;
				case 'n':
					(*mOut++) = '\n';
					escape = FALSE;
					break;
				case 'r':
					(*mOut++) = '\r';
					escape = FALSE;
					break;
				case 'f':
					(*mOut++) = '\f';
					escape = FALSE;
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
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "(escaped character, parsing octal escape val=%d) fault while parsing; )" ) WIDE( " (near %*.*s[%c]%s)" )
							, oct_char
							, (int)((n>3) ? 3 : n), (int)((n>3) ? 3 : n)
							, (*msg_input) - ((n>3) ? 3 : n)
							, c
							, (*msg_input) + 1
						);// fault
						status = -1;
						break;
					}
					else {
						if( oct_char < 128 ) (*mOut++) = oct_char;
						else mOut += ConvertToUTF8( mOut, oct_char );
					}
					escape = FALSE;
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
						else if( c >= 'A' && c <= 'F' ) hex_char += (c - 'A') + 10;
						else if( c >= 'a' && c <= 'f' ) hex_char += (c - 'F') + 10;
						else {
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "(escaped character, parsing hex of \\x) fault while parsing; '%c' unexpected at %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), c, n
								, (int)((n>3) ? 3 : n), (int)((n>3) ? 3 : n)
								, (*msg_input) - ((n>3) ? 3 : n)
								, c
								, (*msg_input) + 1
							);// fault
							status = -1;
						}
					}
					if( hex_char < 128 ) (*mOut++) = hex_char;
					else mOut += ConvertToUTF8( mOut, hex_char );
					escape = FALSE;
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
					for( ofs = 0; ofs < codePointLen && (c != endCode); ofs++ )
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
						else if( c >= 'A' && c <= 'F' ) hex_char += (c - 'A') + 10;
						else if( c >= 'a' && c <= 'f' ) hex_char += (c - 'F') + 10;
						else {
							if( !state->pvtError ) state->pvtError = VarTextCreate();
							vtprintf( state->pvtError, WIDE( "(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), c, n
								, (int)((n > 3) ? 3 : n), (int)((n > 3) ? 3 : n)
								, (*msg_input) - ((n > 3) ? 3 : n)
								, c
								, (*msg_input) + 1
							);// fault
						}
					}
					mOut += ConvertToUTF8( mOut, hex_char );
					escape = FALSE;
				}
				break;
				default:
					if( cr_escaped ) {
						cr_escaped = FALSE;
						escape = FALSE;
						mOut += ConvertToUTF8( mOut, c );
					}
					else {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "(escaped character) fault while parsing; '%c' unexpected %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), c, n
							, (int)((n>3) ? 3 : n), (int)((n>3) ? 3 : n)
							, (*msg_input) - ((n>3) ? 3 : n)
							, c
							, (*msg_input) + 1
						);// fault
						status = -1;
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
	if( status )
		(*mOut++) = 0;  // terminate the string.
	(*pmOut) = mOut;
	return status;
}




static void json_state_init( struct json_parse_state *state )
{
	PPDATALIST ppElements;
	PPLIST ppList;
	PPLINKQUEUE ppQueue;
	PPLINKSTACK ppStack;

	ppElements = GetFromSet( PDATALIST, &jpsd.dataLists );
	if( !ppElements[0] ) ppElements[0] = CreateDataList( sizeof( state->val ) );
	state->elements = ppElements;
	state->elements[0]->Cnt = 0;
	
	ppStack = GetFromSet( PLINKSTACK, &jpsd.linkStacks );
	if( !ppStack[0] ) ppStack[0] = CreateLinkStack();
	state->outBuffers = ppStack;
	state->outBuffers[0]->Top = 0;

	ppQueue = GetFromSet( PLINKQUEUE, &jpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->inBuffers = ppQueue;// CreateLinkQueue();
	state->inBuffers[0]->Top = state->inBuffers[0]->Bottom = 0;
	
	ppQueue = GetFromSet( PLINKQUEUE, &jpsd.linkQueues );
	if( !ppQueue[0] ) ppQueue[0] = CreateLinkQueue();
	state->outQueue = ppQueue;// CreateLinkQueue();
	state->outQueue[0]->Top = state->outQueue[0]->Bottom = 0;

	ppList = GetFromSet( PLIST, &jpsd.listSet );
	if( ppList[0] ) ppList[0]->Cnt = 0;
	state->outValBuffers = ppList;
	

	state->line = 1;
	state->col = 1;
	state->n = 0; // character index;
	state->word = WORD_POS_RESET;
	state->status = TRUE;
	state->negative = FALSE;

	state->context_stack = GetFromSet( PLINKSTACK, &jpsd.linkStacks );// NULL;
	if( state->context_stack[0] ) state->context_stack[0]->Top = 0;
	//state->first_token = TRUE;
	state->context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
	state->parse_context = CONTEXT_UNKNOWN;
	state->comment = 0;
	state->completed = FALSE;
	//state->mOut = msg;// = NewArray( char, msglen );
	//state->msg_input = (char const *)msg;

	state->val.value_type = VALUE_UNSET;
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
struct json_parse_state * json_begin_parse( void )
{
	struct json_parse_state *state = GetFromSet( PARSE_STATE, &jpsd.parseStates );//New( struct json_parse_state );
	json_state_init( state );
	return state;
}

/* I guess this is a good parser */
int json_parse_add_data( struct json_parse_state *state
	, const char *msg
	, size_t msglen )
{
	TEXTRUNE c;
	struct json_input_buffer *input;
	struct json_output_buffer *output;
	int retval = 0;
	int string_status;

	if( msg && msglen ) {
		input = GetFromSet( PARSE_BUFFER, &jpsd.parseBuffers );
		input->pos = input->buf = msg;
		input->size = msglen;
		EnqueLink( state->inBuffers, input );

		if( state->gatheringString || state->gatheringNumber || state->parse_context == CONTEXT_OBJECT_FIELD ) {
			// have to extend the previous output buffer to include this one instead of allocating a split string.
			size_t offset;
			size_t offset2;
			output = (struct json_output_buffer*)DequeLink( state->outQueue );
			//lprintf( "output from before is %p", output );
			offset = (output->pos - output->buf);
			offset2 = state->val.string - output->buf;
			AddLink( state->outValBuffers, output->buf );
			output->buf = NewArray( char, output->size + msglen + 1 );
			MemCpy( output->buf + offset2, state->val.string, offset-offset2 );
			output->size += msglen;
			//lprintf( "previous val:%s", state->val.string, state->val.string );
			state->val.string = output->buf + offset2;
			output->pos = output->buf + offset;
			PrequeLink( state->outQueue, output );
		}
		else {
			output = (struct json_output_buffer*)GetFromSet( PARSE_BUFFER, &jpsd.parseBuffers );
			output->pos = output->buf = NewArray( char, msglen + 1 );
			output->size = msglen;
			EnqueLink( state->outQueue, output );
		}
	}


	while( state->status && (input = (PPARSE_BUFFER)DequeLink( state->inBuffers )) ) {
		output = (struct json_output_buffer*)DequeLink( state->outQueue );
		//lprintf( "output is %p", output );
		state->n = input->pos - input->buf;

		if( state->gatheringString ) {
			string_status = gatherString( input->buf, &input->pos, input->size, &output->pos, &state->line, &state->col, state->gatheringStringFirstChar, state );
			if( string_status < 0 )
				state->status = FALSE;
			else if( string_status > 0 )
			{
				state->gatheringString = FALSE;
				state->n = input->pos - input->buf;
				state->val.stringLen = ( output->pos - state->val.string ) - 1;
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
		while( state->status && (state->n < input->size) && (c = GetUtfChar( &input->pos )) )
		{
			state->n = input->pos - input->buf;
			switch( c )
			{
			case '{':
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
				old_context->context = state->parse_context;
				old_context->elements = state->elements;
				old_context->name = state->val.name;
				old_context->nameLen = state->val.nameLen;
				state->elements = GetFromSet( PDATALIST, &jpsd.dataLists );// CreateDataList( sizeof( state->val ) );
				if( !state->elements[0] ) state->elements[0] = CreateDataList( sizeof( state->val ) );
				else state->elements[0]->Cnt = 0;
				PushLink( state->context_stack, old_context );
				RESET_STATE_VAL();
				state->parse_context = CONTEXT_IN_OBJECT;
			}
			break;

			case '[':
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &jpsd.parseContexts );
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
				if( state->parse_context == CONTEXT_IN_OBJECT )
				{
					if( state->val.name ) {
						lprintf( "two names single value?" );
					}
					state->val.name = state->val.string;
					state->val.nameLen = state->val.stringLen;
					state->val.string = NULL;
					state->val.value_type = VALUE_UNSET;
				}
				else
				{
					if( state->parse_context == CONTEXT_IN_ARRAY ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "(in array, got colon out of string):parsing fault; unexpected %c at %" ) _size_f, c, state->n );
					} 
					else {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "(outside any object, got colon out of string):parsing fault; unexpected %c at %" ) _size_f, c, state->n );
					}
					state->status = FALSE;
				}
				break;
			case '}':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				if( state->parse_context == CONTEXT_IN_OBJECT || state->parse_context == CONTEXT_OBJECT_FIELD_VALUE )
				{
					// first, add the last value
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( state->elements, &state->val );
					}
					//RESET_STATE_VAL();
					state->val.value_type = VALUE_OBJECT;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					state->val.string = NULL;
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
					//n++;
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "Fault while parsing; unexpected %c at %" ) _size_f, c, state->n );
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
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( state->elements, &state->val );
					}
					//RESET_STATE_VAL();
					state->val.value_type = VALUE_ARRAY;
					state->val.contains = state->elements[0];
					state->val._contains = state->elements;
					state->val.string = NULL;
					{
						struct json_parse_context *old_context = (struct json_parse_context *)PopLink( state->context_stack );
						//struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt - 1 );
						//oldVal->contains = state->elements;  // save updated elements list in the old value in the last pushed list.

						state->parse_context = old_context->context;
						state->elements = old_context->elements;
						state->val.name = old_context->name;
						state->val.nameLen = old_context->nameLen;
						DeleteFromSet( PARSE_CONTEXT, jpsd.parseContexts, old_context );					}
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context %d; fault while parsing; '%c' unexpected at %" ) _size_f, state->parse_context, c, state->n );// fault
					state->status = FALSE;
				}
				break;
			case ',':
				if( state->word == WORD_POS_END ) {
					// allow starting a new word
					state->word = WORD_POS_RESET;
				}
				if( (state->parse_context == CONTEXT_IN_ARRAY)
					|| (state->parse_context == CONTEXT_IN_OBJECT) )
				{
					if( state->val.value_type != VALUE_UNSET ) {
						AddDataItem( state->elements, &state->val );
					}
					RESET_STATE_VAL();
				}
				else
				{
					if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "bad context; fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n );// fault
				}
				break;

			default:
				switch( c )
				{
				case '"':
				//case '\'':
					state->val.string = output->pos;
					state->gatheringString = TRUE;
					state->gatheringStringFirstChar = c;
					string_status = gatherString( input->buf, &input->pos, input->size, &output->pos, &state->line, &state->col, c, state );
					//lprintf( "string gather status:%d", string_status );
					if( string_status < 0 )
						state->status = FALSE;
					else if( string_status > 0 ) {
						state->gatheringString = FALSE;
						state->val.stringLen = ( output->pos - state->val.string ) - 1;
					} else if( state->complete_at_end ) {
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, "End of string fail." );
						state->status = FALSE;
					}
					state->n = input->pos - input->buf;

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
				case '\t':
				case '\r':
				case '\n':
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
					else { 
						state->status = FALSE; 
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'r':
					if( state->word == WORD_POS_TRUE_1 ) state->word = WORD_POS_TRUE_2;
					else { 
						state->status = FALSE; 
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'u':
					if( state->word == WORD_POS_TRUE_2 ) state->word = WORD_POS_TRUE_3;
					else if( state->word == WORD_POS_NULL_1 ) state->word = WORD_POS_NULL_2;
					else if( state->word == WORD_POS_RESET ) state->word = WORD_POS_UNDEFINED_1;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
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
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'n':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_NULL_1;
					else if( state->word == WORD_POS_UNDEFINED_1 ) state->word = WORD_POS_UNDEFINED_2;
					else if( state->word == WORD_POS_UNDEFINED_6 ) state->word = WORD_POS_UNDEFINED_7;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'd':
					if( state->word == WORD_POS_UNDEFINED_2 ) state->word = WORD_POS_UNDEFINED_3;
					else if( state->word == WORD_POS_UNDEFINED_8 ) { state->val.value_type = VALUE_UNDEFINED; state->word = WORD_POS_END; }
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'i':
					if( state->word == WORD_POS_UNDEFINED_5 ) state->word = WORD_POS_UNDEFINED_6;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'l':
					if( state->word == WORD_POS_NULL_2 ) state->word = WORD_POS_NULL_3;
					else if( state->word == WORD_POS_NULL_3 ) {
						state->val.value_type = VALUE_NULL;
						state->word = WORD_POS_END;
					}
					else if( state->word == WORD_POS_FALSE_2 ) state->word = WORD_POS_FALSE_3;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'f':
					if( state->word == WORD_POS_RESET ) state->word = WORD_POS_FALSE_1;
					else if( state->word == WORD_POS_UNDEFINED_4 ) state->word = WORD_POS_UNDEFINED_5;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 'a':
					if( state->word == WORD_POS_FALSE_1 ) state->word = WORD_POS_FALSE_2;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
				case 's':
					if( state->word == WORD_POS_FALSE_3 ) state->word = WORD_POS_FALSE_4;
					else { state->status = FALSE; if( !state->pvtError ) state->pvtError = VarTextCreate();
					vtprintf( state->pvtError, WIDE( "fault while parsing; '%c' unexpected at %" ) _size_f, c, state->n ); }// fault
					break;
					//
					//----------------------------------------------------------
				case '-':
					state->negative = !state->negative;
					break;

				default:
					if( (c >= '0' && c <= '9') || (c == '+') )
					{
						const char *_msg_input;
				continueNumber:
						// always reset this here....
						// keep it set to determine what sort of value is ready.
						if( !state->gatheringNumber ) {
							state->exponent = FALSE;
							state->fromHex = FALSE;
							//fromDate = FALSE;
							state->val.float_result = 0;
							state->val.string = output->pos;
							(*output->pos++) = c;  // terminate the string.
						}
						else
						{
							//exponent = state->numberExponent;
							//fromDate = state->numberFromDate;
						}
						while( (_msg_input = input->pos), ((state->n < input->size) && (c = GetUtfChar( &input->pos ))) )
						{
							if( c == '_' )
								continue;
							state->n = (input->pos - input->buf);
							// leading zeros should be forbidden.
							if( (c >= '0' && c <= '9')
								|| (c == '-')
								|| (c == '+')
								)
							{
								(*output->pos++) = c;
							}
							else if( c == 'x' ) {
								// hex conversion.
								if( !state->fromHex ) {
									state->fromHex = TRUE;
									(*output->pos++) = c;
								}
								else {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f, c, state->n );
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
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
							}
							else if( c == '-' ) {
								if( !state->exponent ) {
									state->status = FALSE;
									if( !state->pvtError ) state->pvtError = VarTextCreate();
									vtprintf( state->pvtError, WIDE( "fault wile parsing; '%c' unexpected at %" ) _size_f WIDE( "  %" ) _size_f WIDE( ":%" ) _size_f, c, state->n, state->line, state->col );
									break;
								}
								else {
									(*output->pos++) = c;
								}
							}
							else if( c == '.' )
							{
								state->val.float_result = 1;
								(*output->pos++) = c;
							}
							else
							{
								// in non streaming mode; these would be required to follow
								/*
								if( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xFEFF
								|| c == ',' || c == '\'' || c == '\"' || c == '`' ) {
								}
								*/
								//lprintf( "Non numeric character received; push the value we have" );
								(*output->pos++) = 0;
								state->val.stringLen = ( output->pos - state->val.string ) - 1;
								break;
							}
						}
						input->pos = _msg_input;
						state->n = input->pos - input->buf;

						//LogBinary( (uint8_t*)output->buf, output->size );
						if( (!state->complete_at_end) && state->n == input->size )
						{
							//lprintf( "completion mode is not end of string; and at end of string" );
							state->gatheringNumber = TRUE;
							//state->numberExponent = exponent;
							//state->numberFromDate = fromDate;
						}
						else
						{
							state->gatheringNumber = FALSE;
							(*output->pos++) = 0;
							state->val.stringLen = ( output->pos - state->val.string ) - 1;

							if( state->val.float_result )
							{
								CTEXTSTR endpos;
								state->val.result_d = FloatCreateFromText( state->val.string, &endpos );
								if( state->negative & 1 ) { state->val.result_d = -state->val.result_d; state->negative = FALSE; }
							}
							else
							{
								state->val.result_n = IntCreateFromText( state->val.string );
								if( state->negative & 1 ) { state->val.result_n = -state->val.result_n; state->negative = FALSE; }
							}
							state->val.value_type = VALUE_NUMBER;
							if( state->parse_context == CONTEXT_UNKNOWN ) {
								state->completed = TRUE;
							}
						}
					}
					else
					{
						// fault, illegal characer (whitespace?)
						state->status = FALSE;
						if( !state->pvtError ) state->pvtError = VarTextCreate();
						vtprintf( state->pvtError, WIDE( "fault parsing '%c' unexpected %" )_size_f WIDE( " (near %*.*s[%c]%s)" ), c, state->n
							, (int)((state->n > 4) ? 3 : (state->n - 1)), (int)((state->n > 4) ? 3 : (state->n - 1))
							, input->pos + state->n - ((state->n > 4) ? 4 : state->n)
							, c
							, input->pos + state->n
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
		if( state->n == input->size ) {
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
		if( state->completed )
			break;
	}

	if( !state->status ) {
		// some error condition; cannot resume parsing.
		return -1;
	}

	if( !state->gatheringNumber && !state->gatheringString )
		if( state->val.value_type != VALUE_UNSET ) {
			AddDataItem( state->elements, &state->val );
			RESET_STATE_VAL();
		}

	if( state->completed ) {
		state->completed = FALSE;
	}
	return retval;
}


LOGICAL json_parse_message( const char * msg
	, size_t msglen
	, PDATALIST *_msg_output ) {
	struct json_parse_state *state = json_begin_parse();
	static struct json_parse_state *_state;
	state->complete_at_end = TRUE;
	int result = json_parse_add_data( state, msg, msglen );
	if( _state ) json_parse_dispose_state( &_state );
	if( result > 0 ) {
		(*_msg_output) = json_parse_get_data( state );
		_state = state;
		//json6_parse_dispose_state( &state );
		return TRUE;
	}
	(*_msg_output) = NULL;
	json_parse_dispose_state( &state );
	return FALSE;
}

void json_dispose_decoded_message( struct json_context_object *format
                                 , POINTER msg_data )
{
	// a complex format might have sub-parts .... but for now we'll assume simple flat structures
	Release( msg_data );
}

void _json_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		//if( val->name ) Release( val->name );
		//if( val->string ) Release( val->string );
		if( val->value_type == VALUE_OBJECT || val->value_type == VALUE_ARRAY )
			_json_dispose_message( val->_contains );
	}
	// quick method
	DeleteFromSet( PDATALIST, jpsd.dataLists, msg_data );
	//DeleteDataList( msg_data );

}

static uintptr_t FindDataList( void*p, uintptr_t psv ) {
	if( ((PPDATALIST)p)[0] == (PDATALIST)psv )
		return (uintptr_t)p;
	return 0;
}

void json_dispose_message( PDATALIST *msg_data ) {
	uintptr_t actual = ForAllInSet( PDATALIST, jpsd.dataLists, FindDataList, (uintptr_t)msg_data[0] );
	_json_dispose_message( (PDATALIST*)actual );
	msg_data[0] = NULL;
}


// puts the current collected value into the element; assumes conversion was correct
static void FillDataToElement( struct json_context_object_element *element
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


LOGICAL json_decode_message( struct json_context *format
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



