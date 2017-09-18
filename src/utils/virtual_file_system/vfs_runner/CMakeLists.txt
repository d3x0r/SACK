
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

ExternalProject_Add( ${PROJECT}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)

#add_subdirectory( portable )
