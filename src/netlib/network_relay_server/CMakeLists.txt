
set( project relay_server )

sack_add_executable( ${project} relay_server.c )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )
SET_TARGET_PROPERTIES( ${project} PROPERTIES  FOLDER "Network Examples" )




