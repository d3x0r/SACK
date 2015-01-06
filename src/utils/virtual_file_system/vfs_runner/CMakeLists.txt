set( PROJECT sack_vfs_runner )

set(BASE_SOURCES
    vfs_runner.c memory_dll_loader.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

if( 0 )
set( PROJECT sack_vfs_run_builder )

set(BASE_SOURCES
    vfs_run_builder.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

endif( 0 )


if( 0 )
set( PROJECT sack_vfs_run_builder )

set(BASE_SOURCES
    vfs_run_builder.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

endif( 0 )

add_subdirectory( portable )
