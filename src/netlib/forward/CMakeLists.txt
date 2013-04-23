PROJECT( network_forward_udp )
add_executable( ${PROJECT_NAME} forward )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME}  wsock32 )
endif( WIN32 )

