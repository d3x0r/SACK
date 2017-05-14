#include <sack_types.h>
#ifdef SALTY_RANDOM_GENERATOR_SOURCE
#define SRG_EXPORT EXPORT_METHOD
#else
#define SRG_EXPORT IMPORT_METHOD
#endif

//
// struct random_context *entropy = CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
SRG_EXPORT struct random_context *SRG_CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

//
// struct random_context *entropy = CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
//  uses a larger salt generator...
SRG_EXPORT struct random_context *SRG_CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

// Destroya  context.  Pass the address of your 'struct random_context *entropy;   ... SRG_DestroyEntropy( &entropy ); 
SRG_EXPORT void SRG_DestroyEntropy( struct random_context **ppEntropy );

// get a large number of bits of entropy from the random_context
// buffer needs to be an integral number of 32 bit elements....
SRG_EXPORT void SRG_GetEntropyBuffer( struct random_context *ctx, uint32_t *buffer, uint32_t bits );

// get a number of bits of entropy from the
// if get_signed is not 0, the result will be sign extended if the last bit is set
//  (coded on little endian; tests for if ( result & ( 1 << bits - 1 ) ) then sign extend
SRG_EXPORT int32_t SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed );


// opportunity to reset an entropy generator back to initial condition
// next call to getentropy will be the same as the first call after create.
SRG_EXPORT void SRG_ResetEntropy( struct random_context *ctx );


// restore the random contxt from the external holder specified
// { 
//    POINTER save_context;
//    SRG_RestoreState( ctx, save_context );
// }
SRG_EXPORT void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder );

// save the random context in an external buffer holder.
// external buffer holder needs to be initialized to NULL.
// { 
//    POINTER save_context = NULL;
//    SRG_SaveState( ctx, &save_context );
// }
SRG_EXPORT void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder );

// usage
/// { uint8_t* buf; size_t buflen; SRG_DecryptData( <resultfrom encrypt>, &buf, &buflen ); }
//  buffer result must be released by user

SRG_EXPORT void SRG_DecryptData( CTEXTSTR local_password, uint8_t* *buffer, size_t *chars );

SRG_EXPORT void SRG_DecryptRawData( CPOINTER binary, size_t length, uint8_t* *buffer, size_t *chars );

// text result must release by user
SRG_EXPORT TEXTSTR SRG_DecryptString( CTEXTSTR local_password );

// encrypt a block of binary data to another binary buffer
SRG_EXPORT void SRG_EncryptRawData( CPOINTER buffer, size_t buflen, uint8_t* *result_buf, size_t *result_size );

// text result must release by user
SRG_EXPORT TEXTCHAR * SRG_EncryptData( CPOINTER buffer, size_t buflen );

// text result must release by user
// calls EncrytpData with buffer and string length + 1 to include the null for decryption.
SRG_EXPORT TEXTCHAR * SRG_EncryptString( CTEXTSTR buffer );

// return a unique ID
SRG_EXPORT char * SRG_ID_Generator( void );

