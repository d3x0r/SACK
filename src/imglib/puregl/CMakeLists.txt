

set( project bag.image.puregl )


ADD_DEFINITIONS( -D__3D__ )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole.c )

if( NOT __ANDROID__ )
  INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
endif( NOT __ANDROID__ )

add_library_force_source( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  blotscaled.c blotdirect.c image.c ../image_common.c 
  ../gifimage.c ../pngimage.c ../jpgimage.c 
  ../bmpimage.c line.c interface.c ../alphatab.c ../alphastab.c sprite.c sprite_common.c
  ${font_sources}
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_OPENGL_ENABLED;PURE_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" 
        FOLDER "Image Library" 
)

target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
if( __ANDROID__ )
else( __ANDROID__ )
if( UNIX )
  target_link_libraries( ${project} GL glew )
  if( NOT NEED_PNG ) 
    target_link_libraries( ${project} png )
  endif() 
  if( NOT NEED_JPEG ) 
    target_link_libraries( ${project} jpeg )
  endif() 
  if( NOT NEED_FREETYPE )
    target_link_libraries( ${project} freetype  )
  endif()
else( UNIX )
target_link_libraries( ${project} opengl32 glew )
endif( UNIX )
endif( __ANDROID__ )

target_link_libraries( ${project} bag.psi )

install_mode_dest( ${project} )
