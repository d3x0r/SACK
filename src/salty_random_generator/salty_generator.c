#include <stdhdrs.h>
#include <sha1.h>
#include <sha2.h>
#ifndef SALTY_RANDOM_GENERATOR_SOURCE
#define SALTY_RANDOM_GENERATOR_SOURCE
#endif
#include <salty_generator.h>


#define MY_MASK_MASK(n,length)	(MASK_TOP_MASK(length) << ((n)&0x7) )
#define MY_GET_MASK(v,n,mask_size)  ( ( ((MASKSET_READTYPE*)((((PTRSZVAL)v))+(n)/CHAR_BIT))[0]											\
 & MY_MASK_MASK(n,mask_size) )																									\
	>> (((n))&0x7))


struct random_context {
	LOGICAL use_version2;

	SHA1Context sha1_ctx;
	sha512_ctx  sha512;

	POINTER salt;
	size_t salt_size;
	void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size );
	PTRSZVAL psv_user;
	uint8_t entropy[SHA1HashSize];
	uint8_t entropy2[SHA512_DIGEST_SIZE];
	size_t bits_used;
	size_t bits_avail;
};

static void NeedBits( struct random_context *ctx )
{
	if( ctx->getsalt )
		ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
	else
		ctx->salt_size = 0;
	if( ctx->use_version2 )
	{
		if( ctx->salt_size )
			sha512_update( &ctx->sha512, (const uint8_t*)ctx->salt, ctx->salt_size );
		sha512_final( &ctx->sha512, ctx->entropy2 );
		sha512_init( &ctx->sha512 );
		sha512_update( &ctx->sha512, ctx->entropy2, SHA512_DIGEST_SIZE );
		ctx->bits_avail = sizeof( ctx->entropy2 ) * 8;
	}
	else
	{
		if( ctx->salt_size )
			SHA1Input( &ctx->sha1_ctx, (const uint8_t*)ctx->salt, ctx->salt_size );
		SHA1Result( &ctx->sha1_ctx, ctx->entropy );
		SHA1Reset( &ctx->sha1_ctx );
		SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
		ctx->bits_avail = sizeof( ctx->entropy ) * 8;
	}
	ctx->bits_used = 0;
}

struct random_context *SRG_CreateEntropyInternal( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user, LOGICAL version2 )
{
	struct random_context *ctx = New( struct random_context );
	ctx->use_version2 = version2;
	if( ctx->use_version2 )
		sha512_init( &ctx->sha512 );
	else
		SHA1Reset( &ctx->sha1_ctx );
	ctx->getsalt = getsalt;
	ctx->psv_user = psv_user;
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
	return ctx;
}

struct random_context *SRG_CreateEntropy( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE );
}

struct random_context *SRG_CreateEntropy2( void (*getsalt)( PTRSZVAL, POINTER *salt, size_t *salt_size ), PTRSZVAL psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, TRUE );
}

void SRG_GetEntropyBuffer( struct random_context *ctx, _32 *buffer, _32 bits )
{
	_32 tmp;
	_32 partial_tmp;
	int partial_bits = 0;
	_32 get_bits;

	do
	{
		if( bits > sizeof( tmp ) * 8 )
			get_bits = sizeof( tmp ) * 8;
		else
			get_bits = bits;

		// only greater... if equal just grab the bits.
		if( get_bits > ( ctx->bits_avail - ctx->bits_used ) )
		{
			if( ctx->bits_avail - ctx->bits_used )
			{
				partial_bits = ctx->bits_avail - ctx->bits_used;
				if( partial_bits > sizeof( partial_tmp ) * 8 )
					partial_bits = sizeof( partial_tmp ) * 8;
				if( ctx->use_version2 )
					partial_tmp = MY_GET_MASK( ctx->entropy2, ctx->bits_used, partial_bits );
				else
					partial_tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, partial_bits );
			}
			NeedBits( ctx );
			bits -= partial_bits;
		}
		else
		{
			if( ctx->use_version2 )
				tmp = MY_GET_MASK( ctx->entropy2, ctx->bits_used, get_bits );
			else
				tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, get_bits );
			ctx->bits_used += get_bits;
			if( partial_bits )
			{
				tmp |= partial_tmp << get_bits;
			}
			(*buffer++) = tmp;
			bits -= get_bits;
		}
	} while( bits );
}

S_32 SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed )
{
	S_32 result;
	SRG_GetEntropyBuffer( ctx, (_32*)&result, bits );
	if( get_signed )
		if( result & ( 1 << ( bits - 1 ) ) )
		{
			_32 negone = ~0;
			negone <<= bits;
			return (S_32)( result | negone );
		}
	return result;
}

void SRG_ResetEntropy( struct random_context *ctx )
{
	if( ctx->use_version2 )
		sha512_init( &ctx->sha512 );
	else
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
#if 0  
// if standalone?
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved
						 )
{
	return TRUE;
}
#endif
// this is the watcom deadstart entry point.
// by supplying this routine, then the native runtime doesn't get pulled
// and no external clbr symbols are required.
//void __DLLstart( void )
//{
//}
#endif
