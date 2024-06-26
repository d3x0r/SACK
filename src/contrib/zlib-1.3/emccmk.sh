



ZLIB_SRCS="  adler32.c compress.c crc32.c  uncompr.c deflate.c trees.c  
           zutil.c inflate.c inftrees.c  inffast.c infback.c  "
#           infblock.c infcodes.c infutil.c


# under arm we don't really NEED direct FILEIO stuffs.
#SET( ZLIB_SRCS ${ZLIB_SRCS}   gzclose.c gzlib.c gzread.c gzwrite.c )

CFLAGS="$COMMON_CFLAGS "

SRCS="$ZLIB_SRCS"



 emcc -g -D_DEBUG -o ./zlib.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $SRCS
 emcc -O3 -o ./zlibo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $SRCS

cp *.lo ../../../amalgamate/wasmgui/libs
