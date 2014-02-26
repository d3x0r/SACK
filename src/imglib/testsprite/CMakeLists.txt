
set( project test.sprites )

set(BASE_SOURCES
    test.c
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/Image
)
install_default_dest( ${project} )


set( project test.sprites2 )

set(BASE_SOURCES
    clicktest.c
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/Image
)
install_default_dest( ${project} )

install( DIRECTORY images/ DESTINATION bin/images )
