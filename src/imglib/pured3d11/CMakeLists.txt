
set( project bag.image.pured3d11 )

ADD_DEFINITIONS( -D__3D__ )

    if( WIN8_SDK )
		INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/um )
		INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/shared )
		LINK_DIRECTORIES( ${WIN8_SDK}/lib/win8/um/${TARGET_ARCH} )
    else( WIN8_SDK )
		INCLUDE_DIRECTORIES( ${DIRECTX_SDK}/include )
		LINK_DIRECTORIES( ${DIRECTX_SDK}/lib/${TARGET_ARCH} )
    endif( WIN8_SDK )

set( COMMON_CPP_SOURCES   blotscaled.cpp blotdirect.cpp image.d3d11.cpp 
  line.cpp interface.cpp sprite.cpp 
  shader.cpp simple_shader.cpp simple_texture_shader.cpp simple_multi_shaded_texture_shader.cpp
  )

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
SET_PROPERTY(TARGET ${project} APPEND PROPERTY  COMPILE_DEFINITIONS "_D3D11_DRIVER" )
SET_TARGET_PROPERTIES(${project} PROPERTIES  FOLDER "Image Library" )

target_link_libraries( ${project} ${BAG_PSI_PLUSPLUS} d3d11 D3dcompiler )
install_mode_dest( ${project} )

install( FILES D3D/${TARGET_ARCH}/d3dcompiler_44.dll DESTINATION bin )

