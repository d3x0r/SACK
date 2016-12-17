#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>

#include "json.h"


#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


enum json_parse_state{
	JSON_PARSE_STATE_PICK_WHATS_NEXT       = 0x01
      , JSON_PARSE_STATE_GET_NUMBER       = 0x02
      , JSON_PARSE_STATE_GET_NAME         = 0x04
      , JSON_PARSE_STATE_GET_STRING       = 0x08
      , JSON_PARSE_STATE_GET_ARRAY_MEMBER = 0x10
};

enum word_char_states {
	WORD_POS_RESET = 0,
	WORD_POS_TRUE_1,
	WORD_POS_TRUE_2,
	WORD_POS_TRUE_3,
	WORD_POS_TRUE_4,
	WORD_POS_FALSE_1, // 11
	WORD_POS_FALSE_2,
	WORD_POS_FALSE_3,
	WORD_POS_FALSE_4,
	WORD_POS_NULL_1, // 21  get u
	WORD_POS_NULL_2, //  get l
	WORD_POS_NULL_3, //  get l
};

#define CONTEXT_UNKNOWN 0
#define CONTEXT_IN_ARRAY 1
#define CONTEXT_IN_OBJECT 2

struct json_parse_context {
	int context;
	PDATALIST elements;
	struct json_context_object *object;
};


