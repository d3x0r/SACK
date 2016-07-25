

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



set( project sack_vfs_extracter_sfx )

set( SOURCES_ROOT ../../../../.. )

if( WIN32 )
  set( NET_SOURCES     ${SOURCES_ROOT}/src/netlib/net_winsock2.c  )
  set( MORE_SYSTEM_SOURCES   ${SOURCES_ROOT}/src/systemlib/oswin.c )
endif( WIN32 )

set(BASE_SOURCES
    ../vfs_extracter.c ../memory_dll_loader.c ../../vfs.c ${EXTRACTER_MORE_SORUCES}
    ${SOURCES_ROOT}/src/memlib/sharemem.c
    ${SOURCES_ROOT}/src/memlib/memory_operations.c
    ${SOURCES_ROOT}/src/timerlib/timers.c
    ${SOURCES_ROOT}/src/idlelib/idle.c
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
    ${SOURCES_ROOT}/src/configlib/configscript.c
    ${SHA1_SOURCES}
    ${SHA2_SOURCES}
    #${RANDOM_SOURCES}
	${SOURCES_ROOT}/src/salty_random_generator/salty_generator.c
    ${SNPRINTF_SOURCES}
    ${SOURCES_ROOT}/src/deadstart/deadstart_core.c 
)

set( FLAGS "ALT_VFS_NAME;__NO_IDLE__;__NO_INTERFACE_SUPPORT__;__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__;NO_DEADSTART_DLLMAIN" )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

message( "something is ${CMAKE_CURRENT_BINARY_DIR}\\${project}.exe" )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\..\\extracter.manifest\" -outputresource:\"${CMAKE_CURRENT_BINARY_DIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )



set( project sack_vfs_extracter_sfx_win )

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "STANDALONE_HEADER;${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

message( "something is ${CMAKE_CURRENT_BINARY_DIR}\\${project}.exe" )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\..\\extracter.manifest\" -outputresource:\"${CMAKE_CURRENT_BINARY_DIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )



set( project sack_vfs_extracter_min )

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "ALT_VFS_NAME;${FLAGS}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
      FOLDER utils/vfs
)
if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

message( "something is ${CMAKE_CURRENT_BINARY_DIR}\\sack_vfs_extracter.exe" )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\..\\extracter.manifest\" -outputresource:\"${CMAKE_CURRENT_BINARY_DIR}\\${project}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
target_link_libraries( ${project} ${PLATFORM_LIBRARIES} )
install_default_dest( ${project} )


