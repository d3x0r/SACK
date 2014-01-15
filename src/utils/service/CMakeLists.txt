
cmake_minimum_required(VERSION 2.8)

project( service_test )

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
INSTALL_SACK( . )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    service_test.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

if( asdfasdfasdfasdf )
project( lstrip.portable )

if( __WINDOWS__ )
  set( NET_SOURCES     ../../../netlib/net_winsock2.c  )
endif( __WINDOWS__ )


set( SOURCES_ROOT ../.. )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    strip.c
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/syslfoglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)


add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__"
                  FOLDER utils
)
if( WIN32 )
#ws2_32 wsock32 
target_link_libraries( ${PROJECT_NAME} winmm)
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

endif( asdfasdfasdfasdf)
