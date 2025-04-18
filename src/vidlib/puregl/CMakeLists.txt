

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )
#INCLUDE_DIRECTORIES( cg/include/cg )
# glsl ?

if( WIN32 OR NEED_GLEW )
  INCLUDE_DIRECTORIES( glew-1.9.0/include )
  add_subdirectory( glew-1.9.0 )
endif( WIN32 OR NEED_GLEW )

if( __ANDROID__ OR __QNX__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ OR __QNX__ )

ADD_DEFINITIONS( -D__3D__ )

if( __ANDROID__ )
ADD_DEFINITIONS( -D__ANDROID__ )
endif( __ANDROID__ )

if( UNIX )
   # something more like IF USE_EGL
   if( __ANDROID__ OR __QNX__ )
      set( SOURCES key.c keydefs.c opengl.c vidlib_android.c keymap_android.c )
   else()
      set( SOURCES key.c keydefs.c opengl.c vidlib_x11.c )
   endif()
else( UNIX )
  if( NOT __ANDROID__ AND NOT __QNX__ )
    add_definitions( -D__WINDOWS__ )
    set( SOURCES key.c ../keydefs.c opengl.c vidlib.c )
  else()
    set( SOURCES key.c ../keydefs.c opengl.c vidlib_android.c keymap_android.c )
    set( GLUES_SOURCES glues/source/glues_project.c )
    set( SOURCES ${SOURCES} ${GLUES_SOURCES} )
  endif( NOT __ANDROID__ AND NOT __QNX__ )
endif( UNIX )
if( __NO_ODBC__ OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( __NO_ODBC__ OR NOT USE_OPTIONS )

add_library_force_source( bag.video.puregl SHARED
${FIRST_GCC_LIBRARY_SOURCE}
     ${SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

string( REPLACE "." "_" TARGET_LABEL bag.video.puregl )

SET_TARGET_PROPERTIES( bag.video.puregl PROPERTIES
	COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}${PROJECTNAME}${CMAKE_LIBRARY_SUFFIX}\";TARGET_LABEL=${TARGET_LABEL};_OPENGL_ENABLED;RENDER_LIBRARY_SOURCE;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE"
	FOLDER "Video Renderer" )


if( __ANDROID__ OR __QNX__ )
  # glOrthof is GLES1 and is used by 'glues' project
  my_target_link_libraries( bag.video.puregl ${SACK_CORE_LIBRARY} EGL GLESv2 GLESv1_CM android log )
  #DD_DEFINITIONS( -D__USE_FREEGLUT__ )
  #nclude_directories( ${SOURCES_ROOT}src/freeglut/freeglut-SVN/include )
  if( NOT __QNX__ )
  my_target_link_libraries( bag.video.puregl  m )
  endif( NOT __QNX__ )
else( __ANDROID__ OR __QNX__ )
    my_target_link_libraries( bag.video.puregl ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )
endif( __ANDROID__ OR __QNX__ )

install_mode_dest( bag.video.puregl )

if( __64__ )
FILE( GLOB cgbin "cg/bin/*" )
else( __64__ )
FILE( GLOB cgbin "cg/bin/*" )
endif( __64__ )

#install( FILES ${cgbin} DESTINATION bin )
#install( FILES multishade.fx DESTINATION bin )

if( NOT __ANDROID__ OR NEED_GLEW )
   install( FILES glew-1.9.0/include/GL/glew.h DESTINATION include/SACK/GL )
endif( NOT __ANDROID__ OR NEED_GLEW )


