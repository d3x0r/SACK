/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */


#ifdef MD5_SOURCE
#define MD5_PROC(type,name) EXPORT_METHOD type name
#else
#define MD5_PROC(type,name) IMPORT_METHOD type name
#endif

#include <sack_types.h>

/* MD5 context. */
typedef struct {
	_32 state[4];				    /* state (ABCD) */
	_32 count[2];	 /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];			    /* input buffer */
} MD5_CTX;

MD5_PROC( void, MD5Init )(MD5_CTX *);
MD5_PROC( void, MD5Update )(MD5_CTX *, unsigned char *, unsigned int);
MD5_PROC( void, MD5Final )(unsigned char [16], MD5_CTX *);

//----------------------------------------------------------------------------
// $Log: md5.h,v $
// Revision 1.1  2003/11/21 16:25:48  jim
// INitial commit - port from eltanin
//
// Revision 1.2  2002/11/19 18:06:42  brad
// CVS logging added
//
//
