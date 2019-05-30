


@set PNG_SOURCE=png.c  pngset.c  pngget.c  pngrutil.c  pngtrans.c  pngwutil.c ^
	 pngmem.c  pngpread.c  pngread.c  pngerror.c  pngwrite.c ^
	  pngrtran.c  pngwtran.c  pngrio.c  pngwio.c 


@set CFLAGS=%COMMON_CFLAGS% -I../zlib-1.2.11
@set CFLAGS=%CFLAGS%  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference

@set SRCS=%PNG_SOURCE%

del libpng* 

call emcc -g -D_DEBUG -s WASM=1 -s SIDE_MODULE=1  %CFLAGS% %SRCS%  -L../zlib-1.2.11 -lz.wasm
rename a.out.wasm libpng.wasm.so
rename a.out.wasm.map libpng.wasm.so.map
rename a.out.wast libpng.wast

call emcc -O3 -s WASM=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS% -L../zlib-1.2.11 -lz.wasm
rename a.out.wasm libpng.o.wasm.so

copy libpng* ..\..\..\amalgamate\wasmgui\libs

@echo on
