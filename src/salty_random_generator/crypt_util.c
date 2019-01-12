
#include <stdhdrs.h>
#include <deadstart.h>
#include <configscript.h>
#ifndef SALTY_RANDOM_GENERATOR_SOURCE
#define SALTY_RANDOM_GENERATOR_SOURCE
#endif
#include <salty_generator.h>
#include "srg_internal.h"

static struct crypt_local
{
	char * use_salt;
	struct random_context *entropy;
	PLINKQUEUE plqCrypters;
} crypt_local;

static void FeedSalt( uintptr_t psv, POINTER *salt, size_t *salt_size )
{
	if( crypt_local.use_salt)
	{
		(*salt) = crypt_local.use_salt;
		(*salt_size) = 4;
	}
	else
	{
		static uint32_t tick;
		tick = timeGetTime();
		(*salt) = &tick;
		(*salt_size) = 4;
	}
}

void SRG_DecryptRawData( CPOINTER binary, size_t length, uint8_t* *buffer, size_t *chars )
{
	if( !crypt_local.entropy )
		crypt_local.entropy = SRG_CreateEntropy( FeedSalt, (uintptr_t)0 );
	{
		uint32_t mask;
		uint8_t* pass_byte_in;
		uint8_t* pass_byte_out;
		int index;
		//if( length < chars )
		{
			SRG_ResetEntropy( crypt_local.entropy );
			crypt_local.use_salt = (char *)binary;

			pass_byte_in = ((uint8_t*)binary) + 4;
			length -= 4;
			(*buffer) = NewArray( uint8_t, length );
			pass_byte_out = (*buffer);
			for( index = 0; length; length--, index++ )
			{
				if( ( index & 3 ) == 0 )
					mask = SRG_GetEntropy( crypt_local.entropy, 32, FALSE );

				pass_byte_out[0] = pass_byte_in[0] ^ ((uint8_t*)&mask)[ index & 0x3 ];
				pass_byte_out++;
				pass_byte_in++;
			}
			(*chars) = pass_byte_out - (*buffer);
		}
	}
}

void SRG_DecryptData( CTEXTSTR local_password, uint8_t* *buffer, size_t *chars )
{
	{
		POINTER binary;
		size_t length;
		if( local_password && DecodeBinaryConfig( local_password, &binary, &length ) )
		{
			SRG_DecryptRawData( (uint8_t*)binary, length, buffer, chars );
		}
		else
		{
			(*buffer) = 0;
			(*chars) = 0;
			//lprintf( WIDE("failed to decode data") );
		}
	}
}

TEXTSTR SRG_DecryptString( CTEXTSTR local_password )
{
	uint8_t* buffer;
	size_t chars;
	SRG_DecryptData( local_password, &buffer, &chars );
	return (TEXTSTR)buffer;
}

void SRG_EncryptRawData( CPOINTER buffer, size_t buflen, uint8_t* *result_buf, size_t *result_size )
{
	if( !crypt_local.entropy )
		crypt_local.entropy = SRG_CreateEntropy( FeedSalt, 0 );
	{
		{
			uint32_t mask;
			uint32_t seed;
			uint8_t* pass_byte_in;
			uint8_t* pass_byte_out;
			int index;
			uint8_t* tmpbuf;
			crypt_local.use_salt = NULL;
			(*result_buf) = tmpbuf = NewArray( uint8_t, buflen + 4 );
			(*result_size) = buflen + 4;
			SRG_ResetEntropy( crypt_local.entropy );
			seed = (uint32_t)GetCPUTick();
			tmpbuf[0] = ((seed >> 17) & 0xFF) ^ ((seed >> 8) & 0xFF);
			tmpbuf[1] = ((seed >> 11) & 0xFF) ^ ((seed >> 4) & 0xFF);
			tmpbuf[2] = ((seed >> 5) & 0xFF) ^ ((seed >> 12) & 0xFF);
			tmpbuf[3] = ((seed >> 0) & 0xFF) ^ ((seed >> 13) & 0xFF);

			crypt_local.use_salt = (char*)tmpbuf;

			SRG_ResetEntropy( crypt_local.entropy );
			pass_byte_in = ((uint8_t*)buffer);
			pass_byte_out = (uint8_t*)tmpbuf + 4;
			for( index = 0; buflen; buflen--, index++ )
			{
				if( ( index & 3 ) == 0 )
					mask = SRG_GetEntropy( crypt_local.entropy, 32, FALSE );
				pass_byte_out[0] = pass_byte_in[0] ^ ((uint8_t*)&mask)[ index & 0x3 ];
				pass_byte_out++;
				pass_byte_in++;
			}
		}
	}
}

