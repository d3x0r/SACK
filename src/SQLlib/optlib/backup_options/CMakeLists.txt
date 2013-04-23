
project( OptionBackup )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    backup_options
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARY} ${SACK_PSI_LIBRARY} )
install_mode_dest( ${PROJECT_NAME} )

project( OptionRestore )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    restore_options
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARY} ${SACK_PSI_LIBRARY} )
install_mode_dest( ${PROJECT_NAME} )

