
PROJECT( test_text_blocK_formatter )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} test_block_writer.c ${LAST_GCC_PROGRAM_SOURCE} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT_NAME} )

