cmake_minimum_required(VERSION 3.15)
set( CMAKE_INSTALL_MESSAGE "LAZY" )

PROJECT( vfs_runner.portable )
MESSAGE( "CONFIGURE WITH:" ${CMAKE_TOOLCHAIN_FILE} )

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/makefiles/cmake/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include )
include( ${SACK_BASE}/makefiles/cmake/CMakeDefs.txt )

STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
if( PLATFORM_DEFINES )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
endif()
add_definitions( ${PLATFORM_DEFINES} )

STRING( REPLACE " " ";" K12_SOURCES ${K12_SOURCES} )

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        )
#   if(${flag_var} MATCHES "/MD")
#      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#   endif(${flag_var} MATCHES "/MD")
   if(${flag_var} MATCHES "-br")
      string(REGEX REPLACE "-br" "" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "-br")
endforeach(flag_var)

if( WATCOM )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
endif( WATCOM )

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



set( project sack_vfs_runner_sfx )

set( SOURCES_ROOT ../../../../.. )

if( WIN32 )
  set( NET_SOURCES     ${SOURCES_ROOT}/src/netlib/net_winsock2.c  )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )

set(BASE_SOURCES
    ../vfs_runner.c ../memory_dll_loader.c ../../vfs.c
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
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/src/typelib/typecode.c
    ${SOURCES_ROOT}/src/typelib/text.c
    ${SOURCES_ROOT}/src/typelib/sets.c
    ${SOURCES_ROOT}/src/typelib/binarylist.c
    ${SOURCES_ROOT}/src/filesyslib/filescan.c
    ${SOURCES_ROOT}/src/filesyslib/pathops.c
    ${SOURCES_ROOT}/src/filesyslib/winfiles.c
    ${SHA1_SOURCES}
    ${SHA2_SOURCES}
    ${SHA3_SOURCES}
    ${K12_SOURCES}
    #${RANDOM_SOURCES}
	${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c
      ${SOURCES_ROOT}/src/salty_random_generator/crypt_util.c
      ${SOURCES_ROOT}/src/salty_random_generator/block_shuffle.c
    ${SNPRINTF_SOURCES}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 
)

SET_SOURCE_FILES_PROPERTIES( ${K12_SOURCES}
	${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c 
	${SOURCES_ROOT}/src/salty_random_generator/crypt_util.c
	${SOURCES_ROOT}/src/salty_random_generator/block_shuffle.c
	PROPERTIES COMPILE_FLAGS "${K12_INCLUDES} -DNO_SSL" )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
if( WIN32 )
    # 4197 - top-level volatile in cast is ignored (so?)
    # 4090 - 'function': different 'volatile' qualifiers (usually don't care)
    SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_FLAGS "-wd4197 -wd4090" )
endif( WIN32 )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )



set( project sack_vfs_runner_sfx_win )

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
if( WIN32 )
    # 4197 - top-level volatile in cast is ignored (so?)
    # 4090 - 'function': different 'volatile' qualifiers (usually don't care)
    SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_FLAGS "-wd4197 -wd4090" )
endif( WIN32 )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )





set( project sack_vfs_runner_min )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
if( WIN32 )
    # 4197 - top-level volatile in cast is ignored (so?)
    # 4090 - 'function': different 'volatile' qualifiers (usually don't care)
    SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_FLAGS "-wd4197 -wd4090" )
endif( WIN32 )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


