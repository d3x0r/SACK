#include "common.h"
//
// Pointer_stringify for transforming a char * containing a UTF8 string to a  JS string, and intArrayFromString t
//

struct reviver_data {
	
	LOGICAL revive;
	//int index;
	int value;
	int _this;
	int reviver_func;
	int rootObject;
	//class JSOXObject *parser;

	uint32_t *map;
	uint32_t mapIndex;
	uint32_t *stringMap;
	uint32_t stringIndex;
	uint32_t *intMap;
	uint32_t intIndex;
	double *floatMap;
	uint32_t floatIndex;

};

struct objectStates {
	PDATALIST pldObjects;
};

void InitJSOX(void ) {
	EM_ASM( (

		Module.arrayTypeFunc = [
			Uint8Array, Uint8ClampedArray, Int8Array,
			Uint16Array, Int16Array,
			Uint32Array, Int32Array,
			null, null,
			Float32Array, Float64Array,
		];

		// static buffer state; removing this reallocation saved 10%
		var buf_heap;
		var buf;
		var buf_len = 0;

		var JSOX = {
			begin (cb) {
				return {
					this_ : {
						objects : [],
						parser : Module._jsox_begin_parse(),
						prototypes : null,
						readCallback : JSOX.makeFunction(cb )
					},
					write(data){
						var s;
						var i = allocate(s = intArrayFromString(data), 'i8', ALLOC_NORMAL);
						Module._jsox_write( this.this_.parser,this.this_.readCallback, i, s.length-1 );
						Module._free(i);
						//console.log( "Got strg:", str, cb, s );
					},
					registerFromJSOX(typeName,fromCb) {

					}
				}
			},
		escape () {
			if( !s ) {
				throw new Error( "Missing parameter, string to escape" );
				return;
			}
			var msg;
			var sa;
			var i = allocate(sa = intArrayFromString(s), 'i8', ALLOC_NORMAL);
			var msg = Module._jsox_do_escape( i, sa.length-1 );
			var result = Module.this_.objects[msg];
			Module.this_.objects.length = msg;
			Module._free( i );
			return result;

		},
		registerToJSOX (typeName,prototype,cb)  {
			//var types = 
		},
		registerFromJSOX (typeName,fromCb) {

		},
		registerToFrom (name,prototype,toCb, fromCb) {

		},
		makeFunction (cb){
			return Module.this_.getIndex(cb) 
		},
		makeString (str){
			return Module.this_.getIndex(str)
		},

		parse ( str, cb ) {
			var s;
			var len = str.length*4;
			var outpos = 0;
			if( len > buf_len ) {
				if( buf_heap )
			        	Module._free( buf_heap );
				buf_heap = Module._malloc( len );
				buf = new Uint8Array( Module.HEAPU8.buffer, buf_heap, len );
				buf_len = len;
			}
			for (var i=0; i < str.length; i++) {
				var charcode = str.charCodeAt(i);
				if (charcode < 0x80) buf[outpos++] = (charcode);
				else if (charcode < 0x800) {
					buf[outpos++] = 0xc0 | (charcode >> 6);
					buf[outpos++] = 0x80 | (charcode & 0x3f);
				}
				else if ( ( charcode < 0xd800 ) || ( charcode >= 0xe000 ) ) {
					buf[outpos++] = 0xe0 | (charcode >> 12);
					buf[outpos++] = 0x80 | ((charcode>>6) & 0x3f);
					buf[outpos++] = 0x80 | (charcode & 0x3f);
				}
				else {
					i++;
					// UTF-16 encodes 0x10000-0x10FFFF by
					// subtracting 0x10000 and splitting the
					// 20 bits of 0x0-0xFFFFF into two halves
					charcode = 0x10000 + (((charcode & 0x3ff)<<10)
							  | (str.charCodeAt(i) & 0x3ff));
					buf[outpos++] = 0xf0 | (charcode >>18);
					buf[outpos++] = 0x80 | ((charcode>>12) & 0x3f);
					buf[outpos++] = 0x80 | ((charcode>>6) & 0x3f);
					buf[outpos++] = 0x80 | (charcode & 0x3f);
				}
			}
			//console.log( "Got strg:", str, cb, s );
			var result = Module.this_.objects[ Module._jsox_parse( buf_heap, outpos, Module.makeFunction(cb) ) ];
			Module.this_.objects.length = 0;
	        return result;
		}
		};
      Module.SACK.JSOX = JSOX;
			  ), 0 );

	EM_ASM_INT( (
 		 Module.constants = Module.constants||{};
       Module.constants[UTF8ToString( $1 )]  = ( $0 );
       Module.constants[UTF8ToString( $3 )]  = ( $2 );
       Module.constants[UTF8ToString( $5 )]  = ( $4 );
       Module.constants[UTF8ToString( $7 )]  = ( $6 );
       Module.constants[UTF8ToString( $9 )]  = ( $8 );
       Module.constants[UTF8ToString( $11 )]  = ( $10 );
       Module.constants[UTF8ToString( $13 )]  = ( $12 );
       Module.constants[UTF8ToString( $15 )]  = ( $14 );
       Module.constants[UTF8ToString( $17 )]  = ( $16 );
       Module.constants[UTF8ToString( $19 )]  = ( $18 );
       Module.constants[UTF8ToString( $21 )]  = ( $20 );
       Module.constants[UTF8ToString( $23 )]  = ( $22 );
       Module.constants[UTF8ToString( $23 )]  = ( $24 );
       Module.constants[UTF8ToString( $27 )]  = ( $26 );
       Module.constants[UTF8ToString( $29 )]  = ( $28 );
       Module.constants[UTF8ToString( $31 )]  = ( $30 );
       Module.constants[UTF8ToString( $33 )]  = ( $32 );
       Module.constants[UTF8ToString( $35 )]  = ( $34 );
					),
     JSOX_VALUE_UNDEFINED         , "JSOX_VALUE_UNDEFINED"
	, JSOX_VALUE_UNSET             , "JSOX_VALUE_UNSET"
	, JSOX_VALUE_NULL              , "JSOX_VALUE_NULL"
	, JSOX_VALUE_TRUE              , "JSOX_VALUE_TRUE"
	, JSOX_VALUE_FALSE             , "JSOX_VALUE_FALSE"
	, JSOX_VALUE_STRING            , "JSOX_VALUE_STRING"
	, JSOX_VALUE_NUMBER            , "JSOX_VALUE_NUMBER"
	, JSOX_VALUE_OBJECT            , "JSOX_VALUE_OBJECT"
	, JSOX_VALUE_ARRAY             , "JSOX_VALUE_ARRAY"

	, JSOX_VALUE_NEG_NAN           , "JSOX_VALUE_NEG_NAN"
	, JSOX_VALUE_NAN               , "JSOX_VALUE_NAN"
	, JSOX_VALUE_NEG_INFINITY      , "JSOX_VALUE_NEG_INFINITY"
	, JSOX_VALUE_INFINITY          , "JSOX_VALUE_INFINITY"
	, JSOX_VALUE_DATE              , "JSOX_VALUE_DATE"
	, JSOX_VALUE_BIGINT            , "JSOX_VALUE_BIGINT"
	, JSOX_VALUE_EMPTY             , "JSOX_VALUE_EMPTY"
	, JSOX_VALUE_TYPED_ARRAY       , "JSOX_VALUE_TYPED_ARRAY"
	, JSOX_VALUE_TYPED_ARRAY_MAX   , "JSOX_VALUE_TYPED_ARRAY_MAX"
				 );
}

