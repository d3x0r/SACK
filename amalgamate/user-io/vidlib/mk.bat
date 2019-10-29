:::
:   
:


@set S=../../..

@set SRCS=
@set SRCS= %SRCS%   %S%/src/vidlib/vidlib.c
@set SRCS= %SRCS%   %S%/src/vidlib/key.c
@set SRCS= %SRCS%   %S%/src/vidlib/keydefs.c
@set SRCS= %SRCS%   %S%/src/vidlib/opengl.c

@set OGSRCS=
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/vidlib.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/key.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/keydefs.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/opengl.c

 
del sack_vidlib.c
del sack_vidlib_puregl2.c
del sack_typelib.h

@set OPTS=
@set OPTS=%OPTS%	-DRENDER_LIBRARY_SOURCE
@set OPTS=%OPTS%	-DNEED_SHLAPI

c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_vidlib.c %SRCS% %OPTS%
c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_vidlib_puregl2.c %OGSRCS% %OPTS%

mkdir h
copy config.vidlib.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../include/render.h
@set HDRS= %HDRS% %S%/../include/render3d.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_vidlib.h %HDRS%
cd ..


set C_OPTS=
set C_OPTS=%C_OPTS% -DTARGETNAME=\"a.exe\"

gcc -c -g -o a.o %C_OPTS% sack_vidlib.c
gcc -c -O3 -o a-opt.o %C_OPTS% sack_vidlib.c

@set LIBS=
@set LIBS=%LIBS% -lwinmm 
@set LIBS=%LIBS% -lws2_32
@set LIBS=%LIBS% -liphlpapi
@set LIBS=%LIBS% -lrpcrt4
@set LIBS=%LIBS% -lodbc32 
@set LIBS=%LIBS% -lpsapi
@set LIBS=%LIBS% -lntdll
@set LIBS=%LIBS% -lcrypt32
@set LIBS=%LIBS% -lole32

@set LIBS=%LIBS% -lopengl32
@set LIBS=%LIBS% -lgdi32


gcc -g -o a.exe %C_OPTS% sack_vidlib.c ../../fullcore/sack_ucb_full.c test.c %LIBS%
gcc -O3 -o a-opt.exe %C_OPTS% sack_vidlib.c ../../fullcore/sack_ucb_full.c test.c %LIBS%

gcc -g -o a.exe a.o ../../fullcore/a.o test.c %LIBS%
gcc -O3 -o a-opt.exe a-opt.o ../../fullcore/a-opt.o test.c %LIBS%

gcc -O3 -o a-test2.exe a-opt.o test2.c %LIBS%

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE

