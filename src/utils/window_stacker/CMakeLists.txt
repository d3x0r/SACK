
set( project window_stacker )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    window_stacker.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} psapi )
install_default_dest( ${project} )

set( project window_stacker.portable )

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
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/systemlib/args.c
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


sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
	COMPILE_DEFINITIONS "TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\";__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
	FOLDER utils
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} psapi )
install_default_dest( ${project} )

