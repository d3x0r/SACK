
PROJECT( factorio.rail_composer )

sack_add_executable( ${PROJECT_NAME} main.c )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )
