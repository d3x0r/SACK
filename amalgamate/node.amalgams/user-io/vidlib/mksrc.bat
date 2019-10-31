:::
:   
:


@set S=../../../..

@set SRCS=
@set SRCS= %SRCS%   %S%/src/vidlib/vidlib.c
@set SRCS= %SRCS%   %S%/src/vidlib/key.c
@set SRCS= %SRCS%   %S%/src/vidlib/keydefs.c
@set SRCS= %SRCS%   %S%/src/vidlib/opengl.c

@set OGSRCS=
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_common.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_touch.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_interface.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_3d_mouse.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_render_loop.c
:@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_wasm.c
:@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/keymap_wasm.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/key_state.c

@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/keydefs.c

@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/vidlib_win32.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/keymap_win32.c
@set OGSRCS= %OGSRCS%   %S%/src/vidlib/puregl2/win32_opengl.c



del sack_vidlib.c
del sack_vidlib_puregl2.c
del sack_typelib.h

@set OPTS=
@set OPTS=%OPTS%	-DRENDER_LIBRARY_SOURCE
@set OPTS=%OPTS%	-DNEED_SHLAPI
@set OPTS=%OPTS%	-I%S%/src/vidlib/puregl/glew-1.9.0/include

c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_vidlib.c %SRCS% %OPTS%
c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_vidlib_puregl2.c split.c %OGSRCS% %OPTS%

copy sack_vidlib.c sack_vidlib.cc
copy sack_vidlib_puregl2.c sack_vidlib_puregl2.cc

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

