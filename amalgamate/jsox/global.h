#include <stdint.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

#include "jsox.h"

class reviver_data {
public:
	//Isolate *isolate;
	//Local<Context> context;
	LOGICAL revive;
	int index;
	val value;
	val _this;
	val reviver;
 reviver_data():value(0),_this(NULL),reviver(val::null()) { revive = FALSE; index = 0; }
};

val convertMessageToJS( PDATALIST msg_data, struct reviver_data *reviver );
val convertMessageToJS2( PDATALIST msg_data, struct reviver_data *reviver );

