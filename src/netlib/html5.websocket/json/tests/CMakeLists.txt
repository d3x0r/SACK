

set( project test.json.parser )


set(BASE_SOURCES
    test.json.parser.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/tests
)
install_default_dest( ${project} )

