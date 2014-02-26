

set( project test.html5.websocket )


set(BASE_SOURCES
    test.html5.websocket.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/tests
)
install_default_dest( ${project} )

set( project test.html5.caller )


set(BASE_SOURCES
    test.html5.caller.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/tests
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )
