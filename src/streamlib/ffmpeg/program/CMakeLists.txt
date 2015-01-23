cmake_minimum_required(VERSION 2.8)


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



set( project video_player )



set( SOURCES
       ffmpeg.c control_panel.c
)

add_program(${project} WIN32 ${SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER "Video Player"
)

install_default_dest( ${project}  ) 


