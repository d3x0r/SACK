

set( project bag.image.puregl )


set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole2.c )

if( WIN32 )
    INCLUDE_DIRECTORIES( ../../vidlib/puregl/glew-1.9.0/include )
else( WIN32 )
   
if( __ANDROID__ OR QNX )
	ADD_DEFINITIONS( -DUSE_GLES )
endif( __ANDROID__ OR QNX )

endif( WIN32 )
if( __NO_ODBC__ OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( __NO_ODBC__ OR NOT USE_OPTIONS )

add_library_force_source( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  blotscaled.c blotdirect.c image.c ../image_common.c 
  ../gifimage.c ${IMAGE_SOURCES_PNG_DECODER} ../jpgimage.c 
  ../bmpimage.c line.c interface.c ../alphatab.c ../alphastab.c sprite.c ../sprite_common.c
  ${font_sources}
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};__3D__;_OPENGL_ENABLED;PURE_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" 
        FOLDER "Image Library" 
)

target_link_libraries( ${project} ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )

target_link_libraries( ${project} bag.psi )
if( WIN32 )
target_link_libraries( ${project} glu32 glew ) # private glew library build; from vidlib/puregl/glew-1.9.0
else()
target_link_libraries( ${project} GLEW ) # private glew library build; from vidlib/puregl/glew-1.9.0
endif( WIN32 )



install_mode_dest( ${project} )

