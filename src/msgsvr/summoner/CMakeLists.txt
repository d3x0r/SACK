
project( summoner )

add_program( ${PROJECT_NAME} WIN32 summoner.c )
#target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

project( construct )

project( summoner_avatar )

add_program( ${PROJECT_NAME} WIN32 avatar.c )
#target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )


