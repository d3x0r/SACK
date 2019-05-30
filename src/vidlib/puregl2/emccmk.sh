

CFLAGS="$COMMON_CFLAGS -D__MANUAL_PRELOAD__ -I ../puregl/glew-1.9.0/include"

CFLAGS="$CFLAGS -I../../../include"

CFLAGS="$CFLAGS -DUSE_GLES2"

CFLAGS="$CFLAGS -D__NO_OPTIONS__ -D__STATIC__ -D__WASM__"

CFLAGS="$CFLAGS -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR "

SRCS="
        keydefs.c    
        key_state.c  
        vidlib_common.c  
        vidlib_touch.c   
        vidlib_interface.c  
        vidlib_3d_mouse.c  
        vidlib_render_loop.c "

SRCS="$SRCS  vidlib_wasm.c keymap_wasm.c"

#:      set( SOURCES ${COMMON_SOURCES} vidlib_android.c keymap_android.c vidlib_egl.c vidlib_gles.c )
#:      set( SOURCES ${COMMON_SOURCES} win32_opengl.c keymap_linux.c vidlib_x11.c )
#:    set( SOURCES ${COMMON_SOURCES} keymap_win32.c vidlib_win32.c win32_opengl.c )



CFLAGS="$CFLAGS -DTARGET_LABEL=imglib_puregl2 -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE -DPURE_OPENGL2_ENABLED -DRENDER_LIBRARY_SOURCE "

#:OPTS=
#:OPTS=$OPTS	-I${SACK_BASE}/src/contrib/sqlite/3.23.0-MySqlite


#: -Wno-address-of-packed-member
#:  -Wno-address-of-packed-member
emcc -s USE_WEBGL2=1 -g -D_DEBUG -o ./vidlib_puregl2.lo  -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
emcc -s USE_WEBGL2=1 -O3 -o ./vidlib_puregl2o.lo  -Wno-parentheses -Wno-comment -Wno-null-dereference $CFLAGS $SRCS
cp *.lo ../../../amalgamate/wasmgui/libs

