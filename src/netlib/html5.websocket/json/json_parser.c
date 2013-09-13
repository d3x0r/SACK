#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>

#include "json.h"


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

};


LOGICAL json_parse_message( struct json_context *context
									, struct json_context_object *format
                           , CTEXTSTR msg
									, POINTER msg_output )
{
	/* I guess this is a good parser */
   PLIST elements;
	struct json_context_object_element *element;
   INDEX idx;
	size_t n = 0; // character index;
	PVARTEXT pvt_collector = VarTextCreate();;
	PTEXT name = NULL;
	PTEXT value;
   int word;
	TEXTCHAR c;
	TEXTCHAR quote = 0;
	LOGICAL escape = FALSE;
	LOGICAL use_char = FALSE;
	PLINKSTACK element_lists = NULL;

	PLINKSTACK context_stack = NULL;

	PLINKQUEUE array_values = NULL;
   struct array_element *new_array_element;

   LOGICAL first_token = TRUE;
	enum json_parse_state state;

   int context = CONTEXT_UNKNOWN;
	double result_d;
	S_64 result_n;
   int result_value = 0;
	int float_result;


//	static CTEXTSTR keyword[3] = { "false", "null", "true" };

   while( c = msg[n] )
	{
		switch( c )
		{
		case '{':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = context;
				old_context->elments = elements;
				PushLink( &context_stack, old_context );

				context = CONTEXT_IN_OBJECT;
				if( first_token )
				{
					// begin an object.
					// content is
					elements = format->elements;
					PushLink( &element_lists, elements );
					first_token = 0;
				}
				else
				{
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element, element )
					{
						if( StrCaseCmp( element->name, GetText( name ) ) == 0 )
						{
							if( ( element->type == JSON_Element_Object )
								|| ( element->type == JSON_Element_ObjectPointer ) )
							{
								if( element->object )
								{
									elements = element->object->members;
									break;
								}
							}
							else
							{
                        lprintf( "Incompatible value expected object type, type is %d", element->type );
							}
						}
					}
				}
			}
			break;

		case '[':
			{
				struct json_parse_context *old_context = New( struct json_parse_context );
				old_context->context = context;
				old_context->elments = elements;
				PushLink( &context_stack, old_context );

            context = CONTEXT_IN_ARRAY;
				if( first_token )
				{
					elements = format->elements;
					PushLink( &element_lists, elements );
					first_token = 0;
				}
				else
				{
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element, element )
					{
						if( StrCaseCmp( element->name, GetText( name ) ) == 0 )
						{
							if( ( element->count_offset >= 0 ) || element->count )
							{
                        break;
							}
							else
							{
                        lprintf( "Incompatible value; expected element that can take an array, type is %d", element->type );
							}
						}
					}
				}
			}
			break;

		case ':':
			if( context == CONTEXT_IN_OBJECT )
			{
				// what is next; a array?object?string?number?
				if( name )
					LineRelease( name );
				name = VarTextGet( pvt_collector );
				state |= JSON_PARSE_STATE_PICK_WHATS_NEXT;
			}
			else
			{
			}
			break;
		case '}':
			if( context == CONTEXT_IN_OBJECT )
			{
				// first, add the last value

				switch( element->type )
				{
				case JSON_Element_String:
					if( element->count )
					{
					}
					else if( element->count_offset >= 0 )
					{
					}
					else
					{
						switch( result_value )
						{
						case VALUE_NULL:
							(CTEXTSTR*)( ((PTRSZVAL)msg_output) + element->offset )[0] = NULL;
                     break;
						case VALUE_STRING:
							if( name )
                        LineRelease( name );
                     name = VarTextGet( pvt_collector );
							(CTEXTSTR*)( ((PTRSZVAL)msg_output) + element->offset )[0] = StrDup( GetText( name ) );
							break;
						default:
                     lprintf( "Expected a string, but parsed result was a %d", result_value );
                     break;
						}
					}
               break;
				case JSON_Element_Integer:
					if( element->count )
					{
					}
					else if( element->count_offset >= 0 )
					{
					}
					else
					{
						switch( result_value )
						{
						case VALUE_TRUE:
							(S_64*)( ((PTRSZVAL)msg_output) + element->offset )[0] = 1;
							break;
						case VALUE_FALSE:
							(S_64*)( ((PTRSZVAL)msg_output) + element->offset )[0] = 0;
							break;
						case VALUE_NUMBER:
							if( float_result )
							{
                        lprintf( "warning received float, converting to int" );
								(S_64*)( ((PTRSZVAL)msg_output) + element->offset )[0] = (S_64)result_d;
							}
							else
							{
								(S_64*)( ((PTRSZVAL)msg_output) + element->offset )[0] = result_n;
							}
							break;
						default:
                     lprintf( "Expected a string, but parsed result was a %d", result_value );
                     break;
						}
					}
					break;

				case JSON_Element_Float:
				case JSON_Element_Double:
					if( element->count )
					{
					}
					else if( element->count_offset >= 0 )
					{
					}
					else
					{
						switch( result_value )
						{
						case VALUE_NUMBER:
							if( float_result )
							{
								if( element->type == JSON_Element_Float )
									(float*)( ((PTRSZVAL)msg_output) + element->offset )[0] = (float)result_f;
								else
									(double*)( ((PTRSZVAL)msg_output) + element->offset )[0] = result_n;

							}
							else
							{
                        // this is probably common (0 for instance)
								lprintf( "warning received int, converting to float" );
								if( element->type == JSON_Element_Float )
									(float*)( ((PTRSZVAL)msg_output) + element->offset )[0] = (float)result_n;
								else
									(double*)( ((PTRSZVAL)msg_output) + element->offset )[0] = (double)result_n;

							}
							break;
						default:
                     lprintf( "Expected a float, but parsed result was a %d", result_value );
                     break;
						}
					}

               break;
				}


				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					context = old_context->context;
					elements = old_context->elements;
					Release( old_context );
				}



			}
			else
			{
			}
			break;
		case ']':
			if( context == CONTEXT_IN_ARRAY )
			{
				struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
				context = old_context->context;
				elements = old_context->elements;
            Release( old_context );
			}
			else
			{
				lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
			}
			break;
		case ',':
			if( context == CONTEXT_IN_ARRAY )
			{
				switch( result_value )
				{
				case VALUE_STRING:
					break;
				}
			}
			else if( context == CONTEXT_IN_OBJECT )
			{
				switch( result_value )
				{

				}
			}
			else
			{
				lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
			}
			break;

		default:
			if( first_token )
			{
				lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
            return FALSE;
			}
			switch( c )
			{


			case '"':
				{
					// collect a string
					int escape = 0;
					while( c = msg[++n] )
					{
						if( c == '\\' )
						{
							if( escape )
								VarTextAddCharacter( pvt_collector, '\\' );
							else
								escape = 1;
						}
						else if( c == '"' )
						{
                     if( escape )
								VarTextAddCharacter( pvt_collector, '\"' );
							else
                        break;
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
									VarTextAddCharacter( pvt_collector, c );
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
												lprintf( "(escaped character, parsing hex of \u) fault parsing '%c' unexpected %d (near %*.*s[%c]%s)", c, n
														 , ( (n>3)?3:n ), ( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
										}
                              VarTextAddCharacter( pvt_collector, (TEXTCHAR)hex_char );
									}
									break;
								default:
									lprintf( "(escaped character) fault parsing '%c' unexpected %d (near %*.*s[%c]%s)", c, n
											 , ( (n>3)?3:n ), ( (n>3)?3:n )
											 , msg + n - ( (n>3)?3:n )
											 , c
											 , msg + n + 1
											 );// fault
									break;
								}
                        escape = 0;
							}
                     else
								VarTextAddCharacter( pvt_collector, c );
						}
					}
               result_value = VALUE_STRING;
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
					lprintf( "fault parsing '%c' unexpected at %d", c, n );// fault
				n++;
				break;
			case 'r':
				if( word == 1 )
               word = 2;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'u':
				if( word == 2 )
               word = 3;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'e':
				if( word == 3 )
				{
               result_value = VALUE_TRUE;
					word = 0;
				}
				else if( word == 14 )
				{
               result_value = VALUE_FALSE;
					word = 0;
				}
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'n':
				if( word == 0 )
					word = 21;
				else
					lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'u':
				if( word == 21 )
					word = 22;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'l':
				if( word == 22 )
					word = 23;
				else if( word == 23 )
				{
					word = 0;
               result_value = VALUE_NULL;
				}
				else if( word == 12 )
					word = 13;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'f':
				if( word == 0 )
					word = 11;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 'a':
				if( word == 11 )
					word = 12;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
				n++;
            break;
			case 's':
				if( word == 13 )
					word = 14;
				else
               lprintf( "fault parsing '%c' unexpected %d", c, n );// fault
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
					float_result = 0;
					VarTextAddCharacter( pvt_collector, c );
					while( c = msg[++n] )
					{
						// leading zeros should be forbidden.
						if( ( c >= '0' && c <= '9' )
							|| ( c == '-' )
							|| ( c == '+' )
							|| ( c == '+' )
						  )
						{
							VarTextAddCharacter( pvt_collector, c );
						}
						else if( ( c =='e' ) || ( c == 'E' ) || ( c == '.' ) )
						{
							float_result = 1;
							VarTextAddCharacter( pvt_collector, c );
						}
						else
						{
                     // this character needs to be checked again
                     n--;
							break;
						}
					}
					{
						PTEXT number = VarTextGet( pvt_collector );
						if( float_result )
						{
							result_d = FloatCreateFromSeg( number );
						}
						else
						{
							result_n = IntCreateFromSeg( number );
						}
                  LineRelease( number );
					}
					result_value = VALUE_NUMBER;
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
					lprintf( "fault parsing '%c' unexpected %d (near %*.*s[%c]%s)", c, n
							 , ( (n>3)?3:n ), ( (n>3)?3:n )
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



}
