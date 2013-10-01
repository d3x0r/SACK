
set( project test_service_client )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    service_client
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable( ${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER "Message Service"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )

install_mode_dest( ${project} )

