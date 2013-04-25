PROJECT( unicode_to_ascii )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    unicode_ascii.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source( ${PROJECT_NAME} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT_NAME} )

