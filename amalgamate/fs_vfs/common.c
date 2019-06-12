
#include "common.h"


void throwError( const char *error ){
	EM_ASM( {
		const string = UTF8ToString( $0 );
		throw new Error( str );
	},error);
}

static PLINKQUEUE plqLists;

uintptr_t getValueList( void ) {
	PDATALIST *ppdl;
	if( !(ppdl = (PDATALIST*)DequeLink( &plqLists ) ) ){
		ppdl = New( PDATALIST );
		ppdl[0] = CreateDataList( sizeof( struct jsox_value_container ) );
	}else {
		ppdl[0]->Cnt = 0;		
	}
	return (uintptr_t)ppdl;
}

void dropValueList( PDATALIST *ppdl ) {
	EnqueLink( &plqLists, ppdl );
}


void defineFunction( cb ){
	EM_ASM( {
          return Module.this_.callbacks.push(cb);
	} );
}

#define  makeObject() EM_ASM_INT( { return Module.this_.objects .push( {} )-1; })
#define  makeArray() EM_ASM_INT( { return Module.this_.objects .push( [] )-1; })

int makeString( char *string, int stringlen ) {
	int x = EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.objects.push( string )-1;
	},string, stringlen);
	return x;
}

int fillArrayBuffer(char *data, int len) {
	return EM_ASM_INT( {
		var ab = new ArrayBuffer( $1 );
		var u8 = new Uint8Array(ab);
		for( var i = 0; i < $1; i++ )
			u8[i] = Module.U8HEAP[data+i];
		return Module.this_.objects .push( ab )-1;
	},data, len);
}

int makeArrayBuffer(int len) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( new ArrayBuffer($0) )-1;
	},len);
}

int makeBigInt( char *s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1-1 );
		return Module.this_.objects .push( BigInt(string) )-1;
	},s,n);
}

int makeDate( char *s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.objects .push( new Date(string) )-1;
	},n);
}


int makeNumber( int n ) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( $0 )-1;
	},n);
}

int makeNumberf( double n ) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( $0 )-1;
	},n);
}

int makeTypedArray(int ab, int type ) {
	return EM_ASM_INT( {
		var ab = Module.this_.objects[$0];
		//console.log( "This:", Module.this_, Object.getPrototypeOf( Module.this_ ) );
			switch( $1 ) {
			case 0:
				result = $0;
				break;
			case 1: // "u8"
				Module.this_.objects .push( new Uint8Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 2:// "cu8"
				Module.this_.objects .push( new Uint8ClampedArray(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 3:// "s8"
				Module.this_.objects .push( new Int8Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 4:// "u16"
				Module.this_.objects .push( new Uint16Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 5:// "s16"
				Module.this_.objects .push( new Int16Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 6:// "u32"
				Module.this_.objects .push( new Uint32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 7:// "s32"
				Module.this_.objects .push( new Int32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen );
			//	break;
			case 10:// "f32"
				Module.this_.objects .push( new Float32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 11:// "f64"
				Module.this_.objects .push( new Float64Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			default:
				result = 0; // undefined constant
			}
		return result;
	}, ab, type );
}


void setObject( int object, int field, int value ) {
	EM_ASM_( {
		const fieldName = Module.this_.objects[$1];
		Module.this_.objects[$0][fieldName] = Module.this_.objects[$2]; 
		//console.log( "We Good?", Module.this_.objects )
	}, object, field, value);
}

void setObjectByIndex( int object, int index, int value ) {
	EM_ASM_( {
		Module.this_.objects[$0][$1] = Module.this_.objects[$2]; 
	}, object, index, value);
}

void setObjectByName( int object,const char*field, int value ) {
	EM_ASM_( {
		const fieldName = Pointer_stringify( $1 );
		Module.this_.objects[$0][fieldName] = Module.this_.objects[$2]; 
		//console.log( "We Good?", Module.this_.objects )
	}, object, field, value);
}

#define SET(o,f,v) setObjectByName( o,f,v )
#define SETN(o,f,v) setObjectByIndex( o,f,v )

