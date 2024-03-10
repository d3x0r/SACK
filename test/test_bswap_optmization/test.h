#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
//
#pragma optimize( "st", on )
inline auto bswap( uint32_t v ) noexcept { return _byteswap_ulong( v ); }
#pragma optimize( "", on )

#pragma optimize( "st", on )

constexpr uint32_t const_bswap( uint32_t v ) noexcept {
	return ( ( v & UINT32_C( 0x0000'00FF ) ) << 24 ) |
		( ( v & UINT32_C( 0x0000'FF00 ) ) << 8 ) |
		( ( v & UINT32_C( 0x00FF'0000 ) ) >> 8 ) |
		( ( v & UINT32_C( 0xFF00'0000 ) ) >> 24 );
}

#pragma optimize( "", on )

#endif



#define NTOHL1(x) ( ( ((uint8_t*)&(x))[0] << 24 ) \
        | ( ((uint8_t*)&(x))[1] << 16 ) \
        | ( ((uint8_t*)&(x))[2] << 8 ) \
        | ( ((uint8_t*)&(x))[3] << 0 ) )

#define NTOHL2(x) ((((( (( ((uint8_t*)&(x))[0] << 8 ) \
               |  ( (uint8_t*)&(x))[1] ) << 8 ) \
               |  ( (uint8_t*)&(x))[2] ) << 8 ) \
               |  ( (uint8_t*)&(x))[3] ) )

#define NTOHL3(n)  ((((n) & 0xff) << 24u) |   \
                (((n) & 0xff00) << 8u) |    \
                (((n) & 0xff0000) >> 8u) |  \
                (((n) & 0xff000000) >> 24u)) 

#ifdef _MSC_VER
#  define NTOHL4(n)  _byteswap_ulong( n )
#else
#  error intrinsic for this compiler is not filled in.
#endif


#define NTOHL2_1(a,b) ( ( ( ((uint8_t*)&(a))[0] ) = ( ((uint8_t*)&(b))[3] ) ), \
                      ( ( ((uint8_t*)&(a))[1] ) = ( ((uint8_t*)&(b))[2] ) ), \
                      ( ( ((uint8_t*)&(a))[2] ) = ( ((uint8_t*)&(b))[1] ) ), \
                      ( ( ((uint8_t*)&(a))[3] ) = ( ((uint8_t*)&(b))[0] ) ) )

#define NTOHL2_2(a,b) do { uint8_t ab[4], bb[4]; ((uint32_t*)bb)[0]=b; \
                    ( ( ( ((uint8_t*)ab)[0] ) = ( ((uint8_t*)bb)[3] ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = ( ((uint8_t*)bb)[2] ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = ( ((uint8_t*)bb)[1] ) ), \
                      ( ( ((uint8_t*)ab)[3] ) = ( ((uint8_t*)bb)[0] ) ) );  a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL2_3(a,b) do { uint8_t ab[4]; const uint32_t bb=b;  ( ( ( ((uint8_t*)ab)[3] ) = (uint8_t)( bb ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = (uint8_t)( bb>>8 ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = (uint8_t)( bb>>16 ) ),\
                      ( ( ((uint8_t*)ab)[0] ) = (uint8_t)( bb>>24 ) ) ); \
                      a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL2_4(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                    ( ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) )  );  a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL2_5(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      ab[0] = bb[0];  a = ((uint32_t*)ab)[0]; } while(0)

