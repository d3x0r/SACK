cmake_minimum_required(VERSION 3.0)

project (sack.amalgamate)

set( INCLUDE_ODBC ON )
set( INCLUDE_UUID ON )
set( INCLUDE_MSGSVR_LIBRARY OFF )
set( INCLUDE_UUID ON )

if( NOT WIN32 )
    ADD_DEFINITIONS( -D__LINUX__ )
else( NOT WIN32 )
    ADD_DEFINITIONS( -D_CRT_NONSTDC_NO_DEPRECATE )
endif( NOT WIN32 )

#if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
#    ADD_DEFINITIONS( -D__64__ )
#endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


ADD_DEFINITIONS( -DNO_OPEN_MACRO )

# __NO_OPTIONS__ __NO_INTERFACE_SUPPORT__ __NO_ODBC__
set( DEFINITIONS   __STATIC__
		USE_SQLITE USE_SQLITE_INTERFACE FORCE_COLOR_MACROS NO_FILEOP_ALIAS
                _CRT_SECURE_NO_WARNINGS  NEED_SHLAPI NEED_SHLOBJ JSON_PARSER_MAIN_SOURCE "SQLITE_ENABLE_LOCKING_STYLE=0"
		MINIMAL_JSON_PARSE_ALLOCATE
                 )

if( NOT INCLUDE_ODBC )
	SET( DEFINITIONS ${DEFINITIONS} __NO_ODBC__ )
endif( NOT INCLUDE_ODBC )

if( APPLE )
  set( DEFINITIONS ${DEFINITIONS} __MAC__ )
endif( APPLE )

set( SACK_BASE ../.. )
if( EXISTS ${SACK_BASE} )

set( INCLUDES
	-I${SACK_BASE}/include
	-I${SACK_BASE}/src/contrib/sha1lib
	-I${SACK_BASE}/src/contrib/sqlite/3.23.0-MySqlite
 )

FILE( WRITE ${CMAKE_BINARY_DIR}/config.ppc "/*CMake Option defined*/\n" )


foreach( DEF ${DEFINITIONS} )
   ADD_DEFINITIONS( -D${DEF} )
   #FILE( APPEND ${CMAKE_BINARY_DIR}/config.ppc  "#define ${DEF}\n" )
ENDFOREACH()

FILE( APPEND ${CMAKE_BINARY_DIR}/config.ppc  "#define NO_AUTO_VECTLIB_NAMES\n" )


SET( EXTRA_HEADERS
	${SACK_BASE}/include/stdhdrs.h
        ${SACK_BASE}/include/network.h
        ${SACK_BASE}/include/pssql.h
        ${SACK_BASE}/include/sqlgetoption.h
        ${SACK_BASE}/include/idle.h
        ${SACK_BASE}/include/filesys.h
        ${SACK_BASE}/include/sack_vfs.h
        ${SACK_BASE}/include/json_emitter.h
        ${SACK_BASE}/include/html5.websocket.client.h
        ${SACK_BASE}/include/html5.websocket.h
        ${SACK_BASE}/include/http.h
        ${SACK_BASE}/include/procreg.h
        ${SACK_BASE}/include/configscript.h
	${SACK_BASE}/include/salty_generator.h
        ${SACK_BASE}/include/sackcomm.h
	${SACK_BASE}/include/translation.h
)

