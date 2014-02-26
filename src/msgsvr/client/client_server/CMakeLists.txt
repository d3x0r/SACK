
set( project test_msgclient_service )


set(BASE_SOURCES
    client_service
)

sack_add_executable2(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER "Message Service"
)

install_default_dest( ${project} )

