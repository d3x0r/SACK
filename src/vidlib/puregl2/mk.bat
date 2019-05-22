

set CFLAGS=-I ../puregl/glew-1.9.0/include

set CFLAGS=%CFLAGS% -I../../../include

:set CFLAGS=%CFLAGS% -I../../contrib/zlib-1.2.11
:set CFLAGS=%CFLAGS% -I../../contrib/libpng-1.6.34
:set CFLAGS=%CFLAGS% -I../../contrib/jpeg-9
:set CFLAGS=%CFLAGS% -I../../contrib/freetype-2.8/include

set CFLAGS=%CFLAGS% -DUSE_GLES2

set CFLAGS=%CFLAGS% -D__NO_OPTIONS__ -D__STATIC__ -D__WASM__



set  SRCS= ^
        keydefs.c    ^
        key_state.c  ^
        vidlib_common.c  ^
        vidlib_touch.c   ^
        vidlib_interface.c  ^
        vidlib_3d_mouse.c  ^
        vidlib_render_loop.c 

set SRCS=%SRCS%  vidlib_wasm.c keymap_wasm.c

:      set( SOURCES ${COMMON_SOURCES} vidlib_android.c keymap_android.c vidlib_egl.c vidlib_gles.c )
:      set( SOURCES ${COMMON_SOURCES} win32_opengl.c keymap_linux.c vidlib_x11.c )
:    set( SOURCES ${COMMON_SOURCES} keymap_win32.c vidlib_win32.c win32_opengl.c )



set CFLAGS=%CFLAGS% -DTARGET_LABEL=imglib_puregl2 -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE -DPURE_OPENGL2_ENABLED -DRENDER_LIBRARY_SOURCE 

:@set OPTS=
:@set OPTS=%OPTS%	-I${SACK_BASE}/src/contrib/sqlite/3.23.0-MySqlite

c:\tools\ppc.exe -c -K -once -f -ssio -sd %OPTS% -L -I../../../include -p -o sack_vidlib_puregl.c %SRCS%

call emcc -s USE_WEBGL2=1 -g -o ./vidlib_puregl2.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -s USE_WEBGL2=1 -O3 -o ./vidlib_puregl2o.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
