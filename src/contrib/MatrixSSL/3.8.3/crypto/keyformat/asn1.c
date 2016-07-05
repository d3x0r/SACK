/**
 *	@file    asn1.c
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	DER/BER coding.
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

#include "../cryptoApi.h"

/******************************************************************************/
/*
	On success, p will be updated to point to first character of value and
	len will contain number of bytes in value.

	Indefinite length formats return ASN_UNKNOWN_LEN and *len will simply
	be updated with the overall remaining length
*/
int32_t getAsnLength(const unsigned char **pp, uint16_t size, uint16_t *len)
{
	uint32_t	len32;
	int32_t		rc;

	len32 = *len;
	if ((rc = getAsnLength32(pp, size, &len32, 0)) < 0) {
		return rc;
	}
	/* @note len32 is < size here, so it is <= 0xFFFF */
	*len = (uint16_t)len32;
	return PS_SUCCESS;
}

int32_t getAsnLength32(const unsigned char **pp, uint32_t size, uint32_t *len,
				uint32_t indefinite)
{
	const unsigned char		*c, *end;
	uint32_t				l;

	c = *pp;
	end = c + size;
	*len = 0;
	if (end - c < 1) {
		psTraceCrypto("getAsnLength called on empty buffer\n");
		return PS_LIMIT_FAIL;
	}
/*
	If the high bit is set, the lower 7 bits represent the number of
	bytes that follow defining length
	If the high bit is not set, the lower 7 represent the actual length
*/
	l = *c & 0x7F;
	if (*c & 0x80) {
		/* Point c at first length byte */
		c++;
		/* Ensure we have that many bytes left in the buffer.  */
		if (end - c < l) {
			psTraceCrypto("Malformed stream in getAsnLength\n");
			return PS_LIMIT_FAIL;
		}

		switch (l) {
		case 4:
			l = *c << 24; c++;
			l |= *c << 16; c++;
			l |= *c << 8; c++;
			l |= *c; c++;
			break;
		case 3:
			l = *c << 16; c++;
			l |= *c << 8; c++;
			l |= *c; c++;
			break;
		case 2:
			l = *c << 8; c++;
			l |= *c; c++;
			break;
		case 1:
			l = *c; c++;
			break;
/*
		If the length byte has high bit only set, it's an indefinite
		length. If allowed, return the number of bytes remaining in buffer.
*/
		case 0:
			if (indefinite) {
				*pp = c;
				*len = size - 1;
				return ASN_UNKNOWN_LEN;
			}
			return PS_LIMIT_FAIL;

		/* Make sure there aren't more than 4 bytes of length specifier. */
		default:
			psTraceCrypto("Malformed stream in getAsnLength\n");
			return PS_LIMIT_FAIL;
		}
	} else {
		c++;
	}

	/* Stream parsers will not require the entire data to be present */
	if (!indefinite && (end - c < l)) {
		psTraceCrypto("getAsnLength longer than remaining buffer.\n");
		return PS_LIMIT_FAIL;
	}

	*pp = c;
	*len = l;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Callback to extract a sequence length from the DER stream
	Verifies that 'len' bytes are >= 'seqlen'
	Move pp to the first character in the sequence
*/
/* #define DISABLE_STRICT_ASN_LENGTH_CHECK */
int32_t getAsnSequence32(const unsigned char **pp, uint32_t size,
				uint32_t *len, uint32_t indefinite)
{
	const unsigned char	*p = *pp;
	int32_t				rc;

	rc = PS_PARSE_FAIL;
	if (size < 1 || *(p++) != (ASN_SEQUENCE | ASN_CONSTRUCTED) ||
			((rc = getAsnLength32(&p, size - 1, len, indefinite)) < 0)) {
		psTraceCrypto("ASN getSequence failed\n");
		return rc;
	}
#ifndef DISABLE_STRICT_ASN_LENGTH_CHECK
	/* The (p - *pp) is taking the length encoding bytes into account */
	if (!indefinite && (size - ((uint32_t)(p - *pp))) < *len) {
		/* It isn't cool but some encodings have an outer length layer that
			is smaller than the inner.  Normally you'll want this check but if
			you're hitting this case, you could try skipping it to see if there
			is an error in the encoding */
		psTraceCrypto("ASN_SEQUENCE parse found length greater than total\n");
		psTraceCrypto("Could try enabling DISABLE_STRICT_ASN_LENGTH_CHECK\n");
		return PS_LIMIT_FAIL;
	}
#endif
	*pp = p;
	return rc;
}

int32_t getAsnSequence(const unsigned char **pp, uint16_t size, uint16_t *len)
{
	uint32_t	len32;
	int32_t		rc;

	len32 = *len;
	if ((rc = getAsnSequence32(pp, size, &len32, 0)) < 0) {
		return rc;
	}
	/* @note len32 is < size here, so it is <= 0xFFFF */
	*len = (uint16_t)len32;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Extract a set length from the DER stream.  Will also test that there
	is enough data available to hold it all.  Returns LIMIT_FAIL if not.
*/
int32_t getAsnSet32(const unsigned char **pp, uint32_t size, uint32_t *len,
					uint32_t indefinite)
{
	const unsigned char	*p = *pp;
	int32_t				rc;

	rc = PS_PARSE_FAIL;
	if (size < 1 || *(p++) != (ASN_SET | ASN_CONSTRUCTED) ||
			((rc = getAsnLength32(&p, size - 1, len, indefinite)) < 0)) {
		psTraceCrypto("ASN getSet failed\n");
		return rc;
	}
	/* Account for overhead needed to get the length */
	if (size < ((uint32_t)(p - *pp) + *len)) {
		return PS_LIMIT_FAIL;
	}
	*pp = p;
	return rc;
}

int32_t getAsnSet(const unsigned char **pp, uint16_t size, uint16_t *len)
{
	uint32_t	len32;
	int32_t		rc;

	len32 = *len;
	if ((rc = getAsnSet32(pp, size, &len32, 0)) < 0) {
		return rc;
	}
	/* @note len32 is < size here, so it is <= 0xFFFF */
	*len = (uint16_t)len32;
	return PS_SUCCESS;
}
/******************************************************************************/
/*
	Get an enumerated value
*/
int32_t getAsnEnumerated(const unsigned char **pp, uint32_t len, int32_t *val)
{
	const unsigned char	*p = *pp, *end;
	uint32_t			ui, slen;
	int32_t				rc;
	uint32_t			vlen;

	rc = PS_PARSE_FAIL;
	end = p + len;
	if (len < 1 || *(p++) != ASN_ENUMERATED ||
			((rc = getAsnLength32(&p, len - 1, &vlen, 0)) < 0)) {
		psTraceCrypto("ASN getInteger failed from the start\n");
		return rc;
	}
/*
	This check prevents us from having a big positive integer where the
	high bit is set because it will be encoded as 5 bytes (with leading
	blank byte).  If that is required, a getUnsigned routine should be used
*/
	if (vlen > sizeof(int32_t) || (uint32_t)(end - p) < vlen) {
		psTraceCrypto("ASN getInteger had limit failure\n");
		return PS_LIMIT_FAIL;
	}
	ui = 0;
/*
	If high bit is set, it's a negative integer, so perform the two's compliment
	Otherwise do a standard big endian read (most likely case for RSA)
*/
	if (*p & 0x80) {
		while (vlen > 0) {
			ui = (ui << 8) | (*p ^ 0xFF);
			p++;
			vlen--;
		}
		slen = ui;
		slen++;
		slen = -slen;
		*val = slen;
	} else {
		while (vlen > 0) {
			ui = (ui << 8) | *p;
			p++;
			vlen--;
		}
		*val = ui;
	}
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Get an integer
*/
int32_t getAsnInteger(const unsigned char **pp, uint32_t len, int32_t *val)
{
	const unsigned char	*p = *pp, *end;
	uint32_t			ui, slen;
	int32_t				rc;
	uint32_t			vlen;

	rc = PS_PARSE_FAIL;
	end = p + len;
	if (len < 1 || *(p++) != ASN_INTEGER ||
			((rc = getAsnLength32(&p, len - 1, &vlen, 0)) < 0)) {
		psTraceCrypto("ASN getInteger failed from the start\n");
		return rc;
	}
/*
	This check prevents us from having a big positive integer where the
	high bit is set because it will be encoded as 5 bytes (with leading
	blank byte).  If that is required, a getUnsigned routine should be used
*/
	if (vlen > sizeof(int32_t) || (uint32_t)(end - p) < vlen) {
		psTraceCrypto("ASN getInteger had limit failure\n");
		return PS_LIMIT_FAIL;
	}
	ui = 0;
/*
	If high bit is set, it's a negative integer, so perform the two's compliment
	Otherwise do a standard big endian read (most likely case for RSA)
*/
	if (*p & 0x80) {
		while (vlen > 0) {
			ui = (ui << 8) | (*p ^ 0xFF);
			p++;
			vlen--;
		}
		slen = ui;
		slen++;
		slen = -slen;
		*val = slen;
	} else {
		while (vlen > 0) {
			ui = (ui << 8) | *p;
			p++;
			vlen--;
		}
		*val = ui;
	}
	*pp = p;
	return PS_SUCCESS;
}


/******************************************************************************/
/*
	Implementation specific OID parser
*/
int32_t getAsnAlgorithmIdentifier(const unsigned char **pp, uint32_t len,
				int32_t *oi, uint16_t *paramLen)
{
	const unsigned char	*p = *pp, *end;
	int32_t				rc;
	uint32_t			llen;

	rc = PS_PARSE_FAIL;
	end = p + len;
	if (len < 1 || (rc = getAsnSequence32(&p, len, &llen, 0)) < 0) {
		psTraceCrypto("getAsnAlgorithmIdentifier failed on inital parse\n");
		return rc;
	}
	/* Always checks for parameter length */
	if (end - p < 1) {
		return PS_LIMIT_FAIL;
	}
	rc = getAsnOID(&p, llen, oi, 1, paramLen);
	*pp = p;
	return rc;
}

/******************************************************************************/

int32_t getAsnOID(const unsigned char **pp, uint32_t len, int32_t *oi,
				uint8_t checkForParams, uint16_t *paramLen)
{
	const unsigned char	*p = *pp, *end;
	int32_t				plen, rc;
	uint32_t			arcLen;

	rc = PS_PARSE_FAIL;
	end = p + len;
	plen = end - p;
	if (*(p++) != ASN_OID || (rc = getAsnLength32(&p, (uint32_t)(end - p), &arcLen, 0))
			< 0) {
		psTraceCrypto("Malformed algorithmId 2\n");
		return rc;
	}
	if (end - p < arcLen) {
		return PS_LIMIT_FAIL;
	}
	if (end - p < 2) {
		psTraceCrypto("Malformed algorithmId 3\n");
		return PS_LIMIT_FAIL;
	}
	*oi = 0;
	while (arcLen > 0) {
		*oi += *p;
		p++;
		arcLen--;
	}

	if (checkForParams) {
		plen -= (end - p);
		*paramLen = len - plen;
		if (*p != ASN_NULL) {
			*pp = p;
			/* paramLen tells whether params exist or completely missing (0) */
			if (*paramLen > 0) {
				//psTraceIntCrypto("OID %d has parameters to process\n", *oi);
			}
			return PS_SUCCESS;
		}
		/* NULL parameter case.  Somewhat common.  Skip it for the caller */
		if (end - p < 2) {
			psTraceCrypto("Malformed algorithmId 4\n");
			return PS_LIMIT_FAIL;
		}
		if (*paramLen < 2) {
			psTraceCrypto("Malformed algorithmId 5\n");
			return PS_LIMIT_FAIL;
		}
		*paramLen -= 2; /* 1 for the OID tag and 1 for the NULL */
		*pp = p + 2;
	} else {
		*paramLen = 0;
		*pp = p;
	}
	return PS_SUCCESS;
}


#ifdef USE_RSA
#endif /* USE_RSA */
/******************************************************************************/

