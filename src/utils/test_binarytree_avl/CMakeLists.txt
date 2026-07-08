
set( project test_avl_binary )

set(BASE_SOURCES
 	test_avl_binary.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)

add_program( test_remove_binary test_remove_binary.c )
SET_TARGET_PROPERTIES(test_remove_binary PROPERTIES
        FOLDER tests
)


