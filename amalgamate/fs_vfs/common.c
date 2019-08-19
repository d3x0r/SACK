
#include "common.h"


void InitCommon( void ) 
{
	EM_ASM( (
        
		Module.this_ = {
            callbacks : [],
            consts : [], // strings referenced by C code that are const reference... 
				objects : [undefined,false,true,null,-Infinity,Infinity,NaN],
				getIndex(v){ if( Module.this_.freeObjects.length ) { var next = Module.this_.freeObjects.pop(); 
							Module.this_.objects[next] = v; return next; 
						}
						return Module.this_.objects.push(v)-1; },
				getAndDropValue(i) {
						var o = Module.this_.objects[i];
						if( i > 7 ) {
							Module.this_.freeObjects.push( i );
							Module.this_.objects[i] = null;
						}
						return o;
						
				} ,
            freeObjects : [],
			types : new Map(), // used by jsox
			reset() { this.objects.length = 7 },
        };

		//console.log( "Log:", r );
    ) );
}

//-------------------------------------------------------------------------------

void throwError( char const*error ){
	EM_ASM( {
		const string = UTF8ToString( $0 );
		throw new Error( string );
	},error);
}

//-------------------------------------------------------------------------------

static PLINKQUEUE plqLists;
uintptr_t getValueList( void ) EMSCRIPTEN_KEEPALIVE;
uintptr_t getValueList( void )
{
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

//-------------------------------------------------------------------------------

int makeStringConst( char const*string, int stringlen ) {
	int x = EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.consts.push( string )-1;
	},string, stringlen);
	return x;
}


//-------------------------------------------------------------------------------


int makeString( char const*string, int stringlen ) {
	int x = EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.getIndex(string);
	},string, stringlen);
	return x;
}

int fillArrayBuffer(char const*data, int len) {
	return EM_ASM_INT( {
		var ab = new ArrayBuffer( $1 );
		var u8 = new Uint8Array(ab);
		for( var i = 0; i < $1; i++ )
			u8[i] = Module.U8HEAP[data+i];
		return Module.this_.getIndex( ab );
	},data, len);
}

int makeArrayBuffer(int len) {
	return EM_ASM_INT( {
		return Module.this_.getIndex( new ArrayBuffer($0) );
	},len);
}

int makeBigInt( char const*s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1-1 );
		return Module.this_.getIndex( BigInt(string) );
	},s,n);
}


int makeDate( char const*s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.getIndex( new Date(string) );
	},n);
}

int makeDateFromDouble( double d) {
	return EM_ASM_INT( {
		return Module.this_.getIndex( new Date($0) );
	},d);
}


int makeNumber( int n ) {
	return EM_ASM_INT( {
		return Module.this_.getIndex( $0 );
	},n);
}

int makeNumberf( double n ) {
	return EM_ASM_INT( {
		return Module.this_.getIndex( $0 );
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
				result = Module.this_.getIndex( new Uint8Array(ab) );
				break;
			case 2:// "cu8"
				result = Module.this_.getIndex( new Uint8ClampedArray(ab) );
				break;
			case 3:// "s8"
				result = Module.this_.getIndex( new Int8Array(ab) );
				break;
			case 4:// "u16"
				result = Module.this_.getIndex( new Uint16Array(ab) );
				break;
			case 5:// "s16"
				result = Module.this_.getIndex( new Int16Array(ab) );
				break;
			case 6:// "u32"
				result = Module.this_.getIndex( new Uint32Array(ab) );
				break;
			case 7:// "s32"
				result = Module.this_.getIndex( new Int32Array(ab) );
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen );
			//	break;
			case 10:// "f32"
				result = Module.this_.getIndex( new Float32Array(ab) );
				break;
			case 11:// "f64"
				result = Module.this_.getIndex( new Float64Array(ab) );
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
		Module.this_.freeObjects.push($2);
		Module.this_.objects[$2] = null;
	}, object, field, value);
}

void setObjectByIndex( int object, int index, int value ) {
	EM_ASM_( {
		Module.this_.objects[$0][$1] = Module.this_.objects[$2]; 
		Module.this_.freeObjects.push($2);
		Module.this_.objects[$2] = null;
	}, object, index, value);
}

void setObjectByName( int object, char const*field, int value ) {
	EM_ASM_( {
		const fieldName = UTF8ToString( $1 );
		Module.this_.objects[$0][fieldName] = Module.this_.objects[$2]; 
		//console.log( "We Good?", Module.this_.objects )
		// value moved from floading storage to a value, no longer needs global reference.
		Module.this_.freeObjects.push($2);
		Module.this_.objects[$2] = null;
	}, object, field, value);
}

#define SET(o,f,v) setObjectByName( o,f,v )
#define SETN(o,f,v) setObjectByIndex( o,f,v )

//----------------------------------------------------------------------------
// Local Variations
//----------------------------------------------------------------------------

int getLocal( void ) {
    return makeArray();
}

void dropLocal( int pool ) {
    EM_ASM( { Module.this_.objects[$0] = null; Module.this_.freeObjects.push( $0 ) }, pool );
}

void dropLocalAndSave( int pool, int object ) {
    EM_ASM( { 
		// replace pool with a single object from that pool.
		Module.this_.objects[$0] = Module.this_.objects[$0][$1];
	}, pool, object );
}

int makeLocalString( int pool, char const *string, int stringlen ) {
	int x = EM_ASM_INT( {
		const string = UTF8ToString( $1, $2 );
		return Module.this_.objects[$0].push( string )-1;
	}, pool, string, stringlen);
	return x;
}

int fillLocalArrayBuffer( int pool, char const*data, int len) {
	return EM_ASM_INT( {
		var ab = new ArrayBuffer( $2 );
		var u8 = new Uint8Array(ab);
		for( var i = 0; i < $2; i++ )
			u8[i] = Module.U8HEAP[data+i];
		return Module.this_.objects[$0] .push( ab )-1;
	}, pool, data, len);
}

int makeLocalArrayBuffer( int pool, int len) {
	return EM_ASM_INT( {
		return Module.this_.objects[$0] .push( new ArrayBuffer( $\1 ) )-1;
	}, pool, len);
}

int makeLocalBigInt( int pool, char const*s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $1, $2-1 );
		return Module.this_.objects[$0] .push( BigInt(string) )-1;
	}, pool, s,n);
}

int makeLocalDate( int pool, char const*s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $1, $2 );
		return Module.this_.objects[$0] .push( new Date(string) )-1;
	}, pool, n);
}


int makeLocalNumber( int pool, int n ) {
	return EM_ASM_INT( {
		return Module.this_.objects[$0] .push( $1 )-1;
	}, pool, n);
}

int makeLocalNumberf( int pool, double n ) {
	return EM_ASM_INT( {
		return Module.this_.objects[$0] .push( $1 )-1;
	}, pool, n);
}

int makeLocalTypedArray( int pool, int ab, int type ) {
	return EM_ASM_INT( {
		var ab = Module.this_.objects[$0][$1];
		//console.log( "This:", Module.this_, Object.getPrototypeOf( Module.this_ ) );
			switch( $2 ) {
			case 0:
				result = $1;
				break;
			case 1: // "u8"
				Module.this_.objects[$0] .push( new Uint8Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 2:// "cu8"
				Module.this_.objects[$0] .push( new Uint8ClampedArray(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 3:// "s8"
				Module.this_.objects[$0] .push( new Int8Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 4:// "u16"
				Module.this_.objects[$0] .push( new Uint16Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 5:// "s16"
				Module.this_.objects[$0] .push( new Int16Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 6:// "u32"
				Module.this_.objects[$0] .push( new Uint32Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 7:// "s32"
				Module.this_.objects[$0] .push( new Int32Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen );
			//	break;
			case 10:// "f32"
				Module.this_.objects[$0] .push( new Float32Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			case 11:// "f64"
				Module.this_.objects[$0] .push( new Float64Array(ab) );
				result = Module.this_.objects[$0].length-1;
				break;
			default:
				result = 0; // undefined constant
			}
		return result;
	}, pool, ab, type );
}


void setLocalObject( int pool, int object, int field, int fromPool, int value ) {
	EM_ASM_( {
		const fieldName = Module.this_.objects[$0][$2];
		if( $4 < 0 )
			Module.this_.objects[$0][$1][fieldName] = Module.this_.objects[$3]; 
		else
			Module.this_.objects[$0][$1][fieldName] = Module.this_.objects[$4][$3]; 
	}, pool, object, field, value, fromPool );
}

void setLocalObjectByIndex( int pool, int object, int index, int fromPool, int value ) {
	EM_ASM_( {
		if( $4 < 0 )
			Module.this_.objects[$0][$1][$2] = Module.this_.objects[$3]; 
		else
			Module.this_.objects[$0][$1][$2] = Module.this_.objects[$4][$3]; 
		//console.log( "Local Array is now:", Module.this_.objects[$0][$1] )
	}, pool, object, index, value, fromPool );
}


void setLocalObjectNumberByIndex( int pool, int object,int field, int value ) {
	EM_ASM_( {
		Module.this_.objects[$0][$1][$2] = $3; 
	}, pool, object, field, value);
}

void setLocalObjectNumberfByIndex( int pool, int object,int field, double value ) {
	EM_ASM_( {
		Module.this_.objects[$0][$1][$2] = $3; 
	}, pool, object, field, value);
}


void setLocalObjectByName( int pool, int object, char const*field, int fromPool, int value ) {
	EM_ASM_( {
		const fieldName = UTF8ToString( $2 );
		if( $4 < 0 )
			Module.this_.objects[$0][$1][fieldName] = Module.this_.objects[$3]; 
		else
			Module.this_.objects[$0][$1][fieldName] = Module.this_.objects[$4][$3]; 
		//console.log( "We Good?", Module.this_.objects[$0] )
	}, pool, object, field, value, fromPool );
}

void setLocalObjectNumberByName( int pool, int object, char const*field, int value ) {
	EM_ASM_( {
		const fieldName = UTF8ToString( $2 );
		Module.this_.objects[$0][$1][fieldName] = $3; 
	}, pool, object, field, value);
}

void setLocalObjectNumberfByName( int pool, int object, char const*field, double value ) {
	EM_ASM_( {
		const fieldName = UTF8ToString( $2 );
		Module.this_.objects[$0][$1][fieldName] = $3; 
	}, pool, object, field, value);
}


