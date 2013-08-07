

project( systray_shell.portable )

#########################################################
#### CMake Fixes for RC compilation 
#########################################################

macro( build_resource rcfile realfile ) 

    string( REPLACE "." "_" rc_file_stripped ${rcfile} )
    string( REPLACE "\\" "-" rc_file_stripped ${rc_file_stripped} )
    string( REPLACE "/" "-" rc_file_stripped ${rc_file_stripped} )
    
if( WATCOM )
  if( NOT TARGET generate_foo${rc_file_stripped} )
    string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
    string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
    message( tmp : ${WATCOM_CMAKE_CURRENT_SOURCE_DIR} )
    add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${realfile}
                     DEPENDS ${rcfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\..\\..\\..\\.. -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\${realfile} ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\..\\..\\..\\.. -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\${realfile} ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
 )
  ADD_CUSTOM_TARGET( generate_foo${rc_file_stripped} DEPENDS ${WATCOM_PROJECT_BINARY_DIR}/${realfile})
  endif( NOT TARGET generate_foo${rc_file_stripped} )
  ADD_DEPENDENCIES( ${PROJECT_NAME} generate_foo${rc_file_stripped} )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "option resource=${realfile}")
else( WATCOM )
endif( WATCOM )
endmacro( build_resource ) 

if( ${CMAKE_GENERATOR} MATCHES "Watcom WMake" )

if( NOT ${Android} )
SET( CMAKE_RC_COMPILER "rc" )
endif( NOT ${Android} )
endif( ${CMAKE_GENERATOR} MATCHES "Watcom WMake" )

if( WIN32 )
if( MINGW )
if( NOT ${Android} )
SET(CMAKE_RC_COMPILER_INIT "windres")
SET(CMAKE_RC_COMPILER "windres")
SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENABLE_LANGUAGE(RC)
endif( NOT ${Android} )
endif( MINGW )
endif( WIN32 ) 


#########################################################
####  End RC fixes
#########################################################



set( SOURCES_ROOT ../../../../ )
set( USE_SQLITE ON )

include( ../../../../CMakeSources.lst )

if( NOT WATCOM )
      set( RC  ${SOURCES_ROOT}/all_resources.rc )
endif( NOT WATCOM )

set( BASE_SOURCES 
	${RC}
	${TYPE_LIBRARY_SOURCES} 
        ${FILESYSTEM_SOURCES} 
        ${SYSTEM_LIBRARY_SOURCES}
        ${NETWORK_SOURCES}
        ${UNSORTED_SOURCES} 
        ${ODBC_SOURCES}
        ${OPTION_SOURCE}
        ${SQLITE_SOURCES}
        ${SYSTRAY_SOURCES} 
        ../systray_shell.c )


add_executable(${PROJECT_NAME} WIN32 
	${FIRST_GCC_PROGRAM_SOURCE}
	${BASE_SOURCES}
	${LAST_GCC_PROGRAM_SOURCE}
)
build_resource(  ${SOURCES_ROOT}/all_resources.rc all_resources.res )
target_link_libraries( ${PROJECT_NAME}  ${SACK_PLATFORM_LIBRARIES} )

message( test ${ExtraDefinitions} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}\";${ExtraDefinitions};__STATIC__;__NO_GUI__;__NO_MSGSVR__;USE_ODBC;SACK_BAG_EXPORTS;__NO_DEFAULT_INTERFACES__;__NO_NETWORK__;__DISABLE_UDP_SYSLOG__"
                  FOLDER utils
)
if( WIN32 )
#ws2_32 wsock32 
target_link_libraries( ${PROJECT_NAME} wsock32 ws2_32 winmm odbc32 rpcrt4  )
endif( WIN32 )
install_default_dest( ${PROJECT_NAME} )


