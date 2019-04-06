
#include <jsox_parser.h>

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace jsox {
#endif

#if JSOX_EMITTER_WORKS
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
	PLIST members;   // list of members of this object struct json_context_object_element *
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
	int human_readable;
};
#endif

enum jsox_word_char_states {
	JSOX_WORD_POS_RESET = 0, // not in a keyword
	JSOX_WORD_POS_END,  // at end of a word, waiting for separator
	JSOX_WORD_POS_TRUE_1,
	JSOX_WORD_POS_TRUE_2,
	JSOX_WORD_POS_TRUE_3,
	//JSOX_WORD_POS_TRUE_4,
	JSOX_WORD_POS_FALSE_1, // 11
	JSOX_WORD_POS_FALSE_2,
	JSOX_WORD_POS_FALSE_3,
	JSOX_WORD_POS_FALSE_4,
	JSOX_WORD_POS_NULL_1, // 21  get u
	JSOX_WORD_POS_NULL_2, //  get l
	JSOX_WORD_POS_NULL_3, // 10 get l
	JSOX_WORD_POS_UNDEFINED_1,  // 31 
	JSOX_WORD_POS_UNDEFINED_2,
	JSOX_WORD_POS_UNDEFINED_3,
	JSOX_WORD_POS_UNDEFINED_4,
	JSOX_WORD_POS_UNDEFINED_5,
	JSOX_WORD_POS_UNDEFINED_6,
	JSOX_WORD_POS_UNDEFINED_7,
	JSOX_WORD_POS_UNDEFINED_8,
	//JSOX_WORD_POS_UNDEFINED_9, // instead of stepping to this value here, go to RESET
	JSOX_WORD_POS_NAN_1,
	JSOX_WORD_POS_NAN_2,  // 20 
	//JSOX_WORD_POS_NAN_3,// instead of stepping to this value here, go to RESET
	JSOX_WORD_POS_INFINITY_1,
	JSOX_WORD_POS_INFINITY_2,
	JSOX_WORD_POS_INFINITY_3,
	JSOX_WORD_POS_INFINITY_4,
	JSOX_WORD_POS_INFINITY_5,
	JSOX_WORD_POS_INFINITY_6,
	JSOX_WORD_POS_INFINITY_7,
	//JSOX_WORD_POS_INFINITY_8,// instead of stepping to this value here, go to RESET
	JSOX_WORD_POS_FIELD, 
	JSOX_WORD_POS_AFTER_FIELD, 
	JSOX_WORD_POS_DOT_OPERATOR, // 30
	JSOX_WORD_POS_PROPER_NAME, 
	JSOX_WORD_POS_AFTER_PROPER_NAME, 
	JSOX_WORD_POS_AFTER_GET,
	JSOX_WORD_POS_AFTER_SET,
	
	JSOX_WORD_POS_CLASS_NAME,  //36
	JSOX_WORD_POS_CLASS_VALUES, // 37
};

enum jsox_parse_context_modes {
	JSOX_CONTEXT_UNKNOWN = 0,
	JSOX_CONTEXT_IN_ARRAY = 1,
	//JSOX_CONTEXT_IN_OBJECT = 2,
	JSOX_CONTEXT_OBJECT_FIELD = 3,
	JSOX_CONTEXT_OBJECT_FIELD_VALUE = 4,
	JSOX_CONTEXT_CLASS_FIELD = 5,
	JSOX_CONTEXT_CLASS_VALUE = 6,
	JSOX_CONTEXT_CLASS_FIELD_VALUE = 7, // same as OBJECT_FIELD_VALUE; but within a CLASS_VALUE state
};


#define JSOX_RESET_VAL()  {  \
	val.value_type = JSOX_VALUE_UNSET; \
	val.contains = NULL;              \
	val._contains = NULL;             \
	val.name = NULL;                  \
	val.string = NULL;                \
	val.className = NULL;             \
	negative = FALSE; }
#define JSOX_RESET_STATE_VAL()  {  \
	state->val.value_type = JSOX_VALUE_UNSET; \
	state->val.contains = NULL;              \
	state->val._contains = NULL;             \
	state->val.name = NULL;                  \
	state->val.string = NULL;                \
	state->val.className = NULL;             \
	state->negative = FALSE; }

struct jsox_input_buffer {
	char const * buf;      // prior input buffer
	size_t       size; // size of prior input buffer
	char const * pos;  // last position in _input if context closed before end of buffer
	LOGICAL      tempBuf;
};

struct jsox_output_buffer {
	char * buf;      // prior input buffer
	size_t  size; // size of prior input buffer
	char * pos;  // last position in _input if context closed before end of buffer
	LOGICAL      unusedTempBuf;
};

typedef struct jsox_input_buffer JSOX_PARSE_BUFFER, *PJSOX_PARSE_BUFFER;
#define MAXJSOX_PARSE_BUFFERSPERSET 128
DeclareSet( JSOX_PARSE_BUFFER );


