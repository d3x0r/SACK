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

// ------- C to JS object interop
void InitCommon( void );

// -------- SQL Module
void InitSQL( void );
struct SqlObject* createSqlObject( const char *dsn );

// --------- SRG Module
void InitSRG( void );

// -------- JSON6 Module
void InitJSON6( void );

// -------- JSOX Module
void InitJSOX( void );

// -------- TLS Utilities
void TLSInit(void);

// -------------  Common Utilities

void throwError( const char *error );

// returns a PDATALIST of jsox_value_container
uintptr_t getValueList( void ) ;
void dropValueList( PDATALIST *ppdl );

// probably not used...
//void invokeFunction( int cb, int thisO, int argsArray );

int makeStringConst( char const *string, int stringlen );


#define  makeObject() EM_ASM_INT( { var slot = Module.this_.freeObjects.pop(); if( slot ) return ((Module.this_.objects[slot]={}),slot);return Module.this_.objects .push( {} )-1; })
#define  makeLocalObject(pool) EM_ASM_INT( { return Module.this_.objects[$0].push( {} )-1; }, pool )

#define  makeArray() EM_ASM_INT( { return Module.this_.objects .push( [] )-1; })
#define  makeLocalArray(pool) EM_ASM_INT( { return Module.this_.objects[$0].push( [] )-1; }, pool )

int makeString( char const *string, int stringlen );
int fillArrayBuffer(char const *data, int len);
int makeArrayBuffer(int len) ;
int makeBigInt( char const *s, int n );
int makeDate( char const *s, int n ) ;
int makeNumber( int n );
int makeNumberf( double n );
int makeTypedArray(int ab, int type );

void setObject( int object, int field, int value );
void setObjectByIndex( int object, int index, int value );
void setObjectNumberByIndex( int object, int index, int value );
void setObjectNumberfByIndex( int object, int index, double value );
void setObjectByName( int object, char const*field, int value );

#define SET(o,f,v) setObjectByName( o,f,v )
#define SETN(o,f,v) setObjectByIndex( o,f,v )

int makeLocalString( int pool, char  const*string, int stringlen );
int fillLocalArrayBuffer( int pool, char const *data, int len );
int makeLocalArrayBuffer( int pool, int len );
int makeLocalBigInt( int pool, char const *s, int n );
int makeLocalDate( int pool, char  const*s, int n ) ;
int makeLocalNumber( int pool, int n );
int makeLocalNumberf( int pool, double n );
int makeLocalTypedArray(int pool, int ab, int type );

void setLocalObject( int pool, int object, int field, int fromPool, int value );
void setLocalObjectByIndex( int pool, int object, int index, int fromPool, int value );
void setLocalObjectNumberByIndex( int pool, int object, int index, int value );
void setLocalObjectNumberfByIndex( int pool, int object, int index, double value );
void setLocalObjectByName( int pool, int object, char const*field, int fromPool, int value );

#define LSETG(p,o,f,v) setLocalObjectByName( p,o,f,-1,v )
#define LSET(p,o,f,v) setLocalObjectByName( p,o,f,p,v )
#define LSETN(p,o,f,v) setLocalObjectByIndex( p,o,f,p,v )


int getLocal( void );
void dropLocal( int pool );
void dropLocalAndSave( int pool, int object );
