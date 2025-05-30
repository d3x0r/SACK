
project( bag.image.puregl2 )

if( WIN32 )
  INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
endif( WIN32 )

if( __ANDROID__ OR __QNX__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ OR __QNX__ )


if( ( __NO_ODBC__ AND NOT USE_SQLITE ) OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( ( __NO_ODBC__ AND NOT USE_SQLITE ) OR NOT USE_OPTIONS )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole2.c )
set( loader_sources ../bmpimage.c ../gifimage.c ${IMAGE_SOURCES_PNG_DECODER} ../jpgimage.c )
set( sprite_sources ../sprite_common.c )

if( EMSCRIPTEN )
   set( ALPHA_TABLES   )
else( EMSCRIPTEN )
    set( ALPHA_TABLES   ../alphatab.c 
      ../alphastab.c )
endif( EMSCRIPTEN )


add_library( ${PROJECT_NAME} ${CORE_LINKMODE}
${FIRST_GCC_LIBRARY_SOURCE}
  ../alphatab.c 
  ../alphastab.c
  blotscaled.c blotdirect.c image.c ../image_common.c 
  line.c interface.c sprite.c ${sprite_sources}
  ${loader_sources}
  ${font_sources}
  shader.c simple_shader.c simple_texture_shader.c
  simple_multi_shaded_texture_shader.c
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};__3D__;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE;PURE_OPENGL2_ENABLED" 
        FOLDER "Image Library" 
)

my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )
if( WIN32 )
my_target_link_libraries( ${PROJECT_NAME} glu32 )  # private GLEW library
my_target_link_libraries( ${PROJECT_NAME} glew )  # private GLEW library
else(WIN32)
my_target_link_libraries( ${PROJECT_NAME} GLEW )  # private GLEW library
endif( WIN32 )


if( __ANDROID__ OR __QNX__ )
  my_target_link_libraries( ${PROJECT_NAME} GLESv2 )
endif( __ANDROID__ OR __QNX__ )

my_target_link_libraries( ${PROJECT_NAME} bag.psi )

install_mode_dest( ${PROJECT_NAME} )
