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

INCLUDE_DIRECTORIES( ../../include )

if( WIN32 )
    SET( MORE_SOURCES net_winsock2.c )
endif( WIN32 )

add_library( netlib 
    ${FIRST_GCC_SOURCES}
    network.c
    tcpnetwork.c
    udpnetwork.c
    ../timerlib/timers.c
    ../memlib/sharemem.c 
    ../memlib/memory_operations.c 
    ../procreglib/names.c 
    ../typelib/typecode.c 
    ../typelib/sets.c 
    ../idlelib/idle.c 
    ../typelib/text.c 
    ../typelib/binarylist.c 
    ../deadstart/deadstart_core.c 
    ${LAST_GCC_SOURCES} )

SET_PROPERTY(TARGET netlib APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=netlib;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}netlib${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;NO_LOGGING;__NO_OPTIONS__;SACK_BAG_EXPORTS" )

if( WIN32 )
   target_link_libraries( netlib ws2_32 winmm )
endif( WIN32 )


