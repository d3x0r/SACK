
project( sqlcmd )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    sqlcmd
${LAST_GCC_PROGRAM_SOURCE}
)


add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} ${SACK_PSI_LIBRARY} )
install_default_dest( ${PROJECT_NAME} )


add_subdirectory( portable )



