#include <stdint.h>
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
    function("k12_init", k12_Init, allow_raw_pointers() );
    function("k12_updaet", k12_Update, allow_raw_pointers() );
    function("k12_final", K12_Final, allow_raw_pointers() );
    function("k12_squeeze", K12_Squeeze, allow_raw_pointers() );
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
