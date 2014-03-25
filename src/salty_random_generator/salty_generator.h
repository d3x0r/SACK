
#ifdef SALTY_RANDOM_GENERATOR_SOURCE
#define SRG_EXPORT EXPORT_METHOD
#else
#define SRG_EXPORT IMPORT_METHOD
#endif

//
// struct random_context *entropy = CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user );
SRG_EXPORT struct random_context *SRG_CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user );


SRG_EXPORT S_64 SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed );


