
set( project test_service_client )


set(BASE_SOURCES
    service_client
)

sack_add_executable2( ${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER "Message Service"
)

install_mode_dest( ${project} )

