#include <stdhdrs.h>

#ifndef SALTY_RANDOM_GENERATOR_SOURCE
#define SALTY_RANDOM_GENERATOR_SOURCE
#endif
#include <salty_generator.h>
#include "srg_internal.h"

#define USE_K12_LONG_SQUEEZE 1
#define K12_SQUEEZE_LENGTH   32768

void NeedBits( struct random_context *ctx )
{
	if( ctx->use_versionK12 ) {
#if USE_K12_LONG_SQUEEZE
		if( ctx->f.K12i.phase == ABSORBING || ctx->total_bits_used > K12_SQUEEZE_LENGTH ) {
			if( ctx->f.K12i.phase == SQUEEZING ) {
				KangarooTwelve_Initialize( &ctx->f.K12i, 0 );
				KangarooTwelve_Update( &ctx->f.K12i, ctx->s.entropy4, K12_DIGEST_SIZE );
			}
			if( ctx->getsalt ) {
				ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
				KangarooTwelve_Update( &ctx->f.K12i, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
			}
			KangarooTwelve_Final( &ctx->f.K12i, NULL, NULL, 0 ); // customization is a final pad string.
			ctx->total_bits_used = 0;
		}
		if( ctx->f.K12i.phase == SQUEEZING )
			KangarooTwelve_Squeeze( &ctx->f.K12i, ctx->s.entropy4, K12_DIGEST_SIZE ); // customization is a final pad string.
#else
		if( ctx->getsalt ) {
			ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
			KangarooTwelve_Update( &ctx->f.K12i, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
		}
		KangarooTwelve_Final( &ctx->f.K12i, ctx->s.entropy4, NULL, 0 ); // customization is a final pad string.
		KangarooTwelve_Initialize( &ctx->f.K12i, K12_DIGEST_SIZE );
		KangarooTwelve_Update( &ctx->f.K12i, ctx->s.entropy4, K12_DIGEST_SIZE );
#endif
		ctx->bits_avail = sizeof( ctx->s.entropy4 ) * CHAR_BIT;
		ctx->entropy = ctx->s.entropy4;
	}
	else {
		if( ctx->getsalt )
			ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
		else
			ctx->salt_size = 0;
		if( ctx->use_version3 ) {
			if( ctx->salt_size )
				sha3_update( &ctx->f.sha3, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
			sha3_final( &ctx->f.sha3, ctx->s.entropy3 );
			sha3_init( &ctx->f.sha3, SHA3_DIGEST_SIZE );
			sha3_update( &ctx->f.sha3, ctx->s.entropy3, SHA3_DIGEST_SIZE );
			ctx->bits_avail = sizeof( ctx->s.entropy3 ) * CHAR_BIT;
			ctx->entropy = ctx->s.entropy3;
		}
		else if( ctx->use_version2_256 ) {
			if( ctx->salt_size )
				sha256_update( &ctx->f.sha256, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
			sha256_final( &ctx->f.sha256, ctx->s.entropy2_256 );
			sha256_init( &ctx->f.sha256 );
			sha256_update( &ctx->f.sha256, ctx->s.entropy2_256, SHA256_DIGEST_SIZE );
			ctx->bits_avail = sizeof( ctx->s.entropy2_256 ) * CHAR_BIT;
			ctx->entropy = ctx->s.entropy2_256;
		}
		else if( ctx->use_version2 ) {
			if( ctx->salt_size )
				sha512_update( &ctx->f.sha512, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
			sha512_final( &ctx->f.sha512, ctx->s.entropy2 );
			sha512_init( &ctx->f.sha512 );
			sha512_update( &ctx->f.sha512, ctx->s.entropy2, SHA512_DIGEST_SIZE );
			ctx->bits_avail = sizeof( ctx->s.entropy2 ) * CHAR_BIT;
			ctx->entropy = ctx->s.entropy2;
		}
		else {
			if( ctx->salt_size )
				SHA1Input( &ctx->f.sha1_ctx, (const uint8_t*)ctx->salt, ctx->salt_size );
			SHA1Result( &ctx->f.sha1_ctx, ctx->s.entropy0 );
			SHA1Reset( &ctx->f.sha1_ctx );
			SHA1Input( &ctx->f.sha1_ctx, ctx->s.entropy0, SHA1HashSize );
			ctx->bits_avail = sizeof( ctx->s.entropy0 ) * CHAR_BIT;
			ctx->entropy = ctx->s.entropy0;
		}
	}
	ctx->bits_used = 0;
}

struct random_context *SRG_CreateEntropyInternal( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user
                                                , LOGICAL version2 
                                                , LOGICAL version2_256
                                                , LOGICAL version3
                                                , LOGICAL versionk12
                                                )
{
	struct random_context *ctx = New( struct random_context );
	ctx->use_versionK12 = versionk12;
	ctx->use_version3 = version3;
	ctx->use_version2_256 = version2_256;
	ctx->use_version2 = version2;
	if( ctx->use_versionK12 )
		KangarooTwelve_Initialize( &ctx->f.K12i, USE_K12_LONG_SQUEEZE ?0: K12_DIGEST_SIZE );
	if( ctx->use_version3 )
		sha3_init( &ctx->f.sha3, SHA3_DIGEST_SIZE );
	else if( ctx->use_version2_256 )
		sha256_init( &ctx->f.sha256 );
	else if( ctx->use_version2 )
		sha512_init( &ctx->f.sha512 );
	else
		SHA1Reset( &ctx->f.sha1_ctx );
	ctx->getsalt = getsalt;
	ctx->psv_user = psv_user;
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
	return ctx;
}

struct random_context *SRG_CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, FALSE, FALSE, FALSE );
}

struct random_context *SRG_CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, TRUE, FALSE, FALSE, FALSE );
}

struct random_context *SRG_CreateEntropy2_256( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, TRUE, FALSE, FALSE );
}

struct random_context *SRG_CreateEntropy3( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, FALSE, TRUE, FALSE );
}

struct random_context *SRG_CreateEntropy4( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, FALSE, FALSE, TRUE );
}

void SRG_DestroyEntropy( struct random_context **ppEntropy )
{
	Release( (*ppEntropy) );
	(*ppEntropy) = NULL;
}

uint32_t SRG_GetBit( struct random_context *ctx )
{
	uint32_t tmp;
	if( !ctx ) DebugBreak();
	ctx->total_bits_used += 1;
	//if( ctx->bits_used > 512 ) DebugBreak();
	if( (ctx->bits_used) >= ctx->bits_avail ) {
		NeedBits( ctx );
	}
	tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, 1 );
	ctx->bits_used += 1;
	return tmp;
}


void SRG_GetEntropyBuffer( struct random_context *ctx, uint32_t *buffer, uint32_t bits )
{
	uint32_t tmp;
	uint32_t partial_tmp;
	uint32_t partial_bits = 0;
	uint32_t get_bits;
	uint32_t resultBits = 0;
	if( !ctx ) DebugBreak();
	ctx->total_bits_used += bits;
	//if( ctx->bits_used > 512 ) DebugBreak();
	do {
		if( bits > sizeof( tmp ) * 8 )
			get_bits = sizeof( tmp ) * 8;
		else
			get_bits = bits;

		// if there were 1-31 bits of data in partial, then can only get 32-partial max.
		if( 32 < (get_bits + partial_bits) )
			get_bits = 32 - partial_bits;
		// check1 :
		//    if get_bits == 32
		//    but bits_used is 1-7, then it would have to pull 5 bytes to get the 32 required
		//    so truncate get_bits to 25-31 bits
		if( 32 < (get_bits + (ctx->bits_used & 0x7)) )
			get_bits = (32 - (ctx->bits_used & 0x7));
		// if resultBits is 1-7 offset, then would have to store up to 5 bytes of value
		//    so have to truncate to just the up to 4 bytes that will fit.
		if( (get_bits+ resultBits) > 32 )
			get_bits = 32 - resultBits;
		// only greater... if equal just grab the bits.
		if( (get_bits + ctx->bits_used) > ctx->bits_avail ) {
			// if there are any bits left, grab the partial bits.
			if( ctx->bits_avail > ctx->bits_used ) {
				partial_bits = (uint32_t)(ctx->bits_avail - ctx->bits_used);
				if( partial_bits > get_bits ) partial_bits = get_bits;
				// partial can never be greater than 32; input is only max of 32
				//if( partial_bits > (sizeof( partial_tmp ) * 8) )
				//	partial_bits = (sizeof( partial_tmp ) * 8);
				partial_tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, partial_bits );
			}
			NeedBits( ctx );
			bits -= partial_bits;
		}
		else {
			tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, get_bits );
			ctx->bits_used += get_bits;
			//if( ctx->bits_used > 512 ) DebugBreak();
			if( partial_bits ) {
				tmp = partial_tmp | (tmp << partial_bits);
				partial_bits = 0;
			}
			if( (get_bits+resultBits) > 24 )
				(*buffer) = tmp << resultBits;
			else if( (get_bits+resultBits) > 16 ) {
				(*((uint16_t*)buffer)) |= tmp << resultBits;
				(*(((uint8_t*)buffer)+2)) |= ((tmp << resultBits) & 0xFF0000)>>16;
			} else if( (get_bits+resultBits) > 8 )
				(*((uint16_t*)buffer)) |= tmp << resultBits;
			else
				(*((uint8_t*)buffer)) |= tmp << resultBits;
			resultBits += get_bits;
			while( resultBits >= 8 ) {
#if defined( __cplusplus ) || defined( __GNUC__ )
				buffer = (uint32_t*)(((uintptr_t)buffer) + 1);
#else
				((intptr_t)buffer)++;
#endif
				resultBits -= 8;
			}
			//if( get_bits > bits ) DebugBreak();
			bits -= get_bits;
		}
	} while( bits );
}

