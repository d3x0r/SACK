#include <stdint.h>


#include <stdint.h>
#include <emscripten.h>

#include "sack.h"

enum buildinTypes {
	JS_VALUE_UNDEFINED,
	JS_VALUE_FALSE,
	JS_VALUE_TRUE,
	JS_VALUE_NULL,
	JS_VALUE_NEG_INFINITY,
	JS_VALUE_INFINITY,
	JS_VALUE_NAN,
};

// -------- SQL Module
void InitSQL( void );
struct SqlObject* createSqlObject( const char *dsn );

// --------- SRG Module
void InitSRG( void );

// -------- JSOX Module
void InitJSOX( void );

// -------------  Common Utilities

void throwError( const char *error );

uintptr_t getValueList( void ) ;

void dropValueList( PDATALIST *ppdl );

void invokeFunction( int cb, int thisO, int argsArray );

#define  makeObject() EM_ASM_INT( { return Module.this_.objects .push( {} )-1; })
#define  makeArray() EM_ASM_INT( { return Module.this_.objects .push( [] )-1; })

int makeString( char *string, int stringlen );

int fillArrayBuffer(char *data, int len);

int makeArrayBuffer(int len) ;

int makeBigInt( char *s, int n );

int makeDate( char *s, int n ) ;


int makeNumber( int n );

int makeNumberf( double n );

int makeTypedArray(int ab, int type );

void setObject( int object, int field, int value );

void setObjectByIndex( int object, int index, int value );

void setObjectByName( int object, const char*field, int value );

#define SET(o,f,v) setObjectByName( o,f,v )
#define SETN(o,f,v) setObjectByIndex( o,f,v )

