cmake_minimum_required(VERSION 3.0)

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include/SACK )
STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
add_definitions( ${PLATFORM_DEFINES} )

### END include sack ###

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

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( SHELL_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shell.manifest )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
if( MSVC )
    SET( SHELL_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )


if( WATCOM )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
endif( WATCOM )


macro( build_resource rcfile realfile ) 
if( WATCOM )
  if( NOT TARGET generate_foo${rcfile} )
    string( REPLACE ".rc" ".res" rcfile_out ${realfile} )
    add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${rcfile_out}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${realfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${realfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${realfile}
 )
    ADD_CUSTOM_TARGET( generate_foo${rcfile} DEPENDS ${PROJECT_BINARY_DIR}/${rcfile_out})
  endif( NOT TARGET generate_foo${rcfile} )
  ADD_DEPENDENCIES( ${project} generate_foo${rcfile} )
  SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${rcfile_out}")
else( WATCOM )
endif( WATCOM )
endmacro( build_resource ) 

set( SOURCES_ROOT ../.. )

if( WIN32 )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
set(NET_SOURCES   ${SOURCES_ROOT}/netlib/net_winsock2.c )
endif( WIN32 )

  if( ${CMAKE_COMPILER_IS_GNUCC} ) 
  SET( FIRST_GCC_LIBRARY_SOURCE ${SOURCES_ROOT}/deadstart/deadstart_list.c ) 
  SET( FIRST_GCC_PROGRAM_SOURCE ${SOURCES_ROOT}/deadstart/deadstart_list.c ) 
  SET( LAST_GCC_LIBRARY_SOURCE ${SOURCES_ROOT}/deadstart/deadstart_lib.c ${SOURCES_ROOT}/deadstart/deadstart_end.c ) 
  SET( LAST_GCC_PROGRAM_SOURCE ${SOURCES_ROOT}/deadstart/deadstart_lib.c ${SOURCES_ROOT}/deadstart/deadstart_prog.c ${SOURCES_ROOT}/deadstart/deadstart_end.c ) 
  endif()
  if( ${MSVC}${WATCOM} ) 
  SET( LAST_GCC_PROGRAM_SOURCE ${SOURCES_ROOT}/deadstart/deadstart_prog.c ) 
  endif()

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
#    ${SOURCES_ROOT}/netlib/network.c 
#    ${SOURCES_ROOT}/netlib/udpnetwork.c 
#    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
#    ${NET_SOURCES} 
#    ${SOURCES_ROOT}/msgsvr/client/client.c

    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/typelib/msgqueue.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SHELL_MORE_SOURCES}
    ${SNPRINTF_SOURCES}
########################
# SOURCES
########################
    shell.c 
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)

if( WIN32 )
#  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )


set( project shell )
set( NO_AUTO_DEADSTART_CORE ON )
add_portable_program(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_MSGSVR__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${SHELL_LINK_FLAGS}"
)
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
STRING ( REPLACE "/" "\\" SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${SRCDIR}\\shell.manifest\" -outputresource:\"${THISDIR}\\shell.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
if( WIN32 )
#ws2_32 wsock32 
target_link_libraries( ${project}  winmm odbc32)
endif( WIN32 )

install( TARGETS ${project} DESTINATION bin )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
#    ${SOURCES_ROOT}/netlib/network.c 
#    ${SOURCES_ROOT}/netlib/udpnetwork.c 
#    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
#    ${NET_SOURCES} 
#    ${SOURCES_ROOT}/msgsvr/client/client.c
#    ${SOURCES_ROOT}/SQLlib/sqlstub.c
#    ${SOURCES_ROOT}/SQLlib/sqlutil.c
#    ${SOURCES_ROOT}/SQLlib/sqlwrap.c
#    ${SOURCES_ROOT}/SQLlib/sqlparse3.c
#    ${SQLITE_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${SOURCES_ROOT}/systemlib/system.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/typelib/msgqueue.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SNPRINTF_SOURCES}
    ${SHELL_MORE_SOURCES}
########################
# SOURCES
########################
    wshell.c 
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)

set( project wshell.admin )


set( NO_AUTO_DEADSTART_CORE ON )

if (WIN32)
    #set(CMAKE_SHARED_LINKER_FLAGS /MANIFEST:NO)
endif()


add_portable_program(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_MSGSVR__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__" )

SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${SHELL_LINK_FLAGS}"
)
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
if( WIN32 )
#ws2_32 wsock32 
target_link_libraries( ${project}  winmm odbc32)
endif( WIN32 )
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${SRCDIR}\\shell.manifest\" -outputresource:\"${THISDIR}\\wshell.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
install( TARGETS ${project} DESTINATION bin )


set( project wshell )
add_portable_program(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_MSGSVR__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__" )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER utils )
target_link_libraries( ${project}  ${PLATFORM_LIBRARIES})
install( TARGETS ${project} DESTINATION bin )




project( Shells )
