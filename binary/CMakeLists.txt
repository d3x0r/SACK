set_property(GLOBAL PROPERTY USE_FOLDERS On)
set_property(GLOBAL PROPERTY QUIET_INSTALL On)
set(CMAKE_INSTALL_MESSAGE "LAZY")
cmake_minimum_required(VERSION 2.8)

include( ExternalProject )


PROJECT( "Sack.Binaries" )

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

string( REPLACE "\\" "/" SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

INSTALL_SACK( bin )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
message( "installed to ${SACK_LIBRARY_DIR}" )

# lie... this way we can build binaries without depending on intershell
# but besides that we require direct knowledge of libraries here...
add_definitions( -DINTERSHELL_CORE_BUILD )

if( __ARM__ )
SET( PLATFORM_DEFINES ${PLATFORM_DEFINES} -D__ARM__ )
ADD_DEFINITIONS( -D__ARM__ )
endif( __ARM__ )
if( __NO_ODBC__ )
SET( PLATFORM_DEFINES ${PLATFORM_DEFINES} -D__NO_ODBC__ )
ADD_DEFINITIONS( -D__NO_ODBC__ )
endif( __NO_ODBC__ )


add_subdirectory( deploy )


macro( add_subpath base )
string( REPLACE "." "_" base2 ${base} )
add_subdirectory( ${base} ${base2} )
endmacro(add_subpath)


# not really a test; just a utility
add_subpath( ../src/psilib/paltst )

string( REPLACE ";" " " PLATFORM_LIBRARIES_ARG "${SACK_PLATFORM_LIBRARIES}" )
string( REPLACE ";" " " PLATFORM_DEFINES_ARG "${PLATFORM_DEFINES}" )


if( WIN32 )
add_subpath( ../src/utils/reboot )
add_subpath( ../src/utils/cleaner )
add_subpath( ../src/utils/shutmon )
ExternalProject_Add( shell_util
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/../src/utils/shell
	BINARY_DIR shell_extern
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)
#add_subpath( ../src/utils/shell )
endif( WIN32 )
add_subpath( ../src/utils/guid_generator )
add_subpath( ../src/netlib/network_relay_server )
add_subpath( ../src/netlib/user )
add_subpath( ../src/netlib/echo )
add_subpath( ../src/utils/image_chopper )
add_subpath( ../src/netlib/forward )
add_subpath( ../src/netlib/proxy )
add_subpath( ../src/netlib/scan )
add_subpath( ../src/psilib/calctl )

add_subpath( ../src/utils/factorio )

add_subpath( ../src/SQLlib/testsql )
add_subpath( ../src/utils/launcher )
add_subpath( ../src/streamlib/ffmpeg/program )
add_subpath( ../src/utils/config_secure )
#add_subpath( ../src/salty_random_generator/test )
add_subpath( ../src/sysloglib/logger )
add_subpath( ../src/utils/systray_shell )

if( NOT __ANDROID__ AND NOT __CLR__ )
# some things just don't make sense to build for android... (it's a good line stripper/tabber-detabber)
add_subpath( ../src/utils/strip )
add_subpath( ../src/filesyslib/filemon/filemonitor )
endif( NOT __ANDROID__ AND NOT __CLR__ )

add_subpath( ../src/utils/frame_ID_converter )

ExternalProject_Add( ppc
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/../makefiles/prog/ppc
	BINARY_DIR ppc
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
)
#add_subpath( ../makefiles/prog/ppc )
add_subpath( ../src/utils/image_colorizer )

if( WIN32 )
add_subpath( ../src/utils/shoimg )
add_subpath( ../src/utils/bubbles2 )
add_subpath( ../src/utils/top_picker )
# this is pretty useless :)
#add_subpath( ../src/utils/rotor/harmonic )

if( NOT __CLR__ AND NOT BUILD_UNICODE )
# this is just too mixed-char-ascii to convert easily
add_subpath( ../src/utils/file_mirror )
endif( NOT __CLR__ AND NOT BUILD_UNICODE )
add_subpath( ../src/InterShell.stable/widgets )
add_subpath( ../src/commlib )
add_subpath( ../src/InterShell.stable/widgets/banner/banner_cmd )
add_subpath( ../src/psilib/console/cmdshell )
#add_subpath( ../src/utils/http_post )
add_subpath( ../src/utils/http_server )
add_subpath( ../src/utils/waitwin )
add_subpath( ../src/utils/rundelay )

ExternalProject_Add( get_display_ext
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/../src/utils/get_display
	BINARY_DIR get_display_ext
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)
#add_subpath( ../src/utils/get_display )

add_subpath( ../src/utils/pcopy )
add_subpath( ../src/utils/window_stacker )
add_subpath( ../src/utils/set_registry )
endif( WIN32 )


add_subpath( ../src/utils/opensim/region_maker )
add_subpath( ../src/netlib/html5.websocket/server )
if( __LINUX__ AND NOT __ANDROID__ )
   add_subpath( ../src/utils/linux_ban_scanner )
endif( __LINUX__ AND NOT __ANDROID__ )



SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SACK Binaries")
SET(CPACK_PACKAGE_VENDOR "Freedom Collective")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../howto.compile")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../read.this")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "478")
#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../doc\\\\BlankButton.jpg")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\sack_bag.dll")
  SET(CPACK_NSIS_DISPLAY_NAME "SACK ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_CONTACT "d3ck0r@gmail.com")
  #SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "bin/MyExecutable")
  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")
INCLUDE(CPack)

