
project( test.scroll.text )

set(BASE_SOURCES
    test_scrolltext.c
)

add_program(${PROJECT_NAME} ${BASE_SOURCES} )
if( NOT MONOLITHIC )
	my_target_link_libraries( ${PROJECT_NAME} bag.psi )
endif( NOT MONOLITHIC )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests/PSI
)
install_mode_dest( ${PROJECT_NAME} )


