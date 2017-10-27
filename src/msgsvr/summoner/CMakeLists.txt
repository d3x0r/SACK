cmake_minimum_required(VERSION 2.8)

project( summoner )

add_program( ${PROJECT_NAME} WIN32 summoner )
#target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

project( construct )

project( summoner_avatar )

add_program( ${PROJECT_NAME} WIN32 avatar )
#target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )


