/**
 *	@file    prf.c
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	"Native" Pseudo Random Function.
 */
/*
 *	Copyright (c) 2013-2016 INSIDE Secure Corporation
 *	Copyright (c) PeerSec Networks, 2002-2011
 *	All Rights Reserved
 *
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software
 *	into proprietary programs.  If you are unable to comply with the GPL, a
 *	commercial license for this software may be purchased from INSIDE at
 *	http://www.insidesecure.com/
 *
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */
/******************************************************************************/

#include "matrixsslApi.h"

#if defined(USE_TLS_PRF) || defined(USE_TLS_PRF2)

#ifdef USE_TLS_PRF
int32_t prf(const unsigned char *sec, uint16_t secLen,
			const unsigned char *seed, uint16_t seedLen,
			unsigned char *out, uint16_t outLen)
{
	return psPrf(sec, secLen, seed, seedLen, out, outLen);
}
#endif

#ifdef USE_TLS_PRF2
int32_t prf2(const unsigned char *sec, uint16_t secLen,
			 const unsigned char *seed, uint16_t seedLen,
			 unsigned char *out, uint16_t outLen, uint32_t flags)
{
	return psPrf2(sec, secLen, seed, seedLen, out, outLen,
				  (flags & CRYPTO_FLAGS_SHA3) ?
				  SHA384_HASH_SIZE : SHA256_HASH_SIZE);
}
#endif

#else

#ifdef USE_TLS
#ifndef USE_ONLY_TLS_1_2
/******************************************************************************/
/*
	MD5 portions of the prf
*/
__inline static int32_t pMd5(const unsigned char *key, uint16_t keyLen,
				const unsigned char *text, uint16_t textLen,
				unsigned char *out, uint16_t outLen)
{
	psHmacMd5_t		ctx;
	unsigned char	a[MD5_HASH_SIZE];
	unsigned char	mac[MD5_HASH_SIZE];
	unsigned char	hmacKey[MD5_HASH_SIZE];
	int32_t			rc = PS_FAIL;
	uint16_t		hmacKeyLen, i, keyIter;

	for (keyIter = 1; (uint16_t)(MD5_HASH_SIZE * keyIter) < outLen;) {
		keyIter++;
	}
	if ((rc = psHmacMd5(key, keyLen, text, textLen, a,
			hmacKey, &hmacKeyLen)) < 0) {
		goto L_RETURN;
	}
	if (hmacKeyLen != keyLen) {
/*
		Support for keys larger than 64 bytes.  Must take the hash of
		the original key in these cases which is indicated by different
		outgoing values from the passed in key and keyLen values
*/
		psAssert(keyLen > 64);
		/* Typecast is OK, we don't update key below */
		key = (const unsigned char *)hmacKey;
		keyLen = hmacKeyLen;
	}
	for (i = 0; i < keyIter; i++) {
		if ((rc = psHmacMd5Init(&ctx, key, keyLen)) < 0) {
			goto L_RETURN;
		}
		psHmacMd5Update(&ctx, a, MD5_HASH_SIZE);
		psHmacMd5Update(&ctx, text, textLen);
		psHmacMd5Final(&ctx, mac);
		if (i == keyIter - 1) {
			memcpy(out + (MD5_HASH_SIZE*i), mac, outLen - (MD5_HASH_SIZE*i));
		} else {
			memcpy(out + (MD5_HASH_SIZE * i), mac, MD5_HASH_SIZE);
			if ((rc = psHmacMd5(key, keyLen, a, MD5_HASH_SIZE, a,
					hmacKey, &hmacKeyLen)) < 0) {
				goto L_RETURN;
			}
		}
	}
	rc = PS_SUCCESS;
L_RETURN:
	memzero_s(a, MD5_HASH_SIZE);
	memzero_s(mac, MD5_HASH_SIZE);
	memzero_s(hmacKey, MD5_HASH_SIZE);
	if (rc < 0) {
		memzero_s(out, outLen);	/* zero any partial result on error */
	}
	return rc;
}

/******************************************************************************/
/*
	SHA1 portion of the prf
*/
__inline static int32_t pSha1(const unsigned char *key, uint16_t keyLen,
					const unsigned char *text, uint16_t textLen,
					unsigned char *out, uint16_t outLen)
{
	psHmacSha1_t	ctx;
	unsigned char	a[SHA1_HASH_SIZE];
	unsigned char	mac[SHA1_HASH_SIZE];
	unsigned char	hmacKey[SHA1_HASH_SIZE];
	int32_t			rc = PS_FAIL;
	uint16_t		hmacKeyLen, i, keyIter;

	for (keyIter = 1; (uint16_t)(SHA1_HASH_SIZE * keyIter) < outLen;) {
		keyIter++;
	}
	if ((rc = psHmacSha1(key, keyLen, text, textLen, a,
			hmacKey, &hmacKeyLen)) < 0) {
		goto L_RETURN;
	}
	if (hmacKeyLen != keyLen) {
/*
		Support for keys larger than 64 bytes.  Must take the hash of
		the original key in these cases which is indicated by different
		outgoing values from the passed in key and keyLen values
*/
		psAssert(keyLen > 64);
		/* Typecast is OK, we don't update key below */
		key = (const unsigned char *)hmacKey;
		keyLen = hmacKeyLen;
	}
	for (i = 0; i < keyIter; i++) {
		if ((rc = psHmacSha1Init(&ctx, key, keyLen)) < 0) {
			goto L_RETURN;
		}
		psHmacSha1Update(&ctx, a, SHA1_HASH_SIZE);
		psHmacSha1Update(&ctx, text, textLen);
		psHmacSha1Final(&ctx, mac);
		if (i == keyIter - 1) {
			memcpy(out + (SHA1_HASH_SIZE * i), mac,
				outLen - (SHA1_HASH_SIZE * i));
		} else {
			memcpy(out + (SHA1_HASH_SIZE * i), mac, SHA1_HASH_SIZE);
			if ((rc = psHmacSha1(key, keyLen, a, SHA1_HASH_SIZE, a,
					hmacKey, &hmacKeyLen)) < 0) {
				goto L_RETURN;
			}
		}
	}
	rc = PS_SUCCESS;
L_RETURN:
	memzero_s(a, SHA1_HASH_SIZE);
	memzero_s(mac, SHA1_HASH_SIZE);
	memzero_s(hmacKey, SHA1_HASH_SIZE);
	if (rc < 0) {
		memzero_s(out, outLen);	/* zero any partial result on error */
	}
	return rc;
}

/******************************************************************************/
/*
	Psuedo-random function.  TLS uses this for key generation and hashing
*/
int32_t prf(const unsigned char *sec, uint16_t secLen,
				const unsigned char *seed, uint16_t seedLen,
				unsigned char *out, uint16_t outLen)
{
	const unsigned char	*s1, *s2;
	unsigned char	md5out[SSL_MAX_KEY_BLOCK_SIZE];
	unsigned char	sha1out[SSL_MAX_KEY_BLOCK_SIZE];
	int32_t			rc = PS_FAIL;
	uint16_t		sLen, i;

	psAssert(outLen <= SSL_MAX_KEY_BLOCK_SIZE);

	sLen = (secLen / 2) + (secLen % 2);
	s1 = sec;
	s2 = (sec + sLen) - (secLen % 2);
	if ((rc = pMd5(s1, sLen, seed, seedLen, md5out, outLen)) < 0) {
		goto L_RETURN;
	}
	if ((rc = pSha1(s2, sLen, seed, seedLen, sha1out, outLen)) < 0) {
		goto L_RETURN;
	}
	for (i = 0; i < outLen; i++) {
		out[i] = md5out[i] ^ sha1out[i];
	}
	rc = outLen;
L_RETURN:
	memzero_s(md5out, SSL_MAX_KEY_BLOCK_SIZE);
	memzero_s(sha1out, SSL_MAX_KEY_BLOCK_SIZE);
	return rc;
}

#endif /* !USE_ONLY_TLS_1_2 */

#ifdef USE_TLS_1_2
/******************************************************************************/
/*
	SHA2 prf
*/
__inline static int32_t pSha2(const unsigned char *key, uint16_t keyLen,
					const unsigned char *text, uint16_t textLen,
					unsigned char *out, uint16_t outLen, uint32_t flags)
{
	/* Use a union to save a bit of stack space */
	union {
#ifdef USE_SHA384
		psHmacSha384_t		sha384;
#endif
		psHmacSha256_t		sha256;
	} u;
	unsigned char		a[SHA384_HASH_SIZE];
	unsigned char		mac[SHA384_HASH_SIZE];
	unsigned char		hmacKey[SHA384_HASH_SIZE];
	int32_t				rc = PS_FAIL;
	uint16_t			hashSize, hmacKeyLen, i, keyIter;

#ifdef USE_SHA384
	if (flags & CRYPTO_FLAGS_SHA3) {
		hashSize = SHA384_HASH_SIZE;
		if ((rc = psHmacSha384(key, keyLen, text, textLen, a,
				hmacKey, &hmacKeyLen)) < 0) {
			goto L_RETURN;
		}
	} else
#endif
	{

		hashSize = SHA256_HASH_SIZE;
		if ((rc = psHmacSha256(key, keyLen, text, textLen, a,
				hmacKey, &hmacKeyLen)) < 0) {
			goto L_RETURN;
		}
	}
	for (keyIter = 1; (uint16_t)(hashSize * keyIter) < outLen;) {
		keyIter++;
	}
	if (hmacKeyLen != keyLen) {
/*
		Support for keys larger than 64 bytes.  Must take the hash of
		the original key in these cases which is indicated by different
		outgoing values from the passed in key and keyLen values
*/
		psAssert(keyLen > 64);
		/* Typecast is OK, we don't update key below */
		key = (const unsigned char *)hmacKey;
		keyLen = hmacKeyLen;
	}
	for (i = 0; i < keyIter; i++) {
#ifdef USE_SHA384
		if (flags & CRYPTO_FLAGS_SHA3) {
			if ((rc = psHmacSha384Init(&u.sha384, key, keyLen)) < 0) {
				goto L_RETURN;
			}
			psHmacSha384Update(&u.sha384, a, hashSize);
			psHmacSha384Update(&u.sha384, text, textLen);
			psHmacSha384Final(&u.sha384, mac);
		} else
#endif
		{
			if ((rc = psHmacSha256Init(&u.sha256, key, keyLen)) < 0) {
				goto L_RETURN;
			}
			psHmacSha256Update(&u.sha256, a, hashSize);
			psHmacSha256Update(&u.sha256, text, textLen);
			psHmacSha256Final(&u.sha256, mac);
		}
		if (i == keyIter - 1) {
			memcpy(out + (hashSize * i), mac,
				outLen - (hashSize * i));
		} else {
			memcpy(out + (hashSize * i), mac, hashSize);
#ifdef USE_SHA384
			if (flags & CRYPTO_FLAGS_SHA3) {
				if ((rc = psHmacSha384(key, keyLen, a, hashSize, a,
						hmacKey, &hmacKeyLen)) < 0) {
					goto L_RETURN;
				}
			} else
#endif
			{
				if ((rc = psHmacSha256(key, keyLen, a, hashSize, a,
						hmacKey, &hmacKeyLen)) < 0) {
					goto L_RETURN;
				}
			}
		}
	}
	rc =  PS_SUCCESS;
L_RETURN:
	memzero_s(a, SHA384_HASH_SIZE);
	memzero_s(mac, SHA384_HASH_SIZE);
	memzero_s(hmacKey, SHA384_HASH_SIZE);
	if (rc < 0) {
		memzero_s(out, outLen);	/* zero any partial result on error */
	}
	return rc;
}

/******************************************************************************/
/*
	Psuedo-random function.  TLS uses this for key generation and hashing
*/
int32_t prf2(const unsigned char *sec, uint16_t secLen,
				const unsigned char *seed, uint16_t seedLen,
				unsigned char *out, uint16_t outLen, uint32_t flags)
{
	unsigned char	sha2out[SSL_MAX_KEY_BLOCK_SIZE];
	int32_t			rc;
	uint16_t		i;

	psAssert(outLen <= SSL_MAX_KEY_BLOCK_SIZE);

	if ((rc = pSha2(sec, secLen, seed, seedLen, sha2out, outLen, flags)) < 0) {
		return rc;
	}
	/* Copy out of tmp buffer because outLen typically less than multiple of
		prf block size */
	for (i = 0; i < outLen; i++) {
		out[i] = sha2out[i];
	}
	memzero_s(sha2out, SSL_MAX_KEY_BLOCK_SIZE);
	return outLen;
}
#endif /* USE_TLS_1_2 */

#endif /* USE_TLS */
#endif /* USE_TLS_PRF || USE_TLS_PRF2 */
/******************************************************************************/

