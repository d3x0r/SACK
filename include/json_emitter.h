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
   JSON_Element_Object,
   JSON_Element_ObjectPointer,
   JSON_Element_List,
   JSON_Element_Text,  // ptext type
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
																						  , int type
																						  , size_t count
																						  , size_t count_offset
																						  );

// add a member element to a json object
// if the member element is a object type, then a new context_object results, to which members may be added.
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member )( struct json_context_object *object
																								 , CTEXTSTR name
																								 , size_t offset
																								 , int type
																								 , size_t object_size
																								 );

// more complex method; add_object_member actually calls this to implement a 0 byte array of the same type.
//  object_size is used if the type is JSON_Element_ObjectPointer for the parsing to be able to allocate
// the message part.
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_array )( struct json_context_object *format
																										 , CTEXTSTR name
																										 , size_t offset
																										 , int type
                                                                               , size_t object_size
																										 , size_t count
																										 , size_t count_offset
																										 );

// adds a reference to a PLIST as an array with the content of the array specified as the type
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_list )( struct json_context_object *object
																										, CTEXTSTR name
																										, size_t offset  // offset of the list
																										, int content_type // of of the members of the list
																										, size_t object_size  // object size if required
																										);

// this allows recursive structures, so the structure may contain a reference to itself.
// this allows buildling other objects and referencing them instead of building them in-place
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_object )( struct json_context_object *object
																										  , CTEXTSTR name
																										  , size_t offset
																										  , int type
																										  , struct json_context_object *child_object
																										  );


// take a object format and a pointer to data and return a json message string
JSON_EMITTER_PROC( CTEXTSTR, json_build_message )( struct json_context_object *format
																 , POINTER msg );

// take a json string and a format and fill in a structure from the text.
JSON_EMITTER_PROC( LOGICAL, json_parse_message )( struct json_context_object *format
                                             , CTEXTSTR msg
															, POINTER *msg_data_out
															);
// any allocate mesage parts are released.
JSON_EMITTER_PROC( void, json_dispose_message )( struct json_context_object *format
                                             , CTEXTSTR msg
															, POINTER msg_data_out
															);

#ifdef __cplusplus
} } SACK_NAMESPACE_END 
#endif

#endif
