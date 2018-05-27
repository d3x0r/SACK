#include <stdint.h>
#include <emscripten/bind.h>

using namespace emscripten;

#include "gsm.h"

struct a {
	gsm gsm_inst;
	int frame_size;
	short *input_data;
	uint8_t *compress_buffer;
	size_t compress_segments;
	gsm_signal compress_partial_buffer[160];
	int compress_partial_buffer_idx;
};

void* do_gsm_open( void ) {
	return gsm_create();
}

uint8_t*  do_gsm_encode( void*gsm, float *buffer ) {
	struct a* state = (struct a*)gsm;
	gsm_encode( state->gsm_inst, NULL, NULL );
	return NULL;
}

float * do_gsm_decode( void*gsm, uint8_t* buffer ) {
	return NULL;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("gsm_open", do_gsm_open, allow_raw_pointers() );
    function("gsm_decode", do_gsm_encode, allow_raw_pointers() );
    function("gsm_encode", do_gsm_encode, allow_raw_pointers() );
}

/*
EMSCRIPTEN_BINDINGS(my_value_example) {
    value_array<Point2f>("Point2f")
        .element(&Point2f::x)
        .element(&Point2f::y)
        ;

    value_object<PersonRecord>("PersonRecord")
        .field("name", &PersonRecord::name)
        .field("age", &PersonRecord::age)
        ;

    function("findPersonAtLocation", &findPersonAtLocation);
}
*/