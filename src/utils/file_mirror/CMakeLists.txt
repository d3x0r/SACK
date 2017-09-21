
macro( build_resource rcfile ) 
if( WATCOM )
 SET( CMAKE_RC_COMPILER "rc" )

  if( NOT TARGET generate_foo${rcfile} )
    string( REPLACE ".rc" ".res" rcfile_out ${rcfile} )
     string( REPLACE "/" "\\" resfile_out ${CMAKE_CURRENT_BINARY_DIR}/${rcfile_out} )
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${rcfile_out}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${rcfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${CMAKE_CURRENT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -fo${resfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${rcfile} )
    ADD_CUSTOM_TARGET( generate_foo${rcfile} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${rcfile_out})
  endif( NOT TARGET generate_foo${rcfile} )
  ADD_DEPENDENCIES( ${project} generate_foo${rcfile} )
  SET_TARGET_PROPERTIES(${project} PROPERTIES LINK_FLAGS "option resource=${resfile_out}")
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

