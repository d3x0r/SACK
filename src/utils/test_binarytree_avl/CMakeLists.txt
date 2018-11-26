
set( project test_avl_binary )

set(BASE_SOURCES
 	test_avl_binary.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


