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
add_definitions( ${PLATFORM_DEFINES} )

STRING( REPLACE " " ";" K12_SOURCES ${K12_SOURCES} )

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



IF( NOT SOURCES_ROOT )
	set( SOURCES_ROOT ../../../../.. )
ENDIF( NOT SOURCES_ROOT )

SET_SOURCE_FILES_PROPERTIES( ${K12_SOURCES} ${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c PROPERTIES COMPILE_FLAGS ${K12_INCLUDES} )

if( WIN32 )
  set( NET_SOURCES     ${SOURCES_ROOT}/src/netlib/net_winsock2.c  )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )


if( MINGW OR MSVC )
    set( MORE_RESOURCES ../sfx.rc )
endif( MINGW OR MSVC )


set(BASE_SOURCES
    ../vfs_extracter.c ../../vfs.c ${EXTRACTER_MORE_SORUCES} ${MORE_RESOURCES}
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
    ${SNPRINTF_SOURCES}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 
)

set( FLAGS "ALT_VFS_NAME;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )

set( project sack_vfs_extracter_sfx )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

if( NOT CMAKE_COMPILER_IS_GNUCC )
	set( CMAKE_COMPILER_IS_GNUCC OFF )
endif( NOT CMAKE_COMPILER_IS_GNUCC )

STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

SET( EXTRACTER_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/extracter.manifest )
STRING( REPLACE "/" "\\" MT_EXTRACTER_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/extracter.manifest )
STRING( REPLACE "\\portable" "" MT_EXTRACTER_MORE_SOURCES ${MT_EXTRACTER_MORE_SOURCES} )
#message( "MT_EXTRACTER_MORE_SOURCES ${MT_EXTRACTER_MORE_SOURCES}" )
#SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )

add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${MT_EXTRACTER_MORE_SOURCES}\" -outputresource:\"${THISDIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )




set( project sack_vfs_extracter_sfx_win )

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${MT_EXTRACTER_MORE_SOURCES}\" -outputresource:\"${THISDIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )


# MIN is for portable console extracter(not standalong header)
set( project sack_vfs_extracter_min )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${MT_EXTRACTER_MORE_SOURCES}\" -outputresource:\"${THISDIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

if( WIN32 )
# MIN_WIN is for portable window-mode extracter (not standalong header)
set( project sack_vfs_extracter_min_win )

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${MT_EXTRACTER_MORE_SOURCES}\" -outputresource:\"${THISDIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

endif( WIN32 )
