
#include "jsox.h"

int level = 0;

void DumpMessage( PDATALIST pdl ) {
	struct jsox_value_container *val;
	INDEX idx;
	int leader;
	level++;
	DATA_FORALL( pdl, idx, struct jsox_value_container *, val ) {
		if( idx ) 
			printf( ",\n" );
		for( leader = 3; leader < level*3; leader++ ) {
			putc( ' ', stdout );
		}

		if( val->name ) printf( "%s:", val->name );
		switch( val->value_type ) {
		case VALUE_OBJECT:
			printf( "{ \n" );
			DumpMessage( val->contains );
			printf( " }" );
			break;
		case VALUE_ARRAY:
			printf( "[ " );
			DumpMessage( val->contains );
			printf( " ]" );
			break;
		case VALUE_STRING:
			printf( "\"%s\"", val->string );
			break;
		case VALUE_DATE:
			printf( "%s", val->string );
			break;				
		case VALUE_NUMBER:
			if( val->float_result )
				printf( "%g", val->result_d );
			else
				printf( "%d", val->result_n );
			break;
		case VALUE_BIGINT:
			printf( "%sn", val->string );
			break;
		case VALUE_NAN:
			printf( "NaN" );
			break;
		case VALUE_INFINITY:
			printf( "Infinity" );
			break;
		case VALUE_NEG_INFINITY:
			printf( "-Infinity" );
			break;
		case VALUE_TRUE:
			printf( "true" );
			break;
		case VALUE_FALSE:
			printf( "false" );
			break;
		case VALUE_NULL:
			printf( "null" );
			break;
		case VALUE_EMPTY:
			// array element that is ',,' 
			printf( "<EMPTY>" );
			break;
		case VALUE_UNDEFINED:
			printf( "undefined" );
			break;
		default :
			printf( "\nunhandled value type: %d [%s]\n", val->value_type, val->string );
		}
	}
	level--;
}

void parse( char *fileName ) {
	PDATALIST pdl;
	FILE *file;
	size_t size;
	char *data;
	int r;
	file = fopen( fileName, "rb" );
	fseek( file, 0, SEEK_END );
	size = ftell( file );
	fseek( file, 0, SEEK_SET );
	data = malloc( size );
	fread( data, 1, size, file );
	fclose( file );
	r = jsox_parse_message( data, size, &pdl );
	if( r > 0 )
		DumpMessage( pdl );
	else if( r <= 0 )
		printf( "Error:%s", GetText( jsox_parse_get_error( NULL ) ) );
	printf( "\n" );
	free( data );
}

int main( int argc, char **argv) {
	int n;
	for( n = 1; n < argc; n++ ) {
		parse( argv[n] );
	}
}
