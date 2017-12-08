#ifndef JSON_EMITTER_HEADER_INCLUDED
#define JSON_EMITTER_HEADER_INCLUDED


#ifdef JSON_EMITTER_SOURCE
#define JSON_EMITTER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define JSON_EMITTER_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


enum JSON_ObjectElementTypes
{
   JSON_Element_Integer_8,
   JSON_Element_Integer_16,
   JSON_Element_Integer_32,
   JSON_Element_Integer_64,
   JSON_Element_Unsigned_Integer_8,
   JSON_Element_Unsigned_Integer_16,
   JSON_Element_Unsigned_Integer_32,
   JSON_Element_Unsigned_Integer_64,
   JSON_Element_String,
   JSON_Element_CharArray,
   JSON_Element_Float,
   JSON_Element_Double,
   JSON_Element_Array,  // result will fill a PLIST
   JSON_Element_Object,
   JSON_Element_ObjectPointer,
   JSON_Element_List,
   JSON_Element_Text,  // ptext type
   JSON_Element_PTRSZVAL,  
   JSON_Element_PTRSZVAL_BLANK_0,
	JSON_Element_UserRoutine,
	JSON_Element_Raw_Object, // unparsed object remainder.  Includes bounding { } object indicator for re-parsing
   //JSON_Element_StaticText,  // text type; doesn't happen very often.
};

struct json_context_object_element;
struct json_context_object;
struct json_context;


#define JSON_NO_OFFSET (size_t)-1

// Get a context, which can track message formats.
// Will eventually expose the low level routines so one can use a context
// and the simple message building utility functions to product json output
// without defining objects and members....
JSON_EMITTER_PROC( struct json_context *, json_create_context )( void );

// Begin the definition of a json formatting object.
// the root element must be a array or an object
JSON_EMITTER_PROC( struct json_context_object *, json_create_object )( struct json_context *context
																							, size_t object_size );
// Begin the definition of a json formatting object.
// the root element must be a array or an object
JSON_EMITTER_PROC( struct json_context_object *, json_create_array )( struct json_context *context
																						  , size_t offset
																						  , enum JSON_ObjectElementTypes type
																						  , size_t count
																						  , size_t count_offset
																						  );

// add a member element to a json object
// if the member element is a object type, then a new context_object results, to which members may be added.
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member )( struct json_context_object *object
																								 , CTEXTSTR name
																								 , size_t offset
																								 , enum JSON_ObjectElementTypes type
																								 , size_t object_size
																								 );

// more complex method; add_object_member actually calls this to implement a 0 byte array of the same type.
//  object_size is used if the type is JSON_Element_ObjectPointer for the parsing to be able to allocate
// the message part.
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_array )( struct json_context_object *format
																										 , CTEXTSTR name
																										 , size_t offset
																										 , enum JSON_ObjectElementTypes type
                                                                               , size_t object_size
																										 , size_t count
																										 , size_t count_offset
																										 );

// more complex method; add_object_member actually calls this to implement a 0 byte array of the same type.
//  object_size is used if the type is JSON_Element_ObjectPointer for the parsing to be able to allocate
// the message part.  array is represented as a pointer, which will be dynamically allocated
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_array_pointer )( struct json_context_object *format
																										 , CTEXTSTR name
																										 , size_t offset
																										 , enum JSON_ObjectElementTypes type
																										 , size_t count_offset
																										 );

// adds a reference to a PLIST as an array with the content of the array specified as the type
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_list )( struct json_context_object *object
																										, CTEXTSTR name
																										, size_t offset  // offset of the list
																										, enum JSON_ObjectElementTypes content_type // of of the members of the list
																										, size_t object_size  // object size if required
																										);

// this allows recursive structures, so the structure may contain a reference to itself.
// this allows buildling other objects and referencing them instead of building them in-place
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_object )( struct json_context_object *object
																										  , CTEXTSTR name
																										  , size_t offset
																										  , enum JSON_ObjectElementTypes type
																										  , struct json_context_object *child_object
																										  );

JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_user_routine )( struct json_context_object *object
																												  , CTEXTSTR name
																												  , size_t offset, enum JSON_ObjectElementTypes type
																												  , size_t object_size
																												  , void (*user_formatter)(PVARTEXT,CPOINTER) );

// take a object format and a pointer to data and return a json message string
JSON_EMITTER_PROC( TEXTSTR, json_build_message )( struct json_context_object *format
                                             , POINTER msg );

// take a json string and a format and fill in a structure from the text.
// tests all formats, to first-match; 
// take a json string and a format and fill in a structure from the text.
// if object does not fit all members (may have extra, but must have at least all members in message in format to return TRUE)
// then it returns false; that is if a member is in the 'msg' parameter that is not in
// the format, then the result is FALSE.
//  PDATALIST is full of struct json_value_container
// turns out numbers can be  hex, octal and binary numbers  (0x[A-F,a-f,0-9]*, 0b[0-1]*, 0[0-9]*)
// slightly faster (17%) than json6_parse_message because of fewer possible checks.
JSON_EMITTER_PROC( LOGICAL, json_parse_message )(const char * msg
                                                , size_t msglen
                                                , PDATALIST *msg_data_out
																);

// allocates a parsing context and begins parsing data.
JSON_EMITTER_PROC( struct json_parse_state *, json_begin_parse )( void );
// return TRUE when a completed value/object is available.
// after returning TRUE, call json_parse_get_data.  It is possible that there is
// still unconsumed data that can begin a new object.  Call this with NULL, 0 for data
// to consume this internal data.  if this returns FALSE, then ther is no further object
// to retrieve.
JSON_EMITTER_PROC( int, json_parse_add_data )( struct json_parse_state *context
                                                 , const char * msg
                                                 , size_t msglen
                                                 );


