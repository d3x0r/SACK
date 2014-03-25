#include <stdhdrs.h>
#include <sha1.h>

#define SALTY_RANDOM_GENERATOR_SOURCE
#include "salty_generator.h"

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
	else
		SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
	//lprintf( "added %p %d", ctx->salt, ctx->salt_size );
	SHA1Result( &ctx->sha1_ctx, ctx->entropy );
	SHA1Reset( &ctx->sha1_ctx );
	SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
	LogBinary( ctx->entropy, SHA1HashSize );
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

S_64 SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed )
{
	_64 tmp;
	if( bits > ( ctx->bits_avail - ctx->bits_used ) )
		NeedBits( ctx );
	{
		tmp = GET_MASK( ctx->entropy, ctx->bits_used, bits );
		ctx->bits_used += bits;
		lprintf( " now used %d", ctx->bits_used );
		if( get_signed )
			if( tmp & ( 1 << ( bits - 1 ) ) )
			{
				_64 negone = ~0;
				negone <<= bits;
				return (S_64)( tmp | negone );
			}
	}
	return (S_64)( tmp );

}

