#include <node.h>
#include <node_object_wrap.h>

#include <v8.h>
#include <uv.h>

#define V8_AT_LEAST(major, minor) (V8_MAJOR_VERSION > major || (V8_MAJOR_VERSION == major && V8_MINOR_VERSION >= minor))
//#include <nan.h>

#include "jsox.h"


#undef New

//#include <openssl/ssl.h>
#include <openssl/safestack.h>  // STACK_OF
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
//#include <openssl/>

#ifdef INCLUDE_GUI
#include "gui/gui_global.h"

#endif

#if NODE_MAJOR_VERSION >= 10
#  define USE_ISOLATE(i)   (i),
#else
#  define USE_ISOLATE(i)
#endif


using namespace v8;

//fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekSet" ), Integer::New( isolate, SEEK_SET ), ReadOnlyProperty );

#define SET_READONLY( object, name, data ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name), data, ReadOnlyProperty )
#define SET_READONLY_METHOD( object, name, method ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name), v8::Function::New(isolate, method ), ReadOnlyProperty )


#define ReadOnlyProperty (PropertyAttribute)((int)PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)


struct reviver_data {
	Isolate *isolate;
	Local<Context> context;
	LOGICAL revive;
	int index;
	Handle<Value> value;
	Handle<Object> _this;
	Handle<Function> reviver;
};

Local<Value> convertMessageToJS( PDATALIST msg_data, struct reviver_data *reviver );
Local<Value> convertMessageToJS2( PDATALIST msg_data, struct reviver_data *reviver );

