

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
  SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${rcfile_out}")
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


add_subdirectory( portable )

