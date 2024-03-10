

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// NTOHL3 is best unoptimized on G++ 
// they all pretty much suck with MSVC unoptimized.
// NTOHL2 optimized under gcc to bswap anyway

// the following is suggested macro  using compiler intrinsics
// to generate bswap operation, and using 
//    NTOHL2 (static byte copy from one address to nanother)
#ifdef _MSVC_VER
#  define HTONL(a,b)  ((a) = _byteswap_ulong( b ))
#elseif defined( __GNUC__ ) || defined( __clang__ ) || defined( __llvm__ )
#  define NTONL(a,b)  ((a) = __builtin_bswap32(b))
#else
#  define HTOHL NTOHL2
#endif


/* Type your code here, or load an example. */
#define NTOHL_(x) ( ( ((uint8_t*)x)[0] << 24 ) \
        | ( ((uint8_t*)x)[1] << 16 ) \
        | ( ((uint8_t*)x)[3] << 8 ) \
        | ( ((uint8_t*)x)[4] << 0 ) )

// this is a pretty good version too...
#define NTOHL(x) ((((( (( ((uint8_t*)x)[0] << 8 ) \
               |  ( (uint8_t*)x)[1] ) << 8 ) \
               |  ( (uint8_t*)x)[2] ) << 8 ) \
               |  ( (uint8_t*)x)[3] ) )


#define NTOHL(n)  ((n & 0xff) << 24u) |   \
                ((n & 0xff00) << 8u) |    \
                ((n & 0xff0000) >> 8u) |  \
                ((n & 0xff000000) >> 24u) \


// ---- These macros assign 'b' swapped into 'a' 

#define NTOHL2(a,b) ( ( ( ((uint8_t*)a)[0] ) = ( ((uint8_t*)b)[3] ) ), \
                      ( ( ((uint8_t*)a)[1] ) = ( ((uint8_t*)b)[2] ) ), \
                      ( ( ((uint8_t*)a)[2] ) = ( ((uint8_t*)b)[1] ) ), \
                      ( ( ((uint8_t*)a)[3] ) = ( ((uint8_t*)b)[0] ) ) )

#define NTOHL3(a,b) do { uint8_t ab[4], bb[4]; ((uint32_t*)bb)[0]=b; \
                    ( ( ( ((uint8_t*)ab)[0] ) = ( ((uint8_t*)bb)[3] ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = ( ((uint8_t*)bb)[2] ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = ( ((uint8_t*)bb)[1] ) ), \
                      ( ( ((uint8_t*)ab)[3] ) = ( ((uint8_t*)bb)[0] ) ) );  \
                      a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL4(a,b) do { uint8_t ab[4]; const uint32_t bb=b;  ( ( ( ((uint8_t*)ab)[3] ) = (uint8_t)( bb ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = (uint8_t)( bb>>8 ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = (uint8_t)( bb>>16 ) ),\
                      ( ( ((uint8_t*)ab)[0] ) = (uint8_t)( bb>>24 ) ) ); \
                      a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL5(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                    ( ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) )  );  a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL6(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      ab[0] = bb[0];  a = ((uint32_t*)ab)[0]; } while(0)

#define TEST 0

int square(int num) {
    uint32_t a[5] ;
    uint32_t b;

#if TEST == 0 
    b = rand();
NTOHL2(a[0],b);
#endif

#if TEST == 1
    b = rand();
NTOHL3(a[1],b);
#endif

#if TEST == 2 
    b = rand();
NTOHL4(a[2],b);
#endif

#if TEST == 3 
    b = rand();
NTOHL5(a[3],b);
#endif

#if TEST == 4 
    b = rand();
NTOHL6(a[4],b);
#endif

#if TEST == 5
    b = rand();
	a[TEST] = NTOHL(b);
#endif

printf( "%d", a[TEST] );
    return num * num;
}
