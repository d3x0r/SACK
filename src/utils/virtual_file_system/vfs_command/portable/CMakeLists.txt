cmake_minimum_required(VERSION 3.0)

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

    if( UNIX )
        if( NOT APPLE )
            set( CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed" )
            set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed" )
            set( CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed" )
        endif( NOT APPLE )
    endif( UNIX )

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


if( WATCOM )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
endif( WATCOM )




set( SOURCES_ROOT ${SACK_BASE} )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )

set(PROG_SOURCES
    ../vfs_command.c ../../vfs.c
)

set(PROG2_SOURCES
    ../vfs_command.c ../../vfs_fs.c
)

set(PROG3_SOURCES
    ../vfs_command.c ../../vfs_os.c
)

set(BASE_SOURCES
    ${SOURCES_ROOT}/src/memlib/sharemem.c
    ${SOURCES_ROOT}/src/memlib/memory_operations.c
    ${SOURCES_ROOT}/src/timerlib/timers.c
    #${SOURCES_ROOT}/src/idlelib/idle.c
    ${SOURCES_ROOT}/src/procreglib/names.c
    #${SOURCES_ROOT}/src/configlib/configscript.c
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
    #${RANDOM_SOURCES}
	${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c
    #${SNPRINTF_SOURCES}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c
)

####################################################################
#####
##################################################################

set( project sack_vfs_command.portable )
sack_add_executable(${project} ${PROG_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "VFS_DISK_DATATYPE=uint32_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

####################################################################

set( project sack_vfs_command.64.portable )
sack_add_executable(${project} ${PROG_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "VFS_DISK_DATATYPE=uint64_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

####################################################################
#####
##################################################################

set( project sack_vfs_fs_command.portable )
sack_add_executable(${project} ${PROG2_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "USE_VFS_FS_INTERFACE;VFS_DISK_DATATYPE=uint32_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

####################################################################

set( project sack_vfs_fs_command.64.portable )
sack_add_executable(${project} ${PROG2_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "USE_VFS_FS_INTERFACE;VFS_DISK_DATATYPE=uint64_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

####################################################################
#####
##################################################################

set( project sack_vfs_os_command.portable )
sack_add_executable(${project} ${PROG3_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "USE_VFS_OS_INTERFACE;VFS_DISK_DATATYPE=uint32_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

##################################################################


set( project sack_vfs_os_command.64.portable )
sack_add_executable(${project} ${PROG3_SOURCES} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "USE_VFS_OS_INTERFACE;VFS_DISK_DATATYPE=uint64_t" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

