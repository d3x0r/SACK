
set( project bag.image.pured3d )

ADD_DEFINITIONS( -D__3D__ )

set( COMMON_CPP_SOURCES   blotscaled.cpp blotdirect.cpp image.d3d.cpp 
  line.cpp interface.cpp sprite.cpp )

set( font_sources 
	../alphatab.c 
	../alphastab.c 
	../font.c 
        ../fntcache.c 
        ../fntrender.c 
        ../lucidaconsole.c 
        ../pngimage.c 
        ../bmpimage.c 
        ../gifimage.c 
        ../jpgimage.c 
        ../image_common.c
        ../sprite_common.c
)

set( font_headers 
        ../pngimage.h 
        ../bmpimage.h 
        ../gifimage.h 
        ../jpgimage.h 
        ../fntglobal.h 
        ../cache.h 
        ../blotproto.h 
        local.h
        ../image_common.h
        ../sprite_local.h
)

sack_add_library( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  ${COMMON_CPP_SOURCES}
  ${font_sources}
  ${font_headers}
${LAST_GCC_LIBRARY_SOURCE}
)


SET_SOURCE_FILES_PROPERTIES( ${font_sources} PROPERTIES LANGUAGE CXX )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY  COMPILE_DEFINITIONS "_D3D_DRIVER" )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER "Image Library" )

if( NOT FORCE_CXX )
  # if forced to cxx, don't need extra libraries.
  target_link_libraries( ${project} ${BAG_PSI_PLUSPLUS} )
endif( NOT FORCE_CXX )
install_mode_dest( ${project} )
