
project( bag.image.puregl2 )

if( NOT __ANDROID__ )
  INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
endif( NOT __ANDROID__ )

if( __ANDROID__ OR __QNX__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ OR __QNX__ )

ADD_DEFINITIONS( -D__3D__ )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole.c )
set( loader_sources ../bmpimage.c ../gifimage.c ../pngimage.c ../jpgimage.c )

add_library_force_source( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  ../alphatab.c ../alphastab.c
  blotscaled.c blotdirect.c image.c  
  line.c interface.c sprite.c sprite_common.c
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

my_target_link_libraries( ${PROJECT_NAME} ${SACK_CORE_LIBRARY} )
if( __ANDROID__ OR __QNX__ )
  my_target_link_libraries( ${PROJECT_NAME} GLESv2 )
else( __ANDROID__ OR __QNX__ )
if( UNIX )
  #my_target_link_libraries( ${PROJECT_NAME} GLESv2 EGL GL )
  if( NOT NEED_PNG ) 
    my_target_link_libraries( ${PROJECT_NAME} png )
  endif() 
  if( NOT NEED_JPEG ) 
    my_target_link_libraries( ${PROJECT_NAME} jpeg )
  endif() 
  if( NOT NEED_FREETYPE )
    my_target_link_libraries( ${PROJECT_NAME} freetype  )
  endif()
else( UNIX )
my_target_link_libraries( ${PROJECT_NAME} opengl32 glu32 )
my_target_link_libraries( ${PROJECT_NAME} glew )
endif( UNIX )
endif( __ANDROID__ OR __QNX__ )

my_target_link_libraries( ${PROJECT_NAME} bag.psi )

install_mode_dest( ${PROJECT_NAME} )
