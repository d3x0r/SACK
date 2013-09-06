

#ifdef JSON_EMITTER_SOURCE
#define JSON_EMITTER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define JSON_EMITTER_PROC(type,name) IMPORT_METHOD type CPROC name
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
}

struct json_context_object_element;
struct json_context_object;
struct json_context;




JSON_EMITTER_PROC( struct json_context_object *, json_create_object )( struct json_context *context, CTEXTSTR name );
JSON_EMITTER_PROC( void, json_add_object_member )( struct json_context_object *format, CTEXTSTR name, int offset, int type );

