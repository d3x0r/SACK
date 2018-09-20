
#include "global.h"
#include <math.h>

#define TranslateText(n) (n)

static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive );
static Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive );

static struct timings {
	uint64_t start;
	uint64_t deltas[10];
}timings;

static void makeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void escapeJSOX( const v8::FunctionCallbackInfo<Value>& args );
static void parseJSOX( const v8::FunctionCallbackInfo<Value>& args );

static void showTimings( const v8::FunctionCallbackInfo<Value>& args );

class JSOXObject : public node::ObjectWrap {
	struct jsox_parse_state *state;
public:
	static Persistent<Function> constructor;
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //

public:

	static void Init( Handle<Object> exports );
	JSOXObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void write( const v8::FunctionCallbackInfo<Value>& args );

	~JSOXObject();
};

Persistent<Function> JSOXObject::constructor;

void InitJSOX( Isolate *isolate, Handle<Object> exports ){

	Local<Object> o2 = Object::New( isolate );
	SET_READONLY_METHOD( o2, "parse", parseJSOX );
	NODE_SET_METHOD( o2, "stringify", makeJSOX );
	NODE_SET_METHOD( o2, "escape", escapeJSOX );
	//NODE_SET_METHOD( o2, "timing", showTimings );
	SET_READONLY( exports, "JSOX", o2 );

	{
		Local<FunctionTemplate> parseTemplate;
		parseTemplate = FunctionTemplate::New( isolate, JSOXObject::New );
		parseTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.jsox_parser" ) );
		parseTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
		NODE_SET_PROTOTYPE_METHOD( parseTemplate, "write", JSOXObject::write );

		JSOXObject::constructor.Reset( isolate, parseTemplate->GetFunction() );

		SET_READONLY( o2, "begin", parseTemplate->GetFunction() );
	}

}

JSOXObject::JSOXObject() {
	state = jsox_begin_parse();
}

JSOXObject::~JSOXObject() {
	jsox_parse_dispose_state( &state );
}

#define logTick(n) do { uint64_t tick = GetCPUTick(); if( n >= 0 ) timings.deltas[n] += tick-timings.start; timings.start = tick; } while(0)


void JSOXObject::write( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	JSOXObject *parser = ObjectWrap::Unwrap<JSOXObject>( args.Holder() );
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Missing data parameter." ) ) );
		return;
	}

	String::Utf8Value data( USE_ISOLATE( isolate ) args[0]->ToString() );
	int result;
	for( result = jsox_parse_add_data( parser->state, *data, data.length() );
		result > 0;
		result = jsox_parse_add_data( parser->state, NULL, 0 )
		) {
		struct jsox_value_container * val;
		PDATALIST elements = jsox_parse_get_data( parser->state );
		Local<Object> o;
		Local<Value> v;// = Object::New( isolate );

		Local<Value> argv[1];
		val = (struct jsox_value_container *)GetDataItem( &elements, 0 );
		if( val ) {
			struct reviver_data r;
			r.revive = FALSE;
			r.isolate = isolate;
			r.context = r.isolate->GetCurrentContext();
			argv[0] = convertMessageToJS2( elements, &r );
			Local<Function> cb = Local<Function>::New( isolate, parser->readCallback );
			{
				MaybeLocal<Value> result = cb->Call( isolate->GetCurrentContext()->Global(), 1, argv );
				if( result.IsEmpty() ) // if an exception occurred stop, and return it.
					return;
			}
		}
		jsox_dispose_message( &elements );
		if( result < 2 )
			break;
	}
	if( result < 0 ) {
		PTEXT error = jsox_parse_get_error( parser->state );
		if( error ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, GetText( error ) ) ) );
			LineRelease( error );
		} else
			isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "No Error Text" STRSYM(__LINE__)) ) );
		jsox_parse_clear_state( parser->state );
		return;
	}

}

void JSOXObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must callback to read into." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		// Invoked as constructor: `new MyObject(...)`
		JSOXObject* obj = new JSOXObject();
		Handle<Function> arg0 = Handle<Function>::Cast( args[0] );
		Persistent<Function> cb( isolate, arg0 );
		obj->readCallback = cb;

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}

}







#define MODE NewStringType::kNormal
//#define MODE NewStringType::kInternalized

static inline Local<Value> makeValue( struct jsox_value_container *val, struct reviver_data *revive ) {

	Local<Value> result;
	Local<Script> script;
	switch( val->value_type ) {
	case JSOX_VALUE_UNDEFINED:
		result = Undefined( revive->isolate );
		break;
	default:
		if( val->value_type >= JSOX_VALUE_TYPED_ARRAY && val->value_type <= JSOX_VALUE_TYPED_ARRAY_MAX ) {
			Local<ArrayBuffer> ab = ArrayBuffer::New( revive->isolate, val->string, val->stringLen, ArrayBufferCreationMode::kExternalized );
			switch( val->value_type - JSOX_VALUE_TYPED_ARRAY ) {
			case 0:
				result = ab;
				break;
			case 1: // "u8"
				result = Uint8Array::New( ab, 0, val->stringLen );
				break;
			case 2:// "cu8"
				result = Uint8ClampedArray::New( ab, 0, val->stringLen );
				break;
			case 3:// "s8"
				result = Int8Array::New( ab, 0, val->stringLen );
				break;
			case 4:// "u16"
				result = Uint16Array::New( ab, 0, val->stringLen );
				break;
			case 5:// "s16"
				result = Int16Array::New( ab, 0, val->stringLen );
				break;
			case 6:// "u32"
				result = Uint32Array::New( ab, 0, val->stringLen );
				break;
			case 7:// "s32"
				result = Int32Array::New( ab, 0, val->stringLen );
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen );
			//	break;
			case 10:// "f32"
				result = Float32Array::New( ab, 0, val->stringLen );
				break;
			case 11:// "f64"
				result = Float64Array::New( ab, 0, val->stringLen );
				break;
			default:
				result = Undefined( revive->isolate );
			}
		}
		else {
			lprintf( "Parser faulted; should never have a uninitilized value." );
			result = Undefined( revive->isolate );
		}
		break;
	case JSOX_VALUE_NULL:
		result = Null( revive->isolate );
		break;
	case JSOX_VALUE_TRUE:
		result = True( revive->isolate );
		break;
	case JSOX_VALUE_FALSE:
		result = False( revive->isolate );
		break;
	case JSOX_VALUE_EMPTY:
		result = Undefined(revive->isolate);
		break;
	case JSOX_VALUE_STRING:
		result = String::NewFromUtf8( revive->isolate, val->string, MODE, (int)val->stringLen ).ToLocalChecked();
		break;
	case JSOX_VALUE_NUMBER:
		if( val->float_result )
			result = Number::New( revive->isolate, val->result_d );
		else
			result = Number::New( revive->isolate, (double)val->result_n );
		break;
	case JSOX_VALUE_ARRAY:
		result = Array::New( revive->isolate );
		break;
	case JSOX_VALUE_OBJECT:
		if( val->className )
		result = Object::New( revive->isolate );
		break;
	case JSOX_VALUE_NEG_NAN:
		result = Number::New(revive->isolate, -NAN);
		break;
	case JSOX_VALUE_NAN:
		result = Number::New(revive->isolate, NAN);
		break;
	case JSOX_VALUE_NEG_INFINITY:
		result = Number::New(revive->isolate, -INFINITY);
		break;
	case JSOX_VALUE_INFINITY:
		result = Number::New(revive->isolate, INFINITY);
		break;
	case JSOX_VALUE_BIGINT:
		script = Script::Compile( String::NewFromUtf8( revive->isolate, val->string, NewStringType::kNormal, val->stringLen ).ToLocalChecked()
			, String::NewFromUtf8( revive->isolate, "BigIntFormatter", NewStringType::kInternalized ).ToLocalChecked() );
		result = script->Run();
		//BigInt::NewFromWords();
		//result = BigInt::New( revive->isolate, 0 );
		break;
	case JSOX_VALUE_DATE:
		char buf[64];
		snprintf( buf, 64, "new Date('%s')", val->string );
		script = Script::Compile( String::NewFromUtf8( revive->isolate, buf, NewStringType::kNormal ).ToLocalChecked()
			, String::NewFromUtf8( revive->isolate, "DateFormatter", NewStringType::kInternalized ).ToLocalChecked() );
		result = script->Run();
		//result = Date::New( revive->isolate, 0 );
		break;
	}
	if( revive->revive ) {
		Local<Value> args[2] = { revive->value, result };
		Local<Value> r = revive->reviver->Call( revive->_this, 2, args );
	}
	return result;
}