LOGICAL json_parse_message( CTEXTSTR msg
                                 , size_t msglen
                                 , PDATALIST *_msg_output )
{
	PVARTEXT pvt_collector;
	/* I guess this is a good parser */
	PDATALIST elements = NULL;
	size_t n = 0; // character index;
	int word;
	TEXTRUNE c;
	LOGICAL status = TRUE;
	TEXTRUNE quote = 0;
	LOGICAL use_char = FALSE;
	PLINKSTACK element_lists = NULL;

	PLINKSTACK context_stack = NULL;

	LOGICAL first_token = TRUE;
	//enum json_parse_state state;

	int parse_context = CONTEXT_UNKNOWN;
	struct json_value_container val;

	POINTER msg_output;
	if( !_msg_output )
		return FALSE;

	elements = CreateDataList( sizeof( val ) );
	msg_output = (*_msg_output);

	val.value_type = VALUE_UNDEFINED;
	val.result_value = 0;
	val.name = NULL;

	pvt_collector = VarTextCreate();

//	static CTEXTSTR keyword[3] = { "false", "null", "true" };

	while( status && ( n < msglen ) && ( c = GetUtfCharIndexed( msg, &n ) ) )
	{
		switch( c )
		{
		case '{':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = parse_context;
				old_context->elements = elements;
				elements = CreateDataList( sizeof( val ) );
				PushLink( &context_stack, old_context );

				parse_context = CONTEXT_IN_OBJECT;
			}
			break;

		case '[':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = parse_context;
				old_context->elements = elements;
				elements = CreateDataList( sizeof( val ) );
				PushLink( &context_stack, old_context );

				parse_context = CONTEXT_IN_ARRAY;
			}
			break;

		case ':':
			if( parse_context == CONTEXT_IN_OBJECT )
			{
				if( val.name ) {
					lprintf( "two names single value?" );
					LineRelease( val.name );
				}
				val.name = VarTextGet( pvt_collector );
			}
			else
			{
				lprintf( WIDE("(in array, got colon out of string):parsing fault; unexpected %c at %") _size_f, c, n );
				status = FALSE;
			}
			break;
		case '}':
			if( parse_context == CONTEXT_IN_OBJECT )
			{
				// first, add the last value
				if( VarTextLength( pvt_collector ) ) {
					val.value_type = VALUE_STRING;
					val.string = VarTextGet( pvt_collector );
				}
				if( val.value_type != VALUE_UNDEFINED )
					AddDataItem( &elements, &val );

				val.value_type = VALUE_UNDEFINED;
				val.name = NULL;

				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					parse_context = old_context->context;
					elements = old_context->elements;
					Release( old_context );
				}
				//n++;
			}
			else
			{
				lprintf( WIDE("Fault parsing, unexpected %c at %") _size_f, c, n );
			}
			break;
		case ']':
			if( parse_context == CONTEXT_IN_ARRAY )
			{
				if( VarTextLength( pvt_collector ) ) {
					val.value_type = VALUE_STRING;
					val.string = VarTextGet( pvt_collector );
				}
				if( val.value_type != VALUE_UNDEFINED )
					AddDataItem( &elements, &val );
				val.value_type = VALUE_UNDEFINED;
				val.name = NULL;

				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					parse_context = old_context->context;
					elements = old_context->elements;
					Release( old_context );
				}
			}
			else
			{
				lprintf( WIDE("bad context %d; fault parsing '%c' unexpected %") _size_f, parse_context, c, n );// fault
			}
			break;
		case ',':
			if( ( parse_context == CONTEXT_IN_ARRAY ) 
			   || ( parse_context == CONTEXT_IN_OBJECT ) )
			{
				if( VarTextLength( pvt_collector ) ) {
					val.value_type = VALUE_STRING;
					val.string = VarTextGet( pvt_collector );
				}
				if( val.value_type != VALUE_UNDEFINED )
					AddDataItem( &elements, &val );
				val.value_type = VALUE_UNDEFINED;
				val.name = NULL;
			}
			else
			{
				lprintf( WIDE("bad context; fault parsing '%c' unexpected %") _size_f, c, n );// fault
			}
			//n++;
			break;

		default:
			if( first_token )
			{
				lprintf( WIDE("first token; fault parsing '%c' unexpected %") _size_f, c, n );// fault
				//status = FALSE;
			}
			switch( c )
			{
			case '"':
				{
					// collect a string
					int escape = 0;
					while( ( n < msglen ) && ( c = GetUtfCharIndexed( msg, &n ) ) )
					{
						if( c == '\\' )
						{
							if( escape ) VarTextAddCharacter( pvt_collector, '\\' );
							else escape = 1;
						}
						else if( c == '"' )
						{
							if( escape ) VarTextAddCharacter( pvt_collector, '\"' );
							else break;
						}
						else
						{
							if( escape )
							{
								switch( c )
								{
								case '/':
								case '\\':
								case '"':
									VarTextAddRune( pvt_collector, c );
									break;
								case 't':
									VarTextAddCharacter( pvt_collector, '\t' );
									break;
								case 'b':
									VarTextAddCharacter( pvt_collector, '\b' );
									break;
								case 'n':
									VarTextAddCharacter( pvt_collector, '\n' );
									break;
								case 'r':
									VarTextAddCharacter( pvt_collector, '\r' );
									break;
								case 'f':
									VarTextAddCharacter( pvt_collector, '\f' );
									break;
								case 'u':
									{
										TEXTRUNE hex_char = 0;
										int ofs;
										for( ofs = 0; ofs < 4; ofs++ )
										{
											c = GetUtfCharIndexed( msg, &n );
											hex_char *= 16;
											if( c >= '0' && c <= '9' )      hex_char += c - '0';
											else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
											else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
											else
												lprintf( WIDE("(escaped character, parsing hex of \\u) fault parsing '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
														 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
										}
										VarTextAddRune( pvt_collector, hex_char );
									}
									break;
								default:
									lprintf( WIDE("(escaped character) fault parsing '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
											 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
											 , msg + n - ( (n>3)?3:n )
											 , c
											 , msg + n + 1
											 );// fault
									break;
								}
								escape = 0;
							}
							else
								VarTextAddRune( pvt_collector, c );
						}
					}
					val.result_value = VALUE_STRING;
					break;
				}

			case ' ':
			case '\t':
			case '\r':
			case '\n':
				// skip whitespace
				//n++;
				break;

		//----------------------------------------------------------
		//  catch characters for true/false/null which are values outside of quotes
			case 't':
				if( word == WORD_POS_RESET ) word = WORD_POS_TRUE_1;
				else lprintf( WIDE("fault parsing '%c' unexpected at %")_size_f, c, n );// fault
				break;
			case 'r':
				if( word == WORD_POS_TRUE_1 ) word = WORD_POS_TRUE_2;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'u':
				if( word == WORD_POS_TRUE_2 ) word = WORD_POS_TRUE_3;
				else if( word == WORD_POS_NULL_1 ) word = WORD_POS_NULL_2;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'e':
				if( word == WORD_POS_TRUE_3 ) {
					val.value_type = VALUE_TRUE;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_4 ) {
					val.value_type = VALUE_FALSE;
					word = WORD_POS_RESET;
				} else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'n':
				if( word == WORD_POS_RESET ) word = WORD_POS_NULL_1;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'l':
				if( word == WORD_POS_NULL_2 ) word = WORD_POS_NULL_3;
				else if( word == WORD_POS_NULL_3 ) {
					val.result_value = VALUE_NULL;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_2 ) word = WORD_POS_FALSE_3;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'f':
				if( word == WORD_POS_RESET ) word = WORD_POS_FALSE_1;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 'a':
				if( word == WORD_POS_FALSE_1 ) word = WORD_POS_FALSE_2;
				else lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				break;
			case 's':
				if( word == WORD_POS_FALSE_3 ) word = WORD_POS_FALSE_4;
				else lprintf( WIDE("fault parsing '%c' unexpected %") _size_f, c, n );// fault
				break;
		//
 	 	//----------------------------------------------------------


			default:
				if( ( c >= '0' && c <= '9' )
					|| ( c == '-' ) )
				{
					// always reset this here....
					// keep it set to determine what sort of value is ready.
					val.float_result = 0;
					VarTextAddRune( pvt_collector, c );
					while( ( n < msglen ) && ( c = GetUtfCharIndexed( msg, &n) ) )
					{
						// leading zeros should be forbidden.
						if( ( c >= '0' && c <= '9' )
							|| ( c == '-' )
							|| ( c == '+' )
							|| ( c == '+' )
						  )
						{
							VarTextAddRune( pvt_collector, c );
						}
						else if( ( c =='e' ) || ( c == 'E' ) || ( c == '.' ) )
						{
							val.float_result = 1;
							VarTextAddRune( pvt_collector, c );
						}
						else
						{
							break;
						}
					}
					{
						PTEXT number = VarTextGet( pvt_collector );
						if( val.float_result )
						{
							val.result_d = FloatCreateFromSeg( number );
						}
						else
						{
							val.result_n = IntCreateFromSeg( number );
						}
						LineRelease( number );
					}
					val.result_value = VALUE_NUMBER;
				}
				else if( ( c >= ' ' )
						  || ( c == '\t' )
						  || ( c == '\r' )
						  || ( c == '\n' )
						 )
				{
					// do nothing; white space is allowed.
				}
				else
				{
					// fault, illegal characer (whitespace?)
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
		while( old_context = (struct json_parse_context *)PopLink( &context_stack ) ) {
			lprintf( "warning unclosed contexts...." );
			Release( old_context );
		}
		if( context_stack )
			DeleteLinkStack( &context_stack );
	}
	if( element_lists )
		DeleteLinkStack( &element_lists );

	if( !elements->Cnt )
		if( val.value_type != VALUE_UNDEFINED )
			AddDataItem( &elements, &val );

	(*_msg_output) = elements;
	// clean all contexts
	VarTextDestroy( &pvt_collector );

	if( !status )
	{
		//Release( (*_msg_output) );
		//(*_msg_output) = NULL;
	}
	return status;
}

void json_dispose_decoded_message( struct json_context_object *format
                                 , POINTER msg_data )
{
	// a complex format might have sub-parts .... but for now we'll assume simple flat structures
	Release( msg_data );
}

void json_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		if( val->name ) LineRelease( val->name );
		if( val->string ) LineRelease( val->string );
		if( val->value_type == VALUE_OBJECT )
			json_dispose_message( &val->contains );
	}
	// quick method
	DeleteDataList( msg_data );

}


// puts the current collected value into the element; assumes conversion was correct
static void FillDataToElement( struct json_context_object_element *element
							    , size_t object_offset
								, struct json_value_container *val
								, POINTER msg_output )
{
	PTEXT string;
	if( !val->name )
		return;
	// remove name; indicate that the value has been used.
	LineRelease( val->name );
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
			switch( val->result_value )
			{
			case VALUE_NULL:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = NULL;
				break;
			case VALUE_STRING:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = StrDup( GetText( val->string ) );
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->result_value );
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
			switch( val->result_value )
			{
			case VALUE_TRUE:
				switch( element->type )
				{
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
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->result_value );
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
			switch( val->result_value )
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
				lprintf( WIDE("Expected a float, but parsed result was a %d"), val->result_value );
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
			switch( val->result_value )
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


#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



