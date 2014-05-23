
set( project test_service_client )


set(BASE_SOURCES
    service_client
)

add_program( ${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER "Message Service"
)

install_mode_dest( ${project} )

