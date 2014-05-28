
PROJECT( test_text_blocK_formatter )
add_program( ${PROJECT_NAME} test_block_writer.c )


SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
install_default_dest( ${PROJECT_NAME} )

