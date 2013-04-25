cmake_minimum_required(VERSION 2.8)

project( TouchDiagnostic )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    touch_diag.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS "TARGET_NAME=\"${PROJECT_NAME}\";TARGET_LABEL=${PROJECT_NAME}"
        FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES})
install_mode_dest( ${PROJECT_NAME} )


