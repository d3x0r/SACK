


set PNG_SOURCE=png.c  pngset.c  pngget.c  pngrutil.c  pngtrans.c  pngwutil.c ^
	 pngmem.c  pngpread.c  pngread.c  pngerror.c  pngwrite.c ^
	  pngrtran.c  pngwtran.c  pngrio.c  pngwio.c 


set CFLAGS=-I../zlib-1.2.11
set SRCS=%PNG_SOURCE%



call emcc -g -o ./png.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./pngo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on
