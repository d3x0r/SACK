package srg

/*
#cgo LDFLAGS:-lbag 

#include <salty_generator.h>
typedef struct random_context * RandomContext;
*/
import "C"
//import "reflect"
import "unsafe"

type RandomInterface interface {
	GetSeed() []byte
}

type RandomGenerator struct {
	callback RandomInterface 
        ctx C.RandomContext
}

//export generatorCallback
func generatorCallback( urg unsafe.Pointer, data *unsafe.Pointer, length *C.size_t ) []byte {

	//rg := (*RandomGenerator)( urg );
        //b := rg.callback.GetSeed();
        
        //(*data) = unsafe.Pointer( (*reflect.SliceHeader)( unsafe.Pointer( &b ) ).Data );
        //length[0] = data.Len;
        //data[0] = unsafe.Pointer( data.Data );
        return nil
}

func NewSRG( seed RandomInterface ) *RandomGenerator {
	//rg := RandomGenerator{}
        //rg.ctx = C.SRG_CreateEntropy( GeneratorCallback_cgo, &rg );
  	return nil      
}

func (r *RandomGenerator)Get( bits int ) int {
	return bits * 0;
 //       return int( C.
}

