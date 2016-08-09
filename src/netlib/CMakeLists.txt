# this CMakeList.txt is to build just thread/timer support library with
# its required dependancies.  This is not a full build and typically is
# not in the full build.

set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )
cmake_minimum_required(VERSION 3.0)

if(GCC )
set( FIRST_GCC_SOURCES ..\deadstart\deadstart_list.c )
set( LAST_GCC_SOURCES ..\deadstart\deadstart_lib.c 
          ..\deadstart\deadstart_end.c )
endif(GCC)

if( __ANDROID__ )
	set( MORE_DEFS "__ANDROID__=1" )
	set( MORE_DEFS ${MORE_DEFS} "__LINUX__=1" )
endif( __ANDROID__ )


INCLUDE_DIRECTORIES( ../../include )
INCLUDE( ../timerlib.build/copymac.cmake )

if( WIN32 )
    SET( MORE_SOURCES net_winsock2.c )
else()
    SET( MORE_DEFS ${MORE_DEFS} __LINUX__=1 )
endif( WIN32 )

if( WIN32 )
set(MORE_NETWORK_SOURCES
  net_winsock2.c
  ../systemlib/oswin.c
)
endif( WIN32 )

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    SET( __64__ ON )
    set( MORE_DEFS ${MORE_DEFS} __64__=1 )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    DEFINE_DEFAULT( __64__ OFF )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


set( MATRIXSSL_VERSION 3.8.3 )
add_subdirectory( ../contrib/MatrixSSL/${MATRIXSSL_VERSION} MatrixSSL )
INCLUDE_DIRECTORIES( ../contrib/MatrixSSL/${MATRIXSSL_VERSION} )

set( NETLIB_SOURCES    
    ${FIRST_GCC_SOURCES}
    network.c
    tcpnetwork.c
    udpnetwork.c
    ssl_layer.c
    ${MORE_NETWORK_SOURCES}
    netstruc.h
    ../timerlib/timers.c
    ../memlib/sharemem.c 
    ../memlib/sharestruc.h
    ../memlib/memory_operations.c 
    ../procreglib/names.c 
    ../procreglib/registry.h
    ../typelib/typecode.c 
    ../typelib/sets.c 
    ../idlelib/idle.c 
    ../typelib/text.c 
    ../typelib/binarylist.c 

    ../sysloglib/syslog.c
    ../filesyslib/pathops.c  # syslog requires all of these... 
    ../filesyslib/winfiles.c
    ../filesyslib/filesys_local.h
    ../systemlib/system.c    # default log paths setup here
    ../systemlib/taskinfo.h

    
    ../deadstart/deadstart_core.c 
    ${LAST_GCC_SOURCES} )



set( NETLIB_PP_SOURCES )
set( SACK_BASE .. )
COPY_CPLUSPLUS( NETLIB_PP_SOURCES ${NETLIB_SOURCES} )

add_library( netlib  ${NETLIB_SOURCES} )
add_library( netlibpp  ${NETLIB_PP_SOURCES} )

target_link_libraries( netlib  MatrixSSL )
target_link_libraries( netlibpp  MatrixSSL )

SET_PROPERTY(TARGET netlib APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=netlib;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}netlib${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;NO_LOGGING;__NO_OPTIONS__;SACK_BAG_EXPORTS" ${MORE_DEFS} )
SET_PROPERTY(TARGET netlibpp APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=netlibpp;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}netlibpp${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;NO_LOGGING;__NO_OPTIONS__;SACK_BAG_EXPORTS" ${MORE_DEFS} )


if( WIN32 )
   target_link_libraries( netlib ws2_32 winmm iphlpapi )
   target_link_libraries( netlibpp ws2_32 winmm iphlpapi )
endif( WIN32 )


