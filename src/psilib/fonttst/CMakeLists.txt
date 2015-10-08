if( BUILD_UNICODE )
else( BUILD_UNICODE)

project( test.fonts )

set(BASE_SOURCES
    fnttest.c
)

add_program( ${PROJECT_NAME} ${BASE_SOURCES} )
if( NOT MONOLITHIC )
	my_target_link_libraries( ${PROJECT_NAME} bag.psi )
endif( NOT MONOLITHIC )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests/PSI
)
install_mode_dest( ${PROJECT_NAME} )


endif( BUILD_UNICODE )
