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

#define VALUE_NULL 1
#define VALUE_TRUE 2
#define VALUE_FALSE 3
#define VALUE_STRING 4
#define VALUE_NUMBER 5

#define CONTEXT_UNKNOWN 0
#define CONTEXT_IN_ARRAY 1
#define CONTEXT_IN_OBJECT 2

struct array_element {
	int value_type;
	PTEXT string;
	S_64  n;
	double d;
};

struct json_parse_context {
	int context;
	PLIST elements;
	struct json_context_object *object;
};

struct json_value_container {
	PTEXT name;
	int result_value;
	int float_result;
	double result_d;
	S_64 result_n;
	PVARTEXT pvt_collector;
};

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
				((CTEXTSTR*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = NULL;
				break;
			case VALUE_STRING:
				string = VarTextGet( val->pvt_collector );
				((CTEXTSTR*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = StrDup( GetText( string ) );
				LineRelease( string );
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
					((S_8*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((S_16*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((S_32*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((S_64*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				}
				break;
			case VALUE_FALSE:
				switch( element->type )
				{
				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((S_8*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((S_16*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((S_32*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((S_64*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				}
				break;
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int") );
					((S_64*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (S_64)val->result_d;
				}
				else
				{
					((S_64*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = val->result_n;
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
						((float*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (float)val->result_d;
					else
						((double*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = val->result_d;

				}
				else
				{
               // this is probably common (0 for instance)
					lprintf( WIDE("warning received int, converting to float") );
					if( element->type == JSON_Element_Float )
						((float*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (float)val->result_n;
					else
						((double*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (double)val->result_n;

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
					lprintf( WIDE("warning received float, converting to int (PTRSZVAL)") );
					((PTRSZVAL*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (PTRSZVAL)val->result_d;
				}
				else
				{
					// this is probably common (0 for instance)
					((PTRSZVAL*)( ((PTRSZVAL)msg_output) + element->offset + object_offset ))[0] = (PTRSZVAL)val->result_n;
				}
				break;
			}
		}
		break;
	}
}

LOGICAL json_parse_message_format( struct json_context_object *format
                           , CTEXTSTR msg
						   , size_t msglen
									, POINTER *_msg_output )
{
	/* I guess this is a good parser */
	PLIST elements = NULL;
	struct json_context_object_element *element;
	size_t n = 0; // character index;
	int word;
	TEXTCHAR c;
	LOGICAL status = TRUE;
	TEXTCHAR quote = 0;
	LOGICAL escape = FALSE;
	LOGICAL use_char = FALSE;
	PLINKSTACK element_lists = NULL;

	PLINKSTACK context_stack = NULL;

	//PLINKQUEUE array_values = NULL;
	//struct array_element *new_array_element;

	LOGICAL first_token = TRUE;
	//enum json_parse_state state;

	int parse_context = CONTEXT_UNKNOWN;
	struct json_value_container val;

	POINTER msg_output;
	if( !_msg_output )
		return FALSE;
	msg_output = (*_msg_output);

	// messae is allcoated +7 bytes in case last part is a _8 type
	// all integer stores use _64 type to store the collected value.
	if( !msg_output )
		msg_output = (*_msg_output)
		           = NewArray( _8, format->object_size + 7  ); 


	val.result_value = 0;
	val.pvt_collector = VarTextCreate();
	val.name = NULL;

//	static CTEXTSTR keyword[3] = { "false", "null", "true" };

	while( status && ( n < msglen ) && ( c = msg[n] ) )
	{
		switch( c )
		{
		case '{':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = parse_context;
				old_context->elements = elements;
				old_context->object = format;
				PushLink( &context_stack, old_context );

				parse_context = CONTEXT_IN_OBJECT;
				if( first_token )
				{
					// begin an object.
					// content is
					elements = format->members;
					PushLink( &element_lists, elements );
					first_token = 0;
					n++;
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
									n++;
									break;
								}
								else
								{
									lprintf( "Error; object type does not have an object" );
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
			}
			break;

		case '[':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = parse_context;
				old_context->elements = elements;
				old_context->object = format;
				PushLink( &context_stack, old_context );

				parse_context = CONTEXT_IN_ARRAY;
				if( first_token )
				{
					elements = format->members;
					PushLink( &element_lists, elements );
					first_token = 0;
					n++;
				}
				else
				{
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element *, element )
					{
						if( StrCaseCmp( element->name, GetText( val.name ) ) == 0 )
						{
							if( ( element->count_offset != JSON_NO_OFFSET ) || element->count )
							{
								n++;
								break;
							}
							else
							{
								lprintf( WIDE("Incompatible value; expected element that can take an array, type is %d"), element->type );
								return FALSE;
							}
						}
					}
				}
			}
			break;

		case ':':
			if( parse_context == CONTEXT_IN_OBJECT )
			{
				if( val.name )
					LineRelease( val.name );
				val.name = VarTextGet( val.pvt_collector );
				{
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element *, element )
					{
						if( StrCaseCmp( element->name, GetText( val.name ) ) == 0 )
						{
							break;
						}
					}
					if( !element )
						status = FALSE;
				}
				n++;
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
				FillDataToElement( element, format->offset, &val, msg_output );
				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					format = old_context->object;
					parse_context = old_context->context;
					elements = old_context->elements;
					Release( old_context );
				}
				n++;
			}
			else
			{
				lprintf( WIDE("Fault parsing, unexpected %c at %") _size_f, c, n );
			}
			break;
		case ']':
			if( parse_context == CONTEXT_IN_ARRAY )
			{
				FillDataToElement( element, format->offset, &val, msg_output );
				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					format = old_context->object;
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
			if( parse_context == CONTEXT_IN_ARRAY )
			{
				FillDataToElement( element, format->offset, &val, msg_output );
			}
			else if( parse_context == CONTEXT_IN_OBJECT )
			{
				FillDataToElement( element, format->offset, &val, msg_output );
			}
			else
			{
				lprintf( WIDE("bad context; fault parsing '%c' unexpected %") _size_f, c, n );// fault
			}
			n++;
			break;

		default:
			if( first_token )
			{
				lprintf( WIDE("first token; fault parsing '%c' unexpected %") _size_f, c, n );// fault
				status = FALSE;
			}
			switch( c )
			{
			case '"':
				{
					// collect a string
					int escape = 0;
					while( ( c = msg[++n] ) && ( n < msglen ) )
					{
						if( c == '\\' )
						{
							if( escape )
								VarTextAddCharacter( val.pvt_collector, '\\' );
							else
								escape = 1;
						}
						else if( c == '"' )
						{
							if( escape )
								VarTextAddCharacter( val.pvt_collector, '\"' );
							else
							{
								n++;
								break;
							}
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
									VarTextAddCharacter( val.pvt_collector, c );
									break;
								case 't':
									VarTextAddCharacter( val.pvt_collector, '\t' );
									break;
								case 'b':
									VarTextAddCharacter( val.pvt_collector, '\b' );
									break;
								case 'n':
									VarTextAddCharacter( val.pvt_collector, '\n' );
									break;
								case 'r':
									VarTextAddCharacter( val.pvt_collector, '\r' );
									break;
								case 'f':
									VarTextAddCharacter( val.pvt_collector, '\f' );
									break;
								case 'u':
									{
										_32 hex_char = 0;
										int ofs;
										for( ofs = 0; ofs < 4; ofs++ )
										{
											c = msg[++n];
											hex_char *= 16;
											if( c >= '0' && c <= '9' )
												hex_char += c - '0';
											else if( c >= 'A' && c <= 'F' )
												hex_char += ( c - 'A' ) + 10;
											else if( c >= 'a' && c <= 'f' )
												hex_char += ( c - 'F' ) + 10;
											else
												lprintf( WIDE("(escaped character, parsing hex of \\u) fault parsing '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
														 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
										}
										VarTextAddCharacter( val.pvt_collector, (TEXTCHAR)hex_char );
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
								VarTextAddCharacter( val.pvt_collector, c );
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
				n++;
				break;

		//----------------------------------------------------------
      //    catch characters for true/false/null which are values outside of quotes
			case 't':
				if( word == 0 )
					word = 1;
				else
					lprintf( WIDE("fault parsing '%c' unexpected at %")_size_f, c, n );// fault
				n++;
				break;
			case 'r':
				if( word == 1 )
					word = 2;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'u':
				if( word == 2 )
					word = 3;
				else if( word == 21 )
					word = 22;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'e':
				if( word == 3 )
				{
					val.result_value = VALUE_TRUE;
					word = 0;
				}
				else if( word == 14 )
				{
					val.result_value = VALUE_FALSE;
					word = 0;
				}
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'n':
				if( word == 0 )
					word = 21;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'l':
				if( word == 22 )
					word = 23;
				else if( word == 23 )
				{
					word = 0;
					val.result_value = VALUE_NULL;
				}
				else if( word == 12 )
					word = 13;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'f':
				if( word == 0 )
					word = 11;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 'a':
				if( word == 11 )
					word = 12;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f, c, n );// fault
				n++;
				break;
			case 's':
				if( word == 13 )
					word = 14;
				else
					lprintf( WIDE("fault parsing '%c' unexpected %") _size_f, c, n );// fault
				n++;
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
					VarTextAddCharacter( val.pvt_collector, c );
					while( ( c = msg[++n] ) && ( n < msglen ) )
					{
						// leading zeros should be forbidden.
						if( ( c >= '0' && c <= '9' )
							|| ( c == '-' )
							|| ( c == '+' )
							|| ( c == '+' )
						  )
						{
							VarTextAddCharacter( val.pvt_collector, c );
						}
						else if( ( c =='e' ) || ( c == 'E' ) || ( c == '.' ) )
						{
							val.float_result = 1;
							VarTextAddCharacter( val.pvt_collector, c );
						}
						else
						{
							// this character needs to be checked again
							break;
						}
					}
					{
						PTEXT number = VarTextGet( val.pvt_collector );
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
					n++;
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
					n++;
				}
				break; // default
			}
			break; // default of high level switch
		}
	}

	{
		struct json_parse_context *old_context;
			while( old_context = (struct json_parse_context *)PopLink( &context_stack ) )
				Release( old_context );
		if( context_stack )
			DeleteLinkStack( &context_stack );
	}
	if( element_lists )
		DeleteLinkStack( &element_lists );
	// clean all contexts
	VarTextDestroy( &val.pvt_collector );

	if( !status )
	{
		Release( (*_msg_output) );
		(*_msg_output) = NULL;
	}
	return status;
}

LOGICAL json_parse_message( struct json_context *format
                           , CTEXTSTR msg
						   , size_t msglen
						   , struct json_context_object **result_context_object
									, POINTER *_msg_output )
{
	struct json_context_object *object;
	INDEX idx;
	LIST_FORALL( format->object_types, idx, struct json_context_object *, object )
	{
		if( json_parse_message_format( object, msg, msglen, _msg_output ) )
		{
			(*result_context_object) = object;
			return TRUE;
		}
	}
	return FALSE;
}

void json_dispose_message( struct json_context_object *format
                                             , POINTER msg_data
                                             )
{
	// quick method
	Release( msg_data );
}

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