struct jsox_class_field {
	char *name;
	size_t nameLen;
};

typedef struct jsox_class_field JSOX_CLASS_FIELD, *PJSOX_CLASS_FIELD;
#define MAXJSOX_CLASS_FIELDSPERSET 128
DeclareSet( JSOX_CLASS_FIELD );

struct jsox_class_type {
	char *name;
	size_t nameLen;
	PLIST fields;
};
typedef struct jsox_class_type JSOX_CLASS, *PJSOX_CLASS;
#define MAXJSOX_CLASSSPERSET 128
DeclareSet( JSOX_CLASS );

struct jsox_parse_context {
	enum jsox_parse_context_modes context;
	PDATALIST *elements;
	char *name;	
	size_t nameLen;	
	struct jsox_value_container valState;
	//struct jsox_context_object *object;
	PJSOX_CLASS current_class;
	int current_class_item;
	int arrayType;
};
typedef struct jsox_parse_context JSOX_PARSE_CONTEXT, *PJSOX_PARSE_CONTEXT;
#define MAXJSOX_PARSE_CONTEXTSPERSET 128
DeclareSet( JSOX_PARSE_CONTEXT );


// this is the stack state that can be saved between parsing for streaming.
struct jsox_parse_state {
	//TEXTRUNE c;
	PDATALIST *elements;
	PLINKSTACK *outBuffers; // 
	PLINKQUEUE *outQueue; // matches input queue
	PLIST *outValBuffers;
	//TEXTSTR mOut;// = NewArray( char, msglen );

	size_t line;
	size_t col;
	size_t n; // character index;
	//size_t _n = 0; // character index; (restore1)
	enum jsox_word_char_states word;
	LOGICAL status;
	LOGICAL negative;
	LOGICAL literalString;

	PLINKSTACK *context_stack;

	PLIST classes;
	PJSOX_CLASS current_class;
	int current_class_item;
	int arrayType;

	LOGICAL first_token;
	PJSOX_PARSE_CONTEXT context;
	enum jsox_parse_context_modes parse_context;
	struct jsox_value_container val;
	int comment;
	TEXTRUNE operatorAccum;

	PLINKQUEUE *inBuffers;
	//char const * input;     // current input buffer start
	//char const * msg_input; // current input buffer position (incremented while reading)

	LOGICAL completed;
	LOGICAL complete_at_end;
	LOGICAL gatheringString;
	LOGICAL completedString;
	TEXTRUNE gatheringStringFirstChar;
	TEXTRUNE gatheringCodeLastChar; 
	int codeDepth; /* string overload with code */
	LOGICAL gatheringNumber;
	LOGICAL numberExponent;
	LOGICAL numberFromHex;
	LOGICAL numberFromDate;
	LOGICAL numberFromBigInt;

	PVARTEXT pvtError;
	LOGICAL fromHex;
	LOGICAL exponent;
	LOGICAL exponent_sign;
	LOGICAL exponent_digit;

	LOGICAL escape;
	LOGICAL cr_escaped;
	LOGICAL unicodeWide;
	LOGICAL stringUnicode;
	LOGICAL stringHex;
	TEXTRUNE hex_char;
	int hex_char_len;
	LOGICAL stringOct;

	LOGICAL weakSpace;

	PDATALIST root;
	//char *token_begin;
};
typedef struct jsox_parse_state JSOX_PARSE_STATE, *PJSOX_PARSE_STATE;
#define MAXJSOX_PARSE_STATESPERSET 32
DeclareSet( JSOX_PARSE_STATE );

#ifndef JSON_PARSER_INCLUDED
typedef PLIST *PPLIST;
#define MAXPLISTSPERSET 256
DeclareSet( PLIST );

typedef PLINKSTACK *PPLINKSTACK;
#define MAXPLINKSTACKSPERSET 256
DeclareSet( PLINKSTACK );

typedef PLINKQUEUE *PPLINKQUEUE;
#define MAXPLINKQUEUESPERSET 256
DeclareSet( PLINKQUEUE );

typedef PDATALIST *PPDATALIST;
#define MAXPDATALISTSPERSET 256
DeclareSet( PDATALIST );
#endif

struct jsox_parser_shared_data {
	PJSOX_PARSE_CONTEXTSET parseContexts;
	PJSOX_PARSE_BUFFERSET parseBuffers;
	struct jsox_parse_state *last_parse_state;
	PJSOX_PARSE_STATESET parseStates;
	PPLISTSET listSet;
	PPLINKSTACKSET linkStacks;
	PPLINKQUEUESET linkQueues;
	PPDATALISTSET dataLists;

	PJSOX_CLASSSET  classes;
	PJSOX_CLASS_FIELDSET  class_fields;

	struct jsox_parse_state *_state; // static parsing state for simple message interface.

};
#ifndef JSOX_PARSER_MAIN_SOURCE
extern
#endif
struct jsox_parser_shared_data jxpsd;

void _jsox_dispose_message( PDATALIST *msg_data );


#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif


