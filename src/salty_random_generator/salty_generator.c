#include <stdhdrs.h>
#include <sha1.h>

#define SALTY_RANDOM_GENERATOR_SOURCE
#include "salty_generator.h"


#define MY_MASK_MASK(n,length)   (MASK_TOP_MASK(length) << ((n)&0x7) )
#define MY_GET_MASK(v,n,mask_size)  ( ( ((MASKSET_READTYPE*)((((PTRSZVAL)v))+(n)/CHAR_BIT))[0]                                 \
 & MY_MASK_MASK(n,mask_size) )                                                                           \
	>> (((n))&0x7))


struct random_context {
	SHA1Context sha1_ctx;
	POINTER salt;
	size_t salt_size;
	void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size );
	PTRSZVAL psv_user;
	uint8_t entropy[SHA1HashSize];
	size_t bits_used;
	size_t bits_avail;
};

static void NeedBits( struct random_context *ctx )
{
	if( ctx->getsalt )
		ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
	else
		ctx->salt_size = 0;
	if( ctx->salt_size )
		SHA1Input( &ctx->sha1_ctx, ctx->salt, ctx->salt_size );
	// this is redundant since it's already self seeded with prior result
	//else
	//	SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
	//lprintf( "added %p %d", ctx->salt, ctx->salt_size );
	SHA1Result( &ctx->sha1_ctx, ctx->entropy );
	SHA1Reset( &ctx->sha1_ctx );
	SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
	//LogBinary( ctx->entropy, SHA1HashSize );
	ctx->bits_used = 0;
	ctx->bits_avail = sizeof( ctx->entropy ) * 8;
}

struct random_context *SRG_CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user )
{
	struct random_context *ctx = New( struct random_context );
	SHA1Reset( &ctx->sha1_ctx );
	ctx->getsalt = getsalt;
	ctx->psv_user = psv_user;
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
	return ctx;
}

S_32 SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed )
{
	_32 tmp;
	_32 partial_tmp;
	int partial_bits = 0;
	if( bits > ( ctx->bits_avail - ctx->bits_used ) )
	{
		if( ctx->bits_avail - ctx->bits_used )
		{
			partial_bits = ctx->bits_avail - ctx->bits_used;
			partial_tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, partial_bits );
			bits -= partial_bits;
		}
		NeedBits( ctx );
	}
	{
		tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, bits );
		ctx->bits_used += bits;
		if( partial_bits )
		{
			tmp |= partial_tmp << bits;
			bits += partial_bits;
		}
		if( get_signed )
			if( tmp & ( 1 << ( bits - 1 ) ) )
			{
				_32 negone = ~0;
				negone <<= bits;
				return (S_32)( tmp | negone );
			}
	}
	return (S_32)( tmp );

}

void SRG_ResetEntropy( struct random_context *ctx )
{
	SHA1Reset( &ctx->sha1_ctx );
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
}

void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder )
{
	if( !(*external_buffer_holder) )
		(*external_buffer_holder) = New( struct random_context );
	MemCpy( (*external_buffer_holder), ctx, sizeof( struct random_context ) );
}

void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder )
{
	MemCpy( ctx, (external_buffer_holder), sizeof( struct random_context ) );
}


#if WIN32
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved
						 )
{
	return TRUE;
}

// this is the watcom deadstart entry point.
// by supplying this routine, then the native runtime doesn't get pulled
// and no external clbr symbols are required.
void __DLLstart( void )
{
}
#endif
