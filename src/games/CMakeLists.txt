cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Set build type")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

PROJECT( Games )

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SACK;Install_Dir]" ABSOLUTE CACHE)
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\InterShell;Install_Dir]" ABSOLUTE CACHE)

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
include( ${INTERSHELL_SDK_ROOT_PATH}/CMakePackage )

INSTALL_INTERSHELL( . )
INSTALL_SACK( bin )

include_directories( ${SACK_INCLUDE_DIR} )
include_directories( ${INTERSHELL_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )


IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX
    ${SACK_BASE} CACHE PATH "Project install prefix; defaults to sack installation" FORCE
    )
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)



if( CMAKE_BUILD_TYPE MATCHES "Debug" )
  add_definitions( -D_DEBUG )
endif()

if( NOT MINGW )
# gcc pukes on exported vtable references.
add_subdirectory( automaton )
endif( NOT MINGW )
add_subdirectory( virtuality )
add_subdirectory( flatland )
add_subdirectory( ChainReact )
add_subdirectory( stock_market )
add_subdirectory( train )
add_subdirectory( router )
add_subdirectory( seg )
add_subdirectory( ball_display )
if( __BULLET_ENABLED__ )
add_subdirectory( ball_mixer )
endif( __BULLET_ENABLED__ )
