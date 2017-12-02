cmake_minimum_required(VERSION 3.0)

project (sack_core_amalgmate)



if( NOT WIN32 )
    ADD_DEFINITIONS( -D__LINUX__ )
endif( NOT WIN32 )
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    ADD_DEFINITIONS( -D__64__ )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

if( NOT WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
    ADD_DEFINITIONS( -D__LINUX64__ )
endif( NOT WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
ADD_DEFINITIONS( -DNO_OPEN_MACRO )


set( PPC_TOOL "c:/tools/ppc.exe" CACHE STRING "Define location of amalgamation tool"  )


set( SACK_BASE M:/sack )
set( SACK_BINARY_SDK_BASE C:/general/build/mingw64-x86/sack-r/release_out/binary  )

if( EXISTS ${SACK_BASE} )

set( INCLUDES
	-I${SACK_BASE}/include 
	-I${SACK_BASE}/src/contrib/sha1lib 
	-I${SACK_BASE}/src/contrib/sqlite/3.22.0-pre-nul  )

# __NO_OPTIONS__ __NO_INTERFACE_SUPPORT__ __NO_ODBC__
set( DEFINITIONS   __STATIC__ 
		USE_SQLITE USE_SQLITE_INTERFACE FORCE_COLOR_MACROS 
                _CRT_SECURE_NO_WARNINGS  NEED_SHLAPI NEED_SHLOBJ 
                 )

foreach( DEF ${DEFINITIONS} )
   ADD_DEFINITIONS( -D${DEF} )
ENDFOREACH()

SET( EXTRA_HEADERS
	${SACK_BASE}/include/stdhdrs.h
        ${SACK_BASE}/include/network.h
        ${SACK_BASE}/include/pssql.h
        ${SACK_BASE}/include/sqlgetoption.h
        ${SACK_BASE}/include/idle.h
        ${SACK_BASE}/include/filesys.h
        ${SACK_BASE}/include/sack_vfs.h
        ${SACK_BASE}/include/json_emitter.h
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
	
	${SACK_BASE}/src/netlib/html5.websocket/html5.websocket.common.c
	${SACK_BASE}/src/netlib/html5.websocket/client/html5.websocket.client.c
	${SACK_BASE}/src/netlib/html5.websocket/server/html5.websocket.c
	${SACK_BASE}/src/netlib/html5.websocket/json/json.h
	${SACK_BASE}/src/netlib/html5.websocket/json/json_parser.c
	${SACK_BASE}/src/netlib/html5.websocket/json/json_emitter.c
	${SACK_BASE}/src/netlib/network.c
	${SACK_BASE}/src/netlib/tcpnetwork.c
	${SACK_BASE}/src/netlib/udpnetwork.c
        ${SACK_BASE}/src/netlib/net_winsock2.c
	${SACK_BASE}/src/netlib/ssl_layer.c
        
	${SACK_BASE}/src/contrib/sqlite/sqlite_interface.c
        ${SACK_BASE}/src/sqllib/sqlstruc.h
        ${SACK_BASE}/src/configlib/configscript.c
        ${SACK_BASE}/src/sqllib/sqlstub.c
        ${SACK_BASE}/src/sqllib/sqlwrap.c
        ${SACK_BASE}/src/sqllib/sqlutil.c
        ${SACK_BASE}/src/sqllib/guid.c
        ${SACK_BASE}/src/sqllib/sqlparse3.c
        ${SACK_BASE}/src/sqllib/optlib/getoption.c
        ${SACK_BASE}/src/sqllib/optlib/getoption_new.c
        ${SACK_BASE}/src/sqllib/optlib/getoption_v4.c
        ${SACK_BASE}/src/sqllib/optlib/optionutil.c
        ${SACK_BASE}/src/sqllib/optlib/optionutil_new.c
        ${SACK_BASE}/src/sqllib/optlib/optionutil_v4.c
	${SACK_BASE}/src/deadstart/deadstart_lib.c
)


set( AMALG_COMMAND ${PPC_TOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/src/sack.cc ${EXTRA_SOURCES} )
set( AMALG_COMMAND2 ${PPC_TOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/src/sack.h ${EXTRA_HEADERS} )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/sack.cc
  			#COMMAND cmake -E echo ${AMALG_COMMAND}
   			 COMMAND ${AMALG_COMMAND}
                        #COMMAND cmake -E echo ${AMALG_COMMAND2}
   			COMMAND ${AMALG_COMMAND2}
                   DEPENDS ${EXTRA_SOURCES} ${HEADER_SOURCES}
				   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )                                   
  add_custom_target( doit DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/sack.cc )
endif( EXISTS ${SACK_BASE} )


