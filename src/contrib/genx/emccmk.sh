
CFLAGS="$COMMON_CFLAGS -I../../../include"
CFLAGS="$CFLAGS -D__STATIC__"
SRCS=" charProps.c  genx.c"

emcc -g -D_DEBUG -o ./genx.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
emcc -O3 -o ./genxo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS

cp *.lo ../../../amalgamate/wasmgui/libs
