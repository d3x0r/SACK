
set( MESSAGE_SERVICE_SOURCES 
		client_client.c
                client_common.c
		client_events.c
                client_input.c
                client_local.c
                client_output.c
		client_service.c 
                global.h )
include( copymac.cmake )

COPY_CPLUSPLUS( MESSAGE_SERVICE_PP_SOURCES ${MESSAGE_SERVICE_SOURCES} )



sack_add_library( message_service SHARED ${MESSAGE_SERVICE_SOURCES} )
sack_add_library( message_servicepp SHARED ${MESSAGE_SERVICE_PP_SOURCES} )

target_link_libraries( message_service ${SACK_LIBRARIES} )
target_link_libraries( message_servicepp ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( message_service winmm )
target_link_libraries( message_servicepp winmm )
endif( WIN32 )
SET_PROPERTY( TARGET message_service APPEND PROPERTY
                  COMPILE_FLAGS  "-DCLIENTMSG_SOURCE"
)
SET_PROPERTY( TARGET message_servicepp APPEND PROPERTY
                  COMPILE_FLAGS  "-DCLIENTMSG_SOURCE"
)
        
install_default_dest( message_service )
install_default_dest( message_servicepp )
