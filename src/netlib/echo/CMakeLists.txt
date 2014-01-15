PROJECT( test_echo_server )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} echo ${LAST_GCC_PROGRAM_SOURCE} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT_NAME} )

