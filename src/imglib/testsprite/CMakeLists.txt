
set( project test.sprites )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    test.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests/Image
)
target_link_libraries( ${project} ${SACK_LIBRARIES})
install_mode_dest( ${project} )


set( project test.sprites2 )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    clicktest.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests/Image
)
target_link_libraries( ${project} ${SACK_LIBRARIES})
install_mode_dest( ${project} )

install( DIRECTORY images/ DESTINATION bin/images )
