
cmake_minimum_required(VERSION 2.8)

project( banner_command )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    cmd.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets )
install_default_dest( ${PROJECT_NAME} )


