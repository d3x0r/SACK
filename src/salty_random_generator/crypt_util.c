
#include <stdhdrs.h>
#include "salty_generator.h"


static struct crypt_local
{
	char * use_salt;
	struct random_context *entropy;

} *crypt_local;
#define l (*crypt_local)

static void FeedSalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
	if( l..use_salt)
	{
		(*salt) = l..use_salt;
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


void SRG_DecryptData( CTEXTSTR local_password, P_8 *buffer, size_t *chars )
{
	if( !l.entropy )
		l.entropy = SRG_CreateEntropy( FeedSalt, (PTRSZVAL)0 );
	{
		POINTER binary;
		size_t length;
		if( local_password && DecodeBinaryConfig( local_password, &binary, length ) )
		{
			_32 mask;
			P_8 pass_byte_in;
			P_8 pass_byte_out;
			int index;
			//if( length < chars )
			{
				SRG_ResetEntropy( l.entropy );
				l.use_salt = (char *)binary;

				mask = SRG_GetEntropy( l.entropy, 32, FALSE );
				pass_byte_in = ((P_8)binary) + 4;
				length -= 4;
            (*buffer) = NewArray( _8, length );
				pass_byte_out = (*buffer);
				for( index = 0; length; length--, index++ )
				{
					pass_byte_out[0] = pass_byte_in[0] ^ ((P_8)&mask)[ index & 0x3 ];
					pass_byte_out++;
					pass_byte_in++;
				}
            (*chars) = pass_byte_out - buffer;
			}
		}
		else
		{
			buffer[0] = 0;
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


TEXTCHAR * SRG_EncryptData( P_8 buffer, size_t buflen )
{
	if( !l.entropy )
		l.entropy = SRG_CreateEntropy( FeedSalt, 0 );
	{
		TEXTCHAR * result_buf;
		char tmpbuf[256];
		{
			_32 mask;
			_32 seed;
			P_8 pass_byte_in;
			P_8 pass_byte_out;
			int index;
         P_8 tmpbuf;
			l.use_salt = NULL;
         tmpbuf = NewArray( _8, buflen + 4 );
			SRG_ResetEntropy( l.entropy );
			seed = timeGetTime() & 0xFFFF;
			//seed = seed % 100;
			tmpbuf[0] = '0' + (seed >> 12) & 0xF;
			tmpbuf[1] = '0' + (seed >> 8) & 0xF;
			tmpbuf[2] = '0' + (seed >> 4) & 0xF;
			tmpbuf[3] = '0' + (seed >> 0) & 0xF;
			l.use_salt = tmpbuf;

			SRG_ResetEntropy( l.entropy );
			mask = SRG_GetEntropy( l.entropy, 32, FALSE );
			pass_byte_in = ((P_8)buffer);
			pass_byte_out = (P_8)tmpbuf + 4;
			for( index = 0; pass_byte_in[0]; index++ )
			{
				pass_byte_out[0] = pass_byte_in[0] ^ ((P_8)&mask)[ index & 0x3 ];
				pass_byte_out++;
				pass_byte_in++;
			}

			EncodeBinaryConfig( &result_buf, (P_8)tmpbuf, buflen + 4 );
#ifdef UNICODE
			{
				char *result = CStrDup( result_buf );
				Release( result_buf );
				return result;
			}
#else
			return result_buf;
#endif
		}
	}
   return NULL;
}

TEXTSTR SRG_EncryptString( CTEXTSTR buffer )
{
   return SRG_EncryptData( (P_8)buffer, StrLen( buffer ) + 1 );
}