static int composeJSObject( struct reviver_data *revive, LOGICAL firstObject ) {
	return EM_ASM_INT( (
		var mapArr = new Int32Array( Module.HEAPU32.buffer, $0, $4 * 4 );
		var stringArr = new Int32Array( Module.HEAPU32.buffer, $1, $5 * 2 );
		var floatArr = new Float32Array( Module.HEAPU32.buffer, $2, $6 );
		var intArr = new Int32Array( Module.HEAPU32.buffer, $3, $7 );
		var o = $8?{}:[];
		var objectStack = [];
		var n;
		var str = 0;
		if(0)
		{
			var s = '';
			for( var z = 0; z < $4*4; z++ ) s += mapArr[z] + ', ';
			console.log( "DOTHING:", s);
		}
		//console.log( "----------------------- Revive object:", o, $4 );
		for( n = 0; n < $4; n++ ) {
			var classIndex;
			const typeInfo = mapArr[n*4+0];
			//console.log( "Revive:", typeInfo, n*4, mapArr[n*4+1] );
			if( typeInfo == -6 ){  // close object
				o = objectStack.pop();
				continue;
			} else if( typeInfo == -7 ) { // close array 
				o = objectStack.pop();
				continue;
			}
			const strIndex = mapArr[n*4+2]-1;
			//console.log( "StrIndex : ", strIndex, stringArr[strIndex*2+1] );
			var className = null;
			if( classIndex = mapArr[n*4+3] ) {
				//console.log( "This got a classname?");
				className = UTF8ToString( stringArr[classIndex*2+0], stringArr[classIndex*2+1] );
			}
			else
				className = null;

			var index = mapArr[n*4+1];
			if( index <	 0 )
				index = -(index+1);
			else {
				//console.log( "no string indexes yet..." );
				index = UTF8ToString( stringArr[(index-1)*2+0], stringArr[(index-1)*2+1] );
			}
			//console.log( "Index:", index, typeof( index ) );

			if( typeInfo == (15+13) ) {
				// REF type.
				
			}
			if( typeInfo >= 15 && typeInfo <= (15+12) ) { // arrayBuffer
				const strLen = stringArr[strIndex*2+1];
				var u8 = new Uint8Array( Module.HEAPU8, stringArr[strIndex*2+0], strLen );

				var ab = new Uint8Array( strLen );
				for( var b = 0; b < strLen; b++ )
					ab[b] = u8[b];
				if( typeInfo === 15 )
					o[index] = ab.buffer;
				else if( typeInfo === 16 )
					o[index] = ab;
				else
					o[index] = new Module.arrayTypeFunc[typeInfo-15]( ab );
			} else switch( typeInfo ) {
			case -1: // undefined
				o[index] = undefined;
				break;
			case 0: // UNDSET
			default:
				break;
			case 1: // NULL
				o[index] = null;
				break;
			case 2: // TRUE
				o[index] = true;
				break;
			case 3: // FALSE
				o[index] = false;
				break;
			case 4: // STRING
				o[index] = UTF8ToString( stringArr[strIndex*2+0], stringArr[strIndex*2+1] );
				break;
			case 5: // NUMBER
				{
					const valindex = mapArr[n*4+2];
					if( valindex & 0x40000000 )
						o[index] = intArr[valindex & 0x3FFFFFFF ];
					else
						o[index] = floatArr[valindex];
				}
				break;
			case 6: // OBJECT
				objectStack.push(o);
				o = ( o[index] = {} );
				break;
			case 7: // ARRAY
				objectStack.push(o);
				o = ( o[index] = [] );
				break;
			case 8: // NEG_NAN
				o[index] = -NaN;
				break;
			case 9: // NAN
				o[index] = NaN;
				break;
			case 10: // NEG_INFINITY
				o[index] = -Infinity;
				break;
			case 11: // Infinity
				o[index] = Infinity;
				break;
			case 12: // Date
				o[index] = new Date( UTF8ToString( stringArr[strIndex*2+0], stringArr[strIndex*2+1] ) );
				break;
			case 13: // BigInt
				var s;
				s= UTF8ToString( stringArr[strIndex*2+0], stringArr[strIndex*2+1] -1 );
				console.log( "STRING IS:", s, stringArr[strIndex*2+1] -1 );
				o[index] = BigInt(s= UTF8ToString( stringArr[strIndex*2+0], stringArr[strIndex*2+1] -1 ) );
				console.log( "STRING IS:", s );
				break;
			case 14: // EMPTY
				o[index] = undefined;
				delete o[index];
				break;
			}
		}
		return Module.this_.getIndex( o );
	), revive->map, revive->stringMap, revive->floatMap, revive->intMap 
	, revive->mapIndex, revive->stringIndex, revive->floatIndex, revive->intIndex
	, firstObject
	);
}

