cmake_minimum_required(VERSION 3.20)

project( launchpad.portable )

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

set( project launchpad.portable )

set( RESOURCES ../launchpad.rc )
if( MINGW OR MSVC )
set( MORE_SOURCES ${RESOURCES} )
endif( MINGW OR MSVC )

set( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../ )
include( ${SOURCES_ROOT}CMakeSources.lst )

if( WIN32 )
set( NET_SOURCES     ${SOURCES_ROOT}src/netlib/net_winsock2.c  )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}src/systemlib/oswin.c 
    ${SOURCES_ROOT}src/systraylib/systray.c
)
else( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}src/systraylib/systray_client.c 
    ${SACK_MSG_SERVICES}
)
endif( WIN32 )

if( MSVC ) 
SET( ExtraSources ${SOURCES_ROOT}src/systemlib/args.c )
endif( MSVC )

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
    ../launchpad.c ${MORE_SOURCES}
    ${SOURCES_ROOT}src/netlib/network.c 
    ${SOURCES_ROOT}src/netlib/network${NETWORK_OS}.c 
    ${SOURCES_ROOT}src/netlib/network_addresses.c 
    ${SOURCES_ROOT}src/netlib/udpnetwork.c 
    ${SOURCES_ROOT}src/netlib/tcpnetwork.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}src/memlib/sharemem.c
    ${SOURCES_ROOT}src/memlib/memory_operations.c
    ${SOURCES_ROOT}src/timerlib/timers.c
    ${SOURCES_ROOT}src/idlelib/idle.c
    ${SOURCES_ROOT}src/procreglib/names.c
    ${SOURCES_ROOT}src/configlib/configscript.c
    ${SOURCES_ROOT}src/sysloglib/syslog.c
    ${ExtraSources}
    ${SOURCES_ROOT}src/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
#    ${SOURCES_ROOT}src/systemlib/spawntask.c
    ${SOURCES_ROOT}src/typelib/typecode.c
    ${SOURCES_ROOT}src/typelib/text.c
    ${SOURCES_ROOT}src/typelib/sets.c
    ${SOURCES_ROOT}src/typelib/binarylist.c
    ${SOURCES_ROOT}src/filesyslib/filescan.c
    ${SOURCES_ROOT}src/filesyslib/pathops.c
    ${SOURCES_ROOT}src/filesyslib/winfiles.c
    ${SOURCES_ROOT}src/systemlib/spawntask.c
    ${SOURCES_ROOT}src/systemlib/args.c
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}src/deadstart/deadstart_core.c 
)

if( MINGW )
	if( NOT HOST_UNIX )
		SET(CMAKE_RC_COMPILER_INIT "windres")
		SET(CMAKE_RC_COMPILER "windres")
		SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
		ENABLE_LANGUAGE(RC)
	endif( NOT HOST_UNIX )
elseif( WATCOM )
string( REPLACE ".rc" ".res" WATCOM_RESOURCES ${RESOURCES} )
SET( CMAKE_RC_COMPILER "rc" )
string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${WATCOM_RESOURCES}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES}
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\.. -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\${WATCOM_RESOURCES} ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\${RESOURCES}
 )
ADD_CUSTOM_TARGET( "generate_foo${project}" DEPENDS ${PROJECT_BINARY_DIR}/${WATCOM_RESOURCES})

endif()

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
target_link_libraries( ${project}  ${SACK_PLATFORM_LIBRARIES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__STATIC__;__STATIC_GLOBALS__;NO_DEADSTART_DLLMAIN;NO_SSL" )

if( WATCOM )
ADD_DEPENDENCIES( ${project} "generate_foo${project}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES 
          LINK_FLAGS "option resource=${WATCOM_RESOURCES}")
endif( WATCOM )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER utils/launcher
)
if( WATCOM )
ADD_DEPENDENCIES( ${project} "generate_foo${project}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${WATCOM_RESOURCES}")
endif( WATCOM )
if( WIN32 )
target_link_libraries( ${project} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} )
else( WIN32 )
   if( __ANDROID__ )
      target_link_libraries( ${project} dl )
   else( __ANDROID__ )
      target_link_libraries( ${project} dl pthread )
   endif( __ANDROID__ )
endif( WIN32 )
install_default_dest_binary( ${project} )

if( WIN32 )

set( project launchpad.portable.service )



if( WIN32 )
set( NET_SOURCES     ${SOURCES_ROOT}src/netlib/net_winsock2.c  )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}src/systemlib/oswin.c )
endif( WIN32 )

if( MSVC ) 
SET( ExtraSources ${SOURCES_ROOT}src/systemlib/args.c )
endif( MSVC )

SET( SQLITE_VERSION 3.45.0-TableAlias  )
SET( SQLITE_SOURCES ${BAG_SQLITE_INTERFACE} ${SOURCES_ROOT}src/sqlite/${SQLITE_VERSION}/sqlite3.c )
include_directories( ${SOURCES_ROOT}src/sqlite/${SQLITE_VERSION} )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../launchpad.c ${MORE_SOURCES}
    ${SOURCES_ROOT}src/netlib/network.c 
    ${SOURCES_ROOT}src/netlib/network${NETWORK_OS}.c 
    ${SOURCES_ROOT}src/netlib/network_addresses.c 
    ${SOURCES_ROOT}src/netlib/udpnetwork.c 
    ${SOURCES_ROOT}src/netlib/tcpnetwork.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}src/memlib/sharemem.c
    ${SOURCES_ROOT}src/memlib/memory_operations.c
    ${SOURCES_ROOT}src/timerlib/timers.c
    ${SOURCES_ROOT}src/idlelib/idle.c
    ${SOURCES_ROOT}src/procreglib/names.c
    ${SOURCES_ROOT}src/configlib/configscript.c
    ${SOURCES_ROOT}src/sysloglib/syslog.c
    ${ExtraSources}
    ${SOURCES_ROOT}src/systemlib/system.c
#    ${SOURCES_ROOT}src/systemlib/spawntask.c
    ${SOURCES_ROOT}src/typelib/typecode.c
    ${SOURCES_ROOT}src/typelib/text.c
    ${SOURCES_ROOT}src/typelib/familytree.c
    ${SOURCES_ROOT}src/typelib/sets.c
    ${SOURCES_ROOT}src/typelib/binarylist.c
    ${SOURCES_ROOT}src/filesyslib/filescan.c
    ${SOURCES_ROOT}src/filesyslib/pathops.c
    ${SOURCES_ROOT}src/filesyslib/winfiles.c
    ${SOURCES_ROOT}src/systemlib/spawntask.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}src/systemlib/args.c
    ${SOURCES_ROOT}src/utils/service/service_stub.c
    
#    ${SOURCES_ROOT}src/SQLlib/sqlstub.c
#    ${SOURCES_ROOT}src/SQLlib/sqlwrap.c
#    ${SOURCES_ROOT}src/SQLlib/sqlutil.c
#    ${SOURCES_ROOT}src/SQLlib/sqlparse3.c
#    ${SOURCES_ROOT}src/SQLlib/optlib/getoption.c
#    ${SOURCES_ROOT}src/SQLlib/optlib/getoption_new.c
#    ${SOURCES_ROOT}src/sqlite/sqlite_interface.c
#    ${SOURCES_ROOT}src/psilib/option_frame.c
#    ${SQLITE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}src/deadstart/deadstart_core.c 
)



sack_add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;__STATIC__;__STATIC_GLOBALS__;BUILD_SERVICE;USE_ODBC;USE_SQLITE;__NO_MSGSVR__;__NO_GUI__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;NO_DEADSTART_DLLMAIN;NO_SSL" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER utils/launcher
)
if( WATCOM )
ADD_DEPENDENCIES( ${project} "generate_foo${project}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${WATCOM_RESOURCES}")
endif( WATCOM )
target_link_libraries( ${project}  ${SACK_PLATFORM_LIBRARIES} )

target_link_libraries( ${project} ws2_32 wsock32 winmm odbc32 )
install_default_dest_binary( ${project} )

endif( WIN32 )

