set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE "debug" CACHE STRING "Set build type")


if( NOT PROJECT_NAME )
project( Sack.all )
endif( NOT PROJECT_NAME )

if( MSVC10 )
IF(NOT CMAKE_CROSSCOMPILING)
  FIND_PROGRAM(REAL_CMAKE_MAKE_PROGRAM
    NAMES MSBuild
    HINTS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VS;ProductDir]
    "$ENV{SYSTEMROOT}/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0;CLR Version]/"
    "c:/WINDOWS/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0;CLR Version]/"
    "$ENV{SYSTEMROOT}/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\10.0;CLR Version]/")
ENDIF()
set( CMAKE_MAKE_PROGRAM ${REAL_CMAKE_MAKE_PROGRAM} )
endif( MSVC10 )
if( MSVC11 )
IF(NOT CMAKE_CROSSCOMPILING)
  FIND_PROGRAM(REAL_CMAKE_MAKE_PROGRAM
    NAMES MSBuild
    HINTS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\11.0\\Setup\\VS;ProductDir]
    "$ENV{SYSTEMROOT}/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\11.0;CLR Version]/"
    "c:/WINDOWS/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\11.0;CLR Version]/"
    "$ENV{SYSTEMROOT}/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\11.0;CLR Version]/")
ENDIF()
set( CMAKE_MAKE_PROGRAM ${REAL_CMAKE_MAKE_PROGRAM} )
set( CMAKE_VS_PLATFORM_TOOLSET v110_xp )
endif( MSVC11 )

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
			${CMAKE_CONFIGURATION_TYPES} Debug Delease RelWithDebInfo MinSizeRel )

include( ${CMAKE_CURRENT_LIST_DIR}/CMakeBuild.txt )

DEFINE_DEFAULT( __ANDROID__  OFF )

if( __ANDROID__ )
   set( __NO_ODBC__ 1 )  # someday port unixodbc library?  But then the mysql driver needs to be ported
   set( __LINUX__ 1 )    # build as if it's linux
   DEFINE_DEFAULT( __ARM__ ON )
   set( D__ANDROID__ "-D__ANDROID__=${__ANDROID__} -DANDROID_DIR=${ANDROID_DIR} -DANDROID_NDK_VERSION=${ANDROID_NDK_VERSION}" )
   DEFINE_DEFAULT( BUILD_MONOLITHIC OFF )
   DEFINE_DEFAULT( BUILD_EXTRAS ON )
else( __ANDROID_ )
   DEFINE_DEFAULT( __ARM__ OFF )
   DEFINE_DEFAULT( BUILD_MONOLITHIC ON )
   DEFINE_DEFAULT( BUILD_EXTRAS OFF )
endif( __ANDROID__ )

if( WIN32 )
DEFINE_DEFAULT( NEED_FREETYPE ON )
DEFINE_DEFAULT( NEED_JPEG ON )
DEFINE_DEFAULT( NEED_PNG ON )
DEFINE_DEFAULT( NEED_ZLIB ON )
else( WIN32 )
DEFINE_DEFAULT( NEED_FREETYPE OFF )
DEFINE_DEFAULT( NEED_JPEG OFF )
DEFINE_DEFAULT( NEED_PNG OFF )
DEFINE_DEFAULT( NEED_ZLIB OFF )
endif( WIN32 )

DEFINE_DEFAULT( BUILD_TESTS OFF )
DEFINE_DEFAULT( FORCE_MSVCRT  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_ALL_NOW  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_INSTALL_NOW  OFF )
DEFINE_DEFAULT( GENERATOR_BUILD_PACKAGE_NOW  OFF )

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
OPTION( BUILD_MONOLITHIC "Build as one big library instead of parts" ${BUILD_MONOLITHIC} )
OPTION( BUILD_EXTRAS "Enable building extra projects (dekware, games, ... )" ${BUILD_EXTRAS} )
OPTION( BUILD_TESTS "Enable building test projects" ${BUILD_TESTS} )
OPTION( GENERATOR_BUILD_ALL_NOW "Enable building during the generate" ${GENERATOR_BUILD_ALL_NOW} )
OPTION( GENERATOR_BUILD_INSTALL_NOW "Enable building during the generate" ${GENERATOR_BUILD_INSTALL_NOW} )
OPTION( GENERATOR_BUILD_PACKAGE_NOW "Enable building installable packages (will mostly fail)" ${GENERATOR_BUILD_PACKAGE_NOW} )
OPTION( FORCE_MSVCRT "force using msvcrt instead of current version SDK libraries" ${FORCE_MSVCRT} )
option( BUILD_UNICODE "Enable building everything as unicode" ${BUILD_UNICODE} )
option( __ANDROID__ "Enable target to android" ${__ANDROID__} )

if( DIRECTX_SDK )
set( DDIRECTX_SDK -DDIRECTX_SDK=${DIRECTX_SDK} )
endif( DIRECTX_SDK )

if( BUILD_TESTS )
SET( DBUILD_TESTS -DBUILD_TESTS=${BUILD_TESTS} )
endif( BUILD_TESTS )


if( __LINUX__ )
SET( D__LINUX__ -D__LINUX__=${__LINUX__} )
endif( __LINUX__ )

if( __ARM__ )
SET( D__ARM__ -D__ARM__=${__ARM__} )
endif( __ARM__ )

if( FORCE_MSVCRT )
SET( DFORCE_MSVCRT -DFORCE_MSVCRT=${FORCE_MSVCRT} )
endif( FORCE_MSVCRT )

if( BUILD_UNICODE )
SET( DBUILD_UNICODE -DBUILD_UNICODE=${BUILD_UNICODE} )
endif( BUILD_UNICODE )



SET( SACK_REQUIREMENTS -DNEED_FREETYPE=${NEED_FREETYPE} -DNEED_JPEG=${NEED_JPEG} -DNEED_PNG=${NEED_PNG} -DNEED_ZLIB=${NEED_ZLIB} )
if( UNIX )
message( "Not win32, but MINGW="${MINGW}" maybe android?" )
set( SACK_REQUIREMENTS -D__LINUX__=${__LINUX__} -D__ARM__=${__ARM__} ${D__ANDROID__} )
endif( UNIX )

IF(EXISTS ${BULLET_SOURCE}/CMakeLists.txt )
  if( WATCOM )
  else( WATCOM )
    set( BULLET_ENABLED ON )
    set( DBULLET_SOURCE -DBULLET_SOURCE=${BULLET_SOURCE} )
  endif( WATCOM )
endIF(EXISTS ${BULLET_SOURCE}/CMakeLists.txt )


set( PROJECT core )
set( EXTRA_FLAGS -DBUILD_MONOLITHIC=${BUILD_MONOLITHIC}
			-D__NO_ODBC__=${__NO_ODBC__}
			${DDIRECTX_SDK} 
        	        ${SACK_REQUIREMENTS} 
                	${DFORCE_MSVCRT} 
	                ${DBUILD_TESTS} 
        	        ${DBUILD_UNICODE} 
                        ${DBULLET_SOURCE}
                        ${D__ARM__}
                        ${D__LINUX__}
                        ${D__ANDROID__}
                )
BuildProject( ${PROJECT} sack ${CMAKE_CURRENT_LIST_DIR}/.. SACK_SDK_ROOT_PATH ${EXTRA_FLAGS} )
if( NOT __ANDROID__ )
  # this calls sack_deploy to build cmakepackage
  # on android ended up making a cmake-configured-cmake-script to make this file.
  Add_custom_command( TARGET Build${PROJECT} 
	COMMAND ${SACK_SDK_ROOT_PATH}/bin/sack_deploy${CMAKE_EXECUTABLE_SUFFIX} -nr
	WORKING_DIRECTORY ${SACK_SDK_ROOT_PATH}
  )                
endif( NOT __ANDROID__ )

set( PROJECT binary )
set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
BuildProject( ${PROJECT} sack.binaries ${CMAKE_CURRENT_LIST_DIR}/../binary SACK_BINARY_SDK_ROOT_PATH ${EXTRA_FLAGS} )
if( NOT __ANDROID__ )
  # this calls sack_deploy to build cmakepackage
  # on android ended up making a cmake-configured-cmake-script to make this file.
  Add_custom_command( TARGET Build${PROJECT} 
	COMMAND ${SACK_BINARY_SDK_ROOT_PATH}/Sack.Binary.Deploy${CMAKE_EXECUTABLE_SUFFIX} -nr
	WORKING_DIRECTORY ${SACK_BINARY_SDK_ROOT_PATH}
  )
endif( NOT __ANDROID__ )
add_dependencies( ${LAST_TARGET} Buildcore )

set( PROJECT intershell )
set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/InterShell INTERSHELL_SDK_ROOT_PATH ${EXTRA_FLAGS} )
if( NOT __ANDROID__ )
# this calls sack_deploy to build cmakepackage
# on android ended up making a cmake-configured-cmake-script to make this file.
Add_custom_command( TARGET Build${PROJECT} 
	COMMAND ${INTERSHELL_SDK_ROOT_PATH}/intershell_deploy${CMAKE_EXECUTABLE_SUFFIX} -nr
	WORKING_DIRECTORY ${INTERSHELL_SDK_ROOT_PATH}
)                
endif( NOT __ANDROID__ )
add_dependencies( ${LAST_TARGET} Buildcore Buildbinary )

if( NOT __ANDROID__ )
  set( PROJECT mercurial_version_tagger )
  set( EXTRA_FLAGS  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} ${D__ANDROID__} )
  BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/utils/mercurial_version_tagger MERCURIAL_TAGGER_ROOT_PATH ${EXTRA_FLAGS} )
endif( NOT __ANDROID__ )

if( NOT BUILD_MONOLITHIC )
  IF( BULLET_ENABLED )
    message( "Congratz, you get Bullet projects" )
    if( NOT __ANDROID__ )
      set( PROJECT ball_mixer1 )
      set( EXTRA_FLAGS_MIXER  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} -DSACK_BINARY_SDK_ROOT_PATH=${SACK_BINARY_SDK_ROOT_PATH} -DINTERSHELL_SDK_ROOT_PATH=${INTERSHELL_SDK_ROOT_PATH} ${D__ANDROID__} )
      BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/games/ball_mixer INSTALL ${EXTRA_FLAGS_MIXER} )
      add_dependencies( ${LAST_TARGET} Buildcore Buildbinary Buildintershell )
    endif( NOT __ANDROID__ )

    set( PROJECT ball_mixer2 )
    set( EXTRA_FLAGS_MIXER  -DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH} -DSACK_BINARY_SDK_ROOT_PATH=${SACK_BINARY_SDK_ROOT_PATH} -DINTERSHELL_SDK_ROOT_PATH=${INTERSHELL_SDK_ROOT_PATH} ${D__ANDROID__} )
    BuildProject( ${PROJECT} ${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../src/games/ball_mixer2 INSTALL ${EXTRA_FLAGS_MIXER} )
    add_dependencies( ${LAST_TARGET} Buildcore Buildbinary Buildintershell )

  ENDIF( BULLET_ENABLED )
endif( NOT BUILD_MONOLITHIC )

if( BUILD_EXTRAS )
  include( ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.extras.txt )
endif( BUILD_EXTRAS )


