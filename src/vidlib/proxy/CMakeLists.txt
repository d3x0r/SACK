#cmake_minimum_required(VERSION 2.8)

if( NOT MAKING_SACK_CORE )
if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

INSTALL_SACK( ${CMAKE_INSTALL_PREFIX} )
include_directories( ${SACK_INCLUDE_DIR}/zlib )
include_directories( ${SACK_INCLUDE_DIR}/png )
endif( NOT MAKING_SACK_CORE )


PROJECT( bag.video.proxy )

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

set( PROJECT bag.video.proxy.server )
if( UNIX )
set( MORE_SOURCES keymap_linux.c )
endif( UNIX )
sack_add_library( ${PROJECT} SHARED ${FIRST_GCC_LIBRARY_SOURCE} server.c ${IMAGE_SOURCES_PNG_DECODER} ${MORE_SOURCES} ${LAST_GCC_LIBRARY_SOURCE})
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

set_source_files_properties( ${IMAGE_SOURCES_PNG_DECODER} PROPERTIES COMPILE_DEFINITIONS "USE_API_ALIAS_PREFIX=VidlibProxy_" )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"   
       COMPILE_DEFINITIONS "TARGET_LABEL=bag_video_proxy_server"
   )

set( PROJECT bag.video.proxy.instance.server )
if( UNIX )
set( MORE_SOURCES keymap_linux.c )
endif( UNIX )
sack_add_library( ${PROJECT} SHARED ${FIRST_GCC_LIBRARY_SOURCE} instance_server.c ${MORE_SOURCES} ${LAST_GCC_LIBRARY_SOURCE})
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} bag.image )
install_default_dest( ${PROJECT} )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"   
       COMPILE_DEFINITIONS TARGET_LABEL=bag_video_proxy_instance_server 
   )
   
set( PROJECT bag.video.proxy.client )
add_program( ${PROJECT} WIN32 client.c )
install_default_dest( ${PROJECT} )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
  SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES	FOLDER "Video Renderer"    )

#install(DIRECTORY webclient DESTINATION bin )

