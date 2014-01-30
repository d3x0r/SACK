

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


struct json_context_object_element
{
	enum JSON_ObjectElementTypes type;     // type of the element at this offset
	enum JSON_ObjectElementTypes content_type;     // type of the element at this offset
	size_t object_size;  // how big this element is.
	size_t offset;   // offset into the structure
	CTEXTSTR name; // name of this element in the object
	size_t count; // at offset, this number of these is there; (array)
	size_t count_offset; // at count_offset, is the number of elements that the pointer at this offset
	void (*user_formatter)(PVARTEXT pvt_output,CPOINTER msg_data);
	struct json_context_object *object;
};

struct json_context_object
{
	struct json_context *context;
	PLIST members;   // list of members of this object
	int is_array; // if set is an array format, otherwise is an object format.
	size_t object_size;
	size_t offset;
	struct json_context_object_flags
	{
		BIT_FIELD keep_phrase : 1;  // this is not a root object
		BIT_FIELD dynamic_size : 1;
	} flags;
	struct json_context_object *parent; 
};

struct json_context
{
	int levels;
	PVARTEXT pvt;
	PLIST object_types;
};

#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif


