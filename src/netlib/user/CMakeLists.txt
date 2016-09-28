PROJECT( test_user_connect )

sack_add_executable( ${PROJECT_NAME}
	user.c
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

PROJECT( test_multi_connect )

sack_add_executable( ${PROJECT_NAME} 
multi_user.c
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )
