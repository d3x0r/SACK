cmake_minimum_required(VERSION 3.20)

set( CMAKE_INSTALL_MESSAGE "LAZY" )

if( NOT MAKING_SACK_CORE )
  if( NOT SACK_SDK_ROOT_PATH )
    GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
  else( NOT SACK_SDK_ROOT_PATH )
    set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
  endif( NOT SACK_SDK_ROOT_PATH )

  if( NOT INTERSHELL_SDK_ROOT_PATH )
    GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\InterShell;Install_Dir]" ABSOLUTE CACHE)
  else( NOT INTERSHELL_SDK_ROOT_PATH )
    set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
  endif( NOT INTERSHELL_SDK_ROOT_PATH )

  include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )
  include( ${INTERSHELL_SDK_ROOT_PATH}/share/SACK/build/InterShell/CMakePackage )

  INSTALL_SACK( bin )
  INSTALL_INTERSHELL( . )

  include_directories( ${SACK_INCLUDE_DIR} )
  LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
  include_directories( ${INTERSHELL_INCLUDE_DIR} )
  LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )

endif( NOT MAKING_SACK_CORE )

project( router )

add_subdirectory( board )
if( NOT MAKING_SACK_CORE )
  add_subdirectory( netobjects )
  add_subdirectory( loader )
  Install( FILES router.peice.txt DESTINATION ${DATA_INSTALL_PREFIX} )
endif( NOT MAKING_SACK_CORE )
