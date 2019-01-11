#ifndef SACK_SRG_INTERNAL_INCLUDED
#define SACK_SRG_INTERNAL_INCLUDED

#include <sha1.h>
#ifdef SACK_BAG_EXPORTS
#define SHA2_SOURCE
#endif
#include <sha2.h>
#include "../contrib/sha3lib/sha3.h"
#include <KangarooTwelve.h>


struct random_context {
	LOGICAL use_version2 : 1;
	LOGICAL use_version2_256 : 1;
	LOGICAL use_version3 : 1;
	LOGICAL use_versionK12 : 1;

	union {
		SHA1Context sha1_ctx;
		sha512_ctx  sha512;
		sha256_ctx  sha256;
		sha3_ctx_t  sha3;
		KangarooTwelve_Instance K12i;
	} f;
	size_t total_bits_used;
	POINTER salt;
	size_t salt_size;
	void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size);
	uintptr_t psv_user;
	uint8_t *entropy;
	union {
		uint8_t entropy0[SHA1HashSize];
		uint8_t entropy2[SHA512_DIGEST_SIZE];
		uint8_t entropy2_256[SHA256_DIGEST_SIZE];
#define SHA3_DIGEST_SIZE 64 // 512 bits
		uint8_t entropy3[SHA3_DIGEST_SIZE];
#define K12_DIGEST_SIZE 64  // 512 bits
		uint8_t entropy4[K12_DIGEST_SIZE];
	} s;
	size_t bits_used;
	size_t bits_avail;
};


struct byte_shuffle_key {
	uint8_t map[256];// shuffle works on ints.
	uint8_t dmap[256];
	struct random_context *ctx;
};

#define MY_MASK_MASK(n,length)	(MASK_TOP_MASK(length) << ((n)&0x7) )
#define MY_GET_MASK(v,n,mask_size)  ( ( ((MASKSET_READTYPE*)((((uintptr_t)v))+(n)/CHAR_BIT))[0]											\
 & MY_MASK_MASK(n,mask_size) )																									\
	>> (((n))&0x7))



#define SRG_GetBit_(tmp,ctx)    (    \
	(ctx->total_bits_used += 1),  \
	(( (ctx->bits_used) >= ctx->bits_avail )?  \
		NeedBits( ctx ):0),  \
	( tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, 1 ) ),  \
	( ctx->bits_used += 1 ),  \
	( tmp ) \
)

#define SRG_GetByte_(tmp,ctx)    (    \
	(ctx->total_bits_used += 8),  \
	(( (ctx->bits_used) >= ctx->bits_avail )?  \
		NeedBits( ctx ):0),  \
	( tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, 8 ) ),  \
	( ctx->bits_used += 8 ),  \
	( tmp ) \
)


#ifndef SALTY_RANDOM_GENERATOR_SOURCE
extern 
#endif
 void NeedBits( struct random_context *ctx );

#define BlockShuffle_SubByte_(key, bytes_input, bytes_output )  ( (bytes_output)[0] = key->map[(bytes_input)[0]] )

#define BlockShuffle_SubBytes_(key, in, out, byteCount ) {  \
	size_t n;   \
	uint8_t *bytes_input = in, *bytes_output = out;  \
	uint8_t *map = key->map;  \
	for( n = 0; n < byteCount; n++, bytes_input++, bytes_output++ ) {  \
		bytes_output[0] = map[bytes_input[0]];  \
	}  \
}

#define BlockShuffle_BusByte_(key, bytes_input, bytes_output )  ( (bytes_output)[0] = key->dmap[(bytes_input)[0]] )

#define BlockShuffle_BusBytes_(key, in, out, byteCount )  {  \
	size_t n;   \
	uint8_t *bytes_input = in, *bytes_output = out;  \
	uint8_t *map = key->dmap;  \
	for( n = 0; n < byteCount; n++, bytes_input++, bytes_output++ ) {  \
		bytes_output[0] = map[bytes_input[0]];   \
	}  \
}


#endif