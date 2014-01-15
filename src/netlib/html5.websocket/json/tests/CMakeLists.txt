

set( project test.json.parser )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    test.json.parser.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER utils/tests
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

