

set CFLAGS=-I ../vidlib/puregl/glew-1.9.0/include

set CFLAGS=%CFLAGS% -I../../include

set CFLAGS=%CFLAGS% -I../contrib/zlib-1.2.11
set CFLAGS=%CFLAGS% -I../contrib/libpng-1.6.34
set CFLAGS=%CFLAGS% -I../contrib/jpeg-9
set CFLAGS=%CFLAGS% -I../contrib/freetype-2.8/include

set CFLAGS=%CFLAGS% -DUSE_GLES2

set CFLAGS=%CFLAGS% -D__NO_OPTIONS__ -D__STATIC__


set SRCS=   alphastab.c ^
  alphatab.c ^
  blotdirect.c ^
  blotscaled.c ^
  fntcache.c  fntrender.c  font.c   ^
  bmpimage.c ^
  gifimage.c  image_common.c  image.c  interface.c ^
  jpgimage.c  line.c  lucidaconsole2.c ^
  pngimage.c  sprite_common.c  sprite.c


set CFLAGS=%CFLAGS% -DTARGET_LABEL=imglib -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE -DPURE_OPENGL2_ENABLED


call emcc -g -D_DEBUG -o ./imglib.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./imglibo.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on
