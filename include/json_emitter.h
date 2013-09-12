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
   JSON_Element_Integer,
   JSON_Element_String,
   JSON_Element_CharArray,
   JSON_Element_Float,
   JSON_Element_Double,
   JSON_Element_Object,
   JSON_Element_ObjectPointer,
};

struct json_context_object_element;
struct json_context_object;
struct json_context;


// Get a context, which can track message formats.
// Will eventually expose the low level routines so one can use a context
// and the simple message building utility functions to product json output
// without defining objects and members....
JSON_EMITTER_PROC( struct json_context *, json_create_context )( void );

// Begin the definition of a json formatting object.
JSON_EMITTER_PROC( struct json_context_object *, json_create_object )( struct json_context *context, CTEXTSTR name );

// add a member element to a json object
// if the member element is a object type, then a new context_object results, to which members may be added.
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member )( struct json_context *context, struct json_context_object *format, CTEXTSTR name, int offset, int type );

// take a object format and a pointer to data and return a json message string
JSON_EMITTER_PROC( CTEXTSTR, json_build_message )( struct json_context *context
																 , struct json_context_object *format
																 , POINTER msg );

// take a json string and a format and fill in a structure from the text.
JSON_EMITTER_PROC( void, json_parse_message )( struct json_context *context
															, struct json_context_object *format
                                             , CTEXTSTR msg
															, POINTER msg_data_out
															);

#ifdef __cplusplus
} } SACK_NAMESPACE_END 
#endif

#endif
