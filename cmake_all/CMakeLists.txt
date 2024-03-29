if( ${CMAKE_VERSION} VERSION_GREATER "3.11.0" )
cmake_policy( SET CMP0077 NEW )
endif( ${CMAKE_VERSION} VERSION_GREATER "3.11.0" )
set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 3.20)

set(CMAKE_BUILD_TYPE "debug" CACHE STRING "Set build type")

set( CMAKE_BUILD_RPATH_USE_ORIGIN ON )
set( CMAKE_BUILD_WITH_INSTALL_RPATH ON )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF )
set( CMAKE_SKIP_INSTALL_RPATH OFF )

if( NOT PROJECT_NAME )
project( Sack.all )
endif( NOT PROJECT_NAME )

# include this so default output paths get defined.
include( ${CMAKE_CURRENT_SOURCE_DIR}/../makefiles/cmake/DefaultInstall.cmake )

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
			${CMAKE_CONFIGURATION_TYPES} Debug Release RelWithDebInfo MinSizeRel )

include( ${CMAKE_CURRENT_LIST_DIR}/CMakeExtBuild.txt )

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Android" )
	DEFINE_DEFAULT( __ANDROID__ ON )
else( CMAKE_HOST_SYSTEM_NAME STREQUAL "Android")
	DEFINE_DEFAULT( __ANDROID__ OFF )
endif( CMAKE_HOST_SYSTEM_NAME STREQUAL "Android" )

if( NOT TARGET_INSTALL_PREFIX )
  SET(TARGET_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING  "Used to compile as installation path" )
endif( NOT TARGET_INSTALL_PREFIX )

if( __ANDROID__ )
   set( __NO_ODBC__ 1 )  # someday port unixodbc library?  But then the mysql driver needs to be ported
   set( __LINUX__ 1 )    # build as if it's linux
   DEFINE_DEFAULT( __ARM__ ON )
   set( D__ANDROID__ "-D__ANDROID__=${__ANDROID__} -DANDROID_DIR=${ANDROID_DIR} -DANDROID_NDK_VERSION=${ANDROID_NDK_VERSION}" )
   DEFINE_DEFAULT( BUILD_EXTRAS ON )
else( __ANDROID_ )
   DEFINE_DEFAULT( __ARM__ OFF )
   DEFINE_DEFAULT( BUILD_EXTRAS OFF )
endif( __ANDROID__ )

if( WIN32 OR __ANDROID__ )
DEFINE_DEFAULT( NEED_FREETYPE ON )
DEFINE_DEFAULT( NEED_JPEG ON )
DEFINE_DEFAULT( NEED_PNG ON )
DEFINE_DEFAULT( NEED_ZLIB ON )
DEFINE_DEFAULT( NEED_UUID OFF )
else( WIN32 OR __ANDROID__ )
DEFINE_DEFAULT( NEED_FREETYPE OFF )
DEFINE_DEFAULT( NEED_JPEG OFF )
DEFINE_DEFAULT( NEED_PNG OFF )
DEFINE_DEFAULT( NEED_ZLIB OFF )
set( __NO_ODBC__ 0 )  # someday port unixodbc library?  But then the mysql driver needs to be ported
set( __LINUX__ ON )
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( __LINUX64__ ON)
endif()
if( EXISTS /usr/include/uuid/uuid.h )
DEFINE_DEFAULT( NEED_UUID OFF )
else()
DEFINE_DEFAULT( NEED_UUID ON )
endif()
endif( WIN32 OR __ANDROID__ )

