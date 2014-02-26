
set( project test_filescan )

set(BASE_SOURCES
 	test_filescan.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


