
@set CFLAGS=%COMMON_CFLAGS% -I../../../include
@set CFLAGS=%CFLAGS% -D__STATIC__
@set SRCS= charProps.c  genx.c

call emcc -g -D_DEBUG -o ./genx.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./genxo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on
