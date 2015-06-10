cmake_minimum_required(VERSION 2.8)

if( OFF )
if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )


include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
if( __ANDROID__ )
    INSTALL_SACK( lib )
else( __ANDROID__ )
    INSTALL_SACK( bin )
endif( __ANDROID__ )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

endif( OFF )

INCLUDE_DIRECTORIES( ../../../contrib/gsm-1.0-pl13/inc )

set( project audio_test )



set( SOURCES  audio_test.c )

add_program(${project} ${SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER "Audio Test Program"
)

target_link_libraries( ${project} gsm-1.0-pl13 )

install_default_dest( ${project}  ) 


