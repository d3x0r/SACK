

SET( PROJECT systray_shell.portable )

#########################################################
#### CMake Fixes for RC compilation 
#########################################################
set( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ )

macro( build_resource rcfile realfile ) 

    string( REPLACE "." "_" rc_file_stripped ${rcfile} )
    string( REPLACE ":" "_" rc_file_stripped ${rc_file_stripped} )
    string( REPLACE "\\" "_" rc_file_stripped ${rc_file_stripped} )
    string( REPLACE "/" "_" rc_file_stripped ${rc_file_stripped} )
    
if( WATCOM )
 SET( CMAKE_RC_COMPILER "rc" )
  if( NOT TARGET generate_foo${rc_file_stripped} )
    string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
    string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
    string( REPLACE "/" "\\" WATCOM_SOURCES_ROOT ${SOURCES_ROOT} )
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../${realfile}
                     DEPENDS ${SOURCES_ROOT}/${rcfile} 
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\..\\..\\..\\.. -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${CMAKE_CURRENT_BINARY_DIR}\\..\\${realfile} ${WATCOM_SOURCES_ROOT}\\${rcfile}
 )
  ADD_CUSTOM_TARGET( generate_foo${rc_file_stripped} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../${realfile})
  endif( NOT TARGET generate_foo${rc_file_stripped} )
  ADD_DEPENDENCIES( ${PROJECT} generate_foo${rc_file_stripped} )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES LINK_FLAGS "option resource=${CMAKE_CURRENT_BINARY_DIR}/../${realfile}")
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

if( __ANDROID__ )
set( NEED_UUID 1 )
endif( __ANDROID__ )


set( USE_SQLITE ON )
set( NO_SSL ON )
include( ../../../../CMakeSources.lst )

if( NOT WATCOM )
      set( RC  ${SOURCES_ROOT}/all_resources.rc )
endif( NOT WATCOM )

# update the short list of sources before it gets added to the big list...
if( NEED_UUID )
     # RC is just a extra list of sources
    set( RC ${RC}   ${UUID_SOURCES} )
endif( NEED_UUID )
if( UNIX )
    set( RC ${RC} ${SACK_MSG_SERVICES} )
endif( UNIX )

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

ADD_DEFINITIONS( -DSACK_BAG_EXPORTS)
add_DEFINITIONS( -DUSE_SQLITE )
if( NOT __NO_ODBC__ )
ADD_DEFINITIONS( -DUSE_ODBC )
endif( NOT __NO_ODBC__ )


set( NO_AUTO_DEADSTART_CORE ON )
add_portable_program(${PROJECT} WIN32 
	${FIRST_GCC_PROGRAM_SOURCE}
	${BASE_SOURCES}
	${LAST_GCC_PROGRAM_SOURCE}
)
build_resource(  all_resources.rc all_resources.res )
target_link_libraries( ${PROJECT}  ${SACK_PLATFORM_LIBRARIES} )


