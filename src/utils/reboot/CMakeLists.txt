

set( project rebootnow )


macro( build_resource rcfile ) 
if( WATCOM )
  if( NOT TARGET generate_foo${rcfile} )
    string( REPLACE ".rc" ".res" rcfile_out ${rcfile} )
    add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${rcfile_out}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${rcfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
 )
    ADD_CUSTOM_TARGET( generate_foo${rcfile} DEPENDS ${PROJECT_BINARY_DIR}/${rcfile_out})
  endif( NOT TARGET generate_foo${rcfile} )
  ADD_DEPENDENCIES( ${project} generate_foo${rcfile} )
  SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${PROJECT_BINARY_DIR}/${rcfile_out}")
else( WATCOM )
endif( WATCOM )
endmacro( build_resource ) 

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    reboot.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
SET_PROPERTY( TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "DEDICATE_REBOOT" )
SET_TARGET_PROPERTIES(${project} PROPERTIES         FOLDER utils  )

target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

set( project reboot.monitor )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    reboot.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
if( NOT __CLR__ )
  set( BASE_SOURCES ${BASE_SOURCES}     reboot.rc )
  build_resource( reboot.rc )
endif( NOT __CLR__ )

SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)


string( REPLACE ";" " " PLATFORM_LIBRARIES_ARG "${SACK_PLATFORM_LIBRARIES}" )
string( REPLACE ";" " " PLATFORM_DEFINES_ARG "${PLATFORM_DEFINES}" )
ExternalProject_Add( ${project}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE} 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)
#add_subdirectory( portable )

