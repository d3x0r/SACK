
PROJECT( test_text_blocK_formatter )
add_program( ${PROJECT_NAME} test_block_writer.c )
if( NOT MONOLITHIC )
	my_target_link_libraries( ${PROJECT_NAME} bag.psi )
endif( NOT MONOLITHIC )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
install_default_dest( ${PROJECT_NAME} )
