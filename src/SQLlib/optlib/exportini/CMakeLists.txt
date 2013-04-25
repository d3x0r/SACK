
cmake_minimum_required(VERSION 2.8)

project( exportini ) 

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    exportini.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( MAKING_SACK_CORE )
install_mode_dest( ${PROJECT_NAME} )
else( MAKING_SACK_CORE )
install_default_dest( ${PROJECT_NAME} )
endif( MAKING_SACK_CORE )