static void buildObject( PDATALIST msg_data, Local<Object> o, struct reviver_data *revive ) {
	Local<Value> thisVal;
	Local<String> stringKey;
	Local<Value> thisKey;
	LOGICAL saveRevive = revive->revive;
	struct jsox_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	int index = 0;
	DATA_FORALL( msg_data, idx, struct jsox_value_container*, val )
	{
		//lprintf( "value name is : %d %s", val->value_type, val->name ? val->name : "(NULL)" );
		switch( val->value_type ) {
		default:
			if( val->name ) {
				stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
				revive->value = stringKey;
				o->CreateDataProperty( revive->context, stringKey
						, makeValue( val, revive ) );
			}
			else {
				if( val->value_type == JSOX_VALUE_EMPTY )
					revive->revive = FALSE;
				if( revive->revive )
					revive->value = Integer::New( revive->isolate, index );
				o->Set( index++, thisVal = makeValue( val, revive ) );
				if( val->value_type == JSOX_VALUE_EMPTY )
					o->Delete( revive->context, index - 1 );
			}
			revive->revive = saveRevive;
			break;
		case JSOX_VALUE_ARRAY:
			if( val->name ) {
				o->CreateDataProperty( revive->context,
					stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked()
					, sub_o = Array::New( revive->isolate ) );
				thisKey = stringKey;
			}
			else {
				if( revive->revive )
					thisKey = Integer::New( revive->isolate, index );
				o->Set( index++, sub_o = Array::New( revive->isolate ) );
			}
			buildObject( val->contains, sub_o, revive );
			if( revive->revive ) {
				Local<Value> args[2] = { thisKey, sub_o };
				revive->reviver->Call( revive->_this, 2, args );
			}
			break;
		case JSOX_VALUE_OBJECT:
			if( val->name ) {
				stringKey = String::NewFromUtf8( revive->isolate, val->name, MODE, (int)val->nameLen ).ToLocalChecked();
				o->CreateDataProperty( revive->context, stringKey
							, sub_o = Object::New( revive->isolate ) );
				thisKey = stringKey;
			}
			else {
				if( revive->revive )
					thisKey = Integer::New( revive->isolate, index );
				o->Set( index++, sub_o = Object::New( revive->isolate ) );
			}

			buildObject( val->contains, sub_o, revive );
			if( revive->revive ) {
				Local<Value> args[2] = { thisKey, sub_o };
				revive->reviver->Call( revive->_this, 2, args );
			}
			break;
		}
	}
}

Local<Value> convertMessageToJS2( PDATALIST msg, struct reviver_data *revive ) {
	Local<Object> o;
	Local<Value> v;// = Object::New( revive->isolate );

	struct jsox_value_container *val = (struct jsox_value_container *)GetDataItem( &msg, 0 );
	if( val && val->contains ) {
		if( val->value_type == JSOX_VALUE_OBJECT )
			o = Object::New( revive->isolate );
		else if( val->value_type == JSOX_VALUE_ARRAY )
			o = Array::New( revive->isolate );
		else
			lprintf( "Value has contents, but is not a container type?!" );
		buildObject( val->contains, o, revive );
		return o;
	}
	if( val )
		return makeValue( val, revive );
	return Undefined( revive->isolate );
}



void makeJSON( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}

void showTimings( const v8::FunctionCallbackInfo<Value>& args ) {
     uint32_t val;
#define LOGVAL(n) val = ConvertTickToMicrosecond( timings.deltas[n] ); printf( #n " : %d.%03d\n", val/1000, val%1000 );
LOGVAL(0);
LOGVAL(1);
LOGVAL(2);
LOGVAL(3);
LOGVAL(4);
LOGVAL(5);
LOGVAL(6);
LOGVAL(7);
	{
		int n;for(n=0;n<10;n++) timings.deltas[n] = 0;
	}
	logTick(-1);
}

void escapeJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	if( args.Length() == 0 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, TranslateText( "Missing parameter, string to escape" ) ) ) );
		return;
	}
	char *msg;
	String::Utf8Value tmp( USE_ISOLATE( isolate ) args[0] );
	size_t outlen;
	msg = jsox_escape_string_length( *tmp, tmp.length(), &outlen );
	args.GetReturnValue().Set( String::NewFromUtf8( isolate, msg, NewStringType::kNormal, (int)outlen ).ToLocalChecked() );
	Release( msg );
}


Local<Value> ParseJSOX(  const char *utf8String, size_t len, struct reviver_data *revive ) {
	PDATALIST parsed = NULL;
        //logTick(2);
	if( !jsox_parse_message( (char*)utf8String, len, &parsed ) ) {
		//PTEXT error = jsox_parse_get_error( parser->state );
		//lprintf( "Failed to parse data..." );
		PTEXT error = jsox_parse_get_error( NULL );
		if( error )
			revive->isolate->ThrowException( Exception::Error( String::NewFromUtf8( revive->isolate, GetText( error ) ) ) );
		else
			revive->isolate->ThrowException( Exception::Error( String::NewFromUtf8( revive->isolate, "No Error Text" STRSYM(__LINE__) ) ) );
		LineRelease( error );
		return Undefined(revive->isolate);
	}
	if( parsed && parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return Undefined(revive->isolate);
		// outside should always be a single value
	}
        //logTick(3);
	Local<Value> value = convertMessageToJS2( parsed, revive );
        //logTick(4);

	jsox_dispose_message( &parsed );
        //logTick(5);

	return value;
}

void parseJSOX( const v8::FunctionCallbackInfo<Value>& args )
{
	//logTick(0);
	struct reviver_data r;
	r.isolate = Isolate::GetCurrent();
	if( args.Length() == 0 ) {
		r.isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( r.isolate, TranslateText( "Missing parameter, data to parse" ) ) ) );
		return;
	}
	const char *msg;
	String::Utf8Value tmp( USE_ISOLATE( r.isolate ) args[0] );
	Handle<Function> reviver;
	msg = *tmp;
	if( args.Length() > 1 ) {
		if( args[1]->IsFunction() ) {
			r._this = args.Holder();
			r.value = String::NewFromUtf8( r.isolate, "" );
			r.revive = TRUE;
			r.reviver = Handle<Function>::Cast( args[1] );
		}
		else {
			r.isolate->ThrowException( Exception::TypeError(
				String::NewFromUtf8( r.isolate, TranslateText( "Reviver parameter is not a function." ) ) ) );
			return;
		}
	}
	else
		r.revive = FALSE;

        //logTick(1);
	r.context = r.isolate->GetCurrentContext();

	args.GetReturnValue().Set( ParseJSOX( msg, tmp.length(), &r ) );

}


void makeJSOX( const v8::FunctionCallbackInfo<Value>& args ) {
	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), "undefined :) Stringify is not completed" ) );
}


static void Init( Handle<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();
	InitJSOX( isolate, exports );
}
NODE_MODULE( jsox, Init)

