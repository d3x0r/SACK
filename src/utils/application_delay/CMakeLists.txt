PROJECT( application_delay.module )

set(BASE_SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
    application_delay.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source( ${PROJECT_NAME} SHARED ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}" 
                  SUFFIX ""
                  PREFIX ""
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT_NAME} )

