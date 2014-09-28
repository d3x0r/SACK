
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
message( string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} ) )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
endif( WATCOM )

set( project waitwin.portable )

set( SOURCES_ROOT ../../.. )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../waitwin.c
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
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


sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
    COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

set( project waitproc.portable )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../waitproc.c
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
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


sack_add_executable(${project} ${BASE_SOURCES} )
target_link_libraries( ${project}  ${SACK_PLATFORM_LIBRARIES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
       COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


