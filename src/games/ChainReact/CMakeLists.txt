cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( __ANDROID__ )
INSTALL_SACK( libs/${ANDROID_TARGET_CPU} )
else( __ANDROID__ )
INSTALL_SACK( bin )
endif( __ANDROID__ )

PROJECT( ChainReaction )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

# this is a bad thing to override.
set( BINARY_OUTPUT_DIR bin )
add_program( ${PROJECT_NAME} WIN32
${FIRST_GCC_PROGRAM_SOURCE} 
  chain.c logic.c player.c 
${LAST_GCC_PROGRAM_SOURCE} 
)


if( __ANDROID__ )
install( FILES 
	1atom.gif 357mag.wav atom2.gif atom2.png atom2-24.png grid1.jpg cursor.jpg plink.wav slwhist.wav
        	DESTINATION assets
                )
else( __ANDROID__ )
install( FILES 
	1atom.gif 357mag.wav atom2.gif atom2.png atom2-24.png grid1.jpg cursor.jpg plink.wav slwhist.wav
        	DESTINATION bin
                )
endif( __ANDROID__ )


