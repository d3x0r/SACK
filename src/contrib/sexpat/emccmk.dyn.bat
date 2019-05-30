

@set CFLAGS=%COMMON_CFLAGS% -I../../../include

@set CFLAGS=%CFLAGS% -D__STATIC__

@set SRCS= xmlparse.c xmlrole.c xmltok.c


del libsexpat* 

call emcc -g -D_DEBUG -s WASM=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libsexpat.wasm.so
rename a.out.wasm.map libsexpat.wasm.so.map
rename a.out.wast libsexpat.wast

call emcc -O3 -s WASM=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libsexpat.o.wasm.so

copy libsexpat* ..\..\..\amalgamate\wasmgui\libs

@echo on
