
cmake_minimum_required(VERSION 2.8)

project( EditOptions )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    editopt
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES}  )
install_mode_dest( ${PROJECT_NAME} )