// these are common functions that work for json or json6 stream parsers
JSON_EMITTER_PROC( PDATALIST, json_parse_get_data )( struct json_parse_state *context );
JSON_EMITTER_PROC( void, json_parse_dispose_state )( struct json_parse_state **context );
JSON_EMITTER_PROC( void, json_parse_clear_state )(struct json_parse_state *context);
JSON_EMITTER_PROC( PTEXT, json_parse_get_error )(struct json_parse_state *context);

// take a json string and a format and fill in a structure from the text.
// tests all formats, to first-match; 
// take a json string and a format and fill in a structure from the text.
// if object does not fit all members (may have extra, but must have at least all members in message in format to return TRUE)
// then it returns false; that is if a member is in the 'msg' parameter that is not in
// the format, then the result is FALSE.
//  PDATALIST is full of struct json_value_container
//   JSON5 support - Infinity/Nan, string continuations, and comments,unquoted field names; hex, octal and binary numbers
//       unquoted field names must be a valid javascript keyword using unicode ID_Start/ID_Continue states to determine valid characters.
//       this is arbitrary though; and could be reverted to just accepting any character other than ':'.
//   JSON(6?) support - undefined keyword value
//       accept \uXXXX, \xXX, \[0-3]xx octal, \u{xxxxx} encodings in strings
//       allow underscores in numbers to separate number groups ( works as ZWNBSP )
JSON_EMITTER_PROC( LOGICAL, json6_parse_message )(const char * msg
													, size_t msglen
													, PDATALIST *msg_data_out
																);
JSON_EMITTER_PROC( LOGICAL, _json6_parse_message )(char * msg
	, size_t msglen
	, PDATALIST *msg_data_out
	);

// Add some data to parse for json stream (which may consist of multiple values)
// return 1 when a completed value/object is available.
// after returning 1, call json_parse_get_data.  It is possible that there is
// still unconsumed data that can begin a new object.  Call this with NULL, 0 for data
// to consume this internal data.  if this returns 0, then there is no further object
// to retrieve.
// if this returns -1, an error in parsing has occured, and no further parsing can happen.
JSON_EMITTER_PROC( int, json6_parse_add_data )( struct json_parse_state *context
                                                 , const char * msg
                                                 , size_t msglen
                                                 );



JSON_EMITTER_PROC( LOGICAL, json_decode_message )(  struct json_context *format
                                                  , PDATALIST parsedMsg
                                                  , struct json_context_object **result_format
                                                  , POINTER *msg_data_out
												);

enum json_value_types {
	VALUE_UNDEFINED = -1
	, VALUE_UNSET = 0
	, VALUE_NULL //= 1
	, VALUE_TRUE //= 2
	, VALUE_FALSE //= 3
	, VALUE_STRING //= 4
	, VALUE_NUMBER //= 5
	, VALUE_OBJECT //= 6
	, VALUE_ARRAY //= 7
	, VALUE_NEG_NAN //= 8
	, VALUE_NAN //= 9
	, VALUE_NEG_INFINITY //= 10
	, VALUE_INFINITY //= 11
	, VALUE_DATE  // = 12
	, VALUE_EMPTY
};

struct json_value_container {
	char * name;  // name of this value (if it's contained in an object)
	size_t nameLen;
	enum json_value_types value_type; // value from above indiciating the type of this value
	char *string;   // the string value of this value (strings and number types only)
	size_t stringLen;
	int float_result;  // boolean whether to use result_n or result_d
	double result_d;
	int64_t result_n;
	PDATALIST contains;  // list of struct json_value_container that this contains.
};


// any allocate mesage parts are released.
JSON_EMITTER_PROC( void, json_dispose_message )( PDATALIST *msg_data );
// any allocate mesage parts are released.
JSON_EMITTER_PROC( void, json6_dispose_message )( PDATALIST *msg_data );
JSON_EMITTER_PROC( void, json_dispose_decoded_message )(struct json_context_object *format
	, POINTER msg_data);

// sanitize strings to send in JSON so quotes don't prematurely end strings and output is still valid.
// require Release the result.
JSON_EMITTER_PROC( char*, json_escape_string )( const char * string );
// sanitize strings to send in JSON so quotes don't prematurely end strings and output is still valid.
// require Release the result.  pass by length so \0 characters can be kept and don't early terminate.  Result with new length also.
JSON_EMITTER_PROC( char*, json_escape_string_length )( const char *string, size_t length, size_t *outlen );
// sanitize strings to send in JSON6 so quotes don't prematurely end strings and output is still valid.
// require Release the result.  Also escapes not just double-quotes ("), but also single and ES6 Format quotes (', `)
// this does not translate control chararacters like \n, \t, since strings are allowed to be muliline.
JSON_EMITTER_PROC( char*, json6_escape_string )( const char * string );
// sanitize strings to send in JSON6 so quotes don't prematurely end strings and output is still valid.
// require Release the result.  pass by length so \0 characters can be kept and don't early terminate.  Result with new length also.
// this does not translate control chararacters like \n, \t, since strings are allowed to be muliline.
JSON_EMITTER_PROC( char*, json6_escape_string_length )( const char *string, size_t len, size_t *outlen );

#ifdef __cplusplus
} } SACK_NAMESPACE_END
using namespace sack::network::json;
#endif

#endif
