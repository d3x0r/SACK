cmake_minimum_required(VERSION 3.0)

PROJECT( wait.portable )
set( CMAKE_INSTALL_MESSAGE "LAZY" )

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/share/SACK/build/DefaultInstall.cmake )
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

############### Wait Window #########################################

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
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/idlelib/idle.c
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
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
    COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

############### Wait Process #########################################


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
target_link_libraries( ${project}  ${SPLATFORM_LIBRARIES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
       COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

############### Wait HTTP #########################################

set( project waithttp.portable )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
set( NET_SOURCES     ${SOURCES_ROOT}/netlib/net_winsock2.c  )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

if( WIN32 )
  # WSAWaitMultipleObject network handling
  SET( NETWORK_OS _win32 )
elseif( UNIX )
  if( MAC )
    #kevent event handling (BSD)
	SET( NETWORK_OS _mac )
  else( MAC )
	# epoll event handling
	SET( NETWORK_OS _linux )
   endif( MAC )
endif( WIN32 )

set( LIBRESSL_VERSION 3.8.0 )

set( LIBRESSL_SKIP_INSTALL ON )
set( LIBRESSL_APPS OFF )
set( LIBRESSL_TESTS OFF )

add_subdirectory( ${SOURCES_ROOT}/contrib/libressl/${LIBRESSL_VERSION} libressl )
include_directories( ${SOURCES_ROOT}/contrib/libressl/${LIBRESSL_VERSION}/include )
set( SSL_LIBS ssl tls crypto )



set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../waithttp.c
    ${SOURCES_ROOT}/netlib/network.c
    ${SOURCES_ROOT}/netlib/network${NETWORK_OS}.c
    ${SOURCES_ROOT}/netlib/network_addresses.c
    ${SOURCES_ROOT}/netlib/udpnetwork.c 
    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
    ${SOURCES_ROOT}/netlib/ssl_layer.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/http.c
    ${SOURCES_ROOT}/typelib/url.c
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
target_link_libraries( ${project}  ${SPLATFORM_LIBRARIES} )
target_link_libraries( ${project}  ${SSL_LIBS} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 
       COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;BUILD_PORTABLE_EXECUTABLE" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )

if( WIN32 )
   target_link_libraries( ${project} crypt32 )
endif( WIN32 )

install_default_dest( ${project} )


