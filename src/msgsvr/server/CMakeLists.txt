
set( project sack.msgsvr.service.plugin. )


set(SOURCES server2 )


sack_add_library( ${project} SHARED ${SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  PREFIX ""
                  SUFFIX ""
                  FOLDER "Message Service"
)

if( NOT MSVC )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  OUTPUT_NAME "sack.msgsvr.service.plugin"
)
endif( NOT MSVC )


target_link_libraries( ${project} ${SACK_LIBRARIES} )
if( NOT MAKING_SACK_CORE )
	target_link_libraries( ${project} message_service )
endif( NOT MAKING_SACK_CORE )
install_mode_dest( ${project} )


set( project msgsvr )

set(SERVER_SOURCES 
server3
)

add_program( ${project} WIN32 ${SERVER_SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES
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

add_program( ${project} WIN32 ${SERVER_SOURCES} )
SET_PROPERTY( TARGET ${project} APPEND PROPERTY
                  COMPILE_FLAGS  "-DSERVERMSG_SOURCE -DBAG"
)
SET_TARGET_PROPERTIES( ${project} PROPERTIES FOLDER "Message Service"  )
target_link_libraries( ${project} ${SACK_LIBRARIES} )

install_mode_dest( ${project} )

endif( NOT __LINUX__ )

