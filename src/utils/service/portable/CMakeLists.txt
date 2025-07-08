cmake_minimum_required(VERSION 3.13)

if( MSVC )
# 4197 - top-level volatile in cast is ignored (so?)
# 4090 - 'function': different 'volatile' qualifiers (usually don't care)
  add_definitions( -wd4996  -wd4244 -wd4068 -wd6011 -wd4197 -wd4090 -wd4197 )
endif( MSVC )

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/makefiles/cmake/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include )
STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
if( PLATFORM_DEFINES )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
endif()

# Enable for memory leak discovery
#add_definitions( -DUSE_SACK_CUSTOM_MEMORY_ALLOCATION )
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

if( NOT CMAKE_CXX_FLAGS_DEBUG MATCHES "-D_DEBUG" )
	set(  CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG" )
endif( NOT CMAKE_CXX_FLAGS_DEBUG MATCHES "-D_DEBUG" )

if( NOT CMAKE_CXX_FLAGS_DEBUG MATCHES "-D_DEBUG_INFO" )
	set(  CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG_INFO" )
endif( NOT CMAKE_CXX_FLAGS_DEBUG MATCHES "-D_DEBUG_INFO" )

# visual studio headers change with _DEBUG defined... so it can't be release with _DEBUG defined
if( NOT CMAKE_CXX_FLAGS_RELWITHDEBINFO MATCHES "-D_DEBUG_INFO" )
	set(  CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG_INFO" )
endif( NOT CMAKE_CXX_FLAGS_RELWITHDEBINFO MATCHES "-D_DEBUG_INFO" )

if( NOT CMAKE_C_FLAGS_DEBUG MATCHES "-D_DEBUG" )
	set(  CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG" )
endif( NOT CMAKE_C_FLAGS_DEBUG MATCHES "-D_DEBUG" )
if( NOT CMAKE_C_FLAGS_DEBUG MATCHES "-D_DEBUG_INFO" )
	set(  CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG_INFO" )
endif( NOT CMAKE_C_FLAGS_DEBUG MATCHES "-D_DEBUG_INFO" )

# visual studio headers change with _DEBUG defined... so it can't be release with _DEBUG defined
if( NOT CMAKE_C_FLAGS_RELWITHDEBINFO MATCHES "-D_DEBUG_INFO" )
	set(  CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG_INFO" )
endif( NOT CMAKE_C_FLAGS_RELWITHDEBINFO MATCHES "-D_DEBUG_INFO" )

if( WIN32 )

project( service_test.portable )


if( __ANDROID__ )
	set( MORE_DEFS "__ANDROID__=1" )
	set( MORE_DEFS ${MORE_DEFS} "__LINUX__=1" )
endif( __ANDROID__ )

if( WIN32 )
	set( MORE_TIMER_SOURCES ../../../systemlib/oswin.c )
endif( WIN32 )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
else()
    SET( SERVICE_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../service.manifest )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

if( WIN32 )
    SET( SERVICE_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( WIN32 )


set( TIMERLIB_SOURCES     
    ../../../timerlib/timers.c
    ../../../memlib/sharemem.c 
    ../../../memlib/memory_operations.c 
    ../../../memlib/sharestruc.h
    ../../../typelib/typecode.c 
    ../../../typelib/sets.c 
    ../../../idlelib/idle.c 
    ../../../typelib/text.c 
    ../../../typelib/binarylist.c 
    ../../../procreglib/names.c
    ../../../procreglib/registry.h
    ../../../sysloglib/syslog.c
    ../../../filesyslib/pathops.c  # syslog requires all of these... 
    ../../../filesyslib/winfiles.c
    ../../../filesyslib/filesys_local.h
    ../../../systemlib/system.c    # default log paths setup here
    ../../../systemlib/spawntask.c    # default log paths setup here
    ../../../systemlib/args.c    # default log paths setup here
    ../../../systemlib/taskinfo.h
    ../../../deadstart/deadstart_core.c 
    ${SERVICE_MORE_SOURCES}
    ${MORE_TIMER_SOURCES}
    )


#add_subdirectory( "../../../timerlib.build" minlib )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../service_stub.c
    ../service_test.c
    ${TIMERLIB_SOURCES}
    ${SERVICE_MORE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)


sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__" )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	LINK_FLAGS "${SERVICE_LINK_FLAGS}" 
        FOLDER utils
 
        )
SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "__NO_INTERFACE_SUPPORT__;__NO_OPTIONS__;__STATIC__;SACK_BAG_EXPORTS" )
if( WIN32 )
    # 4267 'initializing': conversion from 'size_t' to 'FPI', possible loss of data 
    # 4244 initializing': conversion from 'uintptr_t' to 'BLOCKINDEX', possible loss of data  
    # 4197 - top-level volatile in cast is ignored (so?)
    # 4090 - 'function': different 'volatile' qualifiers (usually don't care)
    SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_FLAGS "-wd4267 -wd4244 -wd4197 -wd4090" )
endif( WIN32 )

#target_link_libraries( ${PROJECT_NAME} timerlib )

target_link_libraries( ${PROJECT_NAME} ws2_32 winmm )
install_default_dest( ${PROJECT_NAME} )
if( NOT CMAKE_COMPILER_IS_GNUCC )
  set( CMAKE_COMPILER_IS_GNUCC OFF ) # set non-blank option for the following
endif( NOT CMAKE_COMPILER_IS_GNUCC )
  
if( ( WIN32 AND CMAKE_COMPILER_IS_GNUCC ) OR ( WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" ) )
STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
STRING ( REPLACE "/" "\\" SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command(TARGET
                     ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${SRCDIR}\\..\\service.manifest\" -outputresource:\"$<TARGET_FILE:${PROJECT_NAME}>\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

endif( WIN32 )

if( WIN32 )
	add_executable( signal_event ../signal_event.c )
	install_default_dest( signal_event )
endif( WIN32 )
if( UNIX )
	add_executable( signal_event ../signal_event_linux.c )
	install_default_dest( signal_event )
endif( UNIX )