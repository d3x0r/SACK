

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

if( __ANDROID__ OR __QNX__ )
ADD_DEFINITIONS( -DUSE_GLES2 -DUSE_EGL )
endif( __ANDROID__ OR __QNX__ )

ADD_DEFINITIONS( -D__3D__ )

if( __ANDROID__ )
ADD_DEFINITIONS( -D__ANDROID__ )
endif( __ANDROID__ )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
set( COMMON_SOURCES
        ../keydefs.c    # key event binding to global keyboard input or specific displays
        ../key_state.c  # key state handling; is key up/down, delay times, toggles...
        ../vidlib_common.c  # create display surfaces, manage logical display properties
        ../vidlib_touch.c   # common handling for touch input
        ../vidlib_interface.c  # common interface export
        ../vidlib_3d_mouse.c  # mouse utilties; reverse map points into screen
        ../vidlib_render_loop.c  # dispach draw to all windows in a camera
   )     

if( __LINUX__ )
   # something more like IF USE_EGL
   if( __ANDROID__ OR __QNX__ )
      set( SOURCES ${COMMON_SOURCES} ../vidlib_android.c ../keymap_android.c ../vidlib_egl.c ../vidlib_gles.c )
   else()
      set( SOURCES ${COMMON_SOURCES} ../win32_opengl.c ../vidlib_x11.c )
   endif()
else( __LINUX__ )
    set( SOURCES ${COMMON_SOURCES} ../keymap_win32.c ../vidlib_win32.c ../win32_opengl.c )
endif( __LINUX__ )

add_library_force_source( bag.video.puregl2 SHARED
${FIRST_GCC_LIBRARY_SOURCE}
     ${SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

# _OPENGL_ENABLED flag controls member additions to the vidlib structure; HRC for instance
SET_PROPERTY(TARGET bag.video.puregl2 APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" )
SET_TARGET_PROPERTIES( bag.video.puregl2 PROPERTIES FOLDER "Video Renderer"   )


if( __ANDROID__ OR __QNX__ )
  # glOrthof is GLES1 and is used by 'glues' PROJECT
  my_target_link_libraries( bag.video.puregl2 ${SACK_CORE_LIBRARY} EGL GLESv2 GLESv1_CM android log )
  #DD_DEFINITIONS( -D__USE_FREEGLUT__ )
  #nclude_directories( ${SOURCES_ROOT}src/freeglut/freeglut-SVN/include )
  if( NOT __QNX__ )
  my_target_link_libraries( bag.video.puregl2  m )
  endif( NOT __QNX__ )
else( __ANDROID__ OR __QNX__ )
  if( __LINUX__ )
    my_target_link_libraries( bag.video.puregl2 ${SACK_CORE_LIBRARY} GL GLU glew Xxf86vm )
  else( __LINUX__ )
    my_target_link_libraries( bag.video.puregl2 ${SACK_CORE_LIBRARY} opengl32 glu32 glew )
  endif( __LINUX__ )
endif( __ANDROID__ OR __QNX__ )

install_mode_dest( bag.video.puregl2 )


