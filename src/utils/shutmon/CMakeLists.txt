project( shutdown.monitor )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    shutmon.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
           COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
           FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

