package srg

/*
#include <stdhdrs.h>

int GeneratorCallback_cgo( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
        return generatorCallback( (void*)psv, salt, salt_size );
}

*/
import "C"
