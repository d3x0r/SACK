
#include <stdhdrs.h>
#include <configscript.h>
#ifndef SALTY_RANDOM_GENERATOR_SOURCE
#define SALTY_RANDOM_GENERATOR_SOURCE
#endif
#include "salty_generator.h"

static struct crypt_local
{
	char * use_salt;
	struct random_context *entropy;
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
