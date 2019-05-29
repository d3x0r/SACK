

PNG_SOURCE="png.c  pngset.c  pngget.c  pngrutil.c  pngtrans.c  pngwutil.c 
	 pngmem.c  pngpread.c  pngread.c  pngerror.c  pngwrite.c 
	  pngrtran.c  pngwtran.c  pngrio.c  pngwio.c "


CFLAGS="$COMMON_CFLAGS -I../zlib-1.2.11"
SRCS="$PNG_SOURCE"


emcc -g -D_DEBUG -o ./png.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
emcc -O3 -o ./pngo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS

cp *.lo ../../../amalgamate/wasmgui/libs
