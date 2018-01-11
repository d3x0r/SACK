cmake_minimum_required(VERSION 3.0)

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include/SACK )
STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
add_definitions( ${PLATFORM_DEFINES} )


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
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
endif( WATCOM )

set( project pcopy.portable )

set( SOURCES_ROOT ../../.. )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )

if( SNPRINTF_SOURCES ) 
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../pcopy ../docopy
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/systemlib/args.c
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
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
	COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__STATIC__;__STATIC_GLOBALS__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} " 
                  FOLDER utils
)

target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )

if( MAKING_SACK_CORE )
install_mode_dest( ${project} )
else( MAKING_SACK_CORE )
install_default_dest( ${project} )
endif( MAKING_SACK_CORE )