TEXTCHAR * SRG_EncryptData( CPOINTER buffer, size_t buflen )
{
	if( !crypt_local.entropy )
		crypt_local.entropy = SRG_CreateEntropy( FeedSalt, 0 );
	{
		uint8_t* result_buf;
		size_t result_size;
		TEXTSTR tmpbuf;
		SRG_EncryptRawData( buffer, buflen, &result_buf, &result_size );

		EncodeBinaryConfig( &tmpbuf, result_buf, buflen + 4 );
		return tmpbuf;
	}
	return NULL;
}

TEXTSTR SRG_EncryptString( CTEXTSTR buffer )
{
	return SRG_EncryptData( (uint8_t*)buffer, StrLen( buffer ) + 1 );
}

#ifndef NO_SSL
#  include <openssl/evp.h>
#  include <openssl/err.h>

static void handleErrors( void )
{
	ERR_print_errors_fp( stderr );
	abort();
}

size_t SRG_AES_encrypt( uint8_t *plaintext, size_t plaintext_len, uint8_t *key, uint8_t **ciphertext )
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;
	/* Create and initialise the context */
	if( !(ctx = EVP_CIPHER_CTX_new()) ) handleErrors();

	/* Initialise the encryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if( 1 != EVP_EncryptInit_ex( ctx, EVP_aes_256_cbc(), NULL, key, key ) )
		handleErrors();
	EVP_CIPHER_CTX_set_padding( ctx, 0 );
	int blockSize = EVP_CIPHER_CTX_block_size( ctx );
	if( blockSize < 16 ) DebugBreak();
	int outSize = (int)(plaintext_len + sizeof( uint32_t ) + (blockSize - 1));
	uint8_t *block = NewArray( uint8_t, blockSize );
	outSize -= outSize % blockSize;
	ciphertext[0] = NewArray( uint8_t, outSize );

	((uint32_t*)block)[0] = (uint32_t)plaintext_len;
	int remaining = blockSize - sizeof( uint32_t );
	if( remaining > plaintext_len ) {
		memcpy( block + sizeof( uint32_t ), plaintext, plaintext_len );
		remaining = (int)(plaintext_len + sizeof( uint32_t ));
		plaintext_len = 0;
	}
	else {
		memcpy( block + sizeof( uint32_t ), plaintext, blockSize - sizeof( uint32_t ) );
		remaining = blockSize;
		plaintext_len -= (blockSize - sizeof( uint32_t ));
	}
	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if( 1 != EVP_EncryptUpdate( ctx, ciphertext[0], &len, (const unsigned char*)block, remaining ) )
		handleErrors();
	ciphertext_len = len;
	Release( block );

	if( plaintext_len > 0 ) {
		if( plaintext_len % blockSize ) {
			int tailLen = plaintext_len % blockSize;
			if( 1 != EVP_EncryptUpdate( ctx, ciphertext[0] + ciphertext_len, &len
				, plaintext + (blockSize - sizeof( uint32_t ))
				, (int)(plaintext_len - tailLen) ) )
				handleErrors();
			ciphertext_len += len;
			memcpy( block
				, plaintext + (blockSize - sizeof( uint32_t )) + plaintext_len - tailLen
				, tailLen);
			memset( block + tailLen, 0, blockSize - tailLen );
			if( 1 != EVP_EncryptUpdate( ctx, ciphertext[0] + ciphertext_len, &len
				, block
				, blockSize ) )
				handleErrors();
		}
		else {
			if( 1 != EVP_EncryptUpdate( ctx, ciphertext[0] + ciphertext_len, &len
				, plaintext + (blockSize - sizeof( uint32_t ))
				, (int)plaintext_len ) )
				handleErrors();
		}
		ciphertext_len += len;
	}

	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	len = 0;
	if( 1 != EVP_EncryptFinal_ex( ctx, ciphertext[0] + ciphertext_len, &len ) ) handleErrors();
	ciphertext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free( ctx );

	return ciphertext_len;
}

int SRG_AES_decrypt( uint8_t *ciphertext, int ciphertext_len, uint8_t *key, uint8_t **plaintext )
{
	EVP_CIPHER_CTX *ctx;

	int len;
	int used = 0;
	int plaintext_len;

	/* Create and initialise the context */
	if( !(ctx = EVP_CIPHER_CTX_new()) ) handleErrors();

	/* Initialise the decryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if( 1 != EVP_DecryptInit_ex( ctx, EVP_aes_256_cbc(), NULL, key, key ) )
		handleErrors();
	EVP_CIPHER_CTX_set_padding( ctx, 0 );

	int blockSize = EVP_CIPHER_CTX_block_size( ctx );
	uint8_t *block = NewArray( uint8_t, blockSize * 2 );
	// read the first block of 1 block size.  This has the length so we know 
	// how much more to read.
	if( 1 != EVP_DecryptUpdate( ctx, block, &len, ciphertext, blockSize ) )
		handleErrors();
	used += blockSize;
	if( !len ) {
		if( 1 != EVP_DecryptUpdate( ctx, block, &len, ciphertext + used, blockSize ) )
			handleErrors();
		used += blockSize;
		if( !len ) {
			lprintf( "Really? Give me the first block!" );
			DebugBreak();
		}
	}
	plaintext_len = ((uint32_t*)block)[0];

	int outSize = (plaintext_len + (blockSize - 1)); // have to accept over-writes from crypt
	outSize -= outSize % blockSize;

	plaintext[0] = NewArray( uint8_t, outSize );
	memcpy( plaintext[0], block + sizeof( uint32_t ), blockSize - sizeof( uint32_t ) );
	if( ciphertext_len > blockSize ) {
		/* Provide the message to be decrypted, and obtain the plaintext output.
		 * EVP_DecryptUpdate can be called multiple times if necessary
		 */
		if( 1 != EVP_DecryptUpdate( ctx
			, plaintext[0] + (blockSize - sizeof( uint32_t )), &len
			, ciphertext + used
			, ciphertext_len - used ) )
			handleErrors();
		//plaintext_len = len;
	}

	/* Finalise the decryption. Further plaintext bytes may be written at
	 * this stage.
	 */
	if( 1 != EVP_DecryptFinal_ex( ctx, plaintext[0] + plaintext_len, &len ) ) handleErrors();
	plaintext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free( ctx );

	Release( block );

	return plaintext_len;
}
#endif


// bit size of masking hash.
#define RNGHASH 256


static void encryptBlock( struct byte_shuffle_key *bytKey
	, uint8_t *output, size_t outlen 
	, uint8_t bufKey[RNGHASH/8]
) {
	uint8_t *curBuf_out;
	size_t n;

	curBuf_out = output;
#if __64__
	for( n = 0; n < outlen; n += 8, curBuf_out += 8 ) {
		((uint64_t*)curBuf_out)[0] ^= /*((uint64_t*)curBuf_in)[0] ^*/ ((uint64_t*)(bufKey + (n % (RNGHASH / 8))))[0];;
	}
#else
	for( n = 0; n < outlen; n += 4, curBuf_out += 4 ) {
		((uint32_t*)curBuf_out)[0] ^= /* ((uint32_t*)curBuf_in)[0] ^ */ ((uint32_t*)(bufKey + (n % (RNGHASH / 8))))[0];
	}
#endif
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );
	curBuf_out = output;
	uint8_t p = 0x55;
	for( n = 0; n < outlen; n++, curBuf_out++ ) {
		//p = curBuf_out[0] = BlockShuffle_Sub1Byte_( bytKey, curBuf_out[0] ^ p );
		p = curBuf_out[0] = curBuf_out[0] ^ p;
	}
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );
	curBuf_out--;
	p = 0xAA;
	for( n = 0; n < outlen; n++, curBuf_out-- ) {
		p = curBuf_out[0] = curBuf_out[0] ^ p;
	}
	BlockShuffle_SubBytes_( bytKey, output, output, outlen );

}

void SRG_XSWS_encryptData( uint8_t *objBuf, size_t objBufLen
	, uint64_t tick, uint8_t *keyBuf, size_t keyBufLen
	, uint8_t **outBuf, size_t *outBufLen
) {
	struct random_context *signEntropy = (struct random_context *)DequeLink( &crypt_local.plqCrypters );
	if( !signEntropy )
		signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );
	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)&tick, sizeof( tick ) );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)keyBuf, keyBufLen );
	static uint8_t bufKey[RNGHASH /8];
	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)bufKey, RNGHASH );
	struct byte_shuffle_key *bytKey = BlockShuffle_ByteShuffler( signEntropy );

	(*outBufLen) = (sizeof( uint8_t ))
		+ objBufLen
		+ (((objBufLen + sizeof( uint8_t )) & 0x7)
			? (8 - ((objBufLen + sizeof( uint8_t )) & 0x7))
			: 0);

	//outBuf[0] = (uint8_t*)HeapAllocateAligned( NULL, (*outBufLen), 4096 );
	outBuf[0] = (uint8_t*)HeapAllocate( NULL, (*outBufLen) );
	((uint64_t*)(outBuf[0] + (*outBufLen) - 8))[0] = 0; // clear any padding bits.
	memcpy( outBuf[0], objBuf, objBufLen );  // copy contents for in-place encrypt.
	((uint8_t*)(outBuf[0] + (*outBufLen) - 1))[0] = (uint8_t)(*outBufLen - objBufLen);

	for( size_t b = 0; b < (*outBufLen); b += 4096 ) {
		size_t bs = (*outBufLen) - b;
		if( bs > 4096 )
			encryptBlock( bytKey, outBuf[0] + b, 4096, bufKey );
		else
			encryptBlock( bytKey, outBuf[0] + b, bs, bufKey );
	}

	BlockShuffle_DropByteShuffler( bytKey );
	EnqueLink( &crypt_local.plqCrypters, signEntropy );
}

static void decryptBlock( struct byte_shuffle_key *bytKey
	, uint8_t *input, size_t len
	, uint8_t *output
	, uint8_t bufKey[RNGHASH / 8]
	, LOGICAL lastBLock
) {
	int n;
	BlockShuffle_BusBytes_( bytKey, input, output, len );

	uint8_t *curBuf = output;
	for( n = 0; n < (len - 1); n++, curBuf++ ) {
		curBuf[0] = curBuf[0] ^ curBuf[1];
	}
	curBuf[0] = curBuf[0] ^ 0xAA;

	BlockShuffle_BusBytes_( bytKey, output, output, len );
	curBuf = output + len - 1;
	for( n = (int)(len - 1); n > 0; n--, curBuf-- ) {
		//curBuf[0] = BlockShuffle_Bus1Byte_( bytKey, curBuf[0] ) ^ curBuf[-1];
		curBuf[0] = curBuf[0] ^ curBuf[-1];
	}
	//curBuf[0] = BlockShuffle_Bus1Byte_( bytKey, curBuf[0] ) ^ 0x55;
	curBuf[0] = curBuf[0] ^ 0x55;

	BlockShuffle_BusBytes_( bytKey, output, output, len );
#if __64__
	for( n = 0; n < len; n += 8, output += 8 ) {
		((uint64_t*)output)[0] ^= ((uint64_t*)(bufKey + (n % (RNGHASH / 8))))[0];
	}
#else
	for( n = 0; n < len; n += 4, output += 4 ) {
		((uint32_t*)output)[0] ^= ((uint32_t*)(bufKey + (n % (RNGHASH / 8))))[0];
	}
#endif

}

void SRG_XSWS_decryptData( uint8_t *objBuf, size_t objBufLen
	, uint64_t tick, uint8_t *keyBuf, size_t keyBufLen
	, uint8_t **outBuf, size_t *outBufLen
) {
	
	struct random_context *signEntropy = (struct random_context *)DequeLink( &crypt_local.plqCrypters );
	if( !signEntropy )
		signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );
	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)&tick, sizeof( tick ) );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)keyBuf, keyBufLen );

	static uint8_t bufKey[RNGHASH /8];
	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)bufKey, RNGHASH );
	struct byte_shuffle_key *bytKey = BlockShuffle_ByteShuffler( signEntropy );

	outBuf[0] = NewArray( uint8_t, (*outBufLen) = objBufLen );

	for( size_t b = 0; b < objBufLen; b += 4096 ) {
		size_t bs = objBufLen - b;
		if( bs > 4096 )
			decryptBlock( bytKey, objBuf + b, 4096, outBuf[0] + b, bufKey, 0 );
		else
			decryptBlock( bytKey, objBuf + b, bs, outBuf[0] + b, bufKey, 1 );
	}
	(*outBufLen) -= ((uint8_t*)(outBuf[0] + objBufLen - 1))[0];

	BlockShuffle_DropByteShuffler( bytKey );

	EnqueLink( &crypt_local.plqCrypters, signEntropy );
}


#if 0
// internal test code...
// some performance benchmarking for instance.

void logBinary( uint8_t *inbuf, int len ) {
#define BINBUFSIZE 280
#define LINELEN 64
	char buf[280];
	int ofs;
	for( int i = 0; i < 32; i++ ) {
		int j;
		ofs = 0;
		for( j = 0; j < 64; j++ ) {
			if( (i * 64 + j) >= len ) break;
			ofs += snprintf( buf + ofs, BINBUFSIZE - ofs, "%02x ", inbuf[i * LINELEN + j] );

		}
		for( ; j < 64; j++ ) {
			ofs += snprintf( buf + ofs, BINBUFSIZE - ofs, "   " );
		}
		ofs += snprintf( buf + ofs, BINBUFSIZE - ofs, "   " );
		for( int j = 0; j < LINELEN; j++ ) {
			if( (i * 64 + j) >= len ) break;
			ofs += snprintf( buf + ofs, BINBUFSIZE - ofs, "%c", (inbuf[i * LINELEN + j] >= 32 && inbuf[i * LINELEN + j] <= 127) ? inbuf[i * LINELEN + j] : '.' );
		}
		puts( buf );
		if( (i * LINELEN + j) >= len ) break;
	}
}

PRELOAD( CryptTestBuiltIn ) {
	// this sample happened to be 44 bytes + 4 for the length = 48 = 3*16
	// happened to be a perfect pad.
	// with padding (libressl) padds a while extra block.
	static char message[] = "Hello, This is a test, this is Only a test.";
	static char messageBig[2048] = "Hello, This is a test, this is Only a test.";
	static char messageMega[2048 * 2048] = "Hello, This is a test, this is Only a test.";
	// this is a 1 bit change in message from message
	static char message2[] = "Hello, This is a test, this is only a test.";
	// this is a slightly shorter message, which needs padding 
	// (manual pad test to avoid a full 16 byte 0 pad block)
	static char message3[] = "Hello, This is a test, this is the test.";

	static uint8_t key[] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
						   , 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
						   , 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
						   , 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
	};
	uint8_t *output;
	size_t outlen;
	uint8_t *orig;
	size_t origlen;

#define DO_PERF_TESTS 
#define LENGTH_RECOVERY_TESTING

#ifdef LENGTH_RECOVERY_TESTING

	for( int p = 0; p < 10; p++ ) {
		printf( "TESTDATA  %d\n", p );
		logBinary( (uint8_t*)message, sizeof( message ) );
		SRG_XSWS_encryptData( (uint8_t*)message, sizeof( message ) - p, 1234, key, sizeof( key ), &output, &outlen );

		puts( "BINARY" );
		logBinary( output, outlen );
		SRG_XSWS_decryptData( (uint8_t*)output, outlen, 1234, key, sizeof( key ), &orig, &origlen );

		puts( "ORIG" );
		logBinary( orig, origlen );
		Release( output );
		Release( orig );
	}

	SRG_XSWS_encryptData( (uint8_t*)message2, sizeof( message2 ), 1234, key, sizeof( key ), &output, &outlen );
	puts( "BINARY - 1 bit change input" );
	logBinary( output, outlen );

	Release( output );

#endif

#ifdef DO_PERF_TESTS
	uint32_t start, end;
	int i;
	start = timeGetTime();
	for( i = 0; i < 900000; i++ ) {
		SRG_XSWS_encryptData( (uint8_t*)message, sizeof( message ), 1234, key, sizeof( key ), &output, &outlen );
		Release( output );
	}
	end = timeGetTime();
	printf( "Tiny DID %d in %d   %d %d\n", i, end - start, i * 1000 / (end - start)
		, (i * 1000 / (end - start)) * sizeof( message )
	);
	Sleep( 1000 );

	start = timeGetTime();
	for( i = 0; i < 300000; i++ ) {
		SRG_XSWS_encryptData( (uint8_t*)messageBig, sizeof( messageBig ), 1234, key, sizeof( key ), &output, &outlen );
		Release( output );
	}
	end = timeGetTime();
	printf( "Big DID %d in %d   %d %d\n", i, end - start, i * 1000 / (end - start)
		, (i * 1000 / (end - start))*sizeof( messageBig ) 
	);
	Sleep( 1000 );

	start = timeGetTime();
	for( i = 0; i < 300; i++ ) {
		SRG_XSWS_encryptData( (uint8_t*)messageMega, sizeof( messageMega ), 1234, key, sizeof( key ), &output, &outlen );
		Release( output );
	}
	end = timeGetTime();
	printf( "Mega DID %d in %d   %d %d\n", i, end - start, i * 1000 / (end - start) 
		, (i * 1000 / (end - start)) * sizeof( messageMega )
	);
	Sleep( 1000 );
#endif

#ifndef NO_SSL
#  ifdef DO_PERF_TESTS

	// SRG_AES_encrypt and SRG_AES_decrypt are symmetric.
	start = timeGetTime();
	for( i = 0; i < 300000; i++ ) {
		SRG_XSWS_decryptData( (uint8_t*)message, sizeof( message ), 1234, key, sizeof( key ), &output, &outlen );
		Release( output );
	}
	end = timeGetTime();
	printf( "DID %d in %d   %d\n", i, end - start, i * 1000 / (end - start) );
	Sleep( 1000 );
#  endif
	puts( "TESTDATA" );
	logBinary( (uint8_t*)message, sizeof( message ) );
	outlen = SRG_AES_encrypt( (uint8_t*)message, sizeof( message ), key, &output );

	puts( "BINARY" );
	logBinary( output, outlen );
	origlen = SRG_AES_decrypt( output, outlen, key, &orig );
	puts( "ORIG" );
	logBinary( orig, origlen );
	Release( output );
	Release( orig );

	puts( "TESTDATA" );
	logBinary( (uint8_t*)message2, sizeof( message2 ) );
	outlen = SRG_AES_encrypt( (uint8_t*)message2, sizeof( message2 ), key, &output );

	puts( "BINARY" );
	logBinary( output, outlen );
	origlen = SRG_AES_decrypt( output, outlen, key, &orig );
	puts( "ORIG" );
	logBinary( orig, origlen );
	Release( output );
	Release( orig );

	puts( "TESTDATA" );
	logBinary( (uint8_t*)message3, sizeof( message3 ) );
	outlen = SRG_AES_encrypt( (uint8_t*)message3, sizeof( message3 ), key, &output );

	puts( "BINARY" );
	logBinary( output, outlen );
	origlen = SRG_AES_decrypt( output, outlen, key, &orig );
	puts( "ORIG" );
	logBinary( orig, origlen );
	Release( output );
	Release( orig );
#endif

#if 0
	// memory leak tests.... if in 2M tests memory is +0, probably no leaks.
	// is about 5 seconds for these tests each.... 

	start = timeGetTime();
	for( i = 0; i < 4000000; i++ ) {
		outlen = SRG_AES_encrypt( (uint8_t*)message, sizeof( message ), key, &output );
		Release( output );
	}
	end = timeGetTime();
	printf( "tiny DID %d in %d   %d   %d\n", i, end - start, i * 1000 / (end - start)
		, (i * 1000 / (end - start)) * sizeof( message )
	);

	start = timeGetTime();
	for( i = 0; i < 200000; i++ ) {
		outlen = SRG_AES_encrypt( (uint8_t*)messageBig, sizeof( messageBig ), key, &output );
		Release( output );
	}
	end = timeGetTime();
	printf( "Big DID %d in %d   %d   %d\n", i, end - start, i * 1000 / (end - start) 
		, (i * 1000 / (end - start)) * sizeof( messageBig )
	);

	start = timeGetTime();
	for( i = 0; i < 100; i++ ) {
		outlen = SRG_AES_encrypt( (uint8_t*)messageMega, sizeof( messageMega ), key, &output );
		Release( output );
	}
	end = timeGetTime();
	printf( "Mega DID %d in %d   %d   %d\n", i, end - start, i * 1000 / (end - start) 
		, (i * 1000 / (end - start)) * sizeof( messageMega )
	);
#endif

#if 0

	outlen = SRG_AES_encrypt( (uint8_t*)messageBig, sizeof( messageBig ), key, &output );
	start = timeGetTime();
	for( i = 0; i < 100000; i++ ) {
		origlen = SRG_AES_decrypt( output, outlen, key, &orig );
		Release( orig );
	}
	end = timeGetTime();
	printf( "DID %d in %d   %d\n", i, end - start, i * 1000 / (end - start) );
	Release( output );

	Sleep( 1000 );

	start = timeGetTime();
	for( i = 0; i < 100000; i++ ) {
		outlen = SRG_AES_encrypt( (uint8_t*)message, sizeof( message ), key, &output );
		Release( output );
	}
	end = timeGetTime();
	printf( "DID %d in %d   %d\n", i, end - start, i * 1000 / (end - start) );

	outlen = SRG_AES_encrypt( (uint8_t*)message, sizeof( message ), key, &output );
	start = timeGetTime();
	for( i = 0; i < 100000; i++ ) {
		origlen = SRG_AES_decrypt( output, outlen, key, &orig );
		Release( orig );
	}
	end = timeGetTime();
	printf( "DID %d in %d   %d\n", i, end - start, i * 1000 / (end - start) );
	Release( output );

	Sleep( 1000 );
#endif

}

#endif


