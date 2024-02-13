
#include <vesl_emitter.h>

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace vesl {
#endif

enum vesl_word_char_states {
	VESL_WORD_POS_RESET = 0, // not in a keyword
	VESL_WORD_POS_END,  // at end of a word, waiting for separator
	VESL_WORD_POS_TRUE_1,
	VESL_WORD_POS_TRUE_2,
	VESL_WORD_POS_TRUE_3,
	VESL_WORD_POS_TRUE_4,
	VESL_WORD_POS_FALSE_1, // 11
	VESL_WORD_POS_FALSE_2,
	VESL_WORD_POS_FALSE_3,
	VESL_WORD_POS_FALSE_4,
	VESL_WORD_POS_NULL_1, // 21  get u
	VESL_WORD_POS_NULL_2, //  get l
	VESL_WORD_POS_NULL_3, //  get l
	VESL_WORD_POS_UNDEFINED_1,  // 31 
	VESL_WORD_POS_UNDEFINED_2,
	VESL_WORD_POS_UNDEFINED_3,
	VESL_WORD_POS_UNDEFINED_4,
	VESL_WORD_POS_UNDEFINED_5,
	VESL_WORD_POS_UNDEFINED_6,
	VESL_WORD_POS_UNDEFINED_7,
	VESL_WORD_POS_UNDEFINED_8,
	//VESL_WORD_POS_UNDEFINED_9, // instead of stepping to this value here, go to RESET
	VESL_WORD_POS_NAN_1,
	VESL_WORD_POS_NAN_2,
	//VESL_WORD_POS_NAN_3,// instead of stepping to this value here, go to RESET
	VESL_WORD_POS_INFINITY_1,
	VESL_WORD_POS_INFINITY_2,
	VESL_WORD_POS_INFINITY_3,
	VESL_WORD_POS_INFINITY_4,
	VESL_WORD_POS_INFINITY_5,
	VESL_WORD_POS_INFINITY_6,
	VESL_WORD_POS_INFINITY_7,
	//VESL_WORD_POS_INFINITY_8,// instead of stepping to this value here, go to RESET
	VESL_WORD_POS_FIELD, 
	VESL_WORD_POS_AFTER_FIELD, 
	VESL_WORD_POS_DOT_OPERATOR,
	VESL_WORD_POS_PROPER_NAME, 
	VESL_WORD_POS_AFTER_PROPER_NAME, 
	VESL_WORD_POS_AFTER_GET,
	VESL_WORD_POS_AFTER_SET,
};

enum vesl_parse_context_modes {
 VESL_CONTEXT_UNKNOWN = 0,
 VESL_CONTEXT_IN_ARRAY = 1,
 VESL_CONTEXT_IN_OBJECT = 2,
 VESL_CONTEXT_OBJECT_FIELD = 3,
 VESL_CONTEXT_OBJECT_FIELD_VALUE = 4,
 };

struct vesl_parse_context {
	enum vesl_parse_context_modes context;
	PDATALIST *elements;
	char *name;	
	size_t nameLen;	
	struct vesl_value_container valState;
	struct vesl_context_object *object;
};

#ifdef RESET_VAL
#  undef RESET_VAL
#endif
#define RESET_VAL()  {            \
	val.value_type = VESL_VALUE_UNSET; \
	/*val.contains = NULL;     */   \
	val._contains = NULL;         \
	val.string = NULL;            \
	negative = FALSE; }

#ifdef RESET_STATE_VAL
#  undef RESET_STATE_VAL
#endif
#define RESET_STATE_VAL()  {             \
	state->val.value_type = VESL_VALUE_UNSET; \
	/*state->val.contains = NULL; */     \
	state->val._contains = NULL;         \
	state->val.string = NULL;            \
	state->negative = FALSE; }

typedef struct vesl_parse_context VESL_PARSE_CONTEXT, *PVESL_PARSE_CONTEXT;
#define MAXVESL_PARSE_CONTEXTSPERSET 128
DeclareSet( VESL_PARSE_CONTEXT );

struct vesl_input_buffer {
	char const * buf;      // prior input buffer
	size_t       size; // size of prior input buffer
	char const * pos;  // last position in _input if context closed before end of buffer
};

struct vesl_output_buffer {
	char * buf;      // prior input buffer
	size_t  size; // size of prior input buffer
	char * pos;  // last position in _input if context closed before end of buffer
};

typedef struct vesl_input_buffer VESL_PARSE_BUFFER, *PVESL_PARSE_BUFFER;
#define MAXVESL_PARSE_BUFFERSPERSET 128
DeclareSet( VESL_PARSE_BUFFER );


// this is the stack state that can be saved between parsing for streaming.
struct vesl_parse_state {
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
	enum vesl_word_char_states word;
	LOGICAL status;
	LOGICAL negative;
	LOGICAL literalString;

	PLINKSTACK *context_stack;

	LOGICAL first_token;
	//PPARSE_CONTEXT context;
	enum vesl_parse_context_modes parse_context;
	struct vesl_value_container val;
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

	struct vesl_output_buffer *output;
	PDATALIST root;
	//char *token_begin;
};
typedef struct vesl_parse_state VESL_PARSE_STATE, *PVESL_PARSE_STATE;
#define MAXVESL_PARSE_STATESPERSET 32
DeclareSet( VESL_PARSE_STATE );

#ifndef MAXPLISTSPERSET
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

#endif

struct vesl_parser_shared_data {
	PVESL_PARSE_CONTEXTSET parseContexts;
	PVESL_PARSE_BUFFERSET parseBuffers;
	struct vesl_parse_state *last_parse_state;
	PVESL_PARSE_STATESET parseStates;
	PPLISTSET listSet;
	PPLINKSTACKSET linkStacks;
	PPLINKQUEUESET linkQueues;
	PPDATALISTSET dataLists;
};
#ifndef VESL_PARSER_MAIN_SOURCE
extern
#endif
struct vesl_parser_shared_data vpsd;

// shared to code parser...
void _vesl_dispose_message( PDATALIST *msg_data );


#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif


