
SET( SHADERC_SKIP_TESTS ON )
SET( SHADERC_SKIP_INSTALL ON )
SET( SPIRV_SKIP_EXECUTABLES ON )
SET( SKIP_GLSLANG_INSTALL ON )
SET( ENABLE_GLSLANG_BINARIES OFF )


if( WIN32 )
	#GET_FILENAME_COMPONENT(VULKAN_API "[HKEY_LOCAL_MACHINE\\SOFTWARE\\LunarG\\VulkanSDK;VK_SDK_PATHs]" ABSOLUTE CACHE)
	set( SHADERC_ENABLE_SHARED_CRT ON )
	set( VULKAN_API $ENV{VULKAN_SDK} )
	message( "IMGLIB VULKAN FROM ENVIRONMENT:" ${VULKAN_API} )
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

	add_subdirectory( ${VULKAN_API}/shaderc shaderc )
else(WIN32)
	set( VULKAN_API /usr/include )
	add_subdirectory( ${CMAKE_SOURCE_DIR}/../shaderc shaderc )
endif( WIN32 )


set( project bag.image.vulkan )

ADD_DEFINITIONS( -D__3D__ )
ADD_DEFINITIONS( -D_VULKAN_DRIVER )

set( font_sources ../font.c ../fntcache.c ../fntrender.c ../lucidaconsole2.c )
set( loader_sources ../bmpimage.c ../gifimage.c ../pngimage.c ../jpgimage.c )
set( sprite_sources ../sprite_common.c )

sack_add_library( ${project} SHARED
  ../alphatab.c 
  ../alphastab.c
  blotscaled.c blotdirect.c image.c ../image_common.c 
  line.c interface.c sprite.c ${sprite_sources}
  ${loader_sources}
  ${font_sources}
  shader simple_shader simple_texture_shader
  simple_multi_shaded_texture_shader.c
)

SET_TARGET_PROPERTIES(${project} PROPERTIES 
        FOLDER "Image Library" 
)

my_target_link_libraries( ${project} ${SACK_CORE_LIBRARY} ${SACK_PLATFORM_LIBRARIES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "IMAGE_LIBRARY_SOURCE;_VULKAN_DRIVER;__3D__;MAKE_RCOORD_SINGLE" )

if( __LINUX__ OR __ANDROID__ )
    set( VULKAN_SHADERC shaderc_combined )
else()
    set( VULKAN_SHADERC shaderc )
endif()

if( WIN32 )
	my_target_link_libraries( ${project} vulkan-1.lib shaderc )
else()
	my_target_link_libraries( ${project} vulkan shaderc )
endif()

#my_target_link_libraries( ${project} bag.psi )

install_mode_dest( ${project} )
