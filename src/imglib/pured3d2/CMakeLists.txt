
set( project bag.image.pured3d2 )

ADD_DEFINITIONS( -D__3D__ )

set( COMMON_CPP_SOURCES   blotscaled.cpp blotdirect.cpp image.d3d.cpp 
  line.cpp interface.cpp sprite.cpp sprite_common.cpp )

set( font_sources 
	../alphatab.c 
	../alphastab.c 
	../font.c 
        ../fntcache.c 
        ../fntrender.c 
        ../lucidaconsole.c 
        ../pngimage.c 
        ../pngimage.h 
        ../bmpimage.c 
        ../bmpimage.h 
        ../gifimage.c 
        ../gifimage.h 
        ../jpgimage.c 
        ../jpgimage.h 
        ../fntglobal.h 
        ../cache.h 
        ../blotproto.h 
        local.h
        ../image_common.c
        ../image_common.h
)

foreach( SOURCE ${font_sources} )
   if( ${SOURCE} MATCHES "(.*)/(.*)\\.c$" )
      set( BASENAME ${CMAKE_MATCH_2} )
      EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${project}/${BASENAME}.cpp )
      set( COMMON_CPP_SOURCES ${COMMON_CPP_SOURCES} ${CMAKE_BINARY_DIR}/${project}/${BASENAME}.cpp )
   elseif( ${SOURCE} MATCHES "(.*)/(.*)\\.h$" )
      set( BASENAME ${CMAKE_MATCH_2} )
      EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${project}/${BASENAME}.h )
   elseif( ${SOURCE} MATCHES "(.*)\\.h$" )
      set( BASENAME ${CMAKE_MATCH_1} )
      EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${project}/${BASENAME}.h )
   endif()

endforeach()
add_library( ${project} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  ${COMMON_CPP_SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_D3D_DRIVER" 
        FOLDER "Image Library" 
)

if( __ANDROID__ )
target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
else( __ANDROID__ )
target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
endif( __ANDROID__ )

target_link_libraries( ${project} ${BAG_PSI_PLUSPLUS} )

install_mode_dest( ${project} )
