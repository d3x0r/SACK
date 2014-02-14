

set( project test.html5.websocket )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    test.html5.websocket.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER utils/tests
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

set( project test.html5.caller )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    test.html5.caller.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER utils/tests
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )
