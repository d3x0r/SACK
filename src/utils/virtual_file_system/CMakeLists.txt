if( NOT __CLR__ )

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

target_link_libraries( ${PROJECT} ${SACK_CORE_LIBRARY} )
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

if( NEED_FUSE OR FUSE_FOUND )
	sack_add_executable( sack_vfs.fuse.test fuse_private.c )
	target_link_libraries( sack_vfs.fuse.test ${SACK_LIBRARIES} ${FUSE_LIBRARIES} sack_vfs.module )
	install_mode_dest( sack_vfs.fuse.test )
        #SET_PROPERTY( sack_vfs.fuse.test APPEND PROPERTY 
        #	COMPILE_DEFINITIONS "
	SET_TARGET_PROPERTIES(sack_vfs.fuse.test PROPERTIES
                  
                  FOLDER utils/tests
	)
endif( NEED_FUSE OR FUSE_FOUND )

endif( BUILD_TESTS ) 

if( BUILD_EXTRAS )

add_subdirectory( vfs_command )
add_subdirectory( vfs_extract )

# some assembly required...
#if( WIN32 )
   add_subdirectory( vfs_runner )
#endif( WIN32 )

endif( BUILD_EXTRAS )
endif( NOT __CLR__ )
