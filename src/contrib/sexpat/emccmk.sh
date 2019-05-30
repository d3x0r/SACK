


CFLAGS="$COMMON_CFLAGS -I../../../include"

CFLAGS="$CFLAGS -D__STATIC__"

SRCS="xmlparse.c xmlrole.c xmltok.c"


emcc -g -D_DEBUG -o ./expat.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
emcc -O3 -o ./expato.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS

cp *.lo ../../../amalgamate/wasmgui/libs
