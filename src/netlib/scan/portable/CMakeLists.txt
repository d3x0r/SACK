
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        )
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
   if(${flag_var} MATCHES "-br")
      string(REGEX REPLACE "-br" "" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "-br")
endforeach(flag_var)

if( WATCOM )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
endif( WATCOM )

set( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../.. )


if( WIN32 )
set( NET_SOURCES     ${SOURCES_ROOT}/netlib/net_winsock2.c  )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )


set(BASE_SOURCES
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
    ${SOURCES_ROOT}/netlib/network.c 
    ${SOURCES_ROOT}/netlib/udpnetwork.c 
    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
    ${SOURCES_ROOT}/netlib/ping.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${ExtraSources}
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
)


PROJECT( scan_ports.portable )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
../scanner 
${BASE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;"
)
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm ws2_32 wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

if( NOT __ARM__ )
PROJECT( scan_addresses.portable )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
../pinger 
${BASE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;"
)
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm ws2_32 wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )
endif( NOT __ARM__ )

PROJECT( nameof.portable )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
../nameof 
${BASE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;"
)
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm ws2_32 wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )

PROJECT( ipof.portable )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
../ipof 
${BASE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;"
)
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm ws2_32 wsock32 )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )
