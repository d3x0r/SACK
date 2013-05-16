
project( bag.video.puregl )

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )
#INCLUDE_DIRECTORIES( cg/include/cg )
# glsl ?

if( NOT __ANDROID__ )
  INCLUDE_DIRECTORIES( glew-1.9.0/include )
  add_subdirectory( glew-1.9.0 )
endif( NOT __ANDROID__ )

if( UNIX )
set( SOURCES key.c keydefs.c opengl.c vidlib_x11.c )
else( UNIX )
  if( NOT __ANDROID__ )
    add_definitions( -D__WINDOWS__ )
    set( SOURCES key.c ../keydefs.c opengl.c vidlib.c )
  else()
    set( SOURCES key.c ../keydefs.c opengl.c vidlib_android.c )
    set( GLUES_SOURCES glues/source/glues_project.c )
    set( SOURCES ${SOURCES} ${GLUES_SOURCES} )
  endif( NOT __ANDROID__ )
endif( UNIX )

add_library_force_source( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
     ${SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )

SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}${PROJECTNAME}${CMAKE_LIBRARY_SUFFIX}\";TARGET_LABEL=${TARGET_LABEL};_OPENGL_ENABLED;RENDER_LIBRARY_SOURCE;PURE_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" 
	FOLDER "Video Renderer"  
)

if( __ANDROID__ )
  my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} EGL GLESv2 GLESv1_CM )
  ADD_DEFINITIONS( -D__USE_FREEGLUT__ )
  include_directories( ${SOURCES_ROOT}src/freeglut/freeglut-SVN/include )
  my_target_link_libraries( ${PROJECT_NAME} freeglut m )
else( __ANDROID__ )
  if( UNIX )
    my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} GL GLU glew Xxf86vm )
  else( UNIX )
    my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} opengl32 glu32 glew )
  endif( UNIX )
endif( __ANDROID__ )

install_mode_dest( ${PROJECT_NAME} )

if( __64__ )
FILE( GLOB cgbin "cg/bin/*" )
else( __64__ )
FILE( GLOB cgbin "cg/bin/*" )
endif( __64__ )

#install( FILES ${cgbin} DESTINATION bin )
#install( FILES multishade.fx DESTINATION bin )

install( FILES glew-1.9.0/include/GL/glew.h DESTINATION include/SACK/GL )


