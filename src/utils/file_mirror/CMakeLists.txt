
macro( build_resource rcfile ) 
if( WATCOM )
  if( NOT TARGET generate_foo${rcfile} )
    string( REPLACE ".rc" ".res" rcfile_out ${rcfile} )
    add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${rcfile_out}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${rcfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile} )
    ADD_CUSTOM_TARGET( generate_foo${rcfile} DEPENDS ${PROJECT_BINARY_DIR}/${rcfile_out})
  endif( NOT TARGET generate_foo${rcfile} )
  ADD_DEPENDENCIES( ${PROJECT_NAME} generate_foo${rcfile} )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "option resource=${rcfile_out}")
else( WATCOM )
endif( WATCOM )
endmacro( build_resource ) 

set( project FileMirror )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    account.c relay.c
    relay.rc
${LAST_GCC_PROGRAM_SOURCE}
)
add_program(${project} WIN32 ${BASE_SOURCES} )
build_resource( relay.rc )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER utils/filemirror
)


set( project FileMirror.verify )
add_program(${project} WIN32 ${BASE_SOURCES} )
build_resource( relay.rc )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER utils/filemirror
)


add_subdirectory( controller )

