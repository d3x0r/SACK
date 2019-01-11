#include <sack_types.h>
#ifdef SALTY_RANDOM_GENERATOR_SOURCE
#define SRG_EXPORT EXPORT_METHOD
#else
#define SRG_EXPORT IMPORT_METHOD
#endif

//
// struct random_context *entropy = CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
// uses sha1
SRG_EXPORT struct random_context *SRG_CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

//
// struct random_context *entropy = CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
//  uses a larger salt generator... (sha2-512)
SRG_EXPORT struct random_context *SRG_CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

//
// struct random_context *entropy = CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
//  uses a sha2-256
SRG_EXPORT struct random_context *SRG_CreateEntropy2_256( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

//
// struct random_context *entropy = CreateEntropy3( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
//  uses a sha3-512 (keccak)
SRG_EXPORT struct random_context *SRG_CreateEntropy3( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );

//
// struct random_context *entropy = CreateEntropy4( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user );
//  uses a K12-512
SRG_EXPORT struct random_context *SRG_CreateEntropy4( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user );

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

// After SRG_ResetEntropy(), this takes the existing entropy
// already in the random_context and seeds the entropy generator
// with this existing digest;  GetEntropy/GetEntropyBuffer do this
// internally; but for user control, this is separated from just
// ResetEntropy().
//   SRG_ResetEntropy(ctx);   // reset entropy generator to empty.
//   SRG_StreamEntropy(ctx);  // continue from last ending
//   SRG_FeedEntropy(ctx, /*buffer*/ ); // mix in some more entropy
//
SRG_EXPORT void SRG_StreamEntropy( struct random_context *ctx );

// Manually load some salt into the next enropy buffer to e retreived.
// sets up to add the next salt into the buffer.
SRG_EXPORT void SRG_FeedEntropy( struct random_context *ctx, const uint8_t *salt, size_t salt_size );

// restore the random contxt from the external holder specified
// { 
//    POINTER save_context;
//    SRG_SaveState( ctx, &save_context );  // will allocate space for the context
//    SRG_RestoreState( ctx, save_context ); // context should previously be saved
// }

SRG_EXPORT void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder );

// save the random context in an external buffer holder.
// external buffer holder needs to be initialized to NULL.
// { 
//    POINTER save_context = NULL;
//    SRG_SaveState( ctx, &save_context );
// }
SRG_EXPORT void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder, size_t *dataSize );

//
// Randeom Hash generators.  Returns a 256 bit hash in a base 64 string.
// internally seeded by clocks 
// Are thread safe; current thread pool is 32 before having to wait
//
// return a unique ID using SHA1
SRG_EXPORT char * SRG_ID_Generator( void );

// return a unique ID using SHA2_512
SRG_EXPORT char * SRG_ID_Generator2( void );
// return a unique ID using SHA2_256
SRG_EXPORT char *SRG_ID_Generator_256( void );
// return a unique ID using SHA3-keccak-512
SRG_EXPORT char *SRG_ID_Generator3( void );
// return a unique ID using SHA3-K12-512
SRG_EXPORT char *SRG_ID_Generator4( void );

//------------------------------------------------------------------------
//   crypt_util.c extra simple routines - kinda like 'passwd'
//
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

// Simplified encyprtion wrapper around OpenSSL/LibreSSL EVP AES-256-CBC, uses key as IV also.
// result is length; address of pointer to cyphertext is filled in with an Allocated buffer.
// Limitation of 4G-byte encryption.
// automaically adds padding as required.
SRG_EXPORT int SRG_AES_decrypt( uint8_t *ciphertext, int ciphertext_len, uint8_t *key, uint8_t **plaintext );

// Simplified encyprtion wrapper around OpenSSL/LibreSSL EVP AES-256-CBC, uses key as IV also.
// result is length; address of pointer to cyphertext is filled in with an Allocated buffer.
// Limitation of 4G-byte encryption.
// automaically adds padding as required.
SRG_EXPORT size_t SRG_AES_encrypt( uint8_t *plaintext, size_t plaintext_len, uint8_t *key, uint8_t **ciphertext );

// xor-sub-wipe-sub encryption.  
// encrypts objBuf of objBufLen using (keyBuf+tick)
// pointers refrenced passed to outBuf and outBufLen are filled in with the result
// Will automatically add 4 bytes and pad up to 8
SRG_EXPORT void SRG_XSWS_encryptData( uint8_t *objBuf, size_t objBufLen
	, uint64_t tick, uint8_t *keyBuf, size_t keyBufLen
	, uint8_t **outBuf, size_t *outBufLen
);

// xor-sub-wipe-sub decryption.  
// decrypts objBuf of objBufLen using (keyBuf+tick)
// pointers refrenced passed to outBuf and outBufLen are filled in with the result
// 

SRG_EXPORT void SRG_XSWS_decryptData( uint8_t *objBuf, size_t objBufLen
	, uint64_t tick, uint8_t *keyBuf, size_t keyBufLen
	, uint8_t **outBuf, size_t *outBufLen
);


//--------------------------------------------------------------
// block_shuffle.c
//
// Utilities to shuffle 2D data.
//
//  This can use a small swap block to tile over a larger 2D area
//  
//  shuffles a matrix of bytes
//  1D operation is available by setting either height to 1 
//  (arrays are 'wide' before they are 'high')

/*
{
	struct block_shuffle_key *key = BlockShuffle_CreateKey( SRG_CreateEntropy( NULL, 0 ), 8, 8 );
	uint8_t input_bytes[8][18];
	uint8_t encoded_bytes[8][8];
	uint8_t output_bytes[8][36];
	BlockShuffle_SetDataBlock( key, input, 2, 2, 15, 3, sizeof( input_bytes[0] )
		encoded, 0, 0, sizeof( encoded_bytes[0] ) );


	BlockShuffle_GetDataBlock( key, encoded, 2, 2, 15, 3, sizeof( encoded_bytes[0] )
		output_bytes, 0, 0, sizeof( input_bytes[0] ) );


}

{
	struct block_shuffle_key *BlockShuffle_CreateKey( SRG_CreateEntropy( NULL, 0 ), 8, 8 );
	uint8_t input_bytes[8][18];
	uint8_t encoded_bytes[8][8];
	uint8_t output_bytes[8][36];
}
*/

// API subjet to CHANGE!

// creates a swap-matrix of width by height matrix.  Could be a linear
// swap width (or height) is 1
SRG_EXPORT struct block_shuffle_key *BlockShuffle_CreateKey( struct random_context *ctx, size_t width, size_t height );

// do substitution within a range of data
SRG_EXPORT void BlockShuffle_SetDataBlock( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, int y, size_t w, size_t h, size_t output_stride
	, uint8_t* input, int ofs_x, int ofs_y, size_t input_stride );

// do linear substitution over a range
SRG_EXPORT void BlockShuffle_SetData( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, size_t w
	, uint8_t* input, int ofs_x );

// reverse subsittuion within a range of data
SRG_EXPORT void BlockShuffle_GetDataBlock( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, int y, size_t w, size_t h, size_t encrypted_stride
	, uint8_t* output, int ofs_x, int ofs_y, size_t stride );

// reverse linear substituion over a range.
SRG_EXPORT void BlockShuffle_GetData( struct block_shuffle_key *key
	, uint8_t* encrypted, size_t x, size_t w
	, uint8_t* output, size_t ofs_x );

// Allocate a byte shuffler.
// This transformation creates a unique mapping of byteA to byteB.
// The SubByte and BusByte operations may be performed in either order
// but the complimentary function is required to decode the buffer.
//  (A->B) mapping with SubByte is different from (A->B) mapping with BusByte
// Bus(A) != Sub(A)  but  Bus(Sub(A)) == Sub(Bus(A)) == A
SRG_EXPORT struct byte_shuffle_key *BlockShuffle_ByteShuffler( struct random_context *ctx );

// Releases any resource sassociated with_byte shuffler_key.
void BlockShuffle_DropByteShuffler( struct byte_shuffle_key *key );

// BlockSHuffle_SubBytes and BLockShuffle_BusBytes are reflective routines.
//  They read bytes from 'bytes' and otuput to 'out_bytes'
//  in-place operation (bytes == out_bytes) is posssible.
// SubBytes swaps A->B
SRG_EXPORT void BlockShuffle_SubBytes( struct byte_shuffle_key *key
	, uint8_t *bytes, uint8_t *out_bytes, size_t byteCount );
// swap a single byte; can be in-place.
SRG_EXPORT void BlockShuffle_SubByte( struct byte_shuffle_key *key
	, uint8_t *bytes, uint8_t *out_bytes );

// BlockSHuffle_SubBytes and BlockShuffle_BusBytes are reflective routines.
//  They read bytes from 'bytes' and otuput to 'out_bytes'
//  in-place operation (bytes == out_bytes) is posssible.
// BusBytes swaps B->A
SRG_EXPORT void BlockShuffle_BusBytes( struct byte_shuffle_key *key, uint8_t *bytes
	, uint8_t *out_bytes, size_t byteCount );
// swap a single byte; can be in-place.
SRG_EXPORT void BlockShuffle_BusByte( struct byte_shuffle_key *key
	, uint8_t *bytes, uint8_t *out_bytes );
