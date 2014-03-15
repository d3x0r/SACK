
project( bag.image.puregl2 )

if( WIN32 )
  INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
endif( WIN32 )

if( __ANDROID__ OR __QNX__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ OR __QNX__ )

ADD_DEFINITIONS( -D__3D__ )

if( __NO_ODBC__ OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( __NO_ODBC__ OR NOT USE_OPTIONS )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole.c )
set( loader_sources ../bmpimage.c ../gifimage.c ../pngimage.c ../jpgimage.c )
set( sprite_sources ../sprite_common.c )

add_library_force_source( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  ../alphatab.c ../alphastab.c
  blotscaled.c blotdirect.c image.c ../image_common.c 
  line.c interface.c sprite.c ${sprite_sources}
  ${loader_sources}
  ${font_sources}
  shader simple_shader simple_texture_shader
  simple_multi_shaded_texture_shader.c
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_OPENGL_DRIVER;MAKE_RCOORD_SINGLE;PURE_OPENGL2_ENABLED" 
        FOLDER "Image Library" 
)

my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )
if( WIN32 )
my_target_link_libraries( ${PROJECT_NAME} glew glu32 )  # private GLEW library
endif( WIN32 )
if( __ANDROID__ OR __QNX__ )
  my_target_link_libraries( ${PROJECT_NAME} GLESv2 )
endif( __ANDROID__ OR __QNX__ )

my_target_link_libraries( ${PROJECT_NAME} bag.psi )

install_mode_dest( ${PROJECT_NAME} )
