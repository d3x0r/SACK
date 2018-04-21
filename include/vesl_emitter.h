#ifndef VESL_EMITTER_HEADER_INCLUDED
#define VESL_EMITTER_HEADER_INCLUDED


#ifdef VESL_EMITTER_SOURCE
#define VESL_EMITTER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define VESL_EMITTER_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace vesl {
#endif



struct vesl_context_object_element;
struct vesl_context_object;
struct vesl_context;

// take a vesl string and a format and fill in a structure from the text.
// tests all formats, to first-match; 
// take a vesl string and a format and fill in a structure from the text.
// if object does not fit all members (may have extra, but must have at least all members in message in format to return TRUE)
// then it returns false; that is if a member is in the 'msg' parameter that is not in
// the format, then the result is FALSE.
//  PDATALIST is full of struct vesl_value_container
// turns out numbers can be  hex, octal and binary numbers  (0x[A-F,a-f,0-9]*, 0b[0-1]*, 0[0-9]*)
// slightly faster (17%) than vesl6_parse_message because of fewer possible checks.
VESL_EMITTER_PROC( LOGICAL, vesl_parse_message )(const char * msg
                                                , size_t msglen
                                                , PDATALIST *msg_data_out
																);

// allocates a parsing context and begins parsing data.
VESL_EMITTER_PROC( struct vesl_parse_state *, vesl_begin_parse )( void );
// return TRUE when a completed value/object is available.
// after returning TRUE, call vesl_parse_get_data.  It is possible that there is
// still unconsumed data that can begin a new object.  Call this with NULL, 0 for data
// to consume this internal data.  if this returns FALSE, then ther is no further object
// to retrieve.
VESL_EMITTER_PROC( int, vesl_parse_add_data )( struct vesl_parse_state *context
                                                 , const char * msg
                                                 , size_t msglen
                                                 );


// these are common functions that work for VESL stream parsers
VESL_EMITTER_PROC( PDATALIST, vesl_parse_get_data )( struct vesl_parse_state *context );
VESL_EMITTER_PROC( void, vesl_parse_dispose_state )( struct vesl_parse_state **context );
VESL_EMITTER_PROC( void, vesl_parse_clear_state )(struct vesl_parse_state *context);
VESL_EMITTER_PROC( PTEXT, vesl_parse_get_error )(struct vesl_parse_state *context);

// Add some data to parse for vesl stream (which may consist of multiple values)
// return 1 when a completed value/object is available.
// after returning 1, call vesl_parse_get_data.  It is possible that there is
// still unconsumed data that can begin a new object.  Call this with NULL, 0 for data
// to consume this internal data.  if this returns 0, then there is no further object
// to retrieve.
// if this returns -1, an error in parsing has occured, and no further parsing can happen.
VESL_EMITTER_PROC( int, vesl_parse_add_data )( struct vesl_parse_state *context
	, const char * msg
	, size_t msglen
	);

// one shot, just process this one message.
VESL_EMITTER_PROC( LOGICAL, vesl_parse_message )(const char * msg
	, size_t msglen
	, PDATALIST *msg_data_out
	);

// any allocate mesage parts are released.
VESL_EMITTER_PROC( void, vesl_dispose_expressions )(PDATALIST *msg_data);

enum vesl_value_types {
	VESL_VALUE_UNDEFINED = -1
	, VESL_VALUE_UNSET = 0
	, VESL_VALUE_NULL //= 1 no data
	, VESL_VALUE_TRUE //= 2 no data
	, VESL_VALUE_FALSE //= 3 no data
	, VESL_VALUE_STRING //= 4 string
	, VESL_VALUE_NUMBER //= 5 string + result_d | result_n
	, VESL_VALUE_OBJECT //= 6 contains
	, VESL_VALUE_ARRAY //= 7 contains

	// up to here is supported in VESL
	, VESL_VALUE_NEG_NAN //= 8 no data
	, VESL_VALUE_NAN //= 9 no data
	, VESL_VALUE_NEG_INFINITY //= 10 no data
	, VESL_VALUE_INFINITY //= 11 no data
	, VESL_VALUE_DATE  // = 12 UNIMPLEMENTED
	, VESL_VALUE_EMPTY // = 13 no data; used in [,,,] as place holder of empty

	// --- up to here is supports in VESL(6)
	, VESL_VALUE_NEED_EVAL // = 14 string needs to be parsed for expressions.

	, VESL_VALUE_VARIABLE // contains
	, VESL_VALUE_FUNCTION // code (string), contains
	, VESL_VALUE_FUNCTION_CALL // code (string), contains[n] = parameters

	, VESL_VALUE_EXPRESSION //  ( ... ) or { ... } , string, contains[n] = value(s) last is THE value
	, VESL_VALUE_OPERATOR // Symbolic operator, with combination rules so the operator text is complete.

	, VESL_VALUE_OP_IF // 'if'  contains[1], contains[1], contains[2]
	, VESL_VALUE_OP_TRINARY_THEN // '?'  contains[N] expressions to evaluate
	, VESL_VALUE_OP_TRINARY_ELSE // ':'  contains[N] expressions to evaluate
	, VESL_VALUE_OP_SWITCH // 'switch'
	, VESL_VALUE_OP_CASE // 'case'
	, VESL_VALUE_OP_FOR // 'for'   no data, contains[0], contains[1], contains[2],
	, VESL_VALUE_OP_BREAK // 'break'  // strip optional label break
	, VESL_VALUE_OP_WHILE // 'while'
	, VESL_VALUE_OP_DO // 'do'
	, VESL_VALUE_OP_CONTINUE // 'continue'
	, VESL_VALUE_OP_GOTO // 'goto'
	
	, VESL_VALUE_OP_STOP // 'stop'

	, VESL_VALUE_OP_THIS // 'this'
	, VESL_VALUE_OP_HOLDER // 'holder'
	, VESL_VALUE_OP_BASE // 'base'

};

struct vesl_value_container {
	enum vesl_value_types value_type; // value from above indiciating the type of this value
	char *string;   // the string value of this value (strings and number types only)
	size_t stringLen;
	
	int float_result;  // boolean whether to use result_n or result_d
	union {
		double result_d;
		int64_t result_n;
		//struct vesl_value_container *nextToken;
	};
	//PDATALIST contains;  // list of struct vesl_value_container that this contains.
	PDATALIST *_contains;  // acutal source datalist(?)

};




#ifdef __cplusplus
} } SACK_NAMESPACE_END
using namespace sack::network::vesl;
#endif

#endif
