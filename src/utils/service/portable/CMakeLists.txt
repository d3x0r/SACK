cmake_minimum_required(VERSION 2.8)

include( ${SACK_BASE}/makefiles/cmake/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include )
STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
if( PLATFORM_DEFINES )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
endif()
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


project( service_test.portable )


if( __ANDROID__ )
	set( MORE_DEFS "__ANDROID__=1" )
	set( MORE_DEFS ${MORE_DEFS} "__LINUX__=1" )
endif( __ANDROID__ )

  
if( WIN32 )
	set( MORE_TIMER_SOURCES ../../../systemlib/oswin.c )
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
    ${MORE_TIMER_SOURCES}
    )

add_subdirectory( "../../../timerlib.build" minlib )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../service_stub.c
    ../service_test.c
    ${TIMERLIB_SOURCES}
    ${SERVICE_MORE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)


sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__" )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	LINK_FLAGS "${SERVICE_LINK_FLAGS}" 
        FOLDER utils
 
        )
SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "__NO_INTERFACE_SUPPORT__;__NO_OPTIONS__;__STATIC__;SACK_BAG_EXPORTS" )

target_link_libraries( ${PROJECT_NAME} timerlib )

target_link_libraries( ${PROJECT_NAME} ws2_32 )
install_default_dest( ${PROJECT_NAME} )
if( NOT CMAKE_COMPILER_IS_GNUCC )
  set( CMAKE_COMPILER_IS_GNUCC OFF ) # set non-blank option for the following
endif( NOT CMAKE_COMPILER_IS_GNUCC )
  
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
STRING ( REPLACE "/" "\\" SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command(TARGET
                     ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${SRCDIR}\\..\\service.manifest\" -outputresource:\"${THISDIR}\\${PROJECT_NAME}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

