# this CMakeList.txt is to build just thread/timer support library with
# its required dependancies.  This is not a full build and typically is
# not in the full build.

set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )
cmake_minimum_required(VERSION 3.0)

INCLUDE_DIRECTORIES( ../../include )

INCLUDE( copymac.cmake )

if( __ANDROID__ )
	set( MORE_DEFS "__ANDROID__=1" )
	set( MORE_DEFS ${MORE_DEFS} "__LINUX__=1" )
endif( __ANDROID__ )

if( GCC OR __ANDROID__ )
set( THIS_FIRST_GCC_SOURCES ../deadstart/deadstart_list.c )
set( THIS_LAST_GCC_SOURCES ../deadstart/deadstart_lib.c 
          ../deadstart/deadstart_end.c )
endif( GCC OR __ANDROID__ )
  
if( WIN32 )
	set( MORE_TIMER_SOURCES ../systemlib/oswin.c )
endif( WIN32 )

set( TIMERLIB_SOURCES     ${THIS_FIRST_GCC_SOURCES}
    ../timerlib/timers.c
    ../memlib/sharemem.c 
    ../memlib/memory_operations.c 
    ../memlib/sharestruc.h
    ../typelib/typecode.c 
    ../typelib/sets.c 
    ../idlelib/idle.c 
    ../typelib/text.c 
    ../typelib/binarylist.c 
    ../procreglib/names.c
    ../procreglib/registry.h
    ../sysloglib/syslog.c
    ../filesyslib/pathops.c  # syslog requires all of these... 
    ../filesyslib/winfiles.c
    ../filesyslib/filesys_local.h
    ../systemlib/system.c    # default log paths setup here
    ../systemlib/spawntask.c    # default log paths setup here
    ../systemlib/args.c    # default log paths setup here
    ../systemlib/taskinfo.h
    ../deadstart/deadstart_core.c 
	${MORE_TIMER_SOURCES}
    ${THIS_LAST_GCC_SOURCES} )
    
set( TIMERLIB_PP_SOURCES )
set( SACK_BASE .. )
COPY_CPLUSPLUS( TIMERLIB_PP_SOURCES ${TIMERLIB_SOURCES} )

add_library( timerlib  ${TIMERLIB_SOURCES} )
add_library( timerlibpp  ${TIMERLIB_PP_SOURCES} )

SET_PROPERTY(TARGET timerlib APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=timerlib;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}timerlib${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;__NO_OPTIONS__;__STATIC__;SACK_BAG_EXPORTS" 
		${MORE_DEFS} )
SET_PROPERTY(TARGET timerlibpp APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=timerlib;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}timerlib${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;__NO_OPTIONS__;__STATIC__;SACK_BAG_EXPORTS" 
		${MORE_DEFS} )

if( WIN32 )
   target_link_libraries( timerlib winmm )
endif( WIN32 )


