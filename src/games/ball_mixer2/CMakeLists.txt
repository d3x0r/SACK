cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )


if( NOT SACK_BINARY_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_BINARY_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Sack.Binaries;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_BINARY_SDK_ROOT_PATH )
set(SACK_BINARY_SDK_ROOT_PATH ${SACK_BINARY_SDK_ROOT_PATH} CACHE STRING "SACK Binary SDK Root" )
endif( NOT SACK_BINARY_SDK_ROOT_PATH )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\INTERSHELL;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "INTERSHELL SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
include( ${SACK_BINARY_SDK_ROOT_PATH}/CMakePackage )
include( ${INTERSHELL_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

INSTALL_SACK( bin )
INSTALL_INTERSHELL_CORE( bin )
INSTALL_INTERSHELL_PLUGINS( bin )
INSTALL_SACK_BINARY( systray_shell bin )

include_directories( ${INTERSHELL_INCLUDE_DIR} )
LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )
INSTALL_INTERSHELL_WIDGETS( bin )

PROJECT( ball_mixer )

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../altanik )
SET( ALTANIK_ROOT  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../altanik )
SET( EXTRA_SOURCES remote_hook.cpp )
ADD_DEFINITIONS( -DALTANIK_EXISTS )
endif( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../altanik )



include_directories( ../include )

if( EXTRA_SOURCES )
include_directories( ${ALTANIK_ROOT}/include )
endif( EXTRA_SOURCES )

include_directories( ${BULLET_SOURCE}/src )
string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_lower ) 
if( WIN32 )
ADD_DEFINITIONS( -D_WIN32  )
endif( WIN32 )
if( CMAKE_BUILD_lower STREQUAL "debug" )
#ADD_DEFINITIONS( -DBT_DEBUG )
endif( CMAKE_BUILD_lower STREQUAL "debug" )

ADD_LIBRARY( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE} 
  land.cpp
  ${EXTRA_SOURCES}
  shader.cpp
${LAST_GCC_LIBRARY_SOURCE} 
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} ${BULLET_LIBRARIES} winmm opengl32 glu32 )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} glew )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DWINDOWS_MODE"
)

install( FILES ball_mixer.interface.conf DESTINATION bin )
install( FILES NewBallTextGrid.png bingostarlogo.png DESTINATION bin/Images/balls )

LIST( GET systray_shell_targets 0 shell_target )
install( FILES ${shell_target}  DESTINATION bin RENAME ball_mixer.exe )

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

if( ALTANIK_ROOT )
set( MILK2_SYSTEM ON )
set( MILK_PATH milk2 )
set( USE_SACK_WIDGETS ON )

# set this to install altanik_core in correct directory
set( INSTALL_ALONG bin )

# set WIDGET_LIBRARY_ONLY; do not install full resources
set( WIDGET_LIBRARY_ONLY ON )
add_subdirectory( ${ALTANIK_ROOT}/src/fut altanik_core )
add_subdirectory( ${ALTANIK_ROOT}/src/fut/flashdrive flashdrive )
endif( ALTANIK_ROOT )


# get the system libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin )

