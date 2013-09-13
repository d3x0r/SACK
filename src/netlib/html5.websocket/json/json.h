

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


struct json_context_object_element
{
	int type;     // type of the element at this offset
	int offset;   // offset into the structure
	CTEXTSTR name; // name of this element in the object
	int count; // at offset, this number of these is there; (array)
	int count_offset; // at count_offset, is the number of elements that the pointer at this offset
   struct json_context_object *object;
};

struct json_context_object
{
   PLIST members;   // list of members of this object
   CTEXTSTR name;   // name of this object (might be an object within an object)
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


