
set( project test.html5.websocket.client )


set(BASE_SOURCES
    test.html5.websocket.client.c
)

sack_add_executable2(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/tests
)

install_default_dest( ${project} )

