
project( bag.video.pured3d )

#if( NOT DIRECTX_SDK )
#set(DIRECTX_SDK "e:/dx9-jun2010" CACHE STRING "directx sdk path please") 
#endif( NOT DIRECTX_SDK )

INCLUDE_DIRECTORIES( ${DIRECTX_SDK}/include )

add_library( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE}
  key.cpp keydefs.cpp d3d.cpp vidlib.cpp
${LAST_GCC_LIBRARY_SOURCE}
)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )

SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}${PROJECTNAME}${CMAKE_LIBRARY_SUFFIX}\";TARGET_LABEL=${TARGET_LABEL};_D3D_ENABLED;RENDER_LIBRARY_SOURCE;PURE_D3D_ENABLED;_D3D_DRIVER" 
	FOLDER "Video Renderer"  
)

if( __64__ )
SET( LIBDIR ${DIRECTX_SDK}/Lib/x64 )
LINK_DIRECTORIES( ${DIRECTX_SDK}/Lib/x64 )
else( __64__ )
SET( LIBDIR ${DIRECTX_SDK}/Lib/x86 )
LINK_DIRECTORIES( ${DIRECTX_SDK}/Lib/x86 )
endif( __64__ )

if( __ANDROID__ )
target_link_libraries( ${PROJECT_NAME} bag.image.pured3d ${SACK_CORE_LIBRARY} ${LIBDIR}/d3dx9.lib d3d9 )
else( __ANDROID__ )
target_link_libraries( ${PROJECT_NAME} bag.image.pured3d ${SACK_CORE_LIBRARY} ${LIBDIR}/d3dx9.lib d3d9 )
endif( __ANDROID__ )

install_mode_dest( ${PROJECT_NAME} )



