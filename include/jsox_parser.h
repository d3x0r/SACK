/***************************************************************
 * JSOX Parser
 *
 * Parses JSOX (github.com/d3x0r/jsox)
 *
 * This function is meant for a simple utility to just take a known completed packet,
 * and get the values from it.  There may be mulitple top level values, although
 * the JSON standard will only supply a single object or array as the first value.

 * jsox_parse_message( "utf8 data", sizeof( "utf8 data" )-1, &pdlMessage );
 *
 *
 * Example :

 // call to parse a message... and iterate through each value.
 {
parse_message
    PDATALIST pdlMessage;
    LOGICAL gotMessage;
	 if( jsox_parse_message( "utf8 data", sizeof( "utf8 data" )-1, &pdlMessage ) ) {
		  int index;
        struct jsox_value_container *value;
		  DATALIST_FORALL( pdlMessage, index, struct jsox_value_container *. value ) {
           /* for each value in the result.... the first layer will
           always be just one element, either a simple type, or a VALUE_ARRAY or VALUE_OBJECT, which
           then for each value->contains (as a datalist like above), process each of those values.
		  }
        jsox_dispose_mesage( &pdlMessage );
    }
 }


 *  
 *  This is a streaming setup, where a data block can be added,
 *  and the stream of objects can be returned from it....
 *
 *  Example 2:

 // allocate a parser to keep track of the parsing state...
 struct jsox_parse_state *parser = jsox_begin_parse();

 // at some point later, add some data to it...
 jsox_parse_add_data( parser, "utf8-data", sizeof( "utf8-data" ) - 1 );

 // and then get any objects that have been parsed from the stream so far...
 {
    PDATALIST pdlMessage;
	 pdlMessage = jsox_parse_get_data( parser );
    if( pdlMessage )
	 {
        int index;
        struct jsox_value_container *value;
        DATALIST_FORALL( pdlMessage, index, struct jsox_value_container *. value ) {
           /* for each value in the result.... the first layer will
           always be just one element, either a simple type, or a VALUE_ARRAY or VALUE_OBJECT, which
           then for each value->contains (as a datalist like above), process each of those values.
        }
        jsox_dispose_mesage( &pdlMessage );
		  jsox_parse_add_data( parser, NULL, 0 ); // trigger parsing next message.
	 }
 }

 *
 ***************************************************************/

#ifndef JSOX_PARSER_HEADER_INCLUDED
#define JSOX_PARSER_HEADER_INCLUDED
#define JSON_EMITTER_HEADER_INCLUDED

// include types to get namespace, and, well PDATALIST types
#include <stdhdrs.h>
#include <sack_types.h>



#ifdef __cplusplus
SACK_NAMESPACE namespace network {
	namespace jsox {
#endif

#ifdef JSOX_PARSER_SOURCE
#  define JSOX_PARSER_PROC(type,name) EXPORT_METHOD type name
#else
#  define JSOX_PARSER_PROC(type,name) IMPORT_METHOD type name
#endif


enum jsox_value_types {
	JSOX_VALUE_UNDEFINED = -1
	, JSOX_VALUE_UNSET = 0
	, JSOX_VALUE_NULL //= 1 no data
	, JSOX_VALUE_TRUE //= 2 no data
	, JSOX_VALUE_FALSE //= 3 no data
	, JSOX_VALUE_STRING //= 4 string
	, JSOX_VALUE_NUMBER //= 5 string + result_d | result_n
	, JSOX_VALUE_OBJECT //= 6 contains
	, JSOX_VALUE_ARRAY //= 7 contains

	// up to here is supported in JSON
	, JSOX_VALUE_NEG_NAN //= 8 no data
	, JSOX_VALUE_NAN //= 9 no data
	, JSOX_VALUE_NEG_INFINITY //= 10 no data
	, JSOX_VALUE_INFINITY //= 11 no data
	, JSOX_VALUE_DATE  // = 12 comes in as a number, string is data.
	, JSOX_VALUE_BIGINT // = 13 string data, needs bigint library to process...
	, JSOX_VALUE_EMPTY // = 14 no data; used in [,,,] as place holder of empty
	, JSOX_VALUE_TYPED_ARRAY  // = 15 string is base64 encoding of bytes.
	, JSOX_VALUE_TYPED_ARRAY_MAX = JSOX_VALUE_TYPED_ARRAY +12  // = 14 string is base64 encoding of bytes.
};

struct jsox_value_container {
	char * name;  // name of this value (if it's contained in an object)
	size_t nameLen;
	enum jsox_value_types value_type; // value from above indiciating the type of this value
	char *string;   // the string value of this value (strings and number types only)
	size_t stringLen;
	
	int float_result;  // boolean whether to use result_n or result_d
	union {
		double result_d;
		int64_t result_n;
		//struct json_value_container *nextToken;
	};
	PDATALIST contains;  // list of struct json_value_container that this contains.
	PDATALIST *_contains;  // acutal source datalist(?)
	char *className;  // if VALUE_OBJECT or VALUE_TYPED_ARRAY; this may be non NULL indicating what the class name is.
};


// allocates a JSOX parsing context and is prepared to begin parsing data.
JSOX_PARSER_PROC( struct jsox_parse_state *, jsox_begin_parse )(void);

// clear state; after an error state, this can allow reusing a state.
JSOX_PARSER_PROC( void, jsox_parse_clear_state )( struct jsox_parse_state *state );

// destroy current parse state.
JSOX_PARSER_PROC( void, jsox_parse_dispose_state )(struct jsox_parse_state **ppState);

// return >0 when a completed value/object is available.
// after returning >0, call json_parse_get_data.  It is possible that there is
// still unconsumed data that can begin a new object.  Call this with NULL, 0 for data
// to consume this internal data.  if this returns 0, then ther is no further object
// to retrieve.  If this return -1 there was an error, and use jsox_parse_get_error() to
// retrieve the error text.
JSOX_PARSER_PROC( int, jsox_parse_add_data )(struct jsox_parse_state *context
	, const char * msg
	, size_t msglen
	);

JSOX_PARSER_PROC( PTEXT, jsox_parse_get_error )(struct jsox_parse_state *state);

JSOX_PARSER_PROC( PDATALIST, jsox_parse_get_data )(struct jsox_parse_state *context);

// single all-in-one parsing of an input buffer.
JSOX_PARSER_PROC( LOGICAL, jsox_parse_message )(const char * msg
	, size_t msglen
	, PDATALIST *msg_data_out
	);

// release all resources of a message from jsox_parse_message or jsox_parse_get_data
JSOX_PARSER_PROC( void, jsox_dispose_message )(PDATALIST *msg_data);

JSOX_PARSER_PROC( char *, jsox_escape_string_length )(const char *string, size_t len, size_t *outlen);
JSOX_PARSER_PROC( char *, jsox_escape_string )(const char *string);

#ifdef __cplusplus
} } SACK_NAMESPACE_END
using namespace sack::network::jsox;
#endif

#endif

