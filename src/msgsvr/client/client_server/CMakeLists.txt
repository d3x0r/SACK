
set( project test_msgclient_service )


set(BASE_SOURCES
    client_service
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER "Message Service"
)

install_default_dest( ${project} )