//#define MODE NewStringType::kInternalized

static inline int makeValue( struct jsox_value_container *val, struct reviver_data *revive ) {

	int result = -1;
	switch( val->value_type ) {
	case JSOX_VALUE_UNDEFINED:
		result = JS_VALUE_UNDEFINED;
		break;
	default:
		if( val->value_type >= JSOX_VALUE_TYPED_ARRAY && val->value_type <= JSOX_VALUE_TYPED_ARRAY_MAX ) {
			int ab = makeArrayBuffer( val->stringLen );			
			makeTypedArray( ab, val->value_type - JSOX_VALUE_TYPED_ARRAY );
			//Local<ArrayBuffer> ab = ArrayBuffer::New( revive->isolate, val->string, val->stringLen, ArrayBufferCreationMode::kExternalized );
		}
		else {
			lprintf( "Parser faulted; should never have a uninitilized value." );
			result = 0;
		}
		break;
	case JSOX_VALUE_NULL:
		result = JS_VALUE_NULL;
		break;
	case JSOX_VALUE_TRUE:
		result = JS_VALUE_TRUE;
		break;
	case JSOX_VALUE_FALSE:
		result = JS_VALUE_FALSE;
		break;
	case JSOX_VALUE_EMPTY:
		result = JS_VALUE_UNDEFINED;
		break;
	case JSOX_VALUE_STRING:
		result = makeString( val->string, val->stringLen );
		break;
	case JSOX_VALUE_NUMBER:
		if( val->float_result )  {
			result = EM_ASM_INT( { return Module.this_.objects .push( $0 )-1; },val->result_d);
		} else {
			result = EM_ASM_INT( { return Module.this_.objects .push( $0 )-1; },(int)val->result_n);
		}
		break;
	case JSOX_VALUE_ARRAY:
		result = makeArray();
		break;
	case JSOX_VALUE_OBJECT:
		if( val->className )
			result = makeObject();
		break;
	case JSOX_VALUE_NEG_NAN:
		result = JS_VALUE_NAN;
		break;
	case JSOX_VALUE_NAN:
		result = JS_VALUE_NAN;
		break;
	case JSOX_VALUE_NEG_INFINITY:
		result = JS_VALUE_NEG_INFINITY;
		break;
	case JSOX_VALUE_INFINITY:
		result = JS_VALUE_INFINITY;
		break;
	case JSOX_VALUE_BIGINT:
		result = makeBigInt( val->string, val->stringLen-1 ); // cannot convert with 'n' suffix.
		break;
	case JSOX_VALUE_DATE:
		result =makeDate( val->string, val->stringLen );
		break;
	}
	if( revive->revive ) {
		//result = revive->reviver.call<val>( revive->value, result );
	}
	return result;
}

static void buildObject( PDATALIST msg_data, int o, struct reviver_data *revive ) {
	int thisVal = JS_VALUE_NULL;
	//std::string stringKey;
	int thisKey = JS_VALUE_NULL;
	LOGICAL saveRevive = revive->revive;
	struct jsox_value_container *value;
	int sub_o = JS_VALUE_NULL;
	INDEX idx;
	int index = 0;
	DATA_FORALL( msg_data, idx, struct jsox_value_container*, value )
	{
		//lprintf( "value name is : %d %s", value->value_type, value->name ? value->name : "(NULL)" );
		switch( value->value_type ) {
		default:
			if( value->name ) {				
				thisKey = makeString( value->name, value->nameLen );
				revive->value = thisKey;
				setObject( o, thisKey, makeValue( value, revive ) );
			}
			else {
				if( value->value_type == JSOX_VALUE_EMPTY )
					revive->revive = FALSE;
				if( revive->revive )
					revive->value = makeNumber( index );
				setObjectByIndex( o, index++, thisVal = makeValue( value, revive ) );
				if( value->value_type == JSOX_VALUE_EMPTY )
					;//o.delete( revive->context, index - 1 );
			}
			revive->revive = saveRevive;
			break;
		case JSOX_VALUE_ARRAY:
			if( value->name ) {
				setObjectByName( o, value->name, sub_o = makeArray() );
			}
			else {
				if( revive->revive )
					thisKey = makeNumber( index );
				setObjectByIndex( o, index++, sub_o = makeArray() );
			}
			buildObject( value->contains, sub_o, revive );
			if( revive->revive ) {
				//Local<Value> args[2] = { thisKey, sub_o };
				//revive->reviver.call<val>( revive->_this, thisKey, sub_o );
			}
			break;
		case JSOX_VALUE_OBJECT:
			if( value->name ) {
				//if( revive->revive )
				//	revive->value = makeString( value->name, value->nameLen );
				setObjectByName( o, value->name, sub_o = makeObject() );
			}
			else {
				//if( revive->revive )
				//	revive->value = makeNumber( index );
				setObjectByIndex( o, index++, sub_o = makeObject() );
			}

			buildObject( value->contains, sub_o, revive );
			if( revive->revive ) {
				//Local<Value> args[2] = { thisKey, sub_o };
				//revive->reviver->Call( revive->_this, 2, args );
			}
			break;
		}
	}
}

static void countNodes( PDATALIST msg_data, int *counts ) {
	struct jsox_value_container *value;
	INDEX idx;
	int count = 0;
	DATA_FORALL( msg_data, idx, struct jsox_value_container*, value )
	{
		//lprintf( "value name is : %d %s", value->value_type, value->name ? value->name : "(NULL)" );
		counts[0]++;	
		if( value->className )
			counts[3]++;
		if( value->name )
			counts[3]++;
		//if( value->string )
		//	counts[3]++;
		switch( value->value_type ) {
		default:
			break;
		case JSOX_VALUE_STRING:
		case JSOX_VALUE_DATE:
		case JSOX_VALUE_BIGINT:
			counts[3]++;
			break;
		case JSOX_VALUE_NUMBER:
			if( value->float_result ) counts[2]++;
			else counts[1]++;
			break;
		case JSOX_VALUE_ARRAY:
			countNodes( value->contains, counts );
			counts[0] += 1;
			break;
		case JSOX_VALUE_OBJECT:
			countNodes( value->contains, counts );
			counts[0] += 1;
			break;
		}
	}
}


static void buildObjectMapWork( PDATALIST msg_data, struct reviver_data *revive ) {

	int thisVal = JS_VALUE_NULL;
	//std::string stringKey;
	int thisKey = JS_VALUE_NULL;
	LOGICAL saveRevive = revive->revive;
	struct jsox_value_container *value;
	int sub_o = JS_VALUE_NULL;
	INDEX idx;
	int index = 0;
	//lprintf( "----------------- Encoding OBject" );
	DATA_FORALL( msg_data, idx, struct jsox_value_container*, value )
	{
		revive->map[ revive->mapIndex*4 + 0] = value->value_type;
		if( value->className ) {
			//lprintf( "Save Param class:%d %s", revive->stringIndex+1, value->className );
			revive->stringMap[revive->stringIndex * 2+0] = (uintptr_t)value->className;
			revive->stringMap[revive->stringIndex * 2+1] = 0;
			revive->map[ revive->mapIndex*4 + 3] = ++revive->stringIndex;
		}else
			revive->map[ revive->mapIndex*4 + 3] = 0;
			

		if( value->name ) {
			//lprintf( "Save Param Name:%d %s", revive->stringIndex+1, value->name );
			revive->stringMap[revive->stringIndex * 2+0] = (uintptr_t)value->name;
			revive->stringMap[revive->stringIndex * 2+1] = value->nameLen;
			revive->map[ revive->mapIndex*4 + 1] = ++revive->stringIndex;
		}
		else {
			revive->map[ revive->mapIndex*4 + 1] = -(++index);
		}
		//lprintf( "value name is : %d %d %d %s", value->value_type, revive->mapIndex*4
		//			, revive->map[ revive->mapIndex*4 + 1]
		//			,  value->name ? value->name : "(NULL)" );
		switch( value->value_type ) {
		default:
			revive->mapIndex++;
			break;
		case JSOX_VALUE_STRING:
		case JSOX_VALUE_BIGINT:
		case JSOX_VALUE_DATE:
			//lprintf( "Save Param Name:%d %s %d", revive->stringIndex+1, value->string, value->stringLen );
			revive->stringMap[revive->stringIndex * 2+0] = (uintptr_t)value->string;
			revive->stringMap[revive->stringIndex * 2+1] = value->stringLen;
			revive->map[ revive->mapIndex*4 + 2] = ++revive->stringIndex;
			revive->mapIndex++;
			break;

		case JSOX_VALUE_NUMBER:
			if( value->float_result ) {
				revive->map[revive->mapIndex*4 + 2] = revive->floatIndex;
				revive->floatMap[revive->floatIndex++] = value->result_d;
			}
			else {
				revive->map[revive->mapIndex*4 + 2] = 0x40000000  | revive->intIndex;
				revive->intMap[revive->intIndex++] = value->result_n;
			}
			revive->mapIndex++;
			break;
		case JSOX_VALUE_ARRAY:
			revive->mapIndex++;
			buildObjectMapWork( value->contains, revive );
			revive->map[ revive->mapIndex*4 + 0] = -value->value_type;
			revive->mapIndex++;
			break;
		case JSOX_VALUE_OBJECT:
			revive->mapIndex++;
			buildObjectMapWork( value->contains, revive );
			revive->map[ revive->mapIndex*4 + 0] = -value->value_type;
			revive->mapIndex++;
			break;
		}
	}
	
}

static void buildObjectMapping( PDATALIST msg_data, struct reviver_data *revive ) {
	int counts[4] = {};
	countNodes( msg_data, counts );

	//lprintf( "Counts:%d,%d,%d,%d", counts[0], counts[1], counts[2], counts[3] );
//#undef NewArray
//#define NewArray(t,n)  malloc( sizeof(t) * n )
//#undef Release
//#define Release(p) free(p)
//#undef New
//#define New(t)  malloc( sizeof(t))
	revive->map = NewArray( uint32_t, 4 * counts[0] );
	revive->intMap = NewArray( uint32_t, counts[1] );
	revive->floatMap = NewArray( double, counts[2] );
	revive->stringMap = NewArray( uint32_t, 2 * counts[3] );

	revive->mapIndex = 0;
	revive->intIndex = 0;
	revive->floatIndex = 0;
	revive->stringIndex = 0;
	buildObjectMapWork( msg_data, revive );

}



int convertMessageToJS2( PDATALIST msg, struct reviver_data *revive ) {
	int o = JS_VALUE_NULL;
	int v = JS_VALUE_NULL;// = Object::New( revive->isolate );

	struct jsox_value_container *value = (struct jsox_value_container *)GetDataItem( &msg, 0 );
	if( value && value->contains ) {
		LOGICAL firstObject = FALSE;
		if( value->value_type == JSOX_VALUE_OBJECT ) {
#if 0
			o = makeObject();
#else
			firstObject = TRUE;
#endif			
		} else if( value->value_type == JSOX_VALUE_ARRAY )
#if 0		
			o = makeArray()
#endif
			;
		else
			lprintf( "Value has contents, but is not a container type?!" );

#if 0
		buildObject( value->contains, o, revive );
#else
		buildObjectMapping( value->contains, revive );
		o = composeJSObject( revive, firstObject );

		if(revive->map) Release( revive->map );
		if(revive->stringMap) Release( revive->stringMap );
		if(revive->floatMap) Release( revive->floatMap );
		if( revive->intMap ) Release( revive->intMap );
		//DebugDumpMem();
#endif
		return o;
	}
	if( value )
		return makeValue( value, revive );
	return JS_VALUE_UNDEFINED;
}


int ParseJSOX(  const char *utf8String, size_t len, struct reviver_data *revive ) {
	PDATALIST parsed = NULL;

	if( !jsox_parse_message( (char*)utf8String, len, &parsed ) ) {
		//PTEXT error = jsox_parse_get_error( parser->state );
		//lprintf( "Failed to parse data..." );
		//printf( "THROWING AN ERROR\n" );
		PTEXT error = jsox_parse_get_error( NULL );
		if( error )      {
			//printf( "ERROR:%s\n", GetText( error ) );
			throwError( GetText( error ) );
		}
		else {
			//printf( "Not actually an error?\n" );
			throwError( "No Error Text" );
		}
		LineRelease( error );
		return JS_VALUE_UNDEFINED;
	}
	if( parsed && parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return JS_VALUE_UNDEFINED;
		// outside should always be a single value
	}
        //logTick(3);
	int value = convertMessageToJS2( parsed, revive );
        //logTick(4);

	jsox_dispose_message( &parsed );
        //logTick(5);

	return value;
	//return val::undefined();
}

int jsox_parse( char * string, size_t stringLen, int reviver ) EMSCRIPTEN_KEEPALIVE;

int jsox_parse( char * string, size_t stringLen, int reviver )
{
	//logTick(0);
	struct reviver_data *r = New( struct reviver_data );
	r->reviver_func = reviver;

	//.._malloc and stringToUTF8()
	//If you need a string to live forever, you can create it, for example, using _malloc and stringToUTF8(). However, you must later delete it manually!
	//printf( "convertin to string...%p %s\n", string, string );
	//printf( "confted string:%s" );

	if( reviver == JS_VALUE_NULL ) {
		//r._this = args.Holder();
		r->value = makeString( "", 0 );
		r->revive = TRUE;
		r->reviver_func = reviver;
	}
	else {
		r->revive = FALSE;
	}
	int result = ParseJSOX( string, stringLen, r );

	Release( r );

	return result;

} 

static void jsox_write( struct jsox_parse_state *state, int readCallback, char *string, size_t stringLen ) EMSCRIPTEN_KEEPALIVE;
void jsox_write( struct jsox_parse_state *state, int readCallback, char *data, size_t stringLen ) {
	int result;
	for( result = jsox_parse_add_data( state, data, stringLen );
		result > 0;
		result = jsox_parse_add_data( state, NULL, 0 )
		) {
		struct jsox_value_container * val;
		PDATALIST elements = jsox_parse_get_data( state );
		val = (struct jsox_value_container *)GetDataItem( &elements, 0 );
		if( val ) {
			int object;

			struct reviver_data r;
			r.revive = FALSE;
			object = convertMessageToJS2( elements, &r );
			jsox_dispose_message( &elements );
			EM_ASM( (
				Module.this_.objects[$0](Module.this_.objects[$1]);
			));
		}
		if( result < 2 )
			break;
	}
	if( result < 0 ) {
		PTEXT error = jsox_parse_get_error( state );
		if( error ) {
			throwError( GetText( error ) );
			LineRelease( error );
		} else
			throwError( "No Error Text" );
		jsox_parse_clear_state( state );
		return;
	}
	return;	
}


//allocateUTF8