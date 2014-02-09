
set( project sack.msgsvr.service.plugin. )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
server2
${LAST_GCC_LIBRARY_SOURCE}
)


add_library_force_source( ${project} SHARED ${SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  OUTPUT_NAME "sack.msgsvr.service.plugin"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER "Message Service"
)

target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_mode_dest( ${project} )


set( project msgsvr )

set(SERVER_SOURCES 
${FIRST_GCC_PROGRAM_SOURCE}
server3
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source( ${project} WIN32 ${SERVER_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DSERVERMSG_SOURCE -DBAG"
                  FOLDER "Message Service"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )

install_mode_dest( ${project} )

if( NOT __LINUX__ )
set( project sack.msgsvr.service )

set(SERVER_SOURCES 
${FIRST_GCC_PROGRAM_SOURCE}
server_service
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source( ${project} WIN32 ${SERVER_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DSERVERMSG_SOURCE -DBAG"
                  FOLDER "Message Service"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )

install_mode_dest( ${project} )

endif( NOT __LINUX__ )

