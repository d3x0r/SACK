cmake_minimum_required(VERSION 3.20)

PROJECT( logger.portable )

include( ${SACK_BASE}/share/SACK/build/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include/SACK )
STRING( REPLACE " " ";" SACK_PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
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

set( project syslog.logger.portable )

set( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. )

if( WIN32 )
  set( NET_SOURCES    ${SOURCES_ROOT}/src/netlib/net_winsock2.c  )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/src/../${SNPRINTF_SOURCES} )
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



set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../logger.c
    ${SOURCES_ROOT}/src/memlib/sharemem.c
    ${SOURCES_ROOT}/src/memlib/memory_operations.c
    ${SOURCES_ROOT}/src/timerlib/timers.c
    ${SOURCES_ROOT}/src/idlelib/idle.c
    ${SOURCES_ROOT}/src/procreglib/names.c
    ${SOURCES_ROOT}/src/configlib/configscript.c
    ${SOURCES_ROOT}/src/sysloglib/syslog.c
    ${SOURCES_ROOT}/src/systemlib/system.c
    ${SOURCES_ROOT}/src/systemlib/spawntask.c
    ${SOURCES_ROOT}/src/systemlib/args.c
    ${SOURCES_ROOT}/src/netlib/network.c
    ${SOURCES_ROOT}/src/netlib/network${NETWORK_OS}.c
    ${SOURCES_ROOT}/src/netlib/network_addresses.c
    ${SOURCES_ROOT}/src/netlib/udpnetwork.c
    ${SOURCES_ROOT}/src/netlib/tcpnetwork.c
    ${NET_SOURCES}
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/src/typelib/typecode.c
    ${SOURCES_ROOT}/src/typelib/text.c
    ${SOURCES_ROOT}/src/typelib/sets.c
    ${SOURCES_ROOT}/src/typelib/binarylist.c
    ${SOURCES_ROOT}/src/filesyslib/filescan.c
    ${SOURCES_ROOT}/src/filesyslib/pathops.c
    ${SOURCES_ROOT}/src/filesyslib/winfiles.c
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 
)


sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY 	COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN;NO_SSL" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${project}  ${SACK_PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


