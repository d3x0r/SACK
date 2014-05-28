
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

PROJECT( stock_market )

#ADD_DEFINITIONS( -DVIRTUALITY_LIBRARY_SOURCE ) 
include_directories( "../include" )

add_program( ${PROJECT_NAME} 
${FIRST_GCC_PROGRAM_SOURCE} 
   board.c global.c input.c main.c player.c stocks.c
${LAST_GCC_PROGRAM_SOURCE}
)

message( "project ${PROJECT_NAME} needs better install path" )
install( FILES 
	Board.Data Stocks.Data
        	DESTINATION bin
                )
