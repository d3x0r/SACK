
/*
 *  sha1.h
 *
 *  Description:
 *      This is the header file for code which implements the Secure
 *      Hashing Algorithm 1 as defined in FIPS PUB 180-1 published
 *      April 17, 1995.
 *
 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#ifndef INCLUDED_SHA1_H_
#define INCLUDED_SHA1_H_
	
#include <sack_types.h>

#ifdef SHA1_SOURCE
#define SHA1_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SHA1_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#if !defined(  HAS_STDINT )
#ifndef __WATCOMC__
	typedef unsigned long uint32_t;
	typedef short int_least16_t;
	typedef unsigned char uint8_t;
#else
#include <sys/types.h>
#endif
//typedef unsigned char uint8_t;
//typedef int int_least16_t;
#endif
/*
 * If you do not have the ISO standard stdint.h header file, then you
 * must typdef the following:
 *    name              meaning
 *  uint32_t         unsigned 32 bit integer
 *  uint8_t          unsigned 8 bit integer (i.e., unsigned char)
 *  int_least16_t    integer of >= 16 bits
 *
 */

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};
#endif
#define SHA1HashSize 20

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context
{
    uint32_t Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */

    uint32_t Length_Low;            /* Message length in bits      */
    uint32_t Length_High;           /* Message length in bits      */

                               /* Index into message block array   */
    int_least16_t Message_Block_Index;
    uint8_t Message_Block[64];      /* 512-bit message blocks      */

    int Computed;               /* Is the digest computed?         */
    int Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

/*
 *  Function Prototypes
 */



SHA1_PROC( int, SHA1Reset )(  SHA1Context *);
SHA1_PROC( int, SHA1Input )(  SHA1Context *,
                const uint8_t *,
                size_t);
SHA1_PROC( int, SHA1Result )( SHA1Context *,
                uint8_t Message_Digest[SHA1HashSize]);

#endif
// $Log: $
