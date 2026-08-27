/* KangarooTwelve (K12) - self contained implementation.
 *
 * Replaces the eXtended Keccak Code Package (contrib/K12/lib) with a single
 * portable C file; the API and the produced bytes are identical to the
 * reference implementation by Ronny Van Keer (https://keccak.team/), which
 * this is derived from, so it is a drop-in replacement.
 *
 * Fixed parameters (as in the K12 specification):
 *    security 128 bits, capacity 256 bits, rate 1344 bits (168 bytes),
 *    Keccak-p[1600,12], chunk size 8192 bytes, chaining values of 32 bytes.
 *
 * To the extent possible under law, the implementer has waived all copyright
 * and related or neighboring rights to the source code in this file.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef SACK_CONTRIB_K12_INCLUDED
#define SACK_CONTRIB_K12_INCLUDED

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K12_SECURITY         128
#define K12_CAPACITY         ( 2 * K12_SECURITY )
#define K12_CAPACITY_BYTES   ( K12_CAPACITY / 8 )   /* 32 - chaining value size */
#define K12_RATE             ( 1600 - K12_CAPACITY )
#define K12_RATE_BYTES       ( K12_RATE / 8 )       /* 168 */
#define K12_CHUNK_SIZE       8192

typedef enum {
	NOT_INITIALIZED,
	ABSORBING,
	FINAL,
	SQUEEZING
} KCP_Phases;
typedef KCP_Phases KangarooTwelve_Phases;

/* one Keccak-p[1600,12] sponge; rate is always K12_RATE_BYTES.
   `state' is kept in little endian lane order at all times, so extracting
   output is a plain copy on every host. */
typedef struct KeccakWidth1600_12rounds_SpongeInstanceStruct {
	union {
		uint8_t  b[200];
		uint64_t q[25];
	} state;
	unsigned int byteIOIndex;
	int          squeezing;
} KeccakWidth1600_12rounds_SpongeInstance;

typedef struct {
	KeccakWidth1600_12rounds_SpongeInstance queueNode; /* current leaf/chunk node */
	KeccakWidth1600_12rounds_SpongeInstance finalNode; /* root node */
	size_t       fixedOutputLength;
	size_t       blockNumber;
	unsigned int queueAbsorbedLen;
	KangarooTwelve_Phases phase;
} KangarooTwelve_Instance;

/** Extendable output function KangarooTwelve.
  * @param  input           Pointer to the input message (M).
  * @param  inputByteLen    The length of the input message in bytes.
  * @param  output          Pointer to the output buffer.
  * @param  outputByteLen   The desired number of output bytes.
  * @param  customization   Pointer to the customization string (C).
  * @param  customByteLen   The length of the customization string in bytes.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve( const unsigned char *input, size_t inputByteLen, unsigned char *output, size_t outputByteLen, const unsigned char *customization, size_t customByteLen );

/**
  * Function to initialize a KangarooTwelve instance.
  * @param  ktInstance      Pointer to the instance to be initialized.
  * @param  outputByteLen   The desired number of output bytes,
  *                         or 0 for an arbitrarily-long output.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Initialize( KangarooTwelve_Instance *ktInstance, size_t outputByteLen );

/**
  * Function to give input data to be absorbed.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Update( KangarooTwelve_Instance *ktInstance, const unsigned char *input, size_t inputByteLen );

/**
  * Function to call after all the input message has been input, and to get
  * output bytes if the length was specified when calling KangarooTwelve_Initialize().
  * If @a outputByteLen was 0 there, the output bytes must be extracted using
  * KangarooTwelve_Squeeze().
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Final( KangarooTwelve_Instance *ktInstance, unsigned char *output, const unsigned char *customization, size_t customByteLen );

/**
  * Function to squeeze output data; KangarooTwelve_Final() must have been called.
  * May be called repeatedly, the output continues the same stream.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Squeeze( KangarooTwelve_Instance *ktInstance, unsigned char *output, size_t outputByteLen );

#ifdef __cplusplus
}
#endif

#endif
