cmake_minimum_required(VERSION 3.20)

PROJECT( Automaton )

set( CMAKE_INSTALL_MESSAGE "LAZY" )

if( NOT SACK_SDK_ROOT_PATH )
  GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
  set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

if( NOT SACK_CORE_LIBRARY )
  include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )
  if( NOT BUILD_INTO_SACK_INSTALL )
    INSTALL_SACK( bin lib )
  endif( NOT BUILD_INTO_SACK_INSTALL )
endif()
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )


add_subdirectory( brain )
include_directories( brain )
add_subdirectory( board )
#add_subdirectory( automaton )
#add_subdirectory( breadboard )