SET( EXTRA_SOURCES
	${SACK_BASE}/src/utils/virtual_file_system/vfs.c
	${SACK_BASE}/src/utils/virtual_file_system/vfs_internal.h
        ${SACK_BASE}/src/salty_random_generator/salty_generator.c
        ${SACK_BASE}/src/salty_random_generator/crypt_util.c
        ${SACK_BASE}/src/contrib/md5lib/md5c.c
        ${SACK_BASE}/src/contrib/sha1lib/sha1.c
        ${SACK_BASE}/src/contrib/sha2lib/sha2.c
	${SACK_BASE}/src/deadstart/deadstart_core.c
	${SACK_BASE}/src/sysloglib/syslog.c
	${SACK_BASE}/src/systemlib/system.c
	${SACK_BASE}/src/systemlib/spawntask.c
	${SACK_BASE}/src/systemlib/args.c
	${SACK_BASE}/src/systemlib/oswin.c
	${SACK_BASE}/src/systemlib/taskinfo.h
	${SACK_BASE}/src/timerlib/timers.c
	${SACK_BASE}/src/idlelib/idle.c
	${SACK_BASE}/src/procreglib/names.c
	${SACK_BASE}/src/procreglib/registry.h
	${SACK_BASE}/src/memlib/sharemem.c
	${SACK_BASE}/src/memlib/memory_operations.c
	${SACK_BASE}/src/memlib/sharestruc.h
	${SACK_BASE}/src/filesyslib/winfiles.c
	${SACK_BASE}/src/filesyslib/filescan.c
	${SACK_BASE}/src/filesyslib/pathops.c
	${SACK_BASE}/src/filesyslib/filesys_local.h
	${SACK_BASE}/src/typelib/typecode.c
	${SACK_BASE}/src/typelib/text.c
	${SACK_BASE}/src/typelib/binarylist.c
	${SACK_BASE}/src/typelib/familytree.c
	${SACK_BASE}/src/typelib/sets.c
	${SACK_BASE}/src/typelib/http.c
	${SACK_BASE}/src/typelib/url.c
	
        ${SACK_BASE}/src/commlib/sackcomm.c

	${SACK_BASE}/src/netlib/html5.websocket/html5.websocket.common.c
	${SACK_BASE}/src/netlib/html5.websocket/client/html5.websocket.client.c
	${SACK_BASE}/src/netlib/html5.websocket/server/html5.websocket.c
	${SACK_BASE}/src/netlib/html5.websocket/json/json_parser.c
	${SACK_BASE}/src/netlib/html5.websocket/json/json6_parser.c
	${SACK_BASE}/src/netlib/html5.websocket/json/json_emitter.c
	${SACK_BASE}/src/netlib/network.c
	${SACK_BASE}/src/netlib/tcpnetwork.c
	${SACK_BASE}/src/netlib/udpnetwork.c
        ${SACK_BASE}/src/netlib/net_winsock2.c
	${SACK_BASE}/src/netlib/ssl_layer.c

	${SACK_BASE}/src/contrib/sqlite/sqlite_interface.c
        ${SACK_BASE}/src/SQLlib/sqlstruc.h
        ${SACK_BASE}/src/configlib/configscript.c
        ${SACK_BASE}/src/SQLlib/sqlstub.c
        ${SACK_BASE}/src/SQLlib/sqlwrap.c
        ${SACK_BASE}/src/SQLlib/sqlutil.c
        ${SACK_BASE}/src/SQLlib/guid.c
        ${SACK_BASE}/src/SQLlib/sqlparse3.c
        ${SACK_BASE}/src/SQLlib/optlib/getoption.c
        ${SACK_BASE}/src/SQLlib/optlib/getoption_v4.c
        ${SACK_BASE}/src/SQLlib/optlib/optionutil.c
        ${SACK_BASE}/src/SQLlib/optlib/optionutil_v4.c

	${SACK_BASE}/src/translationlib/translate.c

	${SACK_BASE}/src/deadstart/deadstart_lib.c
)

if( INCLUDE_MSGSVR_LIBRARY )
set( EXTRA_SOURCES ${EXTRA_SOURCES} 
		#${SACK_BASE}/src/msgsvr/client/global.h
		${SACK_BASE}/src/typelib/msgqueue.c
		${SACK_BASE}/src/msgsvr/client/client_common.c
		${SACK_BASE}/src/msgsvr/client/client_client.c
		${SACK_BASE}/src/msgsvr/client/client_events.c
		${SACK_BASE}/src/msgsvr/client/client_input.c
		${SACK_BASE}/src/msgsvr/client/client_local.c
		${SACK_BASE}/src/msgsvr/client/client_output.c
		${SACK_BASE}/src/msgsvr/client/client_service.c 
		${SACK_BASE}/src/msgsvr/summoner/construct.c
)
endif( INCLUDE_MSGSVR_LIBRARY )

if( __ANDROID__ )
	set( EXTRA_SOURCES ${EXTRA_SOURCES} 
		${SACK_BASE}/src/netlib/android_ifaddrs.h
		${SACK_BASE}/src/netlib/android_ifaddrs.c
	)
endif( __ANDROID__ )


set( UUID_SOURCES
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_md5.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_sha1.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_dce.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_mac.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_prng.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_str.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_time.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_ui128.c
  ${SACK_BASE}/src/contrib/uuid-1.6.2/uuid_ui64.c
)

if( INCLUDE_UUID )
  set( EXTRA_SOURCES ${EXTRA_SOURCES} ${UUID_SOURCES} )
  if( MAKING_SACK_CORE )
    add_definitions( -D__INTERNAL_UUID__ )
  endif( MAKING_SACK_CORE )
  add_definitions( -DUUID_SOURCE )
endif( INCLUDE_UUID )



SET( BUILD_AMALGAM on )

set( C_SOURCES
        ${SACK_BASE}/src/contrib/sqlite/3.23.0-MySqlite/sqlite3.c
)
add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${C_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}
				   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                   DEPENDS ${C_SOURCES}
)

if( WIN32 )
	set( PPCTOOL c:/tools/ppc.exe )
elseif( APPLE )
	set( PPCTOOL /usr/local/bin/ppc )
else()
	set( PPCTOOL /usr/bin/ppc )
endif( WIN32 )

if( EXISTS ${PPCTOOL} )
  set( AMALG_COMMAND ${PPCTOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/sack.cc ${EXTRA_SOURCES} )
  set( AMALG_COMMAND2 ${PPCTOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/sack.h ${EXTRA_HEADERS} )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sack.cc
  			#COMMAND cmake -E echo ${AMALG_COMMAND}
   			 COMMAND ${AMALG_COMMAND}
                        #COMMAND cmake -E echo ${AMALG_COMMAND2}
   			COMMAND ${AMALG_COMMAND2}
                   DEPENDS ${EXTRA_SOURCES} ${EXTRA_HEADERS} always_rebuild
				   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_custom_target( doit DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sack.cc )

endif( EXISTS ${PPCTOOL} )

else( EXISTS ${SACK_BASE} )

foreach( DEF ${DEFINITIONS} )
   ADD_DEFINITIONS( -D${DEF} )
ENDFOREACH()

endif( EXISTS ${SACK_BASE} )

