# this CMakeList.txt is to build just thread/timer support library with
# its required dependancies.  This is not a full build and typically is
# not in the full build.

set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )
cmake_minimum_required(VERSION 3.0)

if(GCC )
set( FIRST_GCC_SOURCES ../../deadstart/deadstart_list.c )
set( LAST_GCC_SOURCES ../../deadstart/deadstart_lib.c 
          ../../deadstart/deadstart_end.c )
endif(GCC)

if( __ANDROID__ )
	set( MORE_DEFS "__ANDROID__=1" )
	set( MORE_DEFS ${MORE_DEFS} "__LINUX__=1" )
endif( __ANDROID__ )

set( SOURCES_ROOT ../../.. )

INCLUDE_DIRECTORIES( ${SOURCES_ROOT}/include )

#common resource used to copy a list of sources from C to C++
INCLUDE( ${SOURCES_ROOT}/src/timerlib.build/copymac.cmake )

#included to use DEFINE_DEFAULT
include( ${SOURCES_ROOT}/makefiles/cmake/DefaultInstall.cmake )

if( WIN32 )
else()
    SET( MORE_DEFS ${MORE_DEFS} __LINUX__=1 )
endif( WIN32 )

if( WIN32 )
set(MORE_NETWORK_SOURCES
  ${SOURCES_ROOT}/src/netlib/net_winsock2.c
  ${SOURCES_ROOT}/src/systemlib/oswin.c
)
endif( WIN32 )

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    SET( __64__ ON )
    set( MORE_DEFS ${MORE_DEFS} __64__=1 )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    DEFINE_DEFAULT( __64__ OFF )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


set( MATRIXSSL_VERSION 3.8.4 )
set( LIBRESSL_VERSION 2.5.4 )

#add_subdirectory( ${SOURCES_ROOT}/src/contrib/MatrixSSL/${MATRIXSSL_VERSION} MatrixSSL )
#INCLUDE_DIRECTORIES( ${SOURCES_ROOT}/src/contrib/MatrixSSL/${MATRIXSSL_VERSION} )
#set( NETLIB_SSL_SOURCES ${SOURCES_ROOT}/src/netlib/mssl_layer.c )
#set( SSL_LIBS 

add_subdirectory( ${SOURCES_ROOT}/src/contrib/libressl/${LIBRESSL_VERSION} libressl )
include_directories( ${SOURCES_ROOT}/src/contrib/libressl/${LIBRESSL_VERSION}/include )
set( NETLIB_SSL_SOURCES ${SOURCES_ROOT}/src/netlib/ssl_layer.c )
set( SSL_LIBS ssl tls crypto )

SET( SQLITE_VERSION 3.19.3 )
SET( SQLITE_SOURCES ${BAG_SQLITE_INTERFACE} ${SOURCES_ROOT}/src/contrib/sqlite/${SQLITE_VERSION}/sqlite3.c )
include_directories( ${SOURCES_ROOT}/src/contrib/sqlite/${SQLITE_VERSION} )



set( NETLIB_SOURCES    
    ${FIRST_GCC_SOURCES}
    ${SOURCES_ROOT}/src/netlib/network.c
    ${SOURCES_ROOT}/src/netlib/tcpnetwork.c
    ${SOURCES_ROOT}/src/netlib/udpnetwork.c
    ${NETLIB_SSL_SOURCES}
    ${MORE_NETWORK_SOURCES}
    ${SOURCES_ROOT}/src/netlib/netstruc.h
    ${SOURCES_ROOT}/src/typelib/http.c
    ${SOURCES_ROOT}/src/typelib/url.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/html5.websocket.common.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/html5.websocket.common.h
    ${SOURCES_ROOT}/src/netlib/html5.websocket/client/html5.websocket.client.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/client/local.h
    ${SOURCES_ROOT}/src/netlib/html5.websocket/server/html5.websocket.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/json/json_emitter.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/json/json_parser.c
    ${SOURCES_ROOT}/src/netlib/html5.websocket/json/json.h
    ${SOURCES_ROOT}/src/timerlib/timers.c
    ${SOURCES_ROOT}/src/memlib/sharemem.c 
    ${SOURCES_ROOT}/src/memlib/sharestruc.h
    ${SOURCES_ROOT}/src/memlib/memory_operations.c 
    ${SOURCES_ROOT}/src/procreglib/names.c 
    ${SOURCES_ROOT}/src/procreglib/registry.h
    ${SOURCES_ROOT}/src/typelib/typecode.c 
    ${SOURCES_ROOT}/src/typelib/sets.c 
    ${SOURCES_ROOT}/src/idlelib/idle.c 
    ${SOURCES_ROOT}/src/typelib/text.c 
    ${SOURCES_ROOT}/src/typelib/familytree.c 
    ${SOURCES_ROOT}/src/typelib/binarylist.c 

    ${SOURCES_ROOT}/src/sysloglib/syslog.c
    ${SOURCES_ROOT}/src/filesyslib/pathops.c  # syslog requires all of these... 
    ${SOURCES_ROOT}/src/filesyslib/winfiles.c
    ${SOURCES_ROOT}/src/filesyslib/filescan.c  # checkmask
    ${SOURCES_ROOT}/src/filesyslib/filesys_local.h
    ${SOURCES_ROOT}/src/systemlib/system.c    # default log paths setup here
    ${SOURCES_ROOT}/src/systemlib/spawntask.c    # default log paths setup here
    ${SOURCES_ROOT}/src/systemlib/args.c    # default log paths setup here
    ${SOURCES_ROOT}/src/systemlib/taskinfo.h

    ${SOURCES_ROOT}/src/configlib/configscript.c
    ${SOURCES_ROOT}/src/SQLlib/sqlstub.c
    ${SOURCES_ROOT}/src/SQLlib/sqlstruc.h
    ${SOURCES_ROOT}/src/SQLlib/sqlutil.c
    ${SOURCES_ROOT}/src/SQLlib/sqlwrap.c
    ${SOURCES_ROOT}/src/SQLlib/sqlparse3.c
    ${SOURCES_ROOT}/src/SQLlib/guid.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/getoption.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/getoption_new.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/getoption_v4.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/optionutil.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/optionutil_new.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/optionutil_v4.c
    ${SOURCES_ROOT}/src/SQLlib/optlib/optlib.h
    ${SOURCES_ROOT}/src/SQLlib/optlib/makeopts.mysql
    ${SOURCES_ROOT}/src/contrib/md5lib/md5c.c
    ${SOURCES_ROOT}/include/md5.h
    ${SOURCES_ROOT}/src/contrib/sha1lib/sha1.c
    ${SOURCES_ROOT}/include/sha1.h
    ${SOURCES_ROOT}/src/contrib/sha2lib/sha2.c
    ${SOURCES_ROOT}/src/utils/virtual_file_system/vfs.c
    ${SOURCES_ROOT}/src/utils/virtual_file_system/vfs_internal.h
    
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 

     )

if( NOT __NO_ODBC__ )
    set( MORE_DEFS ${MORE_DEFS} USE_ODBC )
else( NOT __ODBC__ )
    set( MORE_DEFS ${MORE_DEFS} __NO_ODBC__ )
endif( NOT __NO_ODBC__ )
set( MORE_DEFS ${MORE_DEFS} USE_SQLITE )
set( MORE_DEFS ${MORE_DEFS} __NO_GUI__ )

set( NETLIB_PP_SOURCES )
set( SACK_BASE ../.. )
COPY_CPLUSPLUS( NETLIB_PP_SOURCES ${NETLIB_SOURCES} )

set( NETLIB_SOURCES ${NETLIB_SOURCES} 
    ${SQLITE_SOURCES}
    ${LAST_GCC_SOURCES}
)

set( NETLIB_PP_SOURCES ${NETLIB_PP_SOURCES} 
    ${SQLITE_SOURCES}
    ${LAST_GCC_SOURCES}
)

set( MORE_DEFS ${MORE_DEFS} MD5_SOURCE )
set( MORE_DEFS ${MORE_DEFS} SHA1_SOURCE )


add_library( netlib  ${NETLIB_SOURCES} )
add_library( netlibpp  ${NETLIB_PP_SOURCES} )

target_link_libraries( netslib  ${SSL_LIBS} )
target_link_libraries( netslibpp ${SSL_LIBS} )

SET_PROPERTY(TARGET netslib APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=netlib;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}netlib${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;NO_LOGGING;SACK_BAG_EXPORTS" ${MORE_DEFS} )
SET_PROPERTY(TARGET netslibpp APPEND PROPERTY COMPILE_DEFINITIONS "TARGET_LABEL=netlibpp;TARGETNAME=\"${CMAKE_LIBRARY_PREFIX}netlibpp${CMAKE_LIBRARY_SUFFIX}\""
		"__NO_INTERFACE_SUPPORT__;NO_LOGGING;SACK_BAG_EXPORTS" ${MORE_DEFS} )


if( WIN32 )
   target_link_libraries( netslib ws2_32 winmm iphlpapi odbc32 Rpcrt4 )
   target_link_libraries( netslibpp ws2_32 winmm iphlpapi odbc32 Rpcrt4 )
else( WIN32 )
   target_link_libraries( netslib odbc )
   target_link_libraries( netslibpp odbc )
endif( WIN32 )

