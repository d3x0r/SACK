

set( project bag.image.puregl )


ADD_DEFINITIONS( -D__3D__ )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole.c )

if( WIN32 )
    INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
else( WIN32 )
    if( ANDROID OR QNX )
	ADD_DEFINITIONS( -DUSE_GLES )
    endif( ANDROID OR QNX )

endif( WIN32 )

add_library_force_source( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  blotscaled.c blotdirect.c image.c ../image_common.c 
  ../gifimage.c ../pngimage.c ../jpgimage.c 
  ../bmpimage.c line.c interface.c ../alphatab.c ../alphastab.c sprite.c ../sprite_common.c
  ${font_sources}
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_OPENGL_ENABLED;PURE_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" 
        FOLDER "Image Library" 
)

target_link_libraries( ${project} ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )

target_link_libraries( ${project} bag.psi )
if( WIN32 )
target_link_libraries( ${project} glu32 glew ) # private glew library build; from vidlib/puregl/glew-1.9.0
endif( WIN32 )



install_mode_dest( ${project} )

