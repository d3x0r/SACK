

CFLAGS="$COMMON_CFLAGS -D__MANUAL_PRELOAD__ -I ../../vidlib/puregl/glew-1.9.0/include"

CFLAGS="$CFLAGS -I../../../include"

CFLAGS="$CFLAGS -I../../contrib/zlib-1.2.11"
CFLAGS="$CFLAGS -I../../contrib/libpng-1.6.34"
CFLAGS="$CFLAGS -I../../contrib/jpeg-9"
CFLAGS="$CFLAGS -I../../contrib/freetype-2.8/include"

CFLAGS="$CFLAGS -DUSE_GLES2"

CFLAGS="$CFLAGS -D__NO_OPTIONS__ -D__STATIC__"

font_sources=" ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole2.c "
loader_sources=" ../bmpimage.c ../gifimage.c ../pngimage.c ../jpgimage.c "
sprite_sources=" ../sprite_common.c "

SRCS="  ../alphatab.c 
  ../alphastab.c  blotscaled.c blotdirect.c image.c ../image_common.c  
  line.c interface.c sprite.c $sprite_sources 
  $loader_sources 
  $font_sources 
  shader.c simple_shader.c simple_texture_shader.c 
  simple_multi_shaded_texture_shader.c "


CFLAGS="$CFLAGS -DTARGET_LABEL=imglib_puregl2 -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE -DPURE_OPENGL2_ENABLED"


emcc -g -D_DEBUG -o ./imglib_puregl2.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
emcc -O3 -o ./imglib_puregl2o.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS

cp *.lo ../../../amalgamate/wasmgui/libs