int32_t SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed )
{
	int32_t result = 0;
	SRG_GetEntropyBuffer( ctx, (uint32_t*)&result, bits );
	if( get_signed )
		if( result & ( 1 << ( bits - 1 ) ) )
		{
			uint32_t negone = ~0;
			negone <<= bits;
			return (int32_t)( result | negone );
		}
	return result;
}

void SRG_ResetEntropy( struct random_context *ctx )
{
	ctx->total_bits_used = 0;
	if( ctx->use_versionK12 )
		KangarooTwelve_Initialize( &ctx->f.K12i, USE_K12_LONG_SQUEEZE ? 0:K12_DIGEST_SIZE  );
	else if( ctx->use_version3 )
		sha3_init( &ctx->f.sha3, SHA3_DIGEST_SIZE );
	else if( ctx->use_version2_256 )
		sha256_init( &ctx->f.sha256 );
	else if( ctx->use_version2 )
		sha512_init( &ctx->f.sha512 );
	else
		SHA1Reset( &ctx->f.sha1_ctx );
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
}

void SRG_StreamEntropy( struct random_context *ctx )
{
	if( ctx->use_versionK12 )
		KangarooTwelve_Update( &ctx->f.K12i, ctx->s.entropy4, K12_DIGEST_SIZE );
	else if( ctx->use_version3 )
		sha3_update( &ctx->f.sha3, ctx->s.entropy4, SHA3_DIGEST_SIZE );
	else if( ctx->use_version2_256 )
		sha256_update( &ctx->f.sha256, ctx->s.entropy2_256, SHA256_DIGEST_SIZE );
	else if( ctx->use_version2 )
		sha512_update( &ctx->f.sha512, ctx->s.entropy2, SHA512_DIGEST_SIZE );
	else
		SHA1Input( &ctx->f.sha1_ctx, ctx->s.entropy0, SHA1HashSize );
}

void SRG_FeedEntropy( struct random_context *ctx, const uint8_t *salt, size_t salt_size )
{
	if( ctx->use_versionK12 )
		KangarooTwelve_Update( &ctx->f.K12i, salt, (unsigned int)salt_size );
	else if( ctx->use_version3 )
		sha3_update( &ctx->f.sha3, salt, (unsigned int)salt_size );
	else if( ctx->use_version2_256 )
		sha256_update( &ctx->f.sha256, salt, (unsigned int)salt_size );
	else if( ctx->use_version2 )
		sha512_update( &ctx->f.sha512, salt, (unsigned int)salt_size );
	else
		SHA1Input( &ctx->f.sha1_ctx, salt, salt_size );
}

void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder, size_t *dataSize )
{
	if( !(*external_buffer_holder) )
		(*external_buffer_holder) = New( struct random_context );
	(*(struct random_context*)(*external_buffer_holder)) = (*ctx);
	if( dataSize )
		(*dataSize) = sizeof( struct random_context );
}

void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder )
{
	(*ctx) = *(struct random_context*)external_buffer_holder;
}

static void salt_generator(uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	static struct tickBuffer {
		uint32_t tick;
		uint64_t cputick;
	} tick;
	(void)psv;
	tick.cputick = GetCPUTick();
	tick.tick = GetTickCount();
	salt[0] = &tick;
	salt_size[0] = sizeof( tick );
}

#define SRG_MAX_GENERATOR_THREADS 32

static struct random_context *getGenerator( 
			struct random_context *pool[SRG_MAX_GENERATOR_THREADS]
			, uint32_t used[SRG_MAX_GENERATOR_THREADS]
			, struct random_context * (*generator)(void( *)(uintptr_t , POINTER *, size_t *), uintptr_t)
			, int *pUsingCtx
		) 
{
	struct random_context *ctx;
	int usingCtx;
	usingCtx = 0;
	do {
		while( used[++usingCtx] ) { if( ++usingCtx >= SRG_MAX_GENERATOR_THREADS ) usingCtx = 0; }
	} while( LockedExchange( used + usingCtx, 1 ) );
	ctx = pool[usingCtx];
	if( !ctx ) ctx = pool[usingCtx] = generator( salt_generator, 0 );
	(*pUsingCtx) = usingCtx;
	return ctx;
}

char *SRG_ID_Generator( void ) {
	struct random_context *ctx;
	uint32_t buf[(16 + 16) / 4];
	size_t outlen;
	static struct random_context *_ctx[SRG_MAX_GENERATOR_THREADS];
	static uint32_t used[SRG_MAX_GENERATOR_THREADS];
	int usingCtx;

	ctx = getGenerator( _ctx, used, SRG_CreateEntropy2, &usingCtx );
	do {
		SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	} while( ( buf[0] & 0x3f ) < 10 );
	used[usingCtx] = 0;
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}

char *SRG_ID_Generator_256( void ) {
	struct random_context *ctx;
	uint32_t buf[(16 + 16) / 4];
	size_t outlen;

	static struct random_context *_ctx[SRG_MAX_GENERATOR_THREADS];
	static uint32_t used[SRG_MAX_GENERATOR_THREADS];
	int usingCtx;
	ctx = getGenerator( _ctx, used, SRG_CreateEntropy2_256, &usingCtx );

	do {
		SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	} while( (buf[0] & 0x3f) < 10 );
	used[usingCtx] = 0;
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}

char *SRG_ID_Generator3( void ) {
	struct random_context *ctx;
	uint32_t buf[(16 + 16) / 4];
	size_t outlen;

	static struct random_context *_ctx[SRG_MAX_GENERATOR_THREADS];
	static uint32_t used[SRG_MAX_GENERATOR_THREADS];
	int usingCtx;
	usingCtx = 0;
	ctx = getGenerator( _ctx, used, SRG_CreateEntropy3, &usingCtx );

	do {
		SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	} while( (buf[0] & 0x3f) < 10 );
	used[usingCtx] = 0;
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}

char *SRG_ID_Generator4( void ) {
	struct random_context *ctx;
	uint32_t buf[(16 + 16)/4];
	size_t outlen;

	static struct random_context *_ctx[SRG_MAX_GENERATOR_THREADS];
	static uint32_t used[SRG_MAX_GENERATOR_THREADS];
	int usingCtx;
	usingCtx = 0;
	ctx = getGenerator( _ctx, used, SRG_CreateEntropy4, &usingCtx );

	do {
		SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	} while( (buf[0] & 0x3f) < 10 );
	used[usingCtx] = 0;
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}
