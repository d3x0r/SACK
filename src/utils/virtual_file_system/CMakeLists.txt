
set( PROJECT sack_vfs.module )

set(BASE_SOURCES
    vfs.c
)

sack_add_library( ${PROJECT} SHARED ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)
if( NOT __ANDROID__ )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
  )
endif( NOT __ANDROID__ )

target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

if( BUILD_TESTS ) 

set( PROJECT sack_vfs.test )

set(BASE_SOURCES
    vfs_tester.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/tests
)

target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} sack_vfs.module )
install_mode_dest( ${PROJECT} )

endif( BUILD_TESTS ) 

if( BUILD_EXTRAS )

add_subdirectory( vfs_command )
add_subdirectory( vfs_runner )
endif( BUILD_EXTRAS )
