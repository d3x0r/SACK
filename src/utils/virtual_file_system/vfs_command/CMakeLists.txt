set( PROJECT sack_vfs_command )

set(BASE_SOURCES
    vfs_command.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

