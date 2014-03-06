
/// Some notes for proper encoding
//

// a mode switch; should define alternates for bigendian archs
#define LITTLE_ENDIAN

// BOM U+FEFF  zero width no-break space (byte order mark)
//

#define UTF8_EXTEND 0x80
#define UTF8_EXTEND_MASK 0xC0
#define UTF8_BEGIN2 0xC0
#define UTF8_BEGIN2_MASK 0xE0
#define UTF8_BEGIN3 0xE0
#define UTF8_BEGIN3_MASK 0xF0
#define UTF8_BEGIN4 0xF0
#define UTF8_BEGIN4_MASK 0xF8

#define UTF8_CHAR(a)    ( (a) & 0x7F )

#define UTF8_CHAR2(a)   ( ( (UTF8_BEGIN2) + ( (a) & 0x1F ) ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0x7e0 ) >> 5 ) ) << 8 ) )

#define UTF8_CHAR3(a)   ( ( (UTF8_BEGIN3) + ( (a) & 0xF ) ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0x03F0 ) >> 4 ) ) << 8 ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0xFC00 ) >> 10 ) ) << 16 ) )

#define UTF8_CHAR4(a)   ( ( (UTF8_BEGIN4) + ( (a) & 0x7 ) ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0x0001F8 ) >> 3 ) ) << 8 ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0x007E00 ) >> 9 ) ) << 16 ) \
                        | ( ( (UTF8_EXTEND) + ( ( (a) & 0x1F8000 ) >> 15 ) ) << 24 ) )

#define UTF8_CHAR(a) ( ( (a) > 127 )   \
                       ? ( (a) > 2047 ) \
                       ? ( (a) > 65535 ) \
                       ? UTF8_CHAR4(a) \
	                    : UTF8_CHAR3(a) \
	                    : UTF8_CHAR2(a) \
	                    : UTF8_CHAR(a) )

#define UTF8_CHAR_SIZE(a) ( ( (a) > 127 )   \
                       ? ( (a) > 2047 ) \
                       ? ( (a) > 65535 ) \
                       ? 4 \
	                    : 3 \
	                    : 2 \
	                    : 1 )

#define VAL_UTF8(a) ( ( ( (a) & UTF8_BEGIN4_MASK ) == ( (a) & UTF8_BEGIN4 ) ) ? ((



