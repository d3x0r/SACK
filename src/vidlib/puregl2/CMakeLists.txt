

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
        keydefs.c    # key event binding to global keyboard input or specific displays
        key_state.c  # key state handling; is key up/down, delay times, toggles...
        vidlib_common.c  # create display surfaces, manage logical display properties
        vidlib_touch.c   # common handling for touch input
        vidlib_interface.c  # common interface export
        vidlib_3d_mouse.c  # mouse utilties; reverse map points into screen
        vidlib_render_loop.c  # dispach draw to all windows in a camera
   )     

foreach( SOURCE ${COMMON_SOURCES} vidlib_win32.c keymap_win32.c )
   if( ${SOURCE} MATCHES "(.*)\\.c$" )
      set( BASENAME ${CMAKE_MATCH_1} )
   endif()
   EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/vidlib.pured3d/${BASENAME}.cpp )
   set( COMMON_CPP_SOURCES ${COMMON_CPP_SOURCES} ${CMAKE_BINARY_DIR}/vidlib.pured3d/${BASENAME}.cpp )
endforeach( SOURCE )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/local.h ${CMAKE_BINARY_DIR}/vidlib.pured3d/local.h )


if( __LINUX__ )
   # something more like IF USE_EGL
   if( __ANDROID__ OR __QNX__ )
      set( SOURCES ${COMMON_SOURCES} vidlib_android.c keymap_android.c vidlib_egl.c vidlib_gles.c )
   else()
      set( SOURCES ${COMMON_SOURCES} win32_opengl.c vidlib_x11.c )
   endif()
else( __LINUX__ )
    set( SOURCES ${COMMON_SOURCES} keymap_win32.c vidlib_win32.c win32_opengl.c )
    #set( GLUES_SOURCES glues/source/glues_project.c )
    #set( SOURCES ${SOURCES} ${GLUES_SOURCES} )
endif( __LINUX__ )

add_library_force_source( bag.video.puregl2 SHARED
${FIRST_GCC_LIBRARY_SOURCE}
     ${SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

# _OPENGL_ENABLED flag controls member additions to the vidlib structure; HRC for instance
  SET_PROPERTY(TARGET bag.video.puregl2 APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" )
SET_TARGET_PROPERTIES( bag.video.puregl2 PROPERTIES FOLDER "Video Renderer"   )


if( DIRECTX_SDK )
  set( PROJECT bag.video.pured3d2 )
  SET( DXINCLUDE ${DIRECTX_SDK}/include )
  if( __64__ )
    SET( LIBDIR ${DIRECTX_SDK}/Lib/x64 )
    SET( DXLIB ${DIRECTX_SDK}/Lib/x64 )
  else( __64__ )
    SET( LIBDIR ${DIRECTX_SDK}/Lib/x86 )
    SET( DXLIB ${DIRECTX_SDK}/Lib/x86 )
  endif( __64__ )

  sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d.cpp
        		${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
  target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} ${LIBDIR}/d3dx9.lib d3d9 )
  install_mode_dest( ${PROJECT} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${DXINCLUDE} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${DXLIB} )
  SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D_DRIVER;MAKE_RCOORD_SINGLE" )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"    )
  set_source_files_properties( ${COMMON_SOURCES} vidlib_win32.c keymap_win32.c PROPERTIES HEADER_FILE_ONLY TRUE)
  
  
  set( PROJECT bag.video.pured3d10 )
  sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d10.cpp
        		${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
  target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} d3d10_1 d3d10 )
  install_mode_dest( ${PROJECT} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${DXINCLUDE} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${DXLIB} )
  SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D10_DRIVER;MAKE_RCOORD_SINGLE;_INCLUDE_NLS" )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"    )
  set_source_files_properties( ${COMMON_SOURCES} vidlib_win32.c keymap_win32.c PROPERTIES HEADER_FILE_ONLY TRUE)
  
  if( WIN8_SDK OR MSVC11 )
    if( WIN8_SDK )
    INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/um )
    INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/shared )
    endif( WIN8_SDK )
    set( PROJECT bag.video.pured3d11 )
    sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d11.cpp
			${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
    target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} d3d11 )
    install_mode_dest( ${PROJECT} )
    if( WIN8_SDK )
    
    SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${WIN8_SDK}/include/um;${WIN8_SDK}/include/shared )
    if( __64__ )
      SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${WIN8_SDK}/lib/win8/um/x64 )
    else( __64__ )
      SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${WIN8_SDK}/lib/win8/um/x86 )
    endif( __64__ )
    endif( WIN8_SDK )
    SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D11_DRIVER;MAKE_RCOORD_SINGLE;_INCLUDE_NLS" )
    SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES FOLDER "Video Renderer"    )
    set_source_files_properties( ${COMMON_SOURCES} vidlib_win32.c keymap_win32.c PROPERTIES HEADER_FILE_ONLY TRUE)
  endif( WIN8_SDK OR MSVC11 )
  
endif( DIRECTX_SDK )
           
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


