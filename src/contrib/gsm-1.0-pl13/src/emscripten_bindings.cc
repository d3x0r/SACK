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
	compress_buffer = malloc( short, datalen * 160 );
	gsm_encode( state->gsm_inst, NULL, NULL );
	return NULL;
}

// , POINTER data, size_t datalen
float * do_gsm_decode( void*gsm, uint8_t* buffer ) {
	struct a* state = (struct a*)gsm;

	int n;
	short *decompress_buffer;
	if( datalen % 33 )
	{
		lprintf( "Invalid bufffer received %d (%d:%d)", datalen, datalen/33, datalen%33 );
		return;
	}
	datalen /= 33;
	decompress_buffer = malloc( short, datalen * 160 );
	for( n = 0; n < datalen; n ++ )
	{
		gsm_decode( ad->gsm_inst, ((gsm_byte*)data) + n*33, decompress_buffer + n * 160 );
	}
	GetPlaybackAudioBuffer( ad, decompress_buffer, n * 320, n * 160 );
	Deallocate( short *, decompress_buffer );

	gsm_decode( state->gsm_inst, NULL, NULL );
	return NULL;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("_gsm_open", do_gsm_open, allow_raw_pointers() );
    function("_gsm_decode", do_gsm_encode, allow_raw_pointers() );
    function("_gsm_encode", do_gsm_encode, allow_raw_pointers() );
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

#ifdef JAVSCRIPT_CODE


    // Import function from Emscripten generated file
_gsm_decode
_gsm_encode

    gsm_encode = Module.cwrap(
        //  'gsm_encode', 'number', ['number', 'number', 'number']
    );

function gsm_open() {
    return _gsm_open;
}

// data is a Float32Array that we need to copy
function gsm_encode( data ) {
    // Get data byte size, allocate memory on Emscripten heap, and get pointer
    var nDataBytes = data.length * 2;//data.BYTES_PER_ELEMENT;
    var dataPtr = Module._malloc(nDataBytes);
    // Copy data to Emscripten heap (directly accessed from Module.HEAPU8)
    var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, nDataBytes);
    for( var n = 0; n < data.length ;n++ ) {
        dataHeap[n] = ((data[n]+1) * 32767.5)|0;
    }
    //dataHeap.set(new Uint8Array(data.buffer));
    // Call function and get result
    gsm_encode( gsm, dataHeap.byteOffset, data.length);
    var result = new Uint8Array(dataHeap.buffer, dataHeap.byteOffset, data.length);
    // Free memory
    Module._free(dataHeap.byteOffset);

    return result;
}

function gsm_decode( data ) {
    // Get data byte size, allocate memory on Emscripten heap, and get pointer
    var nDataBytes = data.length * 2;//data.BYTES_PER_ELEMENT;
    var dataPtr = Module._malloc(nDataBytes);
    // Copy data to Emscripten heap (directly accessed from Module.HEAPU8)
    var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, nDataBytes);
    dataHeap.set(new Uint8Array(data.buffer));
    // Call function and get result
    gsm_encode( gsm, dataHeap.byteOffset, data.length);

    var result = new Float32Array(data.length);
    for( var n = 0; n < data.length ;n++ ) {
        result[n] = (data[n] / 32767.5)-1;
    }
    //var result = new Float32Array(dataHeap.buffer, dataHeap.byteOffset, data.length);
    // Free memory
    Module._free(dataHeap.byteOffset);

    return result;
}
