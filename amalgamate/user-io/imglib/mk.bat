:::                           M:\sack\amalgamate\user-io\imglib\mk.bat
:
:


:
:
:
:
:
:   
:

@set  S=../../..

:--------------------------------------------------------


@set SRCS=
@set SRCS= %SRCS%   %S%/src/imglib/alphastub.c
@set SRCS= %SRCS%   %S%/src/imglib/alphatab.c
@set SRCS= %SRCS%   %S%/src/imglib/blotdirect.c
@set SRCS= %SRCS%   %S%/src/imglib/blotscaled.c
@set SRCS= %SRCS%   %S%/src/imglib/fntcache.c
@set SRCS= %SRCS%   %S%/src/imglib/fntrender.c
@set SRCS= %SRCS%   %S%/src/imglib/font.c
@set SRCS= %SRCS%   %S%/src/imglib/image_common.c
@set SRCS= %SRCS%   %S%/src/imglib/image.c
@set SRCS= %SRCS%   %S%/src/imglib/interface.c
@set SRCS= %SRCS%   %S%/src/imglib/bmpimage.c
@set SRCS= %SRCS%   %S%/src/imglib/gitimage.c
@set SRCS= %SRCS%   %S%/src/imglib/jpgimage.c
@set SRCS= %SRCS%   %S%/src/imglib/pngimage.c
@set SRCS= %SRCS%   %S%/src/imglib/line.c
@set SRCS= %SRCS%   %S%/src/imglib/lucidaconsole2.c
@set SRCS= %SRCS%   %S%/src/imglib/sprite_common.c
@set SRCS= %SRCS%   %S%/src/imglib/sprite.c



 
del sack_imglib.c
del sack_imglib.h

@set OPTS=
@set OPTS=%OPTS%  -DMAKE_RCOORD_SINGLE 

:--------------------------------------------------------

@set OGSRCS=
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/image.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../font.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../fntcache.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../fntrender.c

: can ignore this one
:@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../lucidaconsole2.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../bmpimage.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../gifimage.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../pngimage.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../jpgimage.c

@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../sprite_common.c

:@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../alphatab.c
:@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../alphastab.c

@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/blotscaled.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/blotdirect.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/../image_common.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/line.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/interface.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/sprite.c

@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/shader.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/simple_shader.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/simple_texture_shader.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/simple_multi_shaded.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/simple_multi_shaded_texture_shader.c
@set OGSRCS=%OGSRCS%  %S%/src/imglib/puregl2/shaded_texture_shader.c

del sack_imglib_puregl2.c


@set OGOPTS=
@set OGOPTS=%OGOPTS% -I%S%/src/vidlib/puregl/glew-1.9.0/include
:@set OGOPTS=%OGOPTS% -DUSE_GLES2
@set OGOPTS=%OGOPTS% -D__3D__
@set OGOPTS=%OGOPTS% -D_OPENGL_DRIVER
:@set OGOPTS=%OGOPTS% -DMAKE_RCOORD_SINGLE
@set OGOPTS=%OGOPTS% -DPURE_OPENGL2_ENABLED

@set OPTS=%OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK

:--------------------------------------------------------


c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_imglib.c %SRCS% 
c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_imglib_puregl2.c split.c %OGSRCS%

mkdir h
copy config.vidlib.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../include/render.h
@set HDRS= %HDRS% %S%/../include/render3d.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_vidlib.h %HDRS%
cd ..

@set C_OPTS=
@set C_OPTS=%C_OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK
@set C_OPTS=%C_OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK/jpeg
@set C_OPTS=%C_OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK/png
@set C_OPTS=%C_OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK/zlib
@set C_OPTS=%C_OPTS% -IH:/build/mingw64-64/sack/release_out/core/include/SACK/genx



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
@set LIBS=%LIBS% -lglu32
:@set LIBS=%LIBS% -lglew


gcc -g -o a.so -shared %C_OPTS% sack_imglib.c  sack_imglib_puregl2.c ../../fullcore/a.o %LIBS%
gcc -c -O3 -o a-opt.o %C_OPTS% sack_imglib.c


gcc -g -o a.exe sack_ucb_full.c test.c %LIBS%
gcc -O3 -o a-opt.exe sack_ucb_full.c test.c %LIBS%

gcc -g -o a.exe a.o test.c %LIBS%
gcc -O3 -o a-opt.exe a-opt.o test.c %LIBS%

gcc -O3 -o a-test2.exe a-opt.o test2.c %LIBS%

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE

