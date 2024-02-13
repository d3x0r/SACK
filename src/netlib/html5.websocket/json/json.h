
#include <json_emitter.h>

#define JSON_PARSER_INCLUDED
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


enum word_char_states {
	WORD_POS_RESET = 0, // not in a keyword
	WORD_POS_END,  // at end of a word, waiting for separator
	WORD_POS_TRUE_1,
	WORD_POS_TRUE_2,
	WORD_POS_TRUE_3,
	WORD_POS_TRUE_4,
	WORD_POS_FALSE_1, // 11
	WORD_POS_FALSE_2,
	WORD_POS_FALSE_3,
	WORD_POS_FALSE_4,
	WORD_POS_NULL_1, // 21  get u
	WORD_POS_NULL_2, //  get l
	WORD_POS_NULL_3, //  get l
	WORD_POS_UNDEFINED_1,  // 31 
	WORD_POS_UNDEFINED_2,
	WORD_POS_UNDEFINED_3,
	WORD_POS_UNDEFINED_4,
	WORD_POS_UNDEFINED_5,
	WORD_POS_UNDEFINED_6,
	WORD_POS_UNDEFINED_7,
	WORD_POS_UNDEFINED_8,
	//WORD_POS_UNDEFINED_9, // instead of stepping to this value here, go to RESET
	WORD_POS_NAN_1,
	WORD_POS_NAN_2,
	//WORD_POS_NAN_3,// instead of stepping to this value here, go to RESET
	WORD_POS_INFINITY_1,
	WORD_POS_INFINITY_2,
	WORD_POS_INFINITY_3,
	WORD_POS_INFINITY_4,
	WORD_POS_INFINITY_5,
	WORD_POS_INFINITY_6,
	WORD_POS_INFINITY_7,
	//WORD_POS_INFINITY_8,// instead of stepping to this value here, go to RESET
	WORD_POS_FIELD, 
	WORD_POS_AFTER_FIELD, 
	WORD_POS_DOT_OPERATOR,
	WORD_POS_PROPER_NAME, 
	WORD_POS_AFTER_PROPER_NAME, 
	WORD_POS_AFTER_GET,
	WORD_POS_AFTER_SET,
};

enum parse_context_modes {
 CONTEXT_UNKNOWN = 0,
 CONTEXT_IN_ARRAY = 1,
 CONTEXT_IN_OBJECT = 2,
 CONTEXT_OBJECT_FIELD = 3,
 CONTEXT_OBJECT_FIELD_VALUE = 4,
 };

struct json_parse_context {
	enum parse_context_modes context;
	PDATALIST *elements;
	char *name;	
	size_t nameLen;	
	struct json_value_container valState;
	struct json_context_object *object;
};

#define RESET_VAL()  {  \
	val.value_type = VALUE_UNSET; \
	val.contains = NULL;              \
	val._contains = NULL;             \
	val.name = NULL;                  \
	val.string = NULL;                \
	negative = FALSE; }
#define RESET_STATE_VAL()  {  \
	state->val.value_type = VALUE_UNSET; \
	state->val.contains = NULL;              \
	state->val._contains = NULL;             \
	state->val.name = NULL;                  \
	state->val.string = NULL;                \
	state->negative = FALSE; }

typedef struct json_parse_context PARSE_CONTEXT, *PPARSE_CONTEXT;
#define MAXPARSE_CONTEXTSPERSET 128
DeclareSet( PARSE_CONTEXT );

struct json_input_buffer {
	char const * buf;      // prior input buffer
	size_t       size; // size of prior input buffer
	char const * pos;  // last position in _input if context closed before end of buffer
};

struct json_output_buffer {
	char * buf;      // prior input buffer
	size_t  size; // size of prior input buffer
	char * pos;  // last position in _input if context closed before end of buffer
};

typedef struct json_input_buffer PARSE_BUFFER, *PPARSE_BUFFER;
#define MAXPARSE_BUFFERSPERSET 128
DeclareSet( PARSE_BUFFER );


// this is the stack state that can be saved between parsing for streaming.
struct json_parse_state {
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
	enum word_char_states word;
	LOGICAL status;
	LOGICAL negative;
	LOGICAL literalString;

	PLINKSTACK *context_stack;

	LOGICAL first_token;
	PPARSE_CONTEXT context;
	enum parse_context_modes parse_context;
	struct json_value_container val;
	int comment;
	TEXTRUNE operatorAccum;

	PLINKQUEUE *inBuffers;
	//char const * input;     // current input buffer start
	//char const * msg_input; // current input buffer position (incremented while reading)

	LOGICAL completed;
	LOGICAL complete_at_end;
	LOGICAL gatheringString;
	TEXTRUNE gatheringStringFirstChar;
	TEXTRUNE gatheringCodeLastChar; 
	int codeDepth; /* string overload with code */
	LOGICAL gatheringNumber;
	LOGICAL numberExponent;
	LOGICAL numberFromHex;
	LOGICAL numberFromDate;

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
typedef struct json_parse_state PARSE_STATE, *PPARSE_STATE;
#define MAXPARSE_STATESPERSET 32
DeclareSet( PARSE_STATE );

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
#define MAXPDATALISTSPERSET 8192
DeclareSet( PDATALIST );

struct pendingParser {
	PDATALIST* pdlMessage;
	struct json_parse_state* state;
};

struct json_parser_shared_data {
	PPARSE_CONTEXTSET parseContexts;
	PPARSE_BUFFERSET parseBuffers;
	struct json_parse_state *last_parse_state;
	PPARSE_STATESET parseStates;
	PPLISTSET listSet;
	PPLINKSTACKSET linkStacks;
	PPLINKQUEUESET linkQueues;
	PPDATALISTSET dataLists;
	struct json_parse_state *json_state; // used by simple parse_message interface.
	///struct json_parse_state *json6_state; // used by simple parse_message interface.
	PLIST pendingParsers6;
};
#ifndef JSON_PARSER_MAIN_SOURCE
extern
#endif
struct json_parser_shared_data jpsd;

void _json_dispose_message( PDATALIST *msg_data );


#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif


