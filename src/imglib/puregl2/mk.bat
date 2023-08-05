

set CFLAGS=-I ../../vidlib/puregl/glew-1.9.0/include

set CFLAGS=%CFLAGS% -I../../../include

set CFLAGS=%CFLAGS% -I../../contrib/zlib-1.2.11
set CFLAGS=%CFLAGS% -I../../contrib/libpng-1.6.34
set CFLAGS=%CFLAGS% -I../../contrib/jpeg-9
set CFLAGS=%CFLAGS% -I../../contrib/freetype-2.8/include

set CFLAGS=%CFLAGS% -DUSE_GLES2

set CFLAGS=%CFLAGS% -D__NO_OPTIONS__ -D__STATIC__

set font_sources= ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole2.c 
set loader_sources= ../bmpimage.c ../gifimage.c ../pngimage.c ../jpgimage.c 
set sprite_sources= ../sprite_common.c 

set SRCS=  ../alphatab.c ^
  ../alphastab.c ^
  blotscaled.c blotdirect.c image.c ../image_common.c  ^
  line.c interface.c sprite.c %sprite_sources% ^
  %loader_sources% ^
  %font_sources% ^
  shader.c simple_shader.c simple_texture_shader.c ^
  simple_multi_shaded_texture_shader.c 


set CFLAGS=%CFLAGS% -DTARGET_LABEL=imglib_puregl2 -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE -DPURE_OPENGL2_ENABLED


call emcc -g -D_DEBUG -o ./imglib_puregl2.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./imglib_puregl2o.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on
