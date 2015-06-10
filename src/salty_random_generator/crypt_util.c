
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

static void FeedSalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
	if( crypt_local.use_salt)
	{
		(*salt) = crypt_local.use_salt;
		(*salt_size) = 4;
	}
	else
	{
		static _32 tick;
		tick = timeGetTime();
		(*salt) = &tick;
		(*salt_size) = 4;
	}
}

void SRG_DecryptRawData( CPOINTER binary, size_t length, P_8 *buffer, size_t *chars )
{
	if( !crypt_local.entropy )
		crypt_local.entropy = SRG_CreateEntropy( FeedSalt, (PTRSZVAL)0 );
	{
		_32 mask;
		P_8 pass_byte_in;
		P_8 pass_byte_out;
		int index;
		//if( length < chars )
		{
			SRG_ResetEntropy( crypt_local.entropy );
			crypt_local.use_salt = (char *)binary;

			pass_byte_in = ((P_8)binary) + 4;
			length -= 4;
			(*buffer) = NewArray( _8, length );
			pass_byte_out = (*buffer);
			for( index = 0; length; length--, index++ )
			{
				if( ( index & 3 ) == 0 )
					mask = SRG_GetEntropy( crypt_local.entropy, 32, FALSE );

				pass_byte_out[0] = pass_byte_in[0] ^ ((P_8)&mask)[ index & 0x3 ];
				pass_byte_out++;
				pass_byte_in++;
			}
			(*chars) = pass_byte_out - (*buffer);
		}
	}
}

void SRG_DecryptData( CTEXTSTR local_password, P_8 *buffer, size_t *chars )
{
	{
		POINTER binary;
		size_t length;
		if( local_password && DecodeBinaryConfig( local_password, &binary, &length ) )
		{
			SRG_DecryptRawData( (P_8)binary, length, buffer, chars );
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
	P_8 buffer;
	size_t chars;
	SRG_DecryptData( local_password, &buffer, &chars );
	return (TEXTSTR)buffer;
}

void SRG_EncryptRawData( CPOINTER buffer, size_t buflen, P_8 *result_buf, size_t *result_size )
{
	if( !crypt_local.entropy )
		crypt_local.entropy = SRG_CreateEntropy( FeedSalt, 0 );
	{
		{
			_32 mask;
			_32 seed;
			P_8 pass_byte_in;
			P_8 pass_byte_out;
			int index;
			P_8 tmpbuf;
			crypt_local.use_salt = NULL;
			(*result_buf) = tmpbuf = NewArray( _8, buflen + 4 );
			(*result_size) = buflen + 4;
			SRG_ResetEntropy( crypt_local.entropy );
			seed = (_32)GetCPUTick();
			tmpbuf[0] = ((seed >> 17) & 0xFF) ^ ((seed >> 8) & 0xFF);
			tmpbuf[1] = ((seed >> 11) & 0xFF) ^ ((seed >> 4) & 0xFF);
			tmpbuf[2] = ((seed >> 5) & 0xFF) ^ ((seed >> 12) & 0xFF);
			tmpbuf[3] = ((seed >> 0) & 0xFF) ^ ((seed >> 13) & 0xFF);

			crypt_local.use_salt = (char*)tmpbuf;

			SRG_ResetEntropy( crypt_local.entropy );
			pass_byte_in = ((P_8)buffer);
			pass_byte_out = (P_8)tmpbuf + 4;
			for( index = 0; buflen; buflen--, index++ )
			{
				if( ( index & 3 ) == 0 )
					mask = SRG_GetEntropy( crypt_local.entropy, 32, FALSE );
				pass_byte_out[0] = pass_byte_in[0] ^ ((P_8)&mask)[ index & 0x3 ];
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
		P_8 result_buf;
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
	return SRG_EncryptData( (P_8)buffer, StrLen( buffer ) + 1 );
}
