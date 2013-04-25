
project( window_stacker )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    window_stacker.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} psapi )
install_default_dest( ${PROJECT_NAME} )

project( window_stacker.portable )

set( SOURCES_ROOT ../.. )

if( WIN32 )
  set( NET_SOURCES     ../../../netlib/net_winsock2.c  )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    window_stacker.c
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/systraylib/systray.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)


add_executable(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
	COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}\";__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__"
	FOLDER utils
)
if( WIN32 )
#ws2_32 wsock32 
target_link_libraries( ${PROJECT_NAME} winmm psapi )
else( WIN32 )
target_link_libraries( ${PROJECT_NAME} dl pthread )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

