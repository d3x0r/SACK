
project( banner_command )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    cmd.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets )
install_default_dest( ${PROJECT_NAME} )


