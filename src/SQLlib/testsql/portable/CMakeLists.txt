
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

set( project sqlcmd.portable )

set( SOURCES_ROOT ../../.. )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
set(NET_SOURCES   ${SOURCES_ROOT}/netlib/net_winsock2.c )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)


SET( SQLITE_VERSION 3.7.16.2 )
SET( SQLITE_SOURCES ${BAG_SQLITE_INTERFACE} ${SOURCES_ROOT}/sqlite/${SQLITE_VERSION}/sqlite3.c )
include_directories( ${SOURCES_ROOT}/sqlite/${SQLITE_VERSION} )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../sqlcmd
#    ${SOURCES_ROOT}/netlib/network.c 
#    ${SOURCES_ROOT}/netlib/udpnetwork.c 
#    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
#    ${NET_SOURCES} 
#    ${SOURCES_ROOT}/msgsvr/client/client.c
    ${SOURCES_ROOT}/SQLlib/sqlstub.c
    ${SOURCES_ROOT}/SQLlib/sqlutil.c
    ${SOURCES_ROOT}/SQLlib/sqlwrap.c
    ${SOURCES_ROOT}/SQLlib/sqlparse3.c
    ${SQLITE_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/typelib/msgqueue.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)

if( NOT __NO_ODBC__ )
    set( MoreDefinitions USE_ODBC )
else( NOT __ODBC__ )
    set( MoreDefinitions __NO_ODBC__ )
endif( NOT __NO_ODBC__ )


sack_add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;__STATIC__;__NO_GUI__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;${MoreDefinitions};USE_SQLITE;__DISABLE_UDP_SYSLOG__;__NO_MSGSVR__;NO_DEADSTART_DLLMAIN" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
    COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project}  ${SACK_PLATFORM_LIBRARIES} )
if( WIN32 )
#target_link_libraries( ${project} winmm wsock32 ws2_32 )
target_link_libraries( ${project} winmm odbc32 )
else( WIN32 )
if( NOT __NO_ODBC__ )
   target_link_libraries( ${project} odbc  )
endif( NOT __NO_ODBC__ )
endif( WIN32 )
install_default_dest_binary( ${project} )





