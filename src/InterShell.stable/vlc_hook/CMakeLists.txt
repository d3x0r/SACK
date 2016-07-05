

cmake_minimum_required(VERSION 2.8)

PROJECT( vlc_interface )
set( VLC_INTERFACE ${PROJECT_NAME} )

if( ${CMAKE_GENERATOR} MATCHES "Watcom WMake" )
 set( ExtraFlags "${ExtraFlags} -bd" )
endif()

include_directories( ../../contrib/libvlc/2.2.4/sdk/include )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
	vlc_interface
${LAST_GCC_LIBRARY_SOURCE} )

if( NOT VLC_PATH )
if( MAKING_SACK_CORE )
set( VLC_PATH ${SACK_SDK_ROOT_PATH}/src/contrib/libvlc/2.2.4 )
else( MAKING_SACK_CORE )
set( VLC_PATH ${SACK_SDK_ROOT_PATH}/include/libvlc/2.2.4 )
endif( MAKING_SACK_CORE )
endif( NOT VLC_PATH )
OPTION( VLC_PATH "Path to which version of VLC interface to use" ${VLC_PATH} )

include_directories( ${VLC_PATH}/sdk/include )

add_library_force_source(${PROJECT_NAME} SHARED ${SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES 
        COMPILE_FLAGS "-DTARGET_LABEL=${TARGET_LABEL} ${ExtraFlags}" 
        FOLDER "Video Streaming" 
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm )
endif( WIN32 )

install_default_project( ${PROJECT_NAME} bin/plugins )

if( NOT VLC_INTERFACE_ONLY )

project( vlc_test )

set( SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
 vlc_test.c
${LAST_GCC_PROGRAM_SOURCE} )

add_executable_force_source(${PROJECT_NAME} ${SOURCES} )


string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER "Video Streaming"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARY} ${VLC_INTERFACE} )
install_default_dest( ${PROJECT_NAME} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
add_subdirectory( intershell )

endif( NOT VLC_INTERFACE_ONLY )

