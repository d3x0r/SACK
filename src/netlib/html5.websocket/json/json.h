

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


struct json_context_object_element
{
	int type;     // type of the element at this offset
	int content_type;     // type of the element at this offset
	size_t offset;   // offset into the structure
	CTEXTSTR name; // name of this element in the object
	size_t count; // at offset, this number of these is there; (array)
	size_t count_offset; // at count_offset, is the number of elements that the pointer at this offset
	struct json_context_object *object;
};

struct json_context_object
{
	struct json_context *context;
	PLIST members;   // list of members of this object
	int is_array; // if set is an array format, otherwise is an object format.
   size_t object_size;
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


