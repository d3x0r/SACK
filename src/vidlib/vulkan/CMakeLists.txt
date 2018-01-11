
if( WIN32 )
	#GET_FILENAME_COMPONENT(VULKAN_API "[HKEY_LOCAL_MACHINE\\SOFTWARE\\LunarG\\VulkanSDK;VK_SDK_PATHs]" ABSOLUTE CACHE)
	set( VULKAN_API $ENV{VULKAN_SDK} )
	set( SHADERC_ENABLE_SHARED_CRT ON )
	message( "VIDLIB VULKAN FROM ENVIRONMENT:" ${VULKAN_API} )
	if( NOT VULKAN_API )
		set( VULKAN_API c:/tools/vulkansdk/1.0.61.1 )
	endif( NOT VULKAN_API )
	STRING( REPLACE ";" "" VULKAN_API ${VULKAN_API})
	if( NOT VULKAN_API )
		message( FATAL_ERROR "Environment 'VULKAN_API' has not been set" )
	endif( NOT VULKAN_API )
	message( "Vulkan API found at ${VULKAN_API}" )
        
	INCLUDE_DIRECTORIES( ${VULKAN_API}/Include )
	if( __64__ )
	   LINK_DIRECTORIES( ${VULKAN_API}/Source/lib )
	else()
	   LINK_DIRECTORIES( ${VULKAN_API}/Source/lib32 )
	endif()

else(WIN32)
endif( WIN32 )


ADD_DEFINITIONS( -D__3D__ )
if( ( __NO_ODBC__ AND NOT USE_SQLITE ) OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( ( __NO_ODBC__ AND NOT USE_SQLITE ) OR NOT USE_OPTIONS )

if( __ANDROID__ )
    ADD_DEFINITIONS( -D__ANDROID__ )
endif( __ANDROID__ )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
set( COMMON_SOURCES
        keydefs.c    # key event binding to global keyboard input or specific displays
        key_state.c  # key state handling; is key up/down, delay times, toggles...
        vidlib_common.c  # create display surfaces, manage logical display properties
        vidlib_touch.c   # common handling for touch input
        vidlib_interface.c  # common interface export
        vidlib_3d_mouse.c  # mouse utilties; reverse map points into screen
        vidlib_render_loop.c  # dispach draw to all windows in a camera
   )     

if( __LINUX__ )
   # something more like IF USE_EGL
   if( __ANDROID__ OR __QNX__ )
      set( SOURCES ${COMMON_SOURCES} vidlib_android.c keymap_android.c vidlib_egl.c vidlib_gles.c )
   else()
      ADD_DEFINITIONS( -DUSE_X11 )
      set( SOURCES ${COMMON_SOURCES} vulkan_interface.c keymap_linux.c vidlib_x11.c )
   endif()
else( __LINUX__ )
    set( SOURCES ${COMMON_SOURCES} keymap_win32.c vidlib_win32.c vulkan_interface.c )
endif( __LINUX__ )

sack_add_library( bag.video.vulkan SHARED
     ${SOURCES}
)

# _OPENGL_ENABLED flag controls member additions to the vidlib structure; HRC for instance
SET_PROPERTY(TARGET bag.video.vulkan APPEND PROPERTY COMPILE_DEFINITIONS "RENDER_LIBRARY_SOURCE;_OPENGL_ENABLED;_OPENGL_DRIVER;MAKE_RCOORD_SINGLE" )
SET_TARGET_PROPERTIES( bag.video.vulkan PROPERTIES FOLDER "Video Renderer"   )

if( WIN32 )
  my_target_link_libraries( bag.video.vulkan  vulkan-1 )
else()
  my_target_link_libraries( bag.video.vulkan  vulkan )
endif()

 my_target_link_libraries( bag.video.vulkan ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )

install_mode_dest( bag.video.vulkan )


