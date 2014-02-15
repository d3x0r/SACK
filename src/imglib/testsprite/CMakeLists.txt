
set( project test.sprites )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    test.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/Image
)
install_mode_dest( ${project} )


set( project test.sprites2 )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    clicktest.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/Image
)
install_mode_dest( ${project} )

install( DIRECTORY images/ DESTINATION bin/images )
