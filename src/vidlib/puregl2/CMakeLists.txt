
add_subdirectory( puregl2 )

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

ADD_DEFINITIONS( -D__3D__ )

if( __ANDROID__ )
ADD_DEFINITIONS( -D__ANDROID__ )
endif( __ANDROID__ )

OPTION( WIN8_TARGET "Enable directx 11 target (directcomposition also)" ${MSVC11} )

if( DIRECTX_SDK )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
set( COMMON_SOURCES
        keydefs.c    # key event binding to global keyboard input or specific displays
        key_state.c  # key state handling; is key up/down, delay times, toggles...
        vidlib_common.c  # create display surfaces, manage logical display properties
        vidlib_touch.c   # common handling for touch input
        vidlib_interface.c  # common interface export
        vidlib_3d_mouse.c  # mouse utilties; reverse map points into screen
        vidlib_render_loop.c  # dispach draw to all windows in a camera
		local.h # common data and routine declarations
   )     
  set_source_files_properties( ${COMMON_SOURCES} vidlib_win32.c keymap_win32.c PROPERTIES LANGUAGE CXX)
  set_source_files_properties( local.h PROPERTIES HEADER_FILE_ONLY ON )



  set( PROJECT bag.video.pured3d2 )
  SET( DXINCLUDE ${DIRECTX_SDK}/include )
  if( __64__ )
    SET( LIBDIR ${DIRECTX_SDK}/Lib/x64 )
    SET( DXLIB ${DIRECTX_SDK}/Lib/x64 )
  else( __64__ )
    SET( LIBDIR ${DIRECTX_SDK}/Lib/x86 )
    SET( DXLIB ${DIRECTX_SDK}/Lib/x86 )
  endif( __64__ )

  sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d.cpp
        		${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
  target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} ${LIBDIR}/d3dx9.lib d3d9 )
  install_mode_dest( ${PROJECT} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${DXINCLUDE} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${DXLIB} )
  SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D_DRIVER;MAKE_RCOORD_SINGLE" )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"    )
  
  
  set( PROJECT bag.video.pured3d10 )
  sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d10.cpp
        		${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
  target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} d3d10_1 d3d10 )
  install_mode_dest( ${PROJECT} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${DXINCLUDE} )
  SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${DXLIB} )
  SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D10_DRIVER;MAKE_RCOORD_SINGLE;_INCLUDE_NLS" )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"    )
  
  if( WIN8_SDK AND MSVC11 )
    if( WIN8_SDK )
    INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/um )
    INCLUDE_DIRECTORIES( ${WIN8_SDK}/include/shared )
    endif( WIN8_SDK )
    set( PROJECT bag.video.pured3d11 )
    sack_add_library( ${PROJECT} SHARED
       ${FIRST_GCC_LIBRARY_SOURCE}
            ${COMMON_CPP_SOURCES}
            d3d11.cpp
			${COMMON_SOURCES} vidlib_win32.c keymap_win32.c
       ${LAST_GCC_LIBRARY_SOURCE}
     )
	
	# if NTDDI_VERSION>=0x06020000 /* NTDDI_WIN8 */
	# if building for win8 target
	# ;NTDDI_VERSION=0x06020000
	if( WIN8_TARGET )
	   SET( MOREFLAGS NTDDI_VERSION=0x06020000 )
	   set( MORELIBS dcomp )
	endif( WIN8_TARGET )
    target_link_libraries( ${PROJECT} bag.image.pured3d ${SACK_CORE_LIBRARY} d3d11 DXGI d2d1 ${MORELIBS} Dwmapi )
    install_mode_dest( ${PROJECT} )
    if( WIN8_SDK )
    

    SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY INCLUDE_DIRECTORIES ${WIN8_SDK}/include/um;${WIN8_SDK}/include/shared )
    if( __64__ )
      SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${WIN8_SDK}/lib/win8/um/x64 )
    else( __64__ )
      SET_PROPERTY( TARGET ${PROJECT} APPEND PROPERTY LINK_DIRECTORIES ${WIN8_SDK}/lib/win8/um/x86 )
    endif( __64__ )
    endif( WIN8_SDK )
    SET_PROPERTY(TARGET ${PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_D3D11_DRIVER;MAKE_RCOORD_SINGLE;_INCLUDE_NLS;${MOREFLAGS}" )
    SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES FOLDER "Video Renderer"    )
  endif( WIN8_SDK AND MSVC11 )
  
endif( DIRECTX_SDK )
           