DEFINE_DEFAULT( BUILD_TESTS OFF )
DEFINE_DEFAULT( FORCE_MSVCRT  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_ALL_NOW  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_INSTALL_NOW  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_PACKAGE_NOW  OFF )
DEFINE_DEFAULT( DIRECTX_SDK "" )
DEFINE_DEFAULT( __CLR__ 0 )

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if( CMAKE_BINARY_DIR MATCHES ${CMAKE_BUILD_TYPE}_solution\$ )
  SET(CMAKE_INSTALL_PREFIX
    ${CMAKE_BINARY_DIR}/../${CMAKE_BUILD_TYPE}_out CACHE PATH "Install default 'out' in this directory." FORCE
    )
    else()
  SET(CMAKE_INSTALL_PREFIX
    ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}_out CACHE PATH "Install default 'out' in this directory." FORCE
    )
  endif( CMAKE_BINARY_DIR MATCHES ${CMAKE_BUILD_TYPE}_solution\$ )
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set( BULLET_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../bullet" CACHE STRING "bullet source path "  )
#OPTION( BULLET_SOURCE "bullet source path" "${CMAKE_CURRENT_SOURCE_DIR}/../../bullet/source" )
#OPTION( BUILD_MONOLITHIC "Build as one big library instead of parts" ${BUILD_MONOLITHIC} )
OPTION( BUILD_EXTRAS "Enable building extra projects (dekware, games, ... )" ${BUILD_EXTRAS} )
OPTION( BUILD_TESTS "Enable building test projects" ${BUILD_TESTS} )
OPTION( GENERATOR_BUILD_ALL_NOW "Enable building during the generate" ${GENERATOR_BUILD_ALL_NOW} )
OPTION( GENERATOR_BUILD_INSTALL_NOW "Enable building during the generate" ${GENERATOR_BUILD_INSTALL_NOW} )
OPTION( GENERATOR_BUILD_PACKAGE_NOW "Enable building installable packages (will mostly fail)" ${GENERATOR_BUILD_PACKAGE_NOW} )
OPTION( FORCE_MSVCRT "force using msvcrt instead of current version SDK libraries" ${FORCE_MSVCRT} )
#option( BUILD_UNICODE "Enable building everything as unicode" ${BUILD_UNICODE} )
option( __ANDROID__ "Enable target to android" ${__ANDROID__} )

if( DIRECTX_SDK )
# if directx had to be passed as an environment variable (has spaces)
# then it's already set, and don't have to pass it... otherwise we couldn't pass it.
  STRING( REPLACE "\\" "/" DDIRECTX_SDK -DDIRECTX_SDK=\"${DIRECTX_SDK}\" )
endif( DIRECTX_SDK )

if( BUILD_TESTS )
SET( DBUILD_TESTS -DBUILD_TESTS=${BUILD_TESTS} )
endif( BUILD_TESTS )

if( __CLR__ )
SET( D__CLR__ -D__CLR__=${__CLR__} )
endif( __CLR__ )

if( __LINUX__ )
SET( D__LINUX__ -D__LINUX__=${__LINUX__} )
endif( __LINUX__ )

if( __LINUX64__ )
SET( D__LINUX64__ -D__LINUX64__=${__LINUX64__} )
endif( __LINUX64__ )

if( __ARM__ )
SET( D__ARM__ -D__ARM__=${__ARM__} )
endif( __ARM__ )

if( FORCE_MSVCRT )
SET( DFORCE_MSVCRT -DFORCE_MSVCRT=${FORCE_MSVCRT} )
endif( FORCE_MSVCRT )


SET( SACK_REQUIREMENTS -DNEED_FREETYPE=${NEED_FREETYPE} -DNEED_JPEG=${NEED_JPEG} -DNEED_PNG=${NEED_PNG} -DNEED_ZLIB=${NEED_ZLIB} )
if( UNIX )
#message( "Not win32 (${WIN32}), UNIX(${UNIX}) but MINGW=\"${MINGW}\" maybe android? ; not passing need_externals" )
#set( SACK_REQUIREMENTS -DNEED_UUID=ON )
endif( UNIX )

IF(EXISTS ${BULLET_SOURCE}/CMakeLists.txt )
  if( WATCOM )
  else( WATCOM )
    set( BULLET_ENABLED ON )
    set( DBULLET_SOURCE -DBULLET_SOURCE=${BULLET_SOURCE} )
  endif( WATCOM )
endIF(EXISTS ${BULLET_SOURCE}/CMakeLists.txt )


set( PROJECT core )
set( EXTRA_FLAGS  -DBUILD_EXTRAS=${BUILD_EXTRAS}
			-D__NO_ODBC__=${__NO_ODBC__}
			\"-DTARGET_INSTALL_PREFIX=${TARGET_INSTALL_PREFIX}\"			
			${DDIRECTX_SDK}
        	        ${SACK_REQUIREMENTS} 
                	${DFORCE_MSVCRT} 
	                ${DBUILD_TESTS} 
                        ${DBULLET_SOURCE}
                        ${D__ARM__}
                        ${D__LINUX__}
                        ${D__LINUX64__}
                        ${D__ANDROID__}
                        ${D__CLR__}
                        ${DDX_SDK}
                )

set( PROJECT_BASE_PROJECT ${PROJECT} )
BuildProject( ${PROJECT} sack ${CMAKE_CURRENT_LIST_DIR}/.. SACK_SDK_ROOT_PATH ${EXTRA_FLAGS} )

set( EXTRA_FLAGS -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} )

#BuildProject( ${PROJECT}.framebuffer sack.nosolution ${CMAKE_CURRENT_LIST_DIR}/../not-exists UNUSED_SACK_SDK_ROOT_PATH ${EXTRA_FLAGS} )
#BuildProject( ${PROJECT}.puregl2 sack.nosolution ${CMAKE_CURRENT_LIST_DIR}/../not-exists UNUSED_SACK_SDK_ROOT_PATH ${EXTRA_FLAGS} )

if( NOT __ANDROID__ )
  # this calls sack_deploy to build cmakepackage
  # on android ended up making a cmake-configured-cmake-script to make this file.
#  Add_custom_command( TARGET Build${PROJECT} 
#	COMMAND ${SACK_SDK_ROOT_PATH}/${BINARY_OUTPUT_DIR}/sack_deploy${CMAKE_EXECUTABLE_SUFFIX} -nr
#	WORKING_DIRECTORY ${SACK_SDK_ROOT_PATH}
#  )                
endif( NOT __ANDROID__ )

if( NOT __CLR__ )
set( PROJECT binary )
set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
BuildProject( ${PROJECT} sack.binaries ${CMAKE_CURRENT_LIST_DIR}/../binary SACK_BINARY_SDK_ROOT_PATH ${EXTRA_FLAGS} )
add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} )
else( NOT __CLR__ )
message( "Disabled Binaries output for CLR build" )
endif( NOT __CLR__ )


set( PROJECT intershell )
set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/InterShell.stable INTERSHELL_SDK_ROOT_PATH ${EXTRA_FLAGS} )
add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} binary )

if( NOT __LINUX__ AND NOT __ANDROID__ )
  set( PROJECT intershell_Package )
  set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
  BuildProject( ${PROJECT} Intershell.Package ${CMAKE_CURRENT_LIST_DIR}/../src/InterShell.stable/package INTERSHELL_PACKAGE_ROOT_PATH ${EXTRA_FLAGS} -DINTERSHELL_SDK_ROOT_PATH=${INTERSHELL_SDK_ROOT_PATH} )
  add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} intershell )
endif( NOT __LINUX__ AND NOT __ANDROID__ )


#set( PROJECT intershell2 )
#set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
#BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/InterShell INTERSHELL2_SDK_ROOT_PATH ${EXTRA_FLAGS} )
#add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} binary )

if( NOT __ANDROID__ )
  set( PROJECT mercurial_version_tagger )
  set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
  BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/utils/mercurial_version_tagger MERCURIAL_TAGGER_ROOT_PATH ${EXTRA_FLAGS} )
  add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} )
endif( NOT __ANDROID__ )

IF( BULLET_ENABLED )
  message( "Congratz, you get Bullet projects" )
  if( NOT __ANDROID__ )
    set( PROJECT ball_mixer1 )
    set( EXTRA_FLAGS_MIXER  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} -DSACK_BINARY_SDK_ROOT_PATH=${SACK_BINARY_SDK_ROOT_PATH} -DINTERSHELL_SDK_ROOT_PATH=${INTERSHELL_SDK_ROOT_PATH} ${D__ANDROID__} )
    BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/games/ball_mixer INSTALL ${EXTRA_FLAGS_MIXER} )
    add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} binary intershell )
  endif( NOT __ANDROID__ )

  set( PROJECT ball_mixer2 )
  set( EXTRA_FLAGS_MIXER  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} -DSACK_BINARY_SDK_ROOT_PATH=${SACK_BINARY_SDK_ROOT_PATH} -DINTERSHELL_SDK_ROOT_PATH=${INTERSHELL_SDK_ROOT_PATH} ${D__ANDROID__} )
  BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/games/ball_mixer2 INSTALL ${EXTRA_FLAGS_MIXER} )
  add_dependencies( ${LAST_TARGET} ${PROJECT_BASE_PROJECT} binary intershell )

ENDIF( BULLET_ENABLED )

if( BUILD_EXTRAS )
  include( ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.extras.txt )
endif( BUILD_EXTRAS )


