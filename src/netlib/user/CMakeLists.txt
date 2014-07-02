PROJECT( test_user_connect )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
user 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

PROJECT( test_multi_connect )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
add_executable( ${PROJECT_NAME} ${FIRST_GCC_PROGRAM_SOURCE} 
multi_user 
${LAST_GCC_PROGRAM_SOURCE}
)
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )
