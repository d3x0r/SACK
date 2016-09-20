INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} )

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

set( SOURCES_ROOT ../../../../.. )

if( WIN32 )
  set( NET_SOURCES    ${SOURCES_ROOT}/src/netlib/network.c 
                ${SOURCES_ROOT}/src/netlib/tcpnetwork.c 
				${SOURCES_ROOT}/src/netlib/udpnetwork.c 
				${SOURCES_ROOT}/src/netlib/net_winsock2.c 
				${SOURCES_ROOT}/src/idlelib/idle.c
				 )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )

set(BASE_SOURCES
#        ../../resources.rc

    ../vfs_runner.c ../memory_dll_loader.c ${SOURCES_ROOT}/src/utils/virtual_file_system/vfs.c
	${NET_SOURCES}
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
    ${SOURCES_ROOT}/src/contrib/sha1lib/sha1.c
    ${SOURCES_ROOT}/src/contrib/sha2lib/sha2.c
    #${RANDOM_SOURCES}
	${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c
    ${SNPRINTF_SOURCES}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 
)



#### Console Mode SFX #######
set( project sack_vfs_runner_sfx )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN;NO_EXPORTS" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
#install_default_dest( ${project} )


#### Windows Mode SFX #######

set( project sack_vfs_runner_sfx_win )

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;STANDALONE_HEADER;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN;NO_EXPORTS" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
#install_default_dest( ${project} )




set( project sack_vfs_runner_min )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN;NO_EXPORTS" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
#install_default_dest( ${project} )

set( project sack_vfs_runner_win_min )
sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN;NO_EXPORTS" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${SACK_PLATFORM_LIBRARIES} )
#install_default_dest( ${project} )

if( 0 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
	INSTALL( PROGRAMS $<TARGET_FILE:sack_vfs_runner_min> DESTINATION bin RENAME chatment${CMAKE_EXECUTABLE_SUFFIX} )
else( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
	INSTALL( PROGRAMS $<TARGET_FILE:sack_vfs_runner_win_min> DESTINATION bin RENAME chatment${CMAKE_EXECUTABLE_SUFFIX} )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
endif( 0 )