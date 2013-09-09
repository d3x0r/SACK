
set( project bag.image.pured3d )

ADD_DEFINITIONS( -D__3D__ )

set( font_sources font.cpp fntcache.cpp fntrender.cpp lucidaconsole.cpp )

add_library( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  blotscaled.cpp blotdirect.cpp image.cpp gifimage.cpp pngimage.cpp jpgimage.cpp 
  bmpimage.cpp line.cpp interface.cpp alphatab.cpp alphastab.cpp sprite.cpp sprite_common.cpp
  ${font_sources}
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_D3D_ENABLED;PURE_D3D_ENABLED;_D3D_DRIVER" 
        FOLDER "Image Library" 
)

if( __ANDROID__ )
target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
else( __ANDROID__ )
target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
endif( __ANDROID__ )

target_link_libraries( ${project} ${BAG_PSI_PLUSPLUS} )

install_mode_dest( ${project} )
