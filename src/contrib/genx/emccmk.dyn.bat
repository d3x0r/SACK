
@set CFLAGS=%COMMON_CFLAGS% -I../../../include
@set CFLAGS=%CFLAGS% -D__STATIC__
@set CFLAGS=%CFLAGS% -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference
@set SRCS= charProps.c  genx.c

del libgenx* 

call emcc -g -D_DEBUG -s WASM=1 -s EXPORT_ALL=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libgenx.wasm.so
rename a.out.wasm.map libgenx.wasm.so.map
rename a.out.wast libgenx.wast

call emcc -O3 -s WASM=1 -s EXPORT_ALL=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libgenx.o.wasm.so

copy libgenx* ..\..\..\amalgamate\wasmgui\libs


@echo on
