
set( project test_msgclient_service )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    client_service
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER "Message Service"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )

install_mode_dest( ${project} )

