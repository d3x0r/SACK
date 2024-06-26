SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

set( CMAKE_INSTALL_MESSAGE "LAZY" )

set_property(GLOBAL PROPERTY USE_FOLDERS On)

cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\InterShell;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "SACK InterShell SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )

if( NOT SACK_INCLUDE_DIR )
	include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )
	include( ${INTERSHELL_SDK_ROOT_PATH}/share/SACK/build/InterShell/CMakePackage )
endif( NOT SACK_INCLUDE_DIR )


project( InterShell.Package )


if( NOT DATA_INSTALL_PREFIX )
	SET( DATA_INSTALL_PREFIX sack/SACK )
endif()

if( CMAKE_BUILD_TYPE MATCHES "Debug" )
  add_definitions( -D_DEBUG )
endif()

include_directories( ${SACK_INCLUDE_DIR} )
include_directories( ${INTERSHELL_INCLUDE_DIR} )

LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )


ADD_DEFINITIONS( -DCMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"
-DCPACK_PACKAGE_VERSION_PATCH=\"${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}\"
)


if( WIN32 )
  include( Win32Packaging.cmake )
endif( WIN32 )
