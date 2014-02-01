cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

INSTALL_SACK( bin )

PROJECT( bag.video.proxy )

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

set( PROJECT bag.video.proxy.server )
sack_add_library( ${PROJECT} SHARED server.c imagepng.c )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT} )

set( PROJECT bag.video.proxy.client )
add_program( ${PROJECT} WIN32 client.c )
install_default_dest( ${PROJECT} )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )

