
set( PROJECT network_forward_udp )
sack_add_executable( ${PROJECT} forward.c )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES  FOLDER "Network Examples" )

