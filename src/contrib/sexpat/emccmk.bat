


@set CFLAGS=-I../../../include

@set CFLAGS=%CFLAGS% -D__STATIC__

@set SRCS= xmlparse.c xmlrole.c xmltok.c


call emcc -g -D_DEBUG -o ./expat.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./expato.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on
