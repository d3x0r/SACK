cmake_minimum_required(VERSION 2.8)

PROJECT( GameServer )
if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

#if( NOT SACK_BINARY_SDK_ROOT_PATH )
#GET_FILENAME_COMPONENT(SACK_BINARY_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Sack.Binaries;Install_Dir]" ABSOLUTE CACHE)
#else( NOT SACK_BINARY_SDK_ROOT_PATH )
#set(SACK_BINARY_SDK_ROOT_PATH ${SACK_BINARY_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
#endif( NOT SACK_BINARY_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
#include( ${SACK_BINARY_SDK_ROOT_PATH}/CMakePackage )

INSTALL_SACK( bin )
#INSTALL_SACK_BINARY( bag.msg.core.service bin )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )


###################### Project Defintion ###########
set( project ${PROJECT_NAME} )
sack_add_program( ${project} game_server.c )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )


