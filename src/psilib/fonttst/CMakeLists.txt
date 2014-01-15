if( BUILD_UNICODE )
else( BUILD_UNICODE)

project( test.fonts )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    fnttest.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests/PSI
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES})
install_mode_dest( ${PROJECT_NAME} )


endif( BUILD_UNICODE )
