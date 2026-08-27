/* KangarooTwelve (K12) - self contained implementation.  See k12.h.
 *
 * Derived from the reference implementation by Ronny Van Keer
 * (https://keccak.team/) and produces identical output; the multi-lane
 * (SIMD) leaf paths and the external Keccak code package are dropped in
 * favour of one portable Keccak-p[1600,12] permutation.
 *
 * To the extent possible under law, the implementer has waived all copyright
 * and related or neighboring rights to the source code in this file.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <string.h>
#include "k12.h"

#define K12_SUFFIX_LEAF   0x0B /* '110': message hop, simple padding, inner node */
#define K12_SUFFIX_SINGLE 0x07 /* '11' : message hop, final node (no leaves)     */
#define K12_SUFFIX_TREE   0x06 /* '01' : chaining hop, final node                */

#define K12_ROTL64( x, y ) ( ( (uint64_t)( x ) << ( y ) ) | ( (uint64_t)( x ) >> ( 64 - ( y ) ) ) )

/* the state lanes are stored little endian; on a big endian host they are
   swapped in and out around the permutation. */
#if defined( __BIG_ENDIAN__ )                                                        \
    || ( defined( __BYTE_ORDER__ ) && defined( __ORDER_BIG_ENDIAN__ )                \
         && ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ) )                             \
    || ( defined( __BYTE_ORDER ) && defined( __BIG_ENDIAN )                          \
         && ( __BYTE_ORDER == __BIG_ENDIAN ) )
#  define K12_BIG_ENDIAN 1
#endif

/* Keccak-f[1600] round constants; K12 uses the last 12 of the 24 rounds. */
static const uint64_t k12_rndc[24] = {
	0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
	0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
	0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
	0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
	0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
	0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
	0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
	0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

/* rho offsets, in the rho/pi lane walk order */
static const unsigned k12_rotc[24] = {
	 1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
	27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

/* pi lane walk: lane index visited at each step */
static const unsigned k12_piln[24] = {
	10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
	15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
};

#define K12_ROUNDS 12

static void k12_permute( KeccakWidth1600_12rounds_SpongeInstance *sponge ) {
	uint64_t *st = sponge->state.q;
	uint64_t  bc[5], t;
	unsigned  r, i, j;
#ifdef K12_BIG_ENDIAN
	for( i = 0; i < 25; i++ ) {
		uint8_t *p = sponge->state.b + i * 8;
		st[i] = (uint64_t)p[0] | ( (uint64_t)p[1] << 8 ) | ( (uint64_t)p[2] << 16 ) | ( (uint64_t)p[3] << 24 )
		      | ( (uint64_t)p[4] << 32 ) | ( (uint64_t)p[5] << 40 ) | ( (uint64_t)p[6] << 48 ) | ( (uint64_t)p[7] << 56 );
	}
#endif
	for( r = 24 - K12_ROUNDS; r < 24; r++ ) {
		/* theta */
		for( i = 0; i < 5; i++ )
			bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
		for( i = 0; i < 5; i++ ) {
			t = bc[( i + 4 ) % 5] ^ K12_ROTL64( bc[( i + 1 ) % 5], 1 );
			for( j = 0; j < 25; j += 5 )
				st[j + i] ^= t;
		}
		/* rho and pi */
		t = st[1];
		for( i = 0; i < 24; i++ ) {
			j     = k12_piln[i];
			bc[0] = st[j];
			st[j] = K12_ROTL64( t, k12_rotc[i] );
			t     = bc[0];
		}
		/* chi */
		for( j = 0; j < 25; j += 5 ) {
			for( i = 0; i < 5; i++ )
				bc[i] = st[j + i];
			for( i = 0; i < 5; i++ )
				st[j + i] ^= ( ~bc[( i + 1 ) % 5] ) & bc[( i + 2 ) % 5];
		}
		/* iota */
		st[0] ^= k12_rndc[r];
	}
#ifdef K12_BIG_ENDIAN
	for( i = 0; i < 25; i++ ) {
		uint8_t *p = sponge->state.b + i * 8;
		uint64_t v = st[i];
		p[0] = (uint8_t)v;         p[1] = (uint8_t)( v >> 8 );
		p[2] = (uint8_t)( v >> 16 ); p[3] = (uint8_t)( v >> 24 );
		p[4] = (uint8_t)( v >> 32 ); p[5] = (uint8_t)( v >> 40 );
		p[6] = (uint8_t)( v >> 48 ); p[7] = (uint8_t)( v >> 56 );
	}
#endif
}

/* ---------------------------------------------------------------- */
/* sponge, rate fixed at K12_RATE_BYTES                             */

static void K12_SpongeInitialize( KeccakWidth1600_12rounds_SpongeInstance *sponge ) {
	memset( sponge->state.b, 0, sizeof( sponge->state.b ) );
	sponge->byteIOIndex = 0;
	sponge->squeezing   = 0;
}

static int K12_SpongeAbsorb( KeccakWidth1600_12rounds_SpongeInstance *sponge, const unsigned char *data, size_t dataByteLen ) {
	if( sponge->squeezing )
		return 1; /* too late for additional input */

	while( dataByteLen ) {
		size_t   take = K12_RATE_BYTES - sponge->byteIOIndex;
		unsigned n;
		if( take > dataByteLen )
			take = dataByteLen;
		for( n = 0; n < take; n++ )
			sponge->state.b[sponge->byteIOIndex + n] ^= data[n];
		sponge->byteIOIndex += (unsigned)take;
		data += take;
		dataByteLen -= take;
		if( sponge->byteIOIndex == K12_RATE_BYTES ) {
			k12_permute( sponge );
			sponge->byteIOIndex = 0;
		}
	}
	return 0;
}

static int K12_SpongeAbsorbLastFewBits( KeccakWidth1600_12rounds_SpongeInstance *sponge, unsigned char delimitedData ) {
	if( delimitedData == 0 )
		return 1;
	if( sponge->squeezing )
		return 1; /* too late for additional input */

	/* last few bits, whose delimiter coincides with first bit of padding */
	sponge->state.b[sponge->byteIOIndex] ^= delimitedData;
	/* if the first bit of padding is at position rate-1, a whole new block is
	   needed for the second bit of padding */
	if( ( delimitedData >= 0x80 ) && ( sponge->byteIOIndex == ( K12_RATE_BYTES - 1 ) ) )
		k12_permute( sponge );
	/* second bit of padding */
	sponge->state.b[K12_RATE_BYTES - 1] ^= 0x80;
	k12_permute( sponge );
	sponge->byteIOIndex = 0;
	sponge->squeezing   = 1;
	return 0;
}

static int K12_SpongeSqueeze( KeccakWidth1600_12rounds_SpongeInstance *sponge, unsigned char *data, size_t dataByteLen ) {
	if( !sponge->squeezing )
		K12_SpongeAbsorbLastFewBits( sponge, 0x01 );

	while( dataByteLen ) {
		size_t take;
		if( sponge->byteIOIndex == K12_RATE_BYTES ) {
			k12_permute( sponge );
			sponge->byteIOIndex = 0;
		}
		take = K12_RATE_BYTES - sponge->byteIOIndex;
		if( take > dataByteLen )
			take = dataByteLen;
		memcpy( data, sponge->state.b + sponge->byteIOIndex, take );
		sponge->byteIOIndex += (unsigned)take;
		data += take;
		dataByteLen -= take;
	}
	return 0;
}

/* ---------------------------------------------------------------- */

static unsigned int k12_right_encode( unsigned char *encbuf, size_t value ) {
	unsigned int n, i;
	size_t       v;

	for( v = value, n = 0; v && ( n < sizeof( size_t ) ); ++n, v >>= 8 )
		; /* count significant bytes */
	for( i = 1; i <= n; ++i )
		encbuf[i - 1] = (unsigned char)( value >> ( 8 * ( n - i ) ) );
	encbuf[n] = (unsigned char)n;
	return n + 1;
}

int KangarooTwelve_Initialize( KangarooTwelve_Instance *ktInstance, size_t outputByteLen ) {
	ktInstance->fixedOutputLength = outputByteLen;
	ktInstance->queueAbsorbedLen  = 0;
	ktInstance->blockNumber       = 0;
	ktInstance->phase             = ABSORBING;
	K12_SpongeInitialize( &ktInstance->finalNode );
	K12_SpongeInitialize( &ktInstance->queueNode );
	return 0;
}

int KangarooTwelve_Update( KangarooTwelve_Instance *ktInstance, const unsigned char *input, size_t inLen ) {
	if( ktInstance->phase != ABSORBING )
		return 1;

	if( ktInstance->blockNumber == 0 ) {
		/* first chunk, absorbed directly in the final (root) node */
		unsigned int len = (unsigned int)( ( inLen < ( K12_CHUNK_SIZE - ktInstance->queueAbsorbedLen ) )
		                                       ? inLen
		                                       : ( K12_CHUNK_SIZE - ktInstance->queueAbsorbedLen ) );
		if( K12_SpongeAbsorb( &ktInstance->finalNode, input, len ) != 0 )
			return 1;
		input += len;
		inLen -= len;
		ktInstance->queueAbsorbedLen += len;
		if( ( ktInstance->queueAbsorbedLen == K12_CHUNK_SIZE ) && ( inLen != 0 ) ) {
			/* first chunk complete and more input available, finalize it */
			const unsigned char padding = 0x03; /* '110^6': message hop, simple padding */
			ktInstance->queueAbsorbedLen = 0;
			ktInstance->blockNumber      = 1;
			if( K12_SpongeAbsorb( &ktInstance->finalNode, &padding, 1 ) != 0 )
				return 1;
			/* zero padding up to 64 bits */
			ktInstance->finalNode.byteIOIndex = ( ktInstance->finalNode.byteIOIndex + 7 ) & ~7u;
		}
	} else if( ktInstance->queueAbsorbedLen != 0 ) {
		/* there is data in the queue node, fill it up to a complete chunk */
		unsigned int len = (unsigned int)( ( inLen < ( K12_CHUNK_SIZE - ktInstance->queueAbsorbedLen ) )
		                                       ? inLen
		                                       : ( K12_CHUNK_SIZE - ktInstance->queueAbsorbedLen ) );
		if( K12_SpongeAbsorb( &ktInstance->queueNode, input, len ) != 0 )
			return 1;
		input += len;
		inLen -= len;
		ktInstance->queueAbsorbedLen += len;
		if( ktInstance->queueAbsorbedLen == K12_CHUNK_SIZE ) {
			unsigned char intermediate[K12_CAPACITY_BYTES];
			ktInstance->queueAbsorbedLen = 0;
			++ktInstance->blockNumber;
			if( K12_SpongeAbsorbLastFewBits( &ktInstance->queueNode, K12_SUFFIX_LEAF ) != 0 )
				return 1;
			if( K12_SpongeSqueeze( &ktInstance->queueNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
			if( K12_SpongeAbsorb( &ktInstance->finalNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
		}
	}

	while( inLen > 0 ) {
		unsigned int len = (unsigned int)( ( inLen < K12_CHUNK_SIZE ) ? inLen : K12_CHUNK_SIZE );
		K12_SpongeInitialize( &ktInstance->queueNode );
		if( K12_SpongeAbsorb( &ktInstance->queueNode, input, len ) != 0 )
			return 1;
		input += len;
		inLen -= len;
		if( len == K12_CHUNK_SIZE ) {
			unsigned char intermediate[K12_CAPACITY_BYTES];
			++ktInstance->blockNumber;
			if( K12_SpongeAbsorbLastFewBits( &ktInstance->queueNode, K12_SUFFIX_LEAF ) != 0 )
				return 1;
			if( K12_SpongeSqueeze( &ktInstance->queueNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
			if( K12_SpongeAbsorb( &ktInstance->finalNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
		} else
			ktInstance->queueAbsorbedLen = len;
	}

	return 0;
}

int KangarooTwelve_Final( KangarooTwelve_Instance *ktInstance, unsigned char *output, const unsigned char *customization, size_t customLen ) {
	unsigned char encbuf[sizeof( size_t ) + 1 + 2];
	unsigned char padding;

	if( ktInstance->phase != ABSORBING )
		return 1;

	/* absorb customization | k12_right_encode(customLen) */
	if( ( customLen != 0 ) && ( KangarooTwelve_Update( ktInstance, customization, customLen ) != 0 ) )
		return 1;
	if( KangarooTwelve_Update( ktInstance, encbuf, k12_right_encode( encbuf, customLen ) ) != 0 )
		return 1;

	if( ktInstance->blockNumber == 0 ) {
		/* incomplete first chunk in the final node, pad it */
		padding = K12_SUFFIX_SINGLE;
	} else {
		unsigned int n;

		if( ktInstance->queueAbsorbedLen != 0 ) {
			/* there is data in the queue node */
			unsigned char intermediate[K12_CAPACITY_BYTES];
			++ktInstance->blockNumber;
			if( K12_SpongeAbsorbLastFewBits( &ktInstance->queueNode, K12_SUFFIX_LEAF ) != 0 )
				return 1;
			if( K12_SpongeSqueeze( &ktInstance->queueNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
			if( K12_SpongeAbsorb( &ktInstance->finalNode, intermediate, K12_CAPACITY_BYTES ) != 0 )
				return 1;
		}
		/* absorb k12_right_encode(number of chaining values) || 0xFF || 0xFF */
		--ktInstance->blockNumber;
		n          = k12_right_encode( encbuf, ktInstance->blockNumber );
		encbuf[n++] = 0xFF;
		encbuf[n++] = 0xFF;
		if( K12_SpongeAbsorb( &ktInstance->finalNode, encbuf, n ) != 0 )
			return 1;
		padding = K12_SUFFIX_TREE;
	}
	if( K12_SpongeAbsorbLastFewBits( &ktInstance->finalNode, padding ) != 0 )
		return 1;
	if( ktInstance->fixedOutputLength != 0 ) {
		ktInstance->phase = FINAL;
		return K12_SpongeSqueeze( &ktInstance->finalNode, output, ktInstance->fixedOutputLength );
	}
	ktInstance->phase = SQUEEZING;
	return 0;
}

int KangarooTwelve_Squeeze( KangarooTwelve_Instance *ktInstance, unsigned char *output, size_t outputByteLen ) {
	if( ktInstance->phase != SQUEEZING )
		return 1;
	return K12_SpongeSqueeze( &ktInstance->finalNode, output, outputByteLen );
}

int KangarooTwelve( const unsigned char *input, size_t inLen, unsigned char *output, size_t outLen, const unsigned char *customization, size_t customLen ) {
	KangarooTwelve_Instance ktInstance;

	if( outLen == 0 )
		return 1;
	if( KangarooTwelve_Initialize( &ktInstance, outLen ) != 0 )
		return 1;
	if( KangarooTwelve_Update( &ktInstance, input, inLen ) != 0 )
		return 1;
	return KangarooTwelve_Final( &ktInstance, output, customization, customLen );
}
