
#ifdef SALTY_RANDOM_GENERATOR_SOURCE
#define SRG_EXPORT EXPORT_METHOD
#else
#define SRG_EXPORT IMPORT_METHOD
#endif

//
// struct random_context *entropy = CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user );
SRG_EXPORT struct random_context *SRG_CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user );

// get a number of bits of entropy from the
// if get_signed is not 0, the result will be sign extended if the last bit is set
//  (coded on little endian; tests for if ( result & ( 1 << bits - 1 ) ) then sign extend
SRG_EXPORT S_64 SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed );


// opportunity to reset an entropy generator back to initial condition
// next call to getentropy will be the same as the first call after create.
SRG_EXPORT void SRG_ResetEntropy( struct random_context *ctx );

