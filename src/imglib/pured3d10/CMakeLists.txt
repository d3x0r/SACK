
set( project bag.image.pured3d10 )

ADD_DEFINITIONS( -D__3D__ )
INCLUDE_DIRECTORIES( ${DIRECTX_SDK}/include )
LINK_DIRECTORIES( ${DIRECTX_SDK}/lib/${TARGET_ARCH} )

set( COMMON_CPP_SOURCES   blotscaled.cpp blotdirect.cpp image.d3d10.cpp 
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
        ../sprite_common.c
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
        shaders.h
        ../image_common.c
        ../image_common.h
        ../sprite_local.h
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
        COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_D3D10_DRIVER" 
        FOLDER "Image Library" 
)

if( __ANDROID__ )
target_link_libraries( ${project} ${SACK_CORE_LIBRARY} )
else( __ANDROID__ )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
endif( __ANDROID__ )

if( NOT FORCE_CXX )
  # if forced to cxx, don't need extra libraries.
  target_link_libraries( ${project} ${BAG_PSI_PLUSPLUS} )
endif( NOT FORCE_CXX )
target_link_libraries( ${project} d3dcompiler )

install_mode_dest( ${project} )

if( MSVC11 )
  if( __64__ )
    install( FILES redist.vs11/D3D/x64/d3dcompiler_46.dll DESTINATION bin )
  else( __64__ )
    install( FILES redist.vs11/D3D/x86/d3dcompiler_46.dll DESTINATION bin )
  endif( __64__ )
endif( MSVC11 )
