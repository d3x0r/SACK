
cmake_minimum_required(VERSION 2.8)

project( file_cleaner )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    cleaner.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
COMPILE_DEFINITIONS "DEDICATE_REBOOT"
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

