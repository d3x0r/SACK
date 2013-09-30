

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
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES} )
endif( WATCOM )

set( project launchpad.portable )

set( RESOURCES ../launchpad.rc )
if( MINGW OR MSVC )
set( MORE_SOURCES ${RESOURCES} )
endif( MINGW OR MSVC )

set( SOURCES_ROOT ../../../.. )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

if( WIN32 )
set( NET_SOURCES     ${SOURCES_ROOT}/netlib/net_winsock2.c  )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c 
    ${SOURCES_ROOT}/systraylib/systray.c
)
endif( WIN32 )

if( MSVC ) 
SET( ExtraSources ${SOURCES_ROOT}/systemlib/args.c )
endif( MSVC )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../launchpad ${MORE_SOURCES}
    ${SOURCES_ROOT}/netlib/network.c 
    ${SOURCES_ROOT}/netlib/udpnetwork.c 
    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${ExtraSources}
    ${SOURCES_ROOT}/systemlib/system.c
    ${MORE_SYSTEM_SOURCES}
#    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/systemlib/args.c
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)

if( MINGW )

SET(CMAKE_RC_COMPILER_INIT "windres")
SET(CMAKE_RC_COMPILER "windres")
SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENABLE_LANGUAGE(RC)
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
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__STATIC__;TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\"" )

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


set( SOURCES_ROOT ../../../.. )

if( SNPRINTF_SOURCES )
set( SNPRINTF_SOURCES    ${SOURCES_ROOT}/../${SNPRINTF_SOURCES} )
endif(SNPRINTF_SOURCES)

if( WIN32 )
set( NET_SOURCES     ${SOURCES_ROOT}/netlib/net_winsock2.c  )
set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/systemlib/oswin.c )
endif( WIN32 )

if( MSVC ) 
SET( ExtraSources ${SOURCES_ROOT}/systemlib/args.c )
endif( MSVC )

SET( SQLITE_VERSION 3.7.16.2 )
SET( SQLITE_SOURCES ${BAG_SQLITE_INTERFACE} ${SOURCES_ROOT}/sqlite/${SQLITE_VERSION}/sqlite3.c )
include_directories( ${SOURCES_ROOT}/sqlite/${SQLITE_VERSION} )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../launchpad ${MORE_SOURCES}
    ${SOURCES_ROOT}/netlib/network.c 
    ${SOURCES_ROOT}/netlib/udpnetwork.c 
    ${SOURCES_ROOT}/netlib/tcpnetwork.c 
    ${NET_SOURCES}
    ${SOURCES_ROOT}/memlib/sharemem.c
    ${SOURCES_ROOT}/memlib/memory_operations.c
    ${SOURCES_ROOT}/timerlib/timers.c
    ${SOURCES_ROOT}/idlelib/idle.c
    ${SOURCES_ROOT}/procreglib/names.c
    ${SOURCES_ROOT}/configlib/configscript.c
    ${SOURCES_ROOT}/sysloglib/syslog.c
    ${ExtraSources}
    ${SOURCES_ROOT}/systemlib/system.c
#    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${SOURCES_ROOT}/typelib/typecode.c
    ${SOURCES_ROOT}/typelib/text.c
    ${SOURCES_ROOT}/typelib/familytree.c
    ${SOURCES_ROOT}/typelib/sets.c
    ${SOURCES_ROOT}/typelib/binarylist.c
    ${SOURCES_ROOT}/filesyslib/filescan.c
    ${SOURCES_ROOT}/filesyslib/pathops.c
    ${SOURCES_ROOT}/filesyslib/winfiles.c
    ${SOURCES_ROOT}/systemlib/spawntask.c
    ${MORE_SYSTEM_SOURCES}
    ${SOURCES_ROOT}/systemlib/args.c
    ${SOURCES_ROOT}/utils/service/service_stub.c
    
#    ${SOURCES_ROOT}/SQLlib/sqlstub.c
#    ${SOURCES_ROOT}/SQLlib/sqlwrap.c
#    ${SOURCES_ROOT}/SQLlib/sqlutil.c
#    ${SOURCES_ROOT}/SQLlib/sqlparse3.c
#    ${SOURCES_ROOT}/SQLlib/optlib/getoption.c
#    ${SOURCES_ROOT}/SQLlib/optlib/getoption_new.c
#    ${SOURCES_ROOT}/sqlite/sqlite_interface.c
#    ${SOURCES_ROOT}/psilib/option_frame.c
#    ${SQLITE_SOURCES}
    ${SNPRINTF_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
    ${SOURCES_ROOT}/deadstart/deadstart_core.c 
)

if( MINGW )

SET(CMAKE_RC_COMPILER_INIT "windres")
SET(CMAKE_RC_COMPILER "windres")
SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENABLE_LANGUAGE(RC)

endif()


sack_add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;__STATIC__;BUILD_SERVICE;USE_ODBC;USE_SQLITE;__NO_MSGSVR__;__NO_GUI__;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\"" )
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

