
PROJECT( factorio.rail_slicer )

sack_add_executable( ${PROJECT_NAME} WIN32 main.c )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )
